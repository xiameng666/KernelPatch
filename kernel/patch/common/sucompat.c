/* KernelPatch SU兼容性模块 - 实现SU命令的兼容性处理和权限管理 */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <linux/list.h>
#include <ktypes.h>
#include <compiler.h>
#include <stdbool.h>
#include <linux/syscall.h>
#include <ksyms.h>
#include <hook.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <stdbool.h>
#include <asm/current.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <uapi/scdefs.h>
#include <kputils.h>
#include <linux/ptrace.h>
#include <accctl.h>
#include <linux/string.h>
#include <linux/err.h>
#include <uapi/asm-generic/errno.h>
#include <taskob.h>
#include <linux/kernel.h>
#include <linux/rculist.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <syscall.h>
#include <predata.h>
#include <predata.h>
#include <kconfig.h>
#include <linux/vmalloc.h>
#include <sucompat.h>
#include <symbol.h>
#include <uapi/linux/limits.h>
#include <predata.h>
#include <kstorage.h>

// 系统路径定义
const char sh_path[] = SH_PATH;                     // Shell路径
const char default_su_path[] = SU_PATH;             // 默认SU路径

#ifdef ANDROID
const char legacy_su_path[] = LEGACY_SU_PATH;      // 旧版SU路径
const char apd_path[] = APD_PATH;                   // APD守护进程路径
#endif

static const char *current_su_path = 0;            // 当前SU路径

static int su_kstorage_gid = -1;                   // SU存储组ID
static int exclude_kstorage_gid = -1;              // 排除列表存储组ID

/**
 * 检查指定UID是否被允许使用SU
 * @param uid 要检查的用户ID
 * @return 1表示允许，0表示不允许
 */
int is_su_allow_uid(uid_t uid)
{
    int rc = 0;
    rcu_read_lock();                                // 获取RCU读锁
    const struct kstorage *ks = get_kstorage(su_kstorage_gid, uid);
    if (IS_ERR_OR_NULL(ks) || ks->dlen <= 0) goto out;

    struct su_profile *profile = (struct su_profile *)ks->data;
    rc = profile->uid == uid;                       // 验证UID匹配

out:
    rcu_read_unlock();                              // 释放RCU读锁
    return rc;
}
KP_EXPORT_SYMBOL(is_su_allow_uid);

/**
 * 添加允许使用SU的UID
 * @param uid 用户ID
 * @param to_uid 目标用户ID
 * @param scontext SELinux安全上下文
 * @return 成功返回0，失败返回错误码
 */
int su_add_allow_uid(uid_t uid, uid_t to_uid, const char *scontext)
{
    if (!scontext) scontext = "";                   // 确保上下文不为空
    struct su_profile profile = {
        uid,
        to_uid,
    };
    memcpy(profile.scontext, scontext, SUPERCALL_SCONTEXT_LEN);
    int rc = write_kstorage(su_kstorage_gid, uid, &profile, 0, sizeof(struct su_profile), false);
    logkfd("uid: %d, to_uid: %d, sctx: %s, rc: %d\n", uid, to_uid, scontext, rc);
    return rc;
}
KP_EXPORT_SYMBOL(su_add_allow_uid);

/**
 * 移除允许使用SU的UID
 * @param uid 要移除的用户ID
 * @return 成功返回0，失败返回错误码
 */
int su_remove_allow_uid(uid_t uid)
{
    return remove_kstorage(su_kstorage_gid, uid);
}
KP_EXPORT_SYMBOL(su_remove_allow_uid);

/**
 * 获取允许使用SU的UID数量
 * @return UID数量
 */
int su_allow_uid_nums()
{
    return kstorage_group_size(su_kstorage_gid);
}
KP_EXPORT_SYMBOL(su_allow_uid_nums);

/**
 * 遍历允许UID列表的回调函数
 * @param kstorage 存储项
 * @param udata 用户数据
 * @return 成功返回0，失败返回错误码
 */
