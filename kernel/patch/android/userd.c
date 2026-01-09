/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 * 
 * Android用户空间守护进程 - KernelPatch核心Android集成模块
 * 
 * 本模块实现了KernelPatch与Android系统的深度集成，主要功能包括：
 * 1. 进程监控：通过hook execve/execveat系统调用，监控关键进程启动
 * 2. RC文件重定向：自动修改Android init.rc配置文件，集成KernelPatch初始化流程
 * 3. 安全模式检测：通过监控音量键输入，检测Android安全模式启动
 * 4. 启动时序控制：在不同的Android启动阶段执行相应的初始化操作
 * 5. 文件操作支持：提供内核级别的文件读写功能
 * 
 * 核心机制：
 * - 通过hook系统调用实现对Android启动流程的透明干预
 * - 动态生成并注入初始化脚本到Android启动序列
 * - 实现用户空间守护进程的自动启动和管理
 */

#include <ktypes.h>
#include <hook.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <asm-generic/compat.h>
#include <uapi/asm-generic/errno.h>
#include <syscall.h>
#include <symbol.h>
#include <kconfig.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <taskob.h>
#include <predata.h>
#include <accctl.h>
#include <asm/current.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <syscall.h>
#include <kputils.h>
#include <linux/ptrace.h>
#include <predata.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/umh.h>
#include <uapi/scdefs.h>
#include <uapi/linux/stat.h>

// Android系统文件路径定义
#define ORIGIN_RC_FILE "/system/etc/init/atrace.rc"     // 原始RC文件路径（用于重定向）
#define REPLACE_RC_FILE "/dev/user_init.rc"             // 替换RC文件路径
#define ADB_FLODER "/data/adb/"                         // ADB数据目录
#define AP_DIR "/data/adb/ap/"                          // AP（Android Patch）目录
#define DEV_LOG_DIR "/dev/user_init_log/"               // 临时日志目录
#define AP_BIN_DIR AP_DIR "bin/"                        // AP二进制文件目录
#define AP_LOG_DIR AP_DIR "log/"                        // AP日志目录
#define AP_MAGISKPOLICY_PATH AP_BIN_DIR "magiskpolicy" // Magisk策略工具路径
#define MAGISK_SCTX "u:r:magisk:s0"                    // Magisk SELinux上下文
#define USER_INIT_SH_PATH "/dev/user_init.sh"          // 用户初始化脚本路径

#include "gen/user_init.c"                              // 包含生成的初始化脚本数据

// Android初始化RC文件模板 - 用于在不同启动阶段执行KernelPatch初始化
static const char user_rc_data[] = { //
    "\n"
    "on early-init\n"                                                                    // 早期初始化阶段
    "    exec -- " SUPERCMD " su exec " USER_INIT_SH_PATH " %s early-init\n"           // 执行早期初始化脚本
    "on init\n"                                                                         // 初始化阶段
    "    exec -- " SUPERCMD " su exec " USER_INIT_SH_PATH " %s init\n"                 // 执行初始化脚本
    "on late-init\n"                                                                    // 后期初始化阶段
    "    exec -- " SUPERCMD " su exec " USER_INIT_SH_PATH " %s late-init\n"            // 执行后期初始化脚本
    "on post-fs-data\n"                                                                 // 文件系统数据准备完成阶段
    "    exec -- " SUPERCMD " su exec " USER_INIT_SH_PATH " %s post-fs-data\n"         // 执行数据准备脚本
    "on nonencrypted\n"                                                                 // 非加密设备启动阶段
    "    exec -- " SUPERCMD " su exec " USER_INIT_SH_PATH " %s services\n"             // 启动服务
    "on property:vold.decrypt=trigger_restart_framework\n"                             // 解密完成重启框架时
    "    exec -- " SUPERCMD " su exec " USER_INIT_SH_PATH " %s services\n"             // 启动服务
    "on property:sys.boot_completed=1\n"                                               // 系统启动完成时
    "    exec -- " SUPERCMD " su exec " USER_INIT_SH_PATH " %s boot-completed\n"       // 执行启动完成脚本
    "    rm " REPLACE_RC_FILE "\n"                                                      // 清理临时RC文件
    "    rm " USER_INIT_SH_PATH "\n"                                                    // 清理临时初始化脚本
    "    exec -- " SUPERCMD " su -c \"mv -f " DEV_LOG_DIR " " AP_LOG_DIR "\"\n"        // 移动日志到永久目录
    ""
};

