/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

/**
 * @file supercall.c
 * @brief 超级调用系统 - KernelPatch核心功能接口
 * @details 提供用户空间与内核补丁的通信桥梁，支持模块管理、权限控制、存储等功能
 * 
 * 超级调用系统是KernelPatch的核心组件，通过自定义系统调用号__NR_supercall
 * 为用户空间程序提供与内核补丁交互的统一接口。主要功能包括：
 * - KPM模块的加载、卸载、控制和查询
 * - root权限提升(su)和用户权限管理  
 * - 超级密钥管理和认证
 * - 内核存储(kstorage)的读写操作
 * - 系统信息查询和调试功能
 * 
 * 认证机制：
 * - 超级密钥认证：拥有正确超级密钥的进程可执行所有操作
 * - su认证：特定UID允许执行su相关操作
 * - 公开接口：部分信息查询功能无需认证
 */

#include <ktypes.h>
#include <uapi/scdefs.h>
#include <hook.h>
#include <common.h>
#include <log.h>
#include <predata.h>
#include <pgtable.h>
#include <linux/syscall.h>
#include <uapi/asm-generic/errno.h>
#include <linux/uaccess.h>
#include <linux/cred.h>
#include <asm/current.h>
#include <linux/string.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/security.h>
#include <syscall.h>
#include <accctl.h>
#include <module.h>
#include <kputils.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <kputils.h>
#include <pidmem.h>
#include <predata.h>
#include <linux/random.h>
#include <sucompat.h>
#include <accctl.h>
#include <kstorage.h>

#define MAX_KEY_LEN 128  // 超级密钥最大长度限制

#include <linux/umh.h>

/**
 * @brief 测试调用 - 验证超级调用系统是否正常工作
 * @param arg1 测试参数1
 * @param arg2 测试参数2  
 * @param arg3 测试参数3
 * @return 固定返回0，表示测试成功
 * @details 这是一个简单的测试接口，用于验证超级调用框架的基本功能
 */
static long call_test(long arg1, long arg2, long arg3)
{
    return 0;
}

/**
 * @brief 获取启动日志 - 打印内核补丁启动过程中的日志信息
 * @return 固定返回0
 * @details 将KernelPatch启动过程中记录的日志输出到内核日志缓冲区
 */
static long call_bootlog()
{
    print_bootlog();
    return 0;
}

/**
 * @brief 内核恐慌调用 - 强制触发内核panic，用于紧急调试
 * @return 不返回(系统崩溃)
 * @details 这是一个危险的调试功能，会立即触发内核panic导致系统重启
 * 仅在需要强制收集内核转储信息时使用
 */
static long call_panic()
{
    unsigned long panic_addr = kallsyms_lookup_name("panic");
    ((void (*)(const char *fmt, ...))panic_addr)("!!!! kernel_patch panic !!!!");
    return 0;
}

/**
 * @brief 内核日志记录 - 允许用户空间向内核日志写入信息
 * @param arg1 用户空间的日志字符串指针
 * @return 成功返回0，失败返回负错误码
 * @details 将用户空间传入的字符串写入内核日志，用于调试和监控
 */
static long call_klog(const char __user *arg1)
{
    char buf[1024];
    long len = compat_strncpy_from_user(buf, arg1, sizeof(buf));
    if (len <= 0) return -EINVAL;
    if (len > 0) logki("user log: %s", buf);
    return 0;
}

/**
 * @brief 获取编译时间 - 返回KernelPatch的编译时间戳
 * @param out_buildtime 输出缓冲区，存储编译时间字符串
 * @param u_len 用户缓冲区长度
 * @return 成功返回0，缓冲区不足返回-ENOMEM
 * @details 向用户空间返回KernelPatch的编译时间，用于版本识别
 */
static long call_buildtime(char __user *out_buildtime, int u_len)
{
    const char *buildtime = get_build_time();
    int len = strlen(buildtime);
    if (len >= u_len) return -ENOMEM;
    int rc = compat_copy_to_user(out_buildtime, buildtime, len + 1);
    return rc;
}

/**
 * @brief KernelPatch模块加载 - 从指定路径加载内核模块
 * @param arg1 模块文件路径(用户空间指针)
 * @param arg2 模块启动参数(用户空间指针)
 * @param reserved 保留参数
 * @return 成功返回0，失败返回负错误码
 * @details 动态加载KPM格式的内核模块，支持传递启动参数
 */
