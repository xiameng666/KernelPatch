/* KernelPatch模块示例 - Hello World演示 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>

//< 模块名称，每个KPM必须有唯一名称
//< 在系统中标识模块，用于卸载和管理
KPM_NAME("kpm-hello-demo");

//< 模块版本
//< 用于版本控制和兼容性检查
KPM_VERSION("1.0.0");

//< 许可证类型
//< 定义模块的开源许可证
KPM_LICENSE("GPL v2");

//< 作者
//< 模块开发者信息
KPM_AUTHOR("bmax121");

//< 模块描述
//< 简短的功能说明
KPM_DESCRIPTION("KernelPatch Module Example");

/**
 * @brief hello world模块初始化函数
 * @details 当模块加载时被调用，输出初始化信息
 *          在这里可以执行hook安装、资源分配等初始化操作
 * 
 * @param args 初始化参数字符串，由用户空间传入
 * @param event 事件类型字符串，表示加载原因
 * @param reserved 保留参数，指向用户空间的指针
 * @return 返回0表示成功，非0表示初始化失败
 */
static long hello_init(const char *args, const char *event, void *__user reserved)
{
    // 打印模块初始化信息，包含事件类型和参数
    // event可能是"load"、"boot"等值
    pr_info("kpm hello init, event: %s, args: %s\n", event, args);
    
    // 显示当前kernelpatch版本号
    // kpver是全局变量，包含KernelPatch的版本信息
    pr_info("kernelpatch version: %x\n", kpver);
    return 0;
}

/**
 * @brief 控制接口0：接收参数并回显给用户空间
 * @details 实现简单的字符串回显功能，用于演示用户空间与模块的通信
 *          参数通过args传入，结果通过out_msg返回
 * 
 * @param args 输入的控制命令字符串
 * @param out_msg 指向用户空间的输出缓冲区
 * @param outlen 输出缓冲区的长度
 * @return 返回0表示成功
 */
static long hello_control0(const char *args, char *__user out_msg, int outlen)
{
    // 打印接收到的控制参数，用于调试
    pr_info("kpm hello control0, args: %s\n", args);
    
    // 准备回显消息，前缀为"echo: "
    char echo[64] = "echo: ";
    
    // 将输入参数追加到回显消息中，最多48个字符
    // 防止缓冲区溢出，保留16字节给前缀
    strncat(echo, args, 48);
    
    // 将回显消息复制到用户空间
    // 使用compat_copy_to_user确保跨架构兼容性
    compat_copy_to_user(out_msg, echo, sizeof(echo));
    return 0;
}

/**
 * @brief 控制接口1：接收三个指针参数并打印其地址
 * @details 演示接收多个参数的控制接口，可用于传递复杂数据结构
 *          通常用于配置模块行为或传递大量数据
 * 
 * @param a1 第一个参数指针，可以是任意数据类型
 * @param a2 第二个参数指针，可以是任意数据类型  
 * @param a3 第三个参数指针，可以是任意数据类型
 * @return 返回0表示成功
 */
static long hello_control1(void *a1, void *a2, void *a3)
{
    // 以十六进制格式打印三个指针参数的地址
    // 用于验证参数传递的正确性
    pr_info("kpm hello control1, a1: %llx, a2: %llx, a3: %llx\n", a1, a2, a3);
    return 0;
}

/**
 * @brief 模块退出函数
 * @details 当模块卸载时被调用，执行清理工作
 *          在这里应该释放资源、卸载hook等
 * 
 * @param reserved 保留参数，指向用户空间
 * @return 返回0表示成功退出
 */
static long hello_exit(void *__user reserved)
{
    // 打印模块退出信息
    pr_info("kpm hello exit\n");
    return 0;
}

// 注册模块初始化函数
// KPM_INIT宏将hello_init注册为模块加载时的回调函数
KPM_INIT(hello_init);

// 注册控制接口0
// KPM_CTL0宏将hello_control0注册为控制接口，命令ID为0
KPM_CTL0(hello_control0);

// 注册控制接口1  
// KPM_CTL1宏将hello_control1注册为控制接口，命令ID为1
KPM_CTL1(hello_control1);

// 注册模块退出函数
// KPM_EXIT宏将hello_exit注册为模块卸载时的回调函数
KPM_EXIT(hello_exit);