/**
 * 内核文件读取函数
 * 从指定路径读取文件内容到内存
 * @param path 文件路径
 * @param len 返回读取的文件长度
 * @return 文件内容指针，失败返回NULL
 */
static const void *kernel_read_file(const char *path, loff_t *len)
{
    set_priv_sel_allow(current, true);      // 临时提升权限
    void *data = 0;

    // 打开文件
    struct file *filp = filp_open(path, O_RDONLY, 0);
    if (!filp || IS_ERR(filp)) {
        log_boot("open file: %s error: %d\n", path, PTR_ERR(filp));
        goto out;
    }
    
    // 获取文件大小
    *len = vfs_llseek(filp, 0, SEEK_END);
    vfs_llseek(filp, 0, SEEK_SET);
    
    // 分配内存并读取文件内容
    data = vmalloc(*len);
    loff_t pos = 0;
    kernel_read(filp, data, *len, &pos);
    filp_close(filp, 0);

out:
    set_priv_sel_allow(current, false);     // 恢复权限
    return data;
}

/**
 * 内核文件写入函数
 * 将数据写入到指定路径的文件
 * @param path 文件路径
 * @param data 要写入的数据
 * @param len 数据长度
 * @param mode 文件权限模式
 * @return 实际写入的字节数
 */
static loff_t kernel_write_file(const char *path, const void *data, loff_t len, umode_t mode)
{
    loff_t off = 0;
    set_priv_sel_allow(current, true);      // 临时提升权限

    // 创建或打开文件进行写入
    struct file *fp = filp_open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (!fp || IS_ERR(fp)) {
        log_boot("create file %s error: %d\n", path, PTR_ERR(fp));
        goto out;
    }
    
    // 写入数据
    kernel_write(fp, data, len, &off);
    if (off != len) {
        log_boot("write file %s error: %x\n", path, off);
        goto free;
    }

free:
    filp_close(fp, 0);

out:
    set_priv_sel_allow(current, false);     // 恢复权限
    return off;
}

/**
 * 用户执行初始化预处理
 * 在第一次用户init进程执行前创建初始化脚本文件
 */
static void pre_user_exec_init()
{
    log_boot("event: %s\n", EXTRA_EVENT_PRE_EXEC_INIT);
    // 将内嵌的初始化脚本写入到临时文件中，设置可执行权限
    kernel_write_file(USER_INIT_SH_PATH, user_init, sizeof(user_init), 0700);
}

/**
 * init第二阶段预处理
 * 在Android init进程进入第二阶段前执行的预处理操作
 */
static void pre_init_second_stage()
{
    log_boot("event: %s\n", EXTRA_EVENT_PRE_SECOND_STAGE);
}

/**
 * 第一个app_process启动时的处理
 * 当Android系统启动第一个应用进程时执行
 */
static void on_first_app_process()
{
}

/**
 * execve系统调用前置处理函数
 * 监控关键进程的启动，在特定进程执行前进行干预
 * @param hook_local hook本地数据结构
 * @param u_filename_p 用户空间文件名指针
 * @param uargv 用户空间参数数组
 * @param uenvp 用户空间环境变量数组
 * @param udata 用户数据
 */
