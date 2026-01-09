/* KernelPatch模块示例 - 系统调用Hook演示 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <uapi/asm-generic/unistd.h>
#include <linux/uaccess.h>
#include <syscall.h>
#include <linux/string.h>
#include <kputils.h>
#include <asm/current.h>

//< 系统调用Hook演示模块标识信息
KPM_NAME("kmp-syscall-hook-demo");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("bmax121");
KPM_DESCRIPTION("KernelPatch Module System Call Hook Example");

// 全局变量：模块参数字符串
// 保存初始化时传入的参数，用于决定hook方式
const char *margs = 0;

// 全局变量：记录当前hook类型
// 用于退出时正确清理对应类型的hook
enum hook_type hook_type = NONE;

/**
 * @brief 进程ID类型枚举
 * @details 定义不同类型的进程标识符，与内核保持一致
 */
enum pid_type
{
    PIDTYPE_PID,    // 进程ID
    PIDTYPE_TGID,   // 线程组ID
    PIDTYPE_PGID,   // 进程组ID  
    PIDTYPE_SID,    // 会话ID
    PIDTYPE_MAX,    // 枚举边界值
};

// 前向声明：PID命名空间结构体
struct pid_namespace;

// 内核函数指针：用于获取任务进程ID
// 动态获取内核符号，避免硬编码地址
pid_t (*__task_pid_nr_ns)(struct task_struct *task, enum pid_type type, struct pid_namespace *ns) = 0;

/**
 * @brief Hook链0的前置回调：记录openat系统调用的详细信息
 * @details 演示如何获取系统调用参数并提取进程信息
 *          openat系统调用用于打开文件，有4个参数
 * 
 * @param args 系统调用参数结构体，包含4个参数
 * @param udata 用户自定义数据指针
 */
void before_openat_0(hook_fargs4_t *args, void *udata)
{
    // 提取openat系统调用的4个参数
    // syscall_argn宏用于安全地获取系统调用参数
    int dfd = (int)syscall_argn(args, 0);                       // 目录文件描述符
    const char __user *filename = (typeof(filename))syscall_argn(args, 1);  // 文件名指针
    int flag = (int)syscall_argn(args, 2);                      // 打开标志
    umode_t mode = (int)syscall_argn(args, 3);                  // 文件权限模式

    // 从用户空间复制文件名到内核缓冲区
    // 使用compat_strncpy_from_user确保跨架构兼容性
    char buf[1024];
    compat_strncpy_from_user(buf, filename, sizeof(buf));

    // 获取当前任务结构体和进程ID信息
    struct task_struct *task = current;  // current宏获取当前进程的task_struct
    pid_t pid = -1, tgid = -1;
    
    // 如果成功获取到__task_pid_nr_ns函数指针，则提取PID信息
    if (__task_pid_nr_ns) {
        pid = __task_pid_nr_ns(task, PIDTYPE_PID, 0);   // 获取进程ID
        tgid = __task_pid_nr_ns(task, PIDTYPE_TGID, 0); // 获取线程组ID
    }

    // 在本地存储中保存任务指针供后续hook使用
    // args->local.data0可用于在hook链之间传递数据
    args->local.data0 = (uint64_t)task;

    // 打印详细的openat调用信息
    // 包括进程信息、文件描述符、文件名、标志和权限
    pr_info("hook_chain_0 task: %llx, pid: %d, tgid: %d, openat dfd: %d, filename: %s, flag: %x, mode: %d\n", 
            task, pid, tgid, dfd, buf, flag, mode);
}

// 全局计数器：统计open操作次数
// 演示如何在hook中维护状态信息
uint64_t open_counts = 0;

/**
 * @brief Hook链1的前置回调：统计调用次数
 * @details 演示hook链的链式调用和数据传递
 *          可以在同一个系统调用上安装多个hook
 * 
 * @param args 系统调用参数结构体
 * @param udata 用户自定义数据指针，这里指向计数器
 */
void before_openat_1(hook_fargs4_t *args, void *udata)
{
    // 增加调用计数
    // udata指向open_counts变量
    uint64_t *pcount = (uint64_t *)udata;
    (*pcount)++;
    
    // 打印任务指针和当前计数
    // args->local.data0是前一个hook设置的任务指针
    pr_info("hook_chain_1 before openat task: %llx, count: %llx\n", args->local.data0, *pcount);
}

/**
 * @brief Hook链1的后置回调：记录调用结束
 * @details 在系统调用执行完成后被调用
 *          可以检查返回值或执行清理操作
 * 
 * @param args 系统调用参数结构体，包含返回值
 * @param udata 用户自定义数据指针
 */
