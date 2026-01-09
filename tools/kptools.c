/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

/**
 * @file kptools.c
 * @brief KernelPatch工具集 - 内核镜像补丁操作的命令行工具
 * @details 提供内核镜像的补丁、解补丁、转储符号表、配置管理等功能
 * 支持超级密钥管理、额外模块嵌入、配置信息导出等高级功能
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "../version"  // 版本信息

#include "preset.h"    // 预设配置
#include "image.h"     // 镜像处理
#include "order.h"     // 字节序处理
#include "kallsym.h"   // 内核符号表
#include "patch.h"     // 补丁操作
#include "common.h"    // 通用工具函数
#include "kpm.h"       // KernelPatch模块

uint32_t version = 0;          // 工具版本号
const char *program_name = NULL; // 程序名称

/**
 * @brief 打印使用帮助信息
 * @details 显示完整的命令行参数说明和使用示例
 * @param argv 命令行参数数组
 */
void print_usage(char **argv)
{
    char *c =
        "内核镜像补丁工具. 版本: %x\n"
        "\n"
        "用法: %s COMMAND [选项...]\n"
        "\n"
        "命令:\n"
        "  -h, --help                       打印此帮助信息.\n"
        "  -v, --version                    打印版本号. 如果指定-k则打印kpimg版本.\n"

        "  -p, --patch                      使用指定的kpimg(-k)和超级密钥(-s)对内核镜像(-i)进行补丁或更新补丁.\n"
        "  -u, --unpatch                    解除内核镜像(-i)的补丁.\n"
        "  -r, --reset-skey                 重置已补丁镜像(-i)的超级密钥.\n"
        "  -d, --dump                       转储内核镜像(-i)的kallsyms信息.\n"
        "  -f, --flag                       转储内核镜像(-i)的ikconfig信息.\n"
        "  -l, --list                       如果指定(-i)则打印内核镜像的所有补丁信息.\n"
        "                                   如果指定(-M)则打印额外项信息.\n"
        "                                   如果指定(-k)则打印KernelPatch镜像信息.\n"

        "选项:\n"
        "  -i, --image PATH                 内核镜像路径.\n"
        "  -k, --kpimg PATH                 KernelPatch镜像路径.\n"
        "  -s, --skey KEY                   设置超级密钥并直接保存到boot.img中.\n"
        "  -S, --root-skey KEY              设置根超级密钥使用哈希验证，超级密钥可以动态更改.\n"
        "  -o, --out PATH                   补丁镜像输出路径.\n"
        "  -a  --addition KEY=VALUE         添加附加信息.\n"

        "  -K, --kpatch PATH                将kpatch可执行文件嵌入到补丁中.\n"

        "  -M, --embed-extra-path PATH      嵌入新的额外项.\n"
        "  -E, --embeded-extra-name NAME    保留并修改嵌入的额外项.\n"

        "  -T, --extra-type TYPE            设置前一个额外项的类型.\n"
        "  -N, --extra-name NAME            设置前一个额外项的名称.\n"
        "  -V, --extra-event EVENT          设置前一个额外项的触发事件.\n"
        "  -A, --extra-args ARGS            设置前一个额外项的参数.\n"
        "  -D, --extra-detach               从补丁中分离前一个额外项.\n"
        "\n";
    fprintf(stdout, c, version, program_name);
}

/**
 * @brief 主函数 - 处理命令行参数并执行相应的操作
 * @details 解析命令行参数，根据指定的操作执行相应的内核补丁功能
 * 包括补丁应用、解补丁、符号表转储、配置管理等
 * @param argc 参数个数
 * @param argv 参数数组
 * @return 执行结果，0表示成功，非0表示失败
 */