static void handle_before_execve(hook_local_t *hook_local, char **__user u_filename_p, char **__user uargv,
                                 char **__user uenvp, void *udata)
{
    // 初始化unhook标志为0
    hook_local->data7 = 0;

    // 关键Android进程路径定义
    static char app_process[] = "/system/bin/app_process";      // 32位app_process
    static char app_process64[] = "/system/bin/app_process64";  // 64位app_process
    static int first_app_process_execed = 0;                   // 第一个app_process执行标志

    static const char system_bin_init[] = "/system/bin/init";  // 系统init程序路径
    static const char root_init[] = "/init";                   // 根目录init程序路径
    static int first_user_init_executed = 0;                   // 第一次用户init执行标志
    static int init_second_stage_executed = 0;                 // init第二阶段执行标志

    char __user *ufilename = *u_filename_p;
    char filename[SU_PATH_MAX_LEN];
    // 从用户空间复制文件名
    int flen = compat_strncpy_from_user(filename, ufilename, sizeof(filename));
    if (flen <= 0) return;

    // 检测init进程启动
    if (!strcmp(system_bin_init, filename) || !strcmp(root_init, filename)) {
        // 第一次用户init执行时的处理
        if (!first_user_init_executed) {
            first_user_init_executed = 1;
            log_boot("exec first user init: %s\n", filename);
            pre_user_exec_init();                               // 执行用户初始化预处理
        }

        // 检测init第二阶段启动（通过命令行参数检测）
        if (!init_second_stage_executed) {
            for (int i = 1;; i++) {
                const char __user *p1 = get_user_arg_ptr(0, *uargv, i);
                if (!p1 || IS_ERR(p1)) break;

                char arg[16] = { '\0' };
                if (compat_strncpy_from_user(arg, p1, sizeof(arg)) <= 0) break;

                // 检查是否包含second_stage参数
                if (!strcmp(arg, "second_stage") || !strcmp(arg, "--second-stage")) {
                    log_boot("exec %s second stage 0\n", filename);
                    pre_init_second_stage();                    // 执行第二阶段预处理
                    init_second_stage_executed = 1;
                }
            }
        }

        // 通过环境变量检测init第二阶段
        if (!init_second_stage_executed) {
            for (int i = 0;; i++) {
                const char *__user uenv = get_user_arg_ptr(0, *uenvp, i);
                if (!uenv || IS_ERR(uenv)) break;

                char env[256];
                if (compat_strncpy_from_user(env, uenv, sizeof(env)) <= 0) break;
                char *env_name = env;
                char *env_value = strchr(env, '=');
                if (env_value) {
                    *env_value = '\0';
                    env_value++;
                    // 检查INIT_SECOND_STAGE环境变量
                    if (!strcmp(env_name, "INIT_SECOND_STAGE") &&
                        (!strcmp(env_value, "1") || !strcmp(env_value, "true"))) {
                        log_boot("exec %s second stage 1\n", filename);
                        pre_init_second_stage();                // 执行第二阶段预处理
                        init_second_stage_executed = 1;
                    }
                }
            }
        }
    }

    // 检测第一个app_process启动
    if (!first_app_process_execed && (!strcmp(app_process, filename) || !strcmp(app_process64, filename))) {
        first_app_process_execed = 1;
        log_boot("exec first app_process: %s\n", filename);
        on_first_app_process();                                 // 执行app_process启动处理
        hook_local->data7 = 1;                                  // 设置unhook标志
        return;
    }
}

// 系统调用hook函数声明
static void before_execve(hook_fargs3_t *args, void *udata);
static void after_execve(hook_fargs3_t *args, void *udata);
static void before_execveat(hook_fargs5_t *args, void *udata);
static void after_execveat(hook_fargs5_t *args, void *udata);

/**
 * execve系统调用后置处理函数
 * 根据标志决定是否取消hook
 * @param hook_local hook本地数据结构
 */
static void handle_after_execve(hook_local_t *hook_local)
{
    int unhook = hook_local->data7;
    if (unhook) {
        // 取消execve和execveat系统调用的hook
        unhook_syscalln(__NR_execve, before_execve, after_execve);
        unhook_syscalln(__NR_execveat, before_execveat, after_execveat);
    }
}

// execve系统调用hook实现
// 参考：https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2087
// SYSCALL_DEFINE3(execve, const char __user *, filename, const char __user *const __user *, argv,
//                 const char __user *const __user *, envp)

/**
 * execve系统调用前置hook函数
 * @param args 系统调用参数结构
 * @param udata 用户数据
 */
static void before_execve(hook_fargs3_t *args, void *udata)
{
    void *arg0p = syscall_argn_p(args, 0);     // 获取filename参数地址
    void *arg1p = syscall_argn_p(args, 1);     // 获取argv参数地址
    void *arg2p = syscall_argn_p(args, 2);     // 获取envp参数地址
    handle_before_execve(&args->local, (char **)arg0p, (char **)arg1p, (char **)arg2p, udata);
}