static long call_kpm_load(const char __user *arg1, const char *__user arg2, void *__user reserved)
{
    char path[1024], args[KPM_ARGS_LEN];
    long pathlen = compat_strncpy_from_user(path, arg1, sizeof(path));
    if (pathlen <= 0) return -EINVAL;
    long arglen = compat_strncpy_from_user(args, arg2, sizeof(args));
    return load_module_path(path, arglen <= 0 ? 0 : args, reserved);
}

/**
 * @brief KernelPatch模块控制 - 向已加载的模块发送控制命令
 * @param arg1 模块名称(用户空间指针)
 * @param arg2 控制参数(用户空间指针)
 * @param out_msg 模块响应消息输出缓冲区
 * @param outlen 输出缓冲区长度
 * @return 成功返回0，失败返回负错误码
 * @details 向指定模块发送控制命令并接收响应消息
 */
static long call_kpm_control(const char __user *arg1, const char *__user arg2, void *__user out_msg, int outlen)
{
    char name[KPM_NAME_LEN], args[KPM_ARGS_LEN];
    long namelen = compat_strncpy_from_user(name, arg1, sizeof(name));
    if (namelen <= 0) return -EINVAL;
    long arglen = compat_strncpy_from_user(args, arg2, sizeof(args));
    return module_control0(name, arglen <= 0 ? 0 : args, out_msg, outlen);
}

/**
 * @brief KernelPatch模块卸载 - 卸载指定名称的内核模块
 * @param arg1 模块名称(用户空间指针)
 * @param reserved 保留参数
 * @return 成功返回0，失败返回负错误码
 * @details 根据模块名称卸载已加载的KPM模块
 */
static long call_kpm_unload(const char *__user arg1, void *__user reserved)
{
    char name[KPM_NAME_LEN];
    long len = compat_strncpy_from_user(name, arg1, sizeof(name));
    if (len <= 0) return -EINVAL;
    return unload_module(name, reserved);
}

/**
 * @brief 获取已加载模块数量
 * @return 当前已加载的KPM模块数量
 * @details 返回系统中当前加载的KernelPatch模块总数
 */
static long call_kpm_nums()
{
    return get_module_nums();
}

/**
 * @brief 获取已加载模块列表
 * @param names 输出缓冲区，存储模块名称列表
 * @param len 缓冲区长度
 * @return 成功返回0，缓冲区不足返回-ENOBUFS
 * @details 将所有已加载模块的名称列表复制到用户空间缓冲区
 */
static long call_kpm_list(char *__user names, int len)
{
    if (len <= 0) return -EINVAL;
    char buf[4096];
    int sz = list_modules(buf, sizeof(buf));
    if (sz > len) return -ENOBUFS;
    sz = compat_copy_to_user(names, buf, len);
    return sz;
}

/**
 * @brief 获取指定模块的详细信息
 * @param uname 模块名称(用户空间指针)
 * @param out_info 输出缓冲区，存储模块信息
 * @param out_len 输出缓冲区长度
 * @return 成功返回0，失败返回负错误码
 * @details 获取指定模块的详细信息，包括版本、状态、占用内存等
 */
static long call_kpm_info(const char *__user uname, char *__user out_info, int out_len)
{
    if (out_len <= 0) return -EINVAL;
    char name[64];
    char buf[2048];
    int len = compat_strncpy_from_user(name, uname, sizeof(name));
    if (len <= 0) return -EINVAL;
    int sz = get_module_info(name, buf, sizeof(buf));
    if (sz < 0) return sz;
    if (sz > out_len) return -ENOBUFS;
    sz = compat_copy_to_user(out_info, buf, sz);
    return sz;
}

/**
 * @brief 当前进程root权限提升
 * @param uprofile su配置文件(用户空间指针)
 * @return 成功返回0，失败返回负错误码
 * @details 将当前进程提升为指定UID的权限，并设置SELinux上下文
 */
static long call_su(struct su_profile *__user uprofile)
{
    struct su_profile *profile = memdup_user(uprofile, sizeof(struct su_profile));
    if (!profile || IS_ERR(profile)) return PTR_ERR(profile);
    profile->scontext[sizeof(profile->scontext) - 1] = '\0';
    int rc = commit_su(profile->to_uid, profile->scontext);
    kvfree(profile);
    return rc;
}