static int allow_uids_cb(struct kstorage *kstorage, void *udata)
{
    struct
    {
        int is_user;                                    // 是否为用户空间调用
        uid_t *out_uids;                               // 输出UID数组
        int idx;                                       // 当前索引
        int out_num;                                   // 输出数组大小
    } *up = (typeof(up))udata;

    if (up->idx >= up->out_num) {
        return -ENOBUFS;                               // 缓冲区已满
    }

    struct su_profile *profile = (struct su_profile *)kstorage->data;

    if (up->is_user) {
        // 复制到用户空间
        int cprc = compat_copy_to_user(up->out_uids + up->idx, &profile->uid, sizeof(uid_t));
        if (cprc <= 0) {
            logkfd("compat_copy_to_user error: %d", cprc);
            return cprc;
        }
    } else {
        // 复制到内核空间
        up->out_uids[up->idx] = profile->uid;
    }

    up->idx++;

    return 0;
}

/**
 * 获取允许使用SU的所有UID列表
 * @param is_user 是否为用户空间调用
 * @param out_uids 输出UID数组
 * @param out_num 数组大小
 * @return 实际返回的UID数量
 */
int su_allow_uids(int is_user, uid_t *out_uids, int out_num)
{
    struct
    {
        int iu;
        uid_t *up;
        int idx;
        int out_num;
    } udata = { is_user, out_uids, 0, out_num };

    on_each_kstorage_elem(su_kstorage_gid, allow_uids_cb, &udata);

    return udata.idx;                                  // 返回实际数量
}
KP_EXPORT_SYMBOL(su_allow_uids);

/**
 * 获取指定UID的SU配置信息
 * @param is_user 是否为用户空间调用
 * @param uid 要查询的用户ID
 * @param out_profile 输出配置信息
 * @return 成功返回0，失败返回错误码
 */
int su_allow_uid_profile(int is_user, uid_t uid, struct su_profile *out_profile)
{
    int rc = 0;

    rcu_read_lock();                                   // 获取RCU读锁
    const struct kstorage *ks = get_kstorage(su_kstorage_gid, uid);
    if (IS_ERR(ks)) {
        rc = -ENOENT;                                  // 未找到该UID
        goto out;
    }
    struct su_profile *profile = (struct su_profile *)ks->data;

    if (is_user) {
        // 复制到用户空间
        rc = compat_copy_to_user(out_profile, profile, sizeof(struct su_profile));
        if (rc <= 0) {
            logkfd("compat_copy_to_user error: %d", rc);
            goto out;
        }
    } else {
        // 复制到内核空间
        memcpy(out_profile, profile, sizeof(struct su_profile));
    }

out:
    rcu_read_unlock();                                 // 释放RCU读锁
    return rc;
}
KP_EXPORT_SYMBOL(su_allow_uid_profile);

/**
 * 重置SU路径
 * 设置新的SU命令路径，不进行内存释放和加锁
 * @param path 新的SU路径
 * @return 成功返回0，失败返回错误码
 */
int su_reset_path(const char *path)
{
    if (!path) return -EINVAL;                         // 路径为空
    if (IS_ERR(path)) return PTR_ERR(path);           // 路径错误
    current_su_path = path;                           // 设置新路径
    logkfd("%s\n", current_su_path);
    dsb(ish);                                         // 内存屏障确保可见性
    return 0;
}
KP_EXPORT_SYMBOL(su_reset_path);

/**
 * 获取当前SU路径
 * @return 当前SU路径字符串
 */
const char *su_get_path()
{
    if (!current_su_path) current_su_path = default_su_path; // 使用默认路径
    return current_su_path;
}
KP_EXPORT_SYMBOL(su_get_path);

/**
 * execve系统调用前置处理函数
 * 拦截SU命令执行，进行权限检查和路径重定向
 * @param u_filename_p 指向用户空间文件名指针的指针
 * @param uargv 用户空间参数数组指针
 * @param udata 用户数据
 */
