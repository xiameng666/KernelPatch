/* KernelPatch模块示例 - 内联Hook演示 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <log.h>
#include <compiler.h>
#include <kpmodule.h>
#include <hook.h>
#include <linux/printk.h>

//< 模块标识信息，用于KernelPatch内联hook演示
KPM_NAME("kpm-inline-hook-demo");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("bmax121");
KPM_DESCRIPTION("KernelPatch Module Inline Hook Example");

/**
 * @brief 不内联的加法函数，用作演示目标
 * @details __noinline标记确保函数不会被编译器内联优化
 *          这样才能对函数进行hook操作
 * 
 * @param a 第一个加数
 * @param b 第二个加数  
 * @return 返回两数之和
 */
int __noinline add(int a, int b)
{
    // 打印原始函数被调用的日志
    logkd("origin add called\n");
    
    // 执行加法操作
    int ret = a + b;
    return ret;
}

/**
 * @brief Hook之前的回调函数，打印函数参数
 * @details 在目标函数执行前被调用，可以查看和修改参数
 *          args结构体包含函数的参数信息
 * 
 * @param args 函数参数结构体，包含arg0、arg1等参数
 * @param udata 用户自定义数据，在hook时传入
 */
void before_add(hook_fargs2_t *args, void *udata)
{
    // 打印hook前的两个参数值
    // args->arg0和args->arg1分别对应函数的第一个和第二个参数
    logkd("before add arg0: %d, arg1: %d\n", (int)args->arg0, (int)args->arg1);
}

/**
 * @brief Hook之后的回调函数，打印参数和返回值，并修改返回值
 * @details 在目标函数执行后被调用，可以查看和修改返回值
 *          演示了如何拦截和修改函数的返回结果
 * 
 * @param args 函数参数结构体，包含参数和返回值
 * @param udata 用户自定义数据，在hook时传入
 */
void after_add(hook_fargs2_t *args, void *udata)
{
    // 打印hook后的参数和返回值
    // args->ret包含原始函数的返回值
    logkd("after add arg0: %d, arg1: %d, ret: %d\n", (int)args->arg0, (int)args->arg1, (int)args->ret);
    
    // 将返回值修改为100
    // 这样调用者收到的结果就被篡改了
    args->ret = 100;
}

/**
 * @brief 内联Hook演示模块的初始化函数
 * @details 演示完整的hook流程：安装前测试 -> 安装hook -> 安装后测试
 *          展示了hook如何拦截和修改函数行为
 * 
 * @param args 初始化参数字符串
 * @param event 事件类型字符串
 * @param reserved 保留参数
 * @return 返回0表示成功
 */
static long inline_hook_demo_init(const char *args, const char *event, void *__user reserved)
{
    logkd("kmp inline-hook-demo init\n");

    // 定义测试参数
    int a = 20;
    int b = 10;

    // 在hook之前调用add函数，显示正常结果
    // 此时应该返回30 (20+10)
    int ret = add(a, b);
    logkd("%d + %d = %d\n", a, b, ret);

    // 安装hook，绑定before和after回调函数
    // hook_wrap2用于hook有2个参数的函数
    // 第3个参数0是传递给回调函数的用户数据
    hook_err_t err = hook_wrap2((void *)add, before_add, after_add, 0);
    logkd("hook err: %d\n", err);

    // 在hook之后再次调用add函数，显示hook效果
    // 此时after_add会将返回值修改为100
    ret = add(a, b);
    logkd("%d + %d = %d\n", a, b, ret);

    return 0;
}

/**
 * @brief 控制接口0，打印接收到的参数
 * @details 简单的控制接口演示，可用于运行时配置模块
 * 
 * @param args 输入的控制命令字符串
 * @param out_msg 指向用户空间的输出缓冲区
 * @param outlen 输出缓冲区长度
 * @return 返回0表示成功
 */
static long inline_hook_control0(const char *args, char *__user out_msg, int outlen)
{
    // 打印接收到的控制参数
    pr_info("kpm control, args: %s\n", args);
    return 0;
}

/**
 * @brief 模块退出函数，清理hook并测试恢复情况
 * @details 演示如何正确清理hook，确保原始函数恢复正常
 *          这对于模块的安全卸载非常重要
 * 
 * @param reserved 保留参数
 * @return 返回0表示成功
 */
static long inline_hook_demo_exit(void *__user reserved)
{
    // 移除add函数上的hook
    // unhook函数会恢复原始函数的指令序列
    unhook((void *)add);

    // 定义测试参数
    int a = 20;
    int b = 10;

    // 再次调用add函数，验证hook已经被移除
    // 此时应该返回正常的30，证明hook已完全清理
    int ret = add(a, b);
    logkd("%d + %d = %d\n", a, b, ret);

    logkd("kpm inline-hook-demo exit\n");
}

// 注册模块初始化函数
// KPM_INIT宏将inline_hook_demo_init注册为模块加载时的回调
KPM_INIT(inline_hook_demo_init);

// 注册控制接口0
// 提供运行时控制功能
KPM_CTL0(inline_hook_control0);

// 注册模块退出函数
// KPM_EXIT宏将inline_hook_demo_exit注册为模块卸载时的回调
KPM_EXIT(inline_hook_demo_exit);