/**
 * @brief 指定进程root权限提升
 * @param pid 目标进程ID
 * @param uprofile su配置文件(用户空间指针)
 * @return 成功返回0，失败返回负错误码
 * @details 将指定PID的进程提升为root权限，并设置SELinux上下文
 */
static long call_su_task(pid_t pid, struct su_profile *__user uprofile)
{
    struct su_profile *profile = memdup_user(uprofile, sizeof(struct su_profile));
    if (!profile || IS_ERR(profile)) return PTR_ERR(profile);
    profile->scontext[sizeof(profile->scontext) - 1] = '\0';
    int rc = task_su(pid, profile->to_uid, profile->scontext);
    kvfree(profile);
    return rc;
}

/**
 * @brief 获取超级密钥
 * @param out_key 输出缓冲区，存储超级密钥
 * @param out_len 输出缓冲区长度
 * @return 成功返回0，缓冲区不足返回-ENOMEM
 * @details 将当前的超级密钥复制到用户空间缓冲区
 */
static long call_skey_get(char *__user out_key, int out_len)
{
    const char *key = get_superkey();
    int klen = strlen(key);
    if (klen >= out_len) return -ENOMEM;
    int rc = compat_copy_to_user(out_key, key, klen + 1);
    return rc;
}

/**
 * @brief 设置新的超级密钥
 * @param new_key 新的超级密钥(用户空间指针)
 * @return 成功返回0，密钥过长返回-E2BIG
 * @details 重置系统的超级密钥，影响后续所有需要超级密钥认证的操作
 */
static long call_skey_set(char *__user new_key)
{
    char buf[SUPER_KEY_LEN];
    int len = compat_strncpy_from_user(buf, new_key, sizeof(buf));
    if (len >= SUPER_KEY_LEN && buf[SUPER_KEY_LEN - 1]) return -E2BIG;
    reset_superkey(new_key);
    return 0;
}

/**
 * @brief 启用/禁用root用户超级密钥认证
 * @param enable 1启用，0禁用
 * @return 固定返回0
 * @details 控制root用户是否需要超级密钥认证才能执行特权操作
 */
static long call_skey_root_enable(int enable)
{
    enable_auth_root_key(enable);
    return 0;
}

/**
 * @brief 授权UID进行su操作
 * @param uprofile su配置文件(用户空间指针)
 * @return 成功返回0，失败返回负错误码
 * @details 将指定UID添加到su允许列表中，使其可以进行权限提升
 */
static long call_grant_uid(struct su_profile *__user uprofile)
{
    struct su_profile *profile = memdup_user(uprofile, sizeof(struct su_profile));
    if (!profile || IS_ERR(profile)) return PTR_ERR(profile);
    int rc = su_add_allow_uid(profile->uid, profile->to_uid, profile->scontext);
    kvfree(profile);
    return rc;
}

/**
 * @brief 撤销UID的su权限
 * @param uid 要撤销权限的用户ID
 * @return 成功返回0，失败返回负错误码
 * @details 从su允许列表中移除指定UID，禁止其进行权限提升
 */
static long call_revoke_uid(uid_t uid)
{
    return su_remove_allow_uid(uid);
}

/**
 * @brief 获取su允许的UID数量
 * @return 当前su允许列表中的UID数量
 * @details 返回当前被授权进行su操作的UID总数
 */
static long call_su_allow_uid_nums()
{
    return su_allow_uid_nums();
}

#ifdef ANDROID
extern int android_is_safe_mode;
/**
 * @brief 获取Android安全模式状态
 * @return 1表示安全模式，0表示正常模式
 * @details Android专用功能，检查系统是否处于安全模式
 */
static long call_su_get_safemode()
{
    int result = android_is_safe_mode;
    logkfd("[call_su_get_safemode] %d\n", result);
    return result;
}
#endif

/**
 * @brief 获取su允许的UID列表
 * @param uids 输出缓冲区，存储UID数组
 * @param num UID数组长度
 * @return 成功返回实际UID数量，失败返回负错误码
 * @details 将所有被授权进行su操作的UID列表复制到用户空间
 */
static long call_su_list_allow_uid(uid_t *__user uids, int num)
{
    return su_allow_uids(1, uids, num);
}

