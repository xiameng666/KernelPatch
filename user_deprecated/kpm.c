/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 * 
 * KernelPatch 模块管理器实现
 * 
 * 本文件实现了 KernelPatch 模块（KPM）的管理功能，包括：
 * - 模块加载：将 KPM 文件加载到内核中
 * - 模块控制：向已加载的模块发送控制命令
 * - 模块卸载：从内核中卸载指定模块
 * - 模块查询：获取模块列表、数量和详细信息
 * 
 * 通过 supercall 机制与内核模块管理器通信，实现用户空间的模块管理操作。
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>

#include "kpm.h"
#include "supercall.h"

/**
 * 加载 KernelPatch 模块
 * @param key 超级密钥
 * @param path 模块文件路径
 * @param args 模块参数
 * @return 0表示成功，负数表示错误码
 */
int kpm_load(const char *key, const char *path, const char *args)
{
    int rc = sc_kpm_load(key, path, args, 0);
    return rc;
}

/**
 * 控制已加载的 KernelPatch 模块
 * @param key 超级密钥
 * @param name 模块名称
 * @param ctl_args 控制参数
 * @return 0表示成功，负数表示错误码
 */
int kpm_control(const char *key, const char *name, const char *ctl_args)
{
    char buf[4096] = { '\0' };
    int rc = sc_kpm_control(key, name, ctl_args, buf, sizeof(buf));
    fprintf(stdout, "%s", buf);  // 输出模块的响应信息
    return rc;
}

/**
 * 卸载 KernelPatch 模块
 * @param key 超级密钥
 * @param name 模块名称
 * @return 0表示成功，负数表示错误码
 */
int kpm_unload(const char *key, const char *name)
{
    int rc = sc_kpm_unload(key, name, 0);
    return rc;
}

/**
 * 获取已加载模块的数量
 * @param key 超级密钥
 * @return 0表示成功
 */
int kpm_nums(const char *key)
{
    int nums = sc_kpm_nums(key);
    fprintf(stdout, "%d\n", nums);  // 输出模块数量
    return 0;
}

/**
 * 列出所有已加载的模块
 * @param key 超级密钥
 * @return 0表示成功，负数表示错误码
 */
int kpm_list(const char *key)
{
    char buf[4096];
    int rc = sc_kpm_list(key, buf, sizeof(buf));
    if (rc > 0) {
        fprintf(stdout, "%s", buf);  // 输出模块列表
        return 0;
    }
    return rc;
}

/**
 * 获取指定模块的详细信息
 * @param key 超级密钥
 * @param name 模块名称
 * @return 0表示成功，负数表示错误码
 */
int kpm_info(const char *key, const char *name)
{
    char buf[4096];
    int rc = sc_kpm_info(key, name, buf, sizeof(buf));
    if (rc > 0) {
        fprintf(stdout, "%s", buf);  // 输出模块详细信息
        return 0;
    }
    return rc;
}

extern const char program_name[];  // 程序名称
extern const char *key;            // 超级密钥

/**
 * 显示使用帮助信息
 * @param status 退出状态码
 */
static void usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s help' for more information.\n", program_name);
    else {
        printf("Usage: %s <COMMAND> [ARG]...\n\n", program_name);
        fprintf(stdout, ""
                        "KernelPatch Module command set.\n"
                        "\n"
                        "help                           Print this help message. \n"
                        "load <KPM_PATH> [KPM_ARGS]     Load KernelPatch Module with KPM_PATH and KPM_ARGS.\n"
                        "ctl0 <KPM_NAME> <CTL_ARGS>     Control KernelPatch Module named KPM_PATH with CTL_ARGS.\n"
                        "unload <KPM_NAME>              Unload KernelPatch Module named KPM_NAME.\n"
                        "num                            Get the number of modules that have been loaded.\n"
                        "list                           List names of all loaded modules.\n"
                        "info <KPM_NAME>                Get detailed information about module named KPM_NAME.\n"
                        "");
    }
    exit(status);
}

/**
 * KPM 模块管理主函数
 * 解析命令行参数并执行相应的模块管理操作
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 执行结果状态码
 */
int kpm_main(int argc, char **argv)
{
    if (argc < 2) usage(EXIT_FAILURE);  // 参数不足

    const char *scmd = argv[1];  // 获取子命令
    int cmd = -1;

    // 命令映射表
    struct
    {
        const char *scmd;  // 字符串命令
        int cmd;           // 对应的命令码
    } cmd_arr[] = {
        { "load", SUPERCALL_KPM_LOAD },       // 加载模块
        { "ctl0", SUPERCALL_KPM_CONTROL },    // 控制模块
        { "unload", SUPERCALL_KPM_UNLOAD },   // 卸载模块
        { "num", SUPERCALL_KPM_NUMS },        // 获取模块数量
        { "list", SUPERCALL_KPM_LIST },       // 列出模块
        { "info", SUPERCALL_KPM_INFO },       // 模块信息
        { "help", 0 },                        // 帮助信息
    };

    // 查找匹配的命令
    for (int i = 0; i < sizeof(cmd_arr) / sizeof(cmd_arr[0]); i++) {
        if (strcmp(scmd, cmd_arr[i].scmd)) continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    if (cmd < 0) usage(EXIT_FAILURE);  // 未知命令

    const char *path = NULL;      // 模块路径
    const char *mod_args = NULL;  // 模块参数
    const char *ctl_args = NULL;  // 控制参数
    const char *name = NULL;      // 模块名称

    switch (cmd) {
    case SUPERCALL_KPM_LOAD:
        // 加载模块：需要模块路径，模块参数可选
        if (argc < 3) error(-EINVAL, 0, "module path does not exist");
        path = argv[2];
        mod_args = argc < 4 ? NULL : argv[3];
        return kpm_load(key, path, mod_args);
    case SUPERCALL_KPM_CONTROL:
        // 控制模块：需要模块名称和控制参数
        if (argc < 3) error(-EINVAL, 0, "module name does not exist");
        if (argc < 4) error(-EINVAL, 0, "control argument does not exist");
        name = argv[2];
        ctl_args = argv[3];
        return kpm_control(key, name, ctl_args);
    case SUPERCALL_KPM_UNLOAD:
        // 卸载模块：需要模块名称
        if (argc < 3) error(-EINVAL, 0, "module name does not exist");
        name = argv[2];
        return kpm_unload(key, name);
    case SUPERCALL_KPM_NUMS:
        // 获取模块数量：无需额外参数
        return kpm_nums(key);
    case SUPERCALL_KPM_LIST:
        // 列出模块：无需额外参数
        return kpm_list(key);
    case SUPERCALL_KPM_INFO:
        // 获取模块信息：需要模块名称
        if (argc < 3) error(-EINVAL, 0, "module name does not exist");
        name = argv[2];
        return kpm_info(key, name);
    case 0:
        usage(EXIT_SUCCESS);  // 显示帮助
    default:
        usage(EXIT_FAILURE);  // 无效命令
    }

    return 0;
}