static void handle_before_execve(char **__user u_filename_p, char **__user uargv, void *udata)
{
    char __user *ufilename = *u_filename_p;
    char filename[SU_PATH_MAX_LEN];
    int flen = compat_strncpy_from_user(filename, ufilename, sizeof(filename));
    if (flen <= 0) return;

    if (!strcmp(current_su_path, filename)) {
        // 处理SU命令执行
        uid_t uid = current_uid();                     // 获取当前UID
        struct su_profile profile;
        if (su_allow_uid_profile(0, uid, &profile)) return; // 获取SU配置失败

        uid_t to_uid = profile.to_uid;                 // 目标UID
        const char *sctx = profile.scontext;           // 安全上下文
        commit_su(to_uid, sctx);                       // 提交权限变更

#ifdef ANDROID
        // Android平台特殊处理
        struct file *filp = filp_open(apd_path, O_RDONLY, 0);
        if (!filp || IS_ERR(filp)) {
#endif
            // 重定向到shell执行
            void *uptr = copy_to_user_stack(sh_path, sizeof(sh_path));
            if (uptr && !IS_ERR(uptr)) {
                *u_filename_p = (char *__user)uptr;
            }
            logkfi("call su uid: %d, to_uid: %d, sctx: %s, uptr: %llx\n", uid, to_uid, sctx, uptr);
#ifdef ANDROID
        } else {
            // 使用APD守护进程处理
            filp_close(filp, 0);

            // 设置命令路径
            uint64_t sp = 0;
            sp = current_user_stack_pointer();
            sp -= sizeof(apd_path);
            sp &= 0xFFFFFFFFFFFFFFF8;                   // 8字节对齐
            int cplen = compat_copy_to_user((void *)sp, apd_path, sizeof(apd_path));
            if (cplen > 0) {
                *u_filename_p = (char *)sp;
            }

            // 设置参数数组
            int argv_cplen = 0;
            // 如果文件名与legacy su路径不同，需要重定向到legacy路径
            if (strcmp(legacy_su_path, filename)) {
                if (argv_cplen <= 0) {
                    // 获取当前用户栈指针
                    sp = sp ?: current_user_stack_pointer();
                    // 为legacy su路径分配栈空间
                    sp -= sizeof(legacy_su_path);
                    // 8字节对齐
                    sp &= 0xFFFFFFFFFFFFFFF8;
                    // 复制legacy su路径到用户栈
                    argv_cplen = compat_copy_to_user((void *)sp, legacy_su_path, sizeof(legacy_su_path));
                    if (argv_cplen > 0) {
                        // 修改argv[0]指向新的legacy su路径
                        int rc = set_user_arg_ptr(0, *uargv, 0, sp);
                        if (rc < 0) { // 修改整个argv失败
                            logkfi("call apd argv error, uid: %d, to_uid: %d, sctx: %s, rc: %d\n", uid, to_uid, sctx,
                                   rc);
                        }
                    }
                }
            }
            // 记录APD调用日志
            logkfi("call apd uid: %d, to_uid: %d, sctx: %s, cplen: %d, %d\n", uid, to_uid, sctx, cplen, argv_cplen);
        }
#endif // ANDROID
    } else if (!strcmp(SUPERCMD, filename)) {
        // 处理超级命令执行
        void handle_supercmd(char **__user u_filename_p, char **__user uargv);
        handle_supercmd(u_filename_p, uargv);
        return;
    }
}

// execve系统调用hook处理函数
// https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2107
// COMPAT_SYSCALL_DEFINE3(execve, const char __user *, filename,
// 	const compat_uptr_t __user *, argv,
// 	const compat_uptr_t __user *, envp)

// https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2087
// SYSCALL_DEFINE3(execve, const char __user *, filename, const char __user *const __user *, argv,
//                 const char __user *const __user *, envp)