void after_openat_1(hook_fargs4_t *args, void *udata)
{
    // 打印调用结束信息
    // 展示任务指针，表明调用已完成
    pr_info("hook_chain_1 after openat task: %llx\n", args->local.data0);
}

/**
 * @brief 系统调用Hook演示模块的初始化函数
 * @details 根据传入参数选择不同的hook方式：函数指针hook或内联hook
 *          演示了两种不同的系统调用拦截技术
 * 
 * @param args 初始化参数字符串，决定hook方式
 * @param event 事件类型字符串
 * @param reserved 保留参数
 * @return 返回0表示成功
 */
static long syscall_hook_demo_init(const char *args, const char *event, void *__user reserved)
{
    // 保存初始化参数
    margs = args;
    pr_info("kpm-syscall-hook-demo init ..., args: %s\n", margs);

    // 查找内核函数__task_pid_nr_ns的地址
    // 使用kallsyms_lookup_name动态获取内核符号地址
    __task_pid_nr_ns = (typeof(__task_pid_nr_ns))kallsyms_lookup_name("__task_pid_nr_ns");
    pr_info("kernel function __task_pid_nr_ns addr: %llx\n", __task_pid_nr_ns);

    // 检查是否有参数指定
    if (!margs) {
        pr_warn("no args specified, skip hook\n");
        return 0;
    }

    hook_err_t err = HOOK_NO_ERR;

    // 根据参数选择hook方式
    if (!strcmp("function_pointer_hook", margs)) {
        pr_info("function pointer hook ...");
        hook_type = FUNCTION_POINTER_CHAIN;
        
        // 安装函数指针hook链
        // fp_hook_syscalln用于hook系统调用，支持链式hook
        err = fp_hook_syscalln(__NR_openat, 4, before_openat_0, 0, 0);
        if (err) goto out;
        
        // 安装第二个hook，包含前置和后置回调
        err = fp_hook_syscalln(__NR_openat, 4, before_openat_1, after_openat_1, &open_counts);
        
    } else if (!strcmp("inline_hook", margs)) {
        pr_info("inline hook ...");
        hook_type = INLINE_CHAIN;
        
        // 安装内联hook
        // inline_hook_syscalln直接修改系统调用表或指令
        err = inline_hook_syscalln(__NR_openat, 4, before_openat_0, 0, 0);
        
    } else {
        pr_warn("unknown args: %s\n", margs);
        return 0;
    }

out:
    // 检查hook安装结果
    if (err) {
        pr_err("hook openat error: %d\n", err);
    } else {
        pr_info("hook openat success\n");
    }
    return 0;
}

/**
 * @brief 控制接口0，打印接收到的参数
 * @details 提供运行时控制功能，可用于动态配置hook行为
 * 
 * @param args 输入的控制命令字符串
 * @param out_msg 指向用户空间的输出缓冲区
 * @param outlen 输出缓冲区长度
 * @return 返回0表示成功
 */
static long syscall_hook_control0(const char *args, char *__user out_msg, int outlen)
{
    // 打印控制参数
    pr_info("syscall_hook control, args: %s\n", args);
    return 0;
}

/**
 * @brief 模块退出函数，清理所有hook
 * @details 根据安装时选择的hook类型，执行相应的清理操作
 *          确保系统调用表恢复到原始状态
 * 
 * @param reserved 保留参数
 * @return 返回0表示成功
 */
static long syscall_hook_demo_exit(void *__user reserved)
{
    pr_info("kpm-syscall-hook-demo exit ...\n");

    // 根据hook类型移除相应的hook
    if (hook_type == INLINE_CHAIN) {
        // 移除内联hook
        // 恢复原始的系统调用指令或表项
        inline_unhook_syscalln(__NR_openat, before_openat_0, 0);
        
    } else if (hook_type == FUNCTION_POINTER_CHAIN) {
        // 移除函数指针hook链
        // 按照安装时的相反顺序移除hook
        fp_unhook_syscalln(__NR_openat, before_openat_0, 0);
        fp_unhook_syscalln(__NR_openat, before_openat_1, after_openat_1);
        
    } else {
        // 无需清理，没有安装任何hook
    }
    return 0;
}

// 注册模块初始化函数
// KPM_INIT宏将syscall_hook_demo_init注册为模块加载时的回调
KPM_INIT(syscall_hook_demo_init);

// 注册控制接口0  
// 提供运行时控制能力
KPM_CTL0(syscall_hook_control0);

// 注册模块退出函数
// KPM_EXIT宏将syscall_hook_demo_exit注册为模块卸载时的回调
KPM_EXIT(syscall_hook_demo_exit);