/**
 * @brief 获取指定UID的su配置文件
 * @param uid 目标用户ID
 * @param uprofile 输出su配置文件(用户空间指针)
 * @return 成功返回0，失败返回负错误码
 * @details 获取指定UID的su权限配置信息
 */
static long call_su_allow_uid_profile(uid_t uid, struct su_profile *__user uprofile)
{
    return su_allow_uid_profile(1, uid, uprofile);
}

/**
 * @brief 重置su程序路径
 * @param upath 新的su程序路径(用户空间指针)
 * @return 成功返回0，失败返回负错误码
 * @details 更新系统中su程序的可执行文件路径
 */
static long call_reset_su_path(const char *__user upath)
{
    return su_reset_path(strndup_user(upath, SU_PATH_MAX_LEN));
}

/**
 * @brief 获取当前su程序路径
 * @param ubuf 输出缓冲区，存储su路径
 * @param buf_len 缓冲区长度
 * @return 成功返回0，缓冲区不足返回-ENOBUFS
 * @details 获取当前配置的su程序可执行文件路径
 */
static long call_su_get_path(char *__user ubuf, int buf_len)
{
    const char *path = su_get_path();
    int len = strlen(path);
    if (buf_len <= len) return -ENOBUFS;
    return compat_copy_to_user(ubuf, path, len + 1);
}

/**
 * @brief 获取允许的SELinux上下文
 * @param usctx 输出缓冲区，存储SELinux上下文
 * @param ulen 缓冲区长度
 * @return 成功返回0，缓冲区不足返回-ENOBUFS
 * @details 获取当前配置的全局允许SELinux安全上下文
 */
static long call_su_get_allow_sctx(char *__user usctx, int ulen)
{
    int len = strlen(all_allow_sctx);
    if (ulen <= len) return -ENOBUFS;
    return compat_copy_to_user(usctx, all_allow_sctx, len + 1);
}

/**
 * @brief 设置允许的SELinux上下文
 * @param usctx 新的SELinux上下文(用户空间指针)
 * @return 成功返回0，上下文过长返回-E2BIG
 * @details 配置全局允许的SELinux安全上下文
 */
static long call_su_set_allow_sctx(char *__user usctx)
{
    char buf[SUPERCALL_SCONTEXT_LEN];
    buf[0] = '\0';
    int len = compat_strncpy_from_user(buf, usctx, sizeof(buf));
    if (len >= SUPERCALL_SCONTEXT_LEN && buf[SUPERCALL_SCONTEXT_LEN - 1]) return -E2BIG;
    return set_all_allow_sctx(buf);
}

/**
 * @brief 内核存储读取操作
 * @param gid 组ID
 * @param did 数据ID
 * @param out_data 输出数据缓冲区
 * @param offset 读取偏移量
 * @param dlen 读取数据长度
 * @return 成功返回读取字节数，失败返回负错误码
 * @details 从内核存储中读取指定组和数据ID的数据
 */
static long call_kstorage_read(int gid, long did, void *out_data, int offset, int dlen)
{
    return read_kstorage(gid, did, out_data, offset, dlen, true);
}

/**
 * @brief 内核存储写入操作
 * @param gid 组ID
 * @param did 数据ID
 * @param data 输入数据缓冲区
 * @param offset 写入偏移量
 * @param dlen 写入数据长度
 * @return 成功返回写入字节数，失败返回负错误码
 * @details 向内核存储中写入指定组和数据ID的数据
 */
static long call_kstorage_write(int gid, long did, void *data, int offset, int dlen)
{
    return write_kstorage(gid, did, data, offset, dlen, true);
}

/**
 * @brief 列出内核存储中的数据ID
 * @param gid 组ID
 * @param ids 输出ID数组缓冲区
 * @param ids_len ID数组长度
 * @return 成功返回实际ID数量，失败返回负错误码
 * @details 获取指定组中所有存储数据的ID列表
 */
static long call_list_kstorage_ids(int gid, long *ids, int ids_len)
{
    return list_kstorage_ids(gid, ids, ids_len, false);
}

/**
 * @brief 删除内核存储中的数据
 * @param gid 组ID
 * @param did 数据ID
 * @return 成功返回0，失败返回负错误码
 * @details 从内核存储中删除指定组和数据ID的数据
 */
static long call_kstorage_remove(int gid, long did)
{
    return remove_kstorage(gid, did);
}