/**
 * execve系统调用后置hook函数
 * @param args 系统调用参数结构
 * @param udata 用户数据
 */
static void after_execve(hook_fargs3_t *args, void *udata)
{
    handle_after_execve(&args->local);
}

// execveat系统调用hook实现
// 参考：https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2095
// SYSCALL_DEFINE5(execveat, int, fd, const char __user *, filename, const char __user *const __user *, argv,
//                 const char __user *const __user *, envp, int, flags)

/**
 * execveat系统调用前置hook函数
 * @param args 系统调用参数结构
 * @param udata 用户数据
 */
static void before_execveat(hook_fargs5_t *args, void *udata)
{
    void *arg1p = syscall_argn_p(args, 1);     // 获取filename参数地址
    void *arg2p = syscall_argn_p(args, 2);     // 获取argv参数地址
    void *arg3p = syscall_argn_p(args, 3);     // 获取envp参数地址
    handle_before_execve(&args->local, (char **)arg1p, (char **)arg2p, (char **)arg3p, udata);
}

/**
 * execveat系统调用后置hook函数
 * @param args 系统调用参数结构
 * @param udata 用户数据
 */
static void after_execveat(hook_fargs5_t *args, void *udata)
{
    handle_after_execve(&args->local);
}

// openat系统调用hook实现 - 用于RC文件重定向
// 参考：https://elixir.bootlin.com/linux/v6.1/source/fs/open.c#L1337
// SYSCALL_DEFINE4(openat, int, dfd, const char __user *, filename, int, flags, umode_t, mode)

/**
 * openat系统调用前置hook函数
 * 主要用于拦截并重定向Android init.rc配置文件的访问
 * @param args 系统调用参数结构
 * @param udata 用户数据
 */
