/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 * 
 * KernelPatch SuperKey 管理器实现
 * 
 * 本文件实现了 KernelPatch 的超级密钥管理功能，包括：
 * - 版本信息查询
 * - 系统连通性测试 
 * - 内核版本查询
 * - 启动日志获取
 * - 超级密钥管理（获取、设置、根密钥验证）
 * 
 * 通过 supercall 机制与内核模块进行通信，实现用户空间与内核空间的交互。
 */

#include "kpatch.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/capability.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <error.h>

#include "supercall.h"

/**
 * 获取版本号
 * 将主版本号、次版本号、补丁版本号组合成32位版本代码
 * @return 版本代码 (MAJOR << 16) + (MINOR << 8) + PATCH
 */
uint32_t version()
{
    uint32_t version_code = (MAJOR << 16) + (MINOR << 8) + PATCH;
    return version_code;
}

/**
 * 测试与KernelPatch的连通性
 * 通过supercall发送hello请求，验证内核模块是否正常响应
 * @param key 超级密钥
 */
void hello(const char *key)
{
    long ret = sc_hello(key);
    if (ret == SUPERCALL_HELLO_MAGIC) {
        fprintf(stdout, "%s\n", SUPERCALL_HELLO_ECHO);  // 输出hello回应
    }
}

/**
 * 获取KernelPatch版本
 * @param key 超级密钥
 */
void kpv(const char *key)
{
    uint32_t kpv = sc_kp_ver(key);
    fprintf(stdout, "%x\n", kpv);  // 十六进制输出版本号
}

/**
 * 获取内核版本
 * @param key 超级密钥  
 */
void kv(const char *key)
{
    uint32_t kv = sc_k_ver(key);
    fprintf(stdout, "%x\n", kv);  // 十六进制输出版本号
}

/**
 * 获取启动日志
 * @param key 超级密钥
 */
/**
 * 获取启动日志
 * @param key 超级密钥
 */
void bootlog(const char *key)
{
    sc_bootlog(key);
}

/**
 * 触发内核panic（调试用）
 * @param key 超级密钥
 */
void panic(const char *key)
{
    sc_panic(key);
}

/**
 * 测试函数（当前为空实现）
 * @param key 超级密钥
 * @return 测试结果
 */
int __test(const char *key)
{
    // return __sc_test(key, 0, 0, 0);
    return 0;
}

extern const char program_name[];  // 程序名称
extern const char *key;            // 当前超级密钥

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
                        "KernelPatch SuperKey manager.\n"
                        "\n"
                        "help                           Print this help message. \n"
                        "get                            Print current superkey.\n"
                        "set <SUPERKEY>                 Set current superkey.\n"
                        "rootkey [enable|disable]       Whether to use hash to verify the root superkey.\n"
                        "");
    }
    exit(status);
}

/**
 * 超级密钥管理主函数
 * 处理超级密钥相关的各种操作，包括获取、设置和根密钥验证
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 执行结果状态码
 */
int skey_main(int argc, char **argv)
{
    if (argc < 2) usage(EXIT_FAILURE);  // 参数不足，显示使用帮助

    const char *scmd = argv[1];  // 获取子命令
    int cmd = -1;

    // 命令映射表
    struct
    {
        const char *scmd;  // 字符串命令
        int cmd;           // 对应的命令码
    } cmd_arr[] = {
        { "get", SUPERCALL_SKEY_GET },           // 获取当前超级密钥
        { "set", SUPERCALL_SKEY_SET },           // 设置新的超级密钥
        { "rootkey", SUPERCALL_SKEY_ROOT_ENABLE }, // 根密钥验证开关
        { "help", 0 },                           // 帮助信息
    };

    // 查找匹配的命令
    for (int i = 0; i < sizeof(cmd_arr) / sizeof(cmd_arr[0]); i++) {
        if (strcmp(scmd, cmd_arr[i].scmd)) continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    if (cmd < 0) usage(EXIT_FAILURE);  // 未知命令
    char out_buf[SUPERCALL_KEY_MAX_LEN] = { '\0' };  // 输出缓冲区

    switch (cmd) {
    case SUPERCALL_SKEY_GET:
        // 获取当前超级密钥
        sc_skey_get(key, out_buf, sizeof(out_buf));
        fprintf(stdout, "%s\n", out_buf);
        break;
    case SUPERCALL_SKEY_SET:
        // 设置新的超级密钥
        if (argc < 3) error(-EINVAL, 0, "no new superkey");
        const char *new_key = argv[2];
        return sc_skey_set(key, new_key);
    case SUPERCALL_SKEY_ROOT_ENABLE:
        // 控制根密钥哈希验证
        if (argc < 3) error(-EINVAL, 0, "no enable or disable specified");
        if (!strcmp("enable", argv[2])) {
            sc_skey_root_enable(key, true);   // 启用根密钥验证
        } else if (!strcmp("disable", argv[2])) {
            sc_skey_root_enable(key, false);  // 禁用根密钥验证
        } else {
            error(-EINVAL, 0, "no enable or disable specified");
        }
        break;
    case 0:
        usage(EXIT_SUCCESS);  // 显示帮助
    default:
        usage(EXIT_FAILURE);  // 无效命令
    }

    return 0;
}