static void before_execve(hook_fargs3_t *args, void *udata)
{
    // 获取系统调用参数指针
    void *arg0p = syscall_argn_p(args, 0);
    void *arg1p = syscall_argn_p(args, 1);
    // 调用通用execve处理函数
    handle_before_execve((char **)arg0p, (char **)arg1p, udata);
}

// execveat系统调用hook处理函数
// https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2114
// COMPAT_SYSCALL_DEFINE5(execveat, int, fd,
// 		       const char __user *, filename,
// 		       const compat_uptr_t __user *, argv,
// 		       const compat_uptr_t __user *, envp,
// 		       int,  flags)

// https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2095
// SYSCALL_DEFINE5(execveat, int, fd, const char __user *, filename, const char __user *const __user *, argv,
//                 const char __user *const __user *, envp, int, flags)
__maybe_unused static void before_execveat(hook_fargs5_t *args, void *udata)
{
    // 获取filename和argv参数指针（第1和第2个参数）
    void *arg1p = syscall_argn_p(args, 1);
    void *arg2p = syscall_argn_p(args, 2);
    // 调用通用execve处理函数
    handle_before_execve((char **)arg1p, (char **)arg2p, udata);
}

// 文件状态和访问权限检查系统调用的hook处理函数
// 用于处理SU文件路径伪装
// https://elixir.bootlin.com/linux/v6.1/source/fs/stat.c#L431
// SYSCALL_DEFINE4(newfstatat, int, dfd, const char __user *, filename,
// 		struct stat __user *, statbuf, int, flag)

// https://elixir.bootlin.com/linux/v6.1/source/fs/open.c#L492
// SYSCALL_DEFINE3(faccessat, int, dfd, const char __user *, filename, int, mode)

// https://elixir.bootlin.com/linux/v6.1/source/fs/open.c#L497
// SYSCALL_DEFINE4(faccessat2, int, dfd, const char __user *, filename, int, mode, int, flags)

// https://elixir.bootlin.com/linux/v6.1/source/fs/stat.c#L661
// SYSCALL_DEFINE5(statx,
// 		int, dfd, const char __user *, filename, unsigned, flags,
// 		unsigned int, mask,
// 		struct statx __user *, buffer)
static void su_handler_arg1_ufilename_before(hook_fargs6_t *args, void *udata)
{
    // 获取当前进程UID
    uid_t uid = current_uid();
    if (!is_su_allow_uid(uid)) return;

    // 获取文件名参数指针（第1个参数）
    char __user **u_filename_p = (char __user **)syscall_argn_p(args, 1);

    char filename[SU_PATH_MAX_LEN];
    // 从用户空间复制文件名
    int flen = compat_strncpy_from_user(filename, *u_filename_p, sizeof(filename));
    if (flen <= 0) return;

    // 如果访问的是当前SU路径，重定向到shell路径进行伪装
    if (!strcmp(current_su_path, filename)) {
        void *uptr = copy_to_user_stack(sh_path, sizeof(sh_path));
        if (uptr && !IS_ERR(uptr)) {
            *u_filename_p = uptr;
        } else {
            logkfi("su uid: %d, cp stack error: %d\n", uid, uptr);
        }
    }
}

// 设置应用模块排除列表
// uid: 要设置的用户ID
// exclude: 是否排除（1=排除，0=不排除）
// 返回值: 0=成功，<0=失败
int set_ap_mod_exclude(uid_t uid, int exclude)
{
    int rc = 0;
    if (exclude) {
        // 写入排除标志到内核存储
        rc = write_kstorage(exclude_kstorage_gid, uid, &exclude, 0, sizeof(exclude), false);
    } else {
        // 从排除列表中移除
        rc = remove_kstorage(exclude_kstorage_gid, uid);
    }
    return rc;
}
KP_EXPORT_SYMBOL(set_ap_mod_exclude);