static void before_openat(hook_fargs4_t *args, void *udata)
{
    // 初始化本地数据
    args->local.data0 = 0;                      // 复制长度
    args->local.data1 = 0;                      // 复制指针
    args->local.data2 = 0;                      // unhook标志

    static int replaced = 0;                    // 防止重复替换的标志
    if (replaced) return;

    // 获取要打开的文件名
    const char __user *filename = (typeof(filename))syscall_argn(args, 1);
    char buf[32];
    long rc = compat_strncpy_from_user(buf, filename, sizeof(buf));
    if (rc <= 0) return;
    
    // 检查是否为目标RC文件
    if (strcmp(ORIGIN_RC_FILE, buf)) return;

    replaced = 1;                               // 标记已替换

    loff_t ori_len = 0;
    // 创建替换的RC文件
    struct file *newfp = filp_open(REPLACE_RC_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (!newfp || IS_ERR(newfp)) {
        log_boot("create replace rc error: %d\n", PTR_ERR(newfp));
        goto out;
    }

    loff_t off = 0;
    // 读取原始RC文件内容
    const char *ori_rc_data = kernel_read_file(ORIGIN_RC_FILE, &ori_len);
    if (!ori_rc_data) goto out;
    
    // 写入原始RC文件内容
    kernel_write(newfp, ori_rc_data, ori_len, &off);
    if (off != ori_len) {
        log_boot("write replace rc error: %x\n", off);
        goto free;
    }

    // 生成并添加KernelPatch初始化配置
    char added_rc_data[4096];
    const char *sk = get_superkey();            // 获取超级密钥
    sprintf(added_rc_data, user_rc_data, sk, sk, sk, sk, sk, sk, sk);

    // 写入KernelPatch配置
    kernel_write(newfp, added_rc_data, strlen(added_rc_data), &off);
    if (off != strlen(added_rc_data) + ori_len) {
        log_boot("write replace rc error: %x\n", off);
        goto free;
    }

    // 重定向文件名参数到新的RC文件
    int cplen = 0;
    cplen = compat_copy_to_user((void *)filename, REPLACE_RC_FILE, sizeof(REPLACE_RC_FILE));
    if (cplen > 0) {
        args->local.data0 = cplen;              // 记录复制长度
        args->local.data1 = (uint64_t)args->arg1;  // 记录原始参数地址
        log_boot("redirect rc file: %x\n", args->local.data0);
    } else {
        // 如果直接复制失败，使用栈空间
        void *__user up = copy_to_user_stack(REPLACE_RC_FILE, sizeof(REPLACE_RC_FILE));
        args->arg1 = (uint64_t)up;
        log_boot("redirect rc file stack: %llx\n", up);
    }

free:
    filp_close(newfp, 0);
    kvfree(ori_rc_data);

out:
    args->local.data2 = 1;                      // 设置unhook标志
    return;
}

/**
 * openat系统调用后置hook函数
 * 恢复原始文件名参数并取消hook
 * @param args 系统调用参数结构
 * @param udata 用户数据
 */
static void after_openat(hook_fargs4_t *args, void *udata)
{
    // 如果之前进行了文件名重定向，需要恢复原始文件名
    if (args->local.data0) {
        compat_copy_to_user((void *)args->local.data1, ORIGIN_RC_FILE, sizeof(ORIGIN_RC_FILE));
        log_boot("restore rc file: %x\n", args->local.data0);
    }
    
    // 如果设置了unhook标志，取消openat的hook
    if (args->local.data2) {
        unhook_syscalln(__NR_openat, before_openat, after_openat);
    }
}

// 输入设备事件相关定义
#define EV_KEY 0x01                             // 按键事件类型
#define KEY_VOLUMEDOWN 114                      // 音量减键键码

int android_is_safe_mode = 0;                   // Android安全模式标志
KP_EXPORT_SYMBOL(android_is_safe_mode);         // 导出符号供其他模块使用

/**
 * 输入设备事件处理函数hook
 * 监控音量减键按下事件，检测Android安全模式启动
 * 参考：void input_handle_event(struct input_dev *dev, unsigned int type, unsigned int code, int value)
 * @param args hook参数结构
 * @param udata 用户数据
 */
static void before_input_handle_event(hook_fargs4_t *args, void *udata)
{
    static unsigned int volumedown_pressed_count = 0;   // 音量减键按下计数
    unsigned int type = args->arg1;                     // 事件类型
    unsigned int code = args->arg2;                     // 事件代码
    int value = args->arg3;                             // 事件值
    
    // 检测音量减键按下事件
    if (value && type == EV_KEY && code == KEY_VOLUMEDOWN) {
        volumedown_pressed_count++;
        // 连续按下3次音量减键进入安全模式
        if (volumedown_pressed_count == 3) {
            log_boot("entering safemode ...");
            android_is_safe_mode = 1;                   // 设置安全模式标志
        }
    }
}

/**
 * Android用户初始化主函数
 * 设置所有必要的系统调用hook以实现Android集成功能
 * @return hook操作的错误码，0表示成功
 */
int android_user_init()
{
    hook_err_t ret = 0;
    hook_err_t rc = HOOK_NO_ERR;

    // hook execve系统调用，监控进程执行
    rc = hook_syscalln(__NR_execve, 3, before_execve, after_execve, (void *)__NR_execve);
    log_boot("hook __NR_execve rc: %d\n", rc);
    ret |= rc;

    // hook execveat系统调用，监控进程执行（AT_*版本）
    rc = hook_syscalln(__NR_execveat, 5, before_execveat, after_execveat, (void *)__NR_execveat);
    log_boot("hook __NR_execveat rc: %d\n", rc);
    ret |= rc;

    // hook openat系统调用，实现RC文件重定向
    rc = hook_syscalln(__NR_openat, 4, before_openat, after_openat, 0);
    log_boot("hook __NR_openat rc: %d\n", rc);
    ret |= rc;

    // hook输入事件处理函数，实现安全模式检测
    unsigned long input_handle_event_addr = patch_config->input_handle_event;
    if (input_handle_event_addr) {
        rc = hook_wrap4((void *)input_handle_event_addr, before_input_handle_event, 0, 0);
        ret |= rc;
        log_boot("hook input_handle_event rc: %d\n", rc);
    }

    return ret;
}