/**
 * @brief 超级调用分发函数 - 根据命令码执行相应的功能
 * @param is_key_auth 是否通过超级密钥认证(1=是，0=否)
 * @param cmd 超级调用命令码
 * @param arg1 参数1
 * @param arg2 参数2
 * @param arg3 参数3
 * @param arg4 参数4
 * @return 执行结果，成功通常返回0，失败返回负错误码
 * @details 超级调用的核心分发函数，根据不同的认证级别和命令码
 * 执行相应的功能。支持三种认证模式：
 * - 公开接口：无需认证，如版本查询、日志等
 * - su认证：特定UID可执行su相关操作
 * - 超级密钥认证：拥有正确密钥可执行所有特权操作
 */
static long supercall(int is_key_auth, long cmd, long arg1, long arg2, long arg3, long arg4)
{
    // 公开接口 - 无需认证的基础信息查询
    switch (cmd) {
    case SUPERCALL_HELLO:
        logki(SUPERCALL_HELLO_ECHO "\n");
        return SUPERCALL_HELLO_MAGIC;
    case SUPERCALL_KLOG:
        return call_klog((const char *__user)arg1);
    case SUPERCALL_KERNELPATCH_VER:
        return kpver;
    case SUPERCALL_KERNEL_VER:
        return kver;
    case SUPERCALL_BUILD_TIME:
        return call_buildtime((char *__user)arg1, (int)arg2);
    }

    // su认证接口 - 特定UID可执行的权限操作
    switch (cmd) {
    case SUPERCALL_SU:
        return call_su((struct su_profile * __user) arg1);
    case SUPERCALL_SU_TASK:
        return call_su_task((pid_t)arg1, (struct su_profile * __user) arg2);

    case SUPERCALL_SU_GRANT_UID:
        return call_grant_uid((struct su_profile * __user) arg1);
    case SUPERCALL_SU_REVOKE_UID:
        return call_revoke_uid((uid_t)arg1);
    case SUPERCALL_SU_NUMS:
        return call_su_allow_uid_nums();
    case SUPERCALL_SU_LIST:
        return call_su_list_allow_uid((uid_t *)arg1, (int)arg2);
    case SUPERCALL_SU_PROFILE:
        return call_su_allow_uid_profile((uid_t)arg1, (struct su_profile * __user) arg2);
    case SUPERCALL_SU_RESET_PATH:
        return call_reset_su_path((const char *)arg1);
    case SUPERCALL_SU_GET_PATH:
        return call_su_get_path((char *__user)arg1, (int)arg2);
    case SUPERCALL_SU_GET_ALLOW_SCTX:
        return call_su_get_allow_sctx((char *__user)arg1, (int)arg2);
    case SUPERCALL_SU_SET_ALLOW_SCTX:
        return call_su_set_allow_sctx((char *__user)arg1);

    // 内核存储接口 - 需要su权限或超级密钥认证
    case SUPERCALL_KSTORAGE_READ:
        return call_kstorage_read((int)arg1, (long)arg2, (void *)arg3, (int)((long)arg4 >> 32), (long)arg4 << 32 >> 32);
    case SUPERCALL_KSTORAGE_WRITE:
        return call_kstorage_write((int)arg1, (long)arg2, (void *)arg3, (int)((long)arg4 >> 32),
                                   (long)arg4 << 32 >> 32);
    case SUPERCALL_KSTORAGE_LIST_IDS:
        return call_list_kstorage_ids((int)arg1, (long *)arg2, (int)arg3);
    case SUPERCALL_KSTORAGE_REMOVE:
        return call_kstorage_remove((int)arg1, (long)arg2);

#ifdef ANDROID
    case SUPERCALL_SU_GET_SAFEMODE:
        return call_su_get_safemode();
#endif
    default:
        break;
    }

    // 调试和测试接口 - 中等权限级别
    switch (cmd) {
    case SUPERCALL_BOOTLOG:
        return call_bootlog();
    case SUPERCALL_PANIC:
        return call_panic();
    case SUPERCALL_TEST:
        return call_test(arg1, arg2, arg3);
    default:
        break;
    }

    // 超级密钥认证检查 - 以下操作需要超级密钥认证
    if (!is_key_auth) return -EPERM;

    // 超级密钥管理接口 - 仅超级密钥认证用户可执行
    switch (cmd) {
    case SUPERCALL_SKEY_GET:
        return call_skey_get((char *__user)arg1, (int)arg2);
    case SUPERCALL_SKEY_SET:
        return call_skey_set((char *__user)arg1);
    case SUPERCALL_SKEY_ROOT_ENABLE:
        return call_skey_root_enable((int)arg1);
        break;
    }

    // KPM模块管理接口 - 仅超级密钥认证用户可执行
    switch (cmd) {
    case SUPERCALL_KPM_LOAD:
        return call_kpm_load((const char *__user)arg1, (const char *__user)arg2, (void *__user)arg3);
    case SUPERCALL_KPM_UNLOAD:
        return call_kpm_unload((const char *__user)arg1, (void *__user)arg2);
    case SUPERCALL_KPM_CONTROL:
        return call_kpm_control((const char *__user)arg1, (const char *__user)arg2, (char *__user)arg3, (int)arg4);
    case SUPERCALL_KPM_NUMS:
        return call_kpm_nums();
    case SUPERCALL_KPM_LIST:
        return call_kpm_list((char *__user)arg1, (int)arg2);
    case SUPERCALL_KPM_INFO:
        return call_kpm_info((const char *__user)arg1, (char *__user)arg2, (int)arg3);
    }

    // 扩展功能接口预留
    switch (cmd) {
    default:
        break;
    }

    // 未知命令码
    return -ENOSYS;
}