// 获取应用模块排除状态
// uid: 要查询的用户ID
// 返回值: 1=排除，0=不排除
int get_ap_mod_exclude(uid_t uid)
{
    int exclude = 0;
    // 从内核存储读取排除标志
    int rc = read_kstorage(exclude_kstorage_gid, uid, &exclude, 0, sizeof(exclude), false);
    if (rc < 0) return 0;
    return exclude;
}
KP_EXPORT_SYMBOL(get_ap_mod_exclude);

// 列出所有应用模块排除列表中的UID
// uids: 输出缓冲区，存储UID列表
// len: 缓冲区长度
// 返回值: 实际返回的UID数量
int list_ap_mod_exclude(uid_t *uids, int len)
{
    long ids[len];
    // 从内核存储获取所有排除列表ID
    int cnt = list_kstorage_ids(exclude_kstorage_gid, ids, len, false);
    for (int i = 0; i < len; i++) {
        uids[i] = (uid_t)ids[i];
    }
    return cnt;
}
KP_EXPORT_SYMBOL(list_ap_mod_exclude);

// SU兼容性模块初始化函数
int su_compat_init()
{
    // 设置默认SU路径
    current_su_path = default_su_path;

    // 分配SU列表内核存储组
    su_kstorage_gid = try_alloc_kstroage_group();
    if (su_kstorage_gid != KSTORAGE_SU_LIST_GROUP) return -ENOMEM;

    // 分配排除列表内核存储组
    exclude_kstorage_gid = try_alloc_kstroage_group();
    if (exclude_kstorage_gid != KSTORAGE_EXCLUDE_LIST_GROUP) return -ENOMEM;

#ifdef ANDROID
    // 设置默认shell安全上下文
    if (!all_allow_sctx[0]) {
        strcpy(all_allow_sctx, ALL_ALLOW_SCONTEXT_MAGISK);
    }
    // 添加默认允许的UID（shell和root）
    su_add_allow_uid(2000, 0, all_allow_sctx);
    su_add_allow_uid(0, 0, all_allow_sctx);
#endif

    hook_err_t rc = HOOK_NO_ERR;

    // 获取补丁配置
    uint8_t su_config = patch_config->patch_su_config;
    bool enable = !!(su_config & PATCH_CONFIG_SU_ENABLE);
    bool wrap = !!(su_config & PATCH_CONFIG_SU_HOOK_NO_WRAP);
    log_boot("su config: %x, enable: %d, wrap: %d\n", su_config, enable, wrap);

    // if (!enable) return;

    // hook execve系统调用（64位）
    rc = hook_syscalln(__NR_execve, 3, before_execve, 0, (void *)0);
    log_boot("hook __NR_execve rc: %d\n", rc);

    // hook文件状态检查系统调用（64位）
    rc = hook_syscalln(__NR3264_fstatat, 4, su_handler_arg1_ufilename_before, 0, (void *)0);
    log_boot("hook __NR3264_fstatat rc: %d\n", rc);

    // hook文件访问权限检查系统调用（64位）
    rc = hook_syscalln(__NR_faccessat, 3, su_handler_arg1_ufilename_before, 0, (void *)0);
    log_boot("hook __NR_faccessat rc: %d\n", rc);

    // hook execve系统调用（32位兼容）
    // __NR_execve 11
    rc = hook_compat_syscalln(11, 3, before_execve, 0, (void *)1);
    log_boot("hook 32 __NR_execve rc: %d\n", rc);

    // hook文件状态检查系统调用（32位兼容）
    // __NR_fstatat64 327
    rc = hook_compat_syscalln(327, 4, su_handler_arg1_ufilename_before, 0, (void *)0);
    log_boot("hook 32 __NR_fstatat64 rc: %d\n", rc);

    // hook文件访问权限检查系统调用（32位兼容）
    //  __NR_faccessat 334
    rc = hook_compat_syscalln(334, 3, su_handler_arg1_ufilename_before, 0, (void *)0);
    log_boot("hook 32 __NR_faccessat rc: %d\n", rc);

    return 0;
}