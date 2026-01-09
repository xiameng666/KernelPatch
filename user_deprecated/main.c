/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 * 
 * KernelPatch 用户空间命令行工具（已废弃版本）
 * 
 * 功能说明：
 * - 提供与内核 KernelPatch 模块交互的用户空间接口
 * - 支持超级调用、权限提升、模块管理等功能
 * - 通过超级密钥进行安全验证
 * - 包含 Android 特定功能的支持
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <error.h>

#include "../banner"
#include "uapi/scdefs.h"
#include "kpatch.h"
#include "su.h"
#include "kpm.h"

#ifdef ANDROID
#include "android/sumgr.h"
#include "android/android_user.h"
#endif

char program_name[128] = { '\0' };    // 程序名称缓冲区
const char *key = NULL;               // 超级密钥指针

/**
 * 显示程序使用帮助信息
 * 
 * @param status 退出状态码
 * 
 * 功能说明：
 * - 根据状态码决定输出到 stderr 或 stdout
 * - 显示完整的命令用法和可用子命令
 * - 包含 Android 特定命令的条件显示
 * - 自动退出程序
 */
static void usage(int status)
{
    if (status != EXIT_SUCCESS) {
        fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
    } else {
        fprintf(stdout, "\nKernelPatch userspace cli.\n");
        fprintf(stdout, KERNEL_PATCH_BANNER);
        fprintf(stdout,
                " \n"
                "Options: \n"
                "%s -h, --help       Print this help message. \n"
                "%s -v, --version    Print version. \n"
                "\n",
                program_name, program_name);
        fprintf(stdout, "Usage: %s <COMMAND> [-h, --help] [COMMAND_ARGS]...\n", program_name);
        fprintf(stdout,
                "\n"
                "Commands:\n"
                "hello       If KernelPatch installed, '%s' will echoed.\n"
                "kpver       Print KernelPatch version.\n"
                "kver        Print Kernel version.\n"
                "key         Manager the superkey.\n"
                "su          KernelPatch Substitute User.\n"
                "kpm         KernelPatch Module manager.\n"
#ifdef ANDROID
                "sumgr       SU permission manager for Android.\n"
#endif
                "\n",
                SUPERCALL_HELLO_ECHO);
    }
    exit(status);
}

/**
 * 主函数 - KernelPatch 用户空间工具入口点
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出状态码
 * 
 * 功能说明：
 * - 解析命令行参数和超级密钥
 * - 分发到相应的子命令处理函数
 * - 验证超级密钥的有效性和长度
 * - 支持直接选项和子命令两种调用方式
 * - 提供调试和测试功能
 * 
 * TODO: 需要重构以改善代码结构
 */
int main(int argc, char **argv)
{
    // 构建程序名称
    strcat(program_name, argv[0]);

    // 如果没有参数，显示用法信息
    if (argc == 1) usage(EXIT_FAILURE);

    // 第一个参数作为超级密钥
    key = argv[1];
    strcat(program_name, " <SUPERKEY>");

    // 处理仅有两个参数的情况（直接选项）
    if (argc == 2) {
        if (!strcmp(argv[1], "-v") || !(strcmp(argv[1], "--version"))) {
            fprintf(stdout, "%x\n", version());
        } else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            usage(EXIT_SUCCESS);
        } else {
            usage(EXIT_FAILURE);
        }
        return 0;
    }

    // 验证超级密钥
    if (!key[0]) error(-EINVAL, 0, "invalid superkey");

    if (strnlen(key, SUPERCALL_KEY_MAX_LEN) >= SUPERCALL_KEY_MAX_LEN) error(-EINVAL, 0, "superkey too long");

    // 获取子命令
    const char *scmd = argv[2];
    int cmd = -1;

    // 命令映射表 - 将字符串命令映射到命令码
    struct
    {
        const char *scmd;    // 字符串命令
        int cmd;             // 命令码
    } cmd_arr[] = {
        // 基本超级调用命令
        { "hello", SUPERCALL_HELLO },
        { "kpver", SUPERCALL_KERNELPATCH_VER },
        { "kver", SUPERCALL_KERNEL_VER },
        
        // 子系统命令
        { "key", 'K' },      // 密钥管理
        { "su", 's' },       // 权限提升
        { "kpm", 'k' },      // 模块管理

        // 调试和测试命令
        { "bootlog", 'l' },  // 启动日志
        { "panic", '.' },    // 内核恐慌
        { "test", 't' },     // 测试功能

        // 帮助和版本信息
        { "--help", 'h' },
        { "-h", 'h' },
        { "--version", 'v' },
        { "-v", 'v' },
        
#ifdef ANDROID
        // Android 特定命令
        { "sumgr", 'm' },        // SU 权限管理器
        { "android_user", 'a' }, // Android 用户管理
#endif
    };

    // 查找匹配的命令
    for (int i = 0; i < sizeof(cmd_arr) / sizeof(cmd_arr[0]); i++) {
        if (strcmp(scmd, cmd_arr[i].scmd)) continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    // 如果命令无效，报错退出
    if (cmd < 0) error(-EINVAL, 0, "Invalid command: %s!\n", scmd);

    // 根据命令码分发到相应的处理函数
    switch (cmd) {
    case SUPERCALL_HELLO:
        // 测试 KernelPatch 是否正常工作
        hello(key);
        return 0;
    case SUPERCALL_KERNELPATCH_VER:
        // 获取 KernelPatch 版本信息
        kpv(key);
        return 0;
    case SUPERCALL_KERNEL_VER:
        // 获取内核版本信息
        kv(key);
        return 0;
    case 's':
        // 权限提升 (su) 功能
        strcat(program_name, " su");
        return su_main(argc - 2, argv + 2);
    case 'K':
        // 超级密钥管理
        strcat(program_name, " key");
        return skey_main(argc - 2, argv + 2);
    case 'k':
        // KernelPatch 模块管理
        strcat(program_name, " kpm");
        return kpm_main(argc - 2, argv + 2);
    case 'l':
        // 获取内核启动日志
        bootlog(key);
        break;
    case '.':
        // 触发内核恐慌（调试用）
        panic(key);
        break;
    case 't':
        // 执行测试功能
        __test(key);
        break;

    case 'h':
        // 显示帮助信息
        usage(EXIT_SUCCESS);
        break;
    case 'v':
        // 显示版本信息
        fprintf(stdout, "%x\n", version());
        break;

#ifdef ANDROID
    case 'm':
        // Android SU 权限管理器
        strcat(program_name, " sumgr");
        return sumgr_main(argc - 2, argv + 2);
    case 'a':
        // Android 用户管理功能
        return android_user(argc - 2, argv + 2);
#endif

    default:
        // 未知命令错误
        fprintf(stderr, "Invalid command: %s!\n", scmd);
        return -EINVAL;
    }

    return 0;
}