int main(int argc, char *argv[])
{
    version = (MAJOR << 16) + (MINOR << 8) + PATCH;  // 构建版本号
    program_name = argv[0];  // 设置程序名称

    // 定义长选项数组，支持GNU风格的长参数名
    struct option longopts[] = { { "help", no_argument, NULL, 'h' },
                                 { "version", no_argument, NULL, 'v' },

                                 { "patch", no_argument, NULL, 'p' },         // 应用补丁
                                 { "unpatch", no_argument, NULL, 'u' },       // 移除补丁
                                 { "resetkey", no_argument, NULL, 'r' },      // 重置密钥
                                 { "dump", no_argument, NULL, 'd' },          // 转储符号表
                                 { "flag", no_argument, NULL, 'f' },          // 转储配置标志
                                 { "list", no_argument, NULL, 'l' },          // 列出信息

                                 { "image", required_argument, NULL, 'i' },   // 内核镜像路径
                                 { "kpimg", required_argument, NULL, 'k' },   // KernelPatch镜像路径
                                 { "skey", required_argument, NULL, 's' },    // 超级密钥
                                 { "root-skey", required_argument, NULL, 'S' }, // 根超级密钥
                                 { "out", required_argument, NULL, 'o' },     // 输出路径
                                 { "addition", required_argument, NULL, 'a' }, // 附加信息

                                 { "embed-extra-path", required_argument, NULL, 'M' },     // 嵌入额外项路径
                                 { "embeded-extra-name", required_argument, NULL, 'E' },   // 嵌入额外项名称
                                 { "extra-type", required_argument, NULL, 'T' },          // 额外项类型
                                 { "extra-name", required_argument, NULL, 'N' },          // 额外项名称
                                 { "extra-event", required_argument, NULL, 'V' },         // 额外项触发事件
                                 { "extra-args", required_argument, NULL, 'A' },          // 额外项参数
                                 { 0, 0, 0, 0 } };
    char *optstr = "hvpurdfli:s:S:k:o:a:M:E:T:N:V:A:";  // 短选项字符串

    // 命令行参数变量初始化
    char *kimg_path = NULL;         // 内核镜像路径
    char *kpimg_path = NULL;        // KernelPatch镜像路径
    char *out_path = NULL;          // 输出路径
    char *superkey = NULL;          // 超级密钥
    bool root_skey = false;         // 是否为根密钥

    int additional_num = 0;         // 附加信息数量
    const char *additional[16] = { 0 };  // 附加信息数组（最多16个）

    // 额外配置相关变量
    int extra_config_num = 0;       // 额外配置数量
    extra_config_t *extra_configs = (extra_config_t *)malloc(sizeof(extra_config_t) * EXTRA_ITEM_MAX_NUM);
    memset(extra_configs, 0, sizeof(extra_config_t) * EXTRA_ITEM_MAX_NUM);
    extra_config_t *config = NULL;  // 当前配置项

    char cmd = '\0';                // 命令字符
    int opt = -1;                   // 当前选项
    int opt_index = -1;             // 选项索引

    // 使用getopt_long解析命令行参数
    while ((opt = getopt_long(argc, argv, optstr, longopts, &opt_index)) != -1) {
        switch (opt) {
        // 命令选项（不需要参数）
        case 'h':  // 显示帮助信息
        case 'v':  // 显示版本信息
        case 'p':  // 应用补丁操作
        case 'u':  // 解除补丁操作
        case 'r':  // 重置超级密钥
        case 'd':  // 转储内核符号表
        case 'f':  // 转储内核配置标志
        case 'l':  // 列出补丁信息
            cmd = opt;  // 设置当前执行的命令
            break;
        // 参数选项（需要参数）
        case 'i':  // 指定内核镜像文件路径
            kimg_path = optarg;
            break;
        case 'k':  // 指定KernelPatch镜像文件路径
            kpimg_path = optarg;
            break;
        case 'S':  // 设置根超级密钥（支持哈希验证）
            root_skey = true;
        case 's':  // 设置普通超级密钥
            superkey = optarg;
            break;
        case 'o':  // 指定输出文件路径
            out_path = optarg;
            break;
        case 'a':  // 添加附加信息（键值对格式）
            additional[additional_num++] = optarg;
            break;
        // 额外项配置选项
        case 'M':  // 嵌入新的额外项文件路径
            config = &extra_configs[extra_config_num++];
            config->is_path = true;  // 标记为文件路径类型
            config->path = optarg;
            break;
        case 'E':  // 修改已嵌入的额外项名称
            config = &extra_configs[extra_config_num++];
            config->is_path = false;  // 标记为名称类型
            config->name = optarg;
            break;
        case 'T':  // 设置额外项的类型（如KPM、脚本等）
            config->extra_type = extra_str_type(optarg);
            if (config->extra_type == EXTRA_TYPE_NONE) {
                tools_loge_exit("invalid extra type: %s\n", optarg);
            }
            break;
        case 'V':  // 设置额外项的触发事件
            config->set_event = optarg;
            break;
        case 'N':  // 设置额外项的名称标识
            config->name = optarg;
            break;
        case 'A':  // 设置额外项的运行参数
            config->set_args = optarg;
            break;
        default:
            break;  // 忽略未知选项
        }
    }
    int ret = 0;  // 返回值，0表示成功

    // 根据解析到的命令字符执行相应操作
    if (cmd == 'h') {
        print_usage(argv);  // 显示详细的使用帮助信息
    } else if (cmd == 'v') {
        // 显示版本信息
        if (kpimg_path)
            fprintf(stdout, "%x\n", get_kpimg_version(kpimg_path));  // 显示KernelPatch镜像版本
        else
            fprintf(stdout, "%x\n", version);  // 显示工具程序版本
    } else if (cmd == 'p') {
        // 执行内核镜像补丁操作或更新已有补丁
        ret = patch_update_img(kimg_path, kpimg_path, out_path, superkey, root_skey, additional, extra_configs,
                               extra_config_num);
    } else if (cmd == 'd') {
        ret = dump_kallsym(kimg_path);  // 转储并显示内核符号表信息
    } else if (cmd == 'f') {
        ret = dump_ikconfig(kimg_path);  // 转储并显示内核编译配置标志
    } else if (cmd == 'u') {
        ret = unpatch_img(kimg_path, out_path);  // 移除内核镜像中的所有补丁
    } else if (cmd == 'r') {
        ret = reset_key(kimg_path, out_path, superkey);  // 重置已补丁镜像的超级密钥
    } else if (cmd == 'l') {
        // 列出各种信息，根据提供的参数决定显示内容
        if (kimg_path) return print_image_patch_info_path(kimg_path);      // 显示镜像中的补丁信息
        if (config && config->path) return print_kpm_info_path(config->path);  // 显示KPM模块信息
        if (kpimg_path) return print_kp_image_info_path(kpimg_path);       // 显示KernelPatch镜像信息
    }

    else {
        print_usage(argv);  // 无效或缺失命令，显示使用帮助
    }

    free(extra_configs);  // 释放动态分配的额外配置数组内存

    return ret;  // 返回操作执行结果
}