/**
 * @brief 超级调用前置hook函数 - 系统调用拦截和认证处理
 * @param args 系统调用参数结构体
 * @param udata 用户数据(未使用)
 * @details 这是超级调用系统的入口点，负责：
 * 1. 从系统调用参数中提取认证密钥和命令码
 * 2. 执行认证逻辑(超级密钥认证或su权限认证)
 * 3. 调用超级调用分发函数处理具体命令
 * 4. 设置系统调用返回值并跳过原始系统调用
 * 
 * 参数约定:
 * - args[0]: 认证密钥字符串指针
 * - args[1]: 版本和命令码(低16位为命令码)
 * - args[2-5]: 超级调用参数1-4
 */
static void before(hook_fargs6_t *args, void *udata)
{
    const char *__user ukey = (const char *__user)syscall_argn(args, 0);
    long ver_xx_cmd = (long)syscall_argn(args, 1);

    // TODO: 从0.10.5版本开始支持版本检查
    // uint32_t ver = (ver_xx_cmd & 0xFFFFFFFF00000000ul) >> 32;
    // long xx = (ver_xx_cmd & 0xFFFF0000) >> 16;

    // 提取命令码(低16位)
    long cmd = ver_xx_cmd & 0xFFFF;
    if (cmd < SUPERCALL_HELLO || cmd > SUPERCALL_MAX) return;

    // 从用户空间拷贝认证密钥
    char key[MAX_KEY_LEN];
    long len = compat_strncpy_from_user(key, ukey, MAX_KEY_LEN);
    if (len <= 0) return;

    int is_key_auth = 0;

    // 认证逻辑处理
    if (!auth_superkey(key)) {
        // 超级密钥认证成功
        is_key_auth = 1;
    } else if (!strcmp("su", key)) {
        // su权限认证：检查当前UID是否被授权
        uid_t uid = current_uid();
        if (!is_su_allow_uid(uid)) return;
    } else {
        // 认证失败，拒绝访问
        return;
    }

    // 提取超级调用参数
    long a1 = (long)syscall_argn(args, 2);
    long a2 = (long)syscall_argn(args, 3);
    long a3 = (long)syscall_argn(args, 4);
    long a4 = (long)syscall_argn(args, 5);

    // 跳过原始系统调用，设置返回值
    args->skip_origin = 1;
    args->ret = supercall(is_key_auth, cmd, a1, a2, a3, a4);
}

/**
 * @brief 安装超级调用系统
 * @return 成功返回0，失败返回错误码
 * @details 将超级调用hook安装到__NR_supercall系统调用号上，
 * 使得用户空间程序可以通过该系统调用与KernelPatch通信
 */
int supercall_install()
{
    int rc = 0;

    // 安装6参数的系统调用hook
    hook_err_t err = hook_syscalln(__NR_supercall, 6, before, 0, 0);
    if (err) {
        log_boot("install supercall hook error: %d\n", err);
        rc = err;
        goto out;
    }
out:
    return rc;
}
