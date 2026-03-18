/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * kpmctl.c — KernelPatch Module 命令行控制工具
 *
 * 用法:
 *   kpmctl <superkey> load <path> [args]     加载 KPM
 *   kpmctl <superkey> unload <name>          卸载 KPM
 *   kpmctl <superkey> ctl <name> <args>      发送控制命令
 *   kpmctl <superkey> list                   列出已加载模块
 *   kpmctl <superkey> info <name>            模块详情
 *   kpmctl <superkey> hello                  测试 KernelPatch 是否可用
 *
 * 也可以用环境变量 SUPERKEY 代替第一个参数:
 *   export SUPERKEY=your_key
 *   kpmctl load /sdcard/Download/demo-spawn.kpm "com.example.app"
 *
 * 编译:
 *   aarch64-none-elf-gcc 不行 (裸机工具链没有 libc)
 *   需要用 Android NDK 的 aarch64-linux-android-clang
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <stdint.h>

/* ========== 从 KernelPatch 复制的必要定义 ========== */

#define __NR_supercall 45

#define SUPERCALL_HELLO      0x1000
#define SUPERCALL_KPM_LOAD   0x1020
#define SUPERCALL_KPM_UNLOAD 0x1021
#define SUPERCALL_KPM_CONTROL 0x1022
#define SUPERCALL_KPM_NUMS   0x1030
#define SUPERCALL_KPM_LIST   0x1031
#define SUPERCALL_KPM_INFO   0x1032

#define SUPERCALL_HELLO_MAGIC 0x11581158

/* 版本: 必须和设备上的 KernelPatch 版本匹配 (>= 0x0a05 格式) */
#define KP_MAJOR 0
#define KP_MINOR 13
#define KP_PATCH 0

static inline long ver_and_cmd(const char *key, long cmd)
{
    uint32_t version_code = (KP_MAJOR << 16) + (KP_MINOR << 8) + KP_PATCH;
    return ((long)version_code << 32) | (0x1158 << 16) | (cmd & 0xFFFF);
}

/* ========== supercall 封装 ========== */

static long sc_hello(const char *key)
{
    return syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_HELLO));
}

static long sc_kpm_load(const char *key, const char *path, const char *args)
{
    return syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_LOAD),
                   path, args, NULL);
}

static long sc_kpm_unload(const char *key, const char *name)
{
    return syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_UNLOAD),
                   name, NULL);
}

static long sc_kpm_control(const char *key, const char *name,
                            const char *ctl_args, char *out_msg, long outlen)
{
    return syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_CONTROL),
                   name, ctl_args, out_msg, outlen);
}

static long sc_kpm_list(const char *key, char *buf, int buf_len)
{
    return syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_LIST),
                   buf, buf_len);
}

static long sc_kpm_info(const char *key, const char *name, char *buf, int buf_len)
{
    return syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_INFO),
                   name, buf, buf_len);
}

static long sc_kpm_nums(const char *key)
{
    return syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_NUMS));
}

/* ========== 命令实现 ========== */

static void usage(const char *prog)
{
    fprintf(stderr,
        "用法: %s [superkey] <command> [args...]\n"
        "\n"
        "命令:\n"
        "  hello                       测试 KernelPatch 连接\n"
        "  load <path> [kpm_args]      加载 KPM 模块\n"
        "  unload <name>               卸载 KPM 模块\n"
        "  ctl <name> <ctl_args>       发送控制命令\n"
        "  list                        列出已加载模块\n"
        "  info <name>                 模块详情\n"
        "\n"
        "superkey 可以用第一个参数传入, 或设置环境变量 SUPERKEY\n"
        "\n"
        "示例:\n"
        "  %s mykey load /sdcard/Download/demo-spawn.kpm \"com.tencent.mm\"\n"
        "  %s mykey ctl demo-spawn release\n"
        "  %s mykey unload demo-spawn\n"
        "  %s mykey list\n"
        "\n"
        "使用环境变量:\n"
        "  export SUPERKEY=mykey\n"
        "  %s load /sdcard/Download/demo-spawn.kpm \"com.tencent.mm\"\n"
        "  %s ctl demo-spawn release\n",
        prog, prog, prog, prog, prog, prog, prog);
}

static int is_command(const char *s)
{
    return strcmp(s, "hello") == 0 ||
           strcmp(s, "load") == 0 ||
           strcmp(s, "unload") == 0 ||
           strcmp(s, "ctl") == 0 ||
           strcmp(s, "list") == 0 ||
           strcmp(s, "info") == 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *key = NULL;
    int cmd_idx = 1;

    /* 判断第一个参数是 superkey 还是命令 */
    if (is_command(argv[1])) {
        /* 从环境变量获取 key */
        key = getenv("SUPERKEY");
        if (!key || !key[0]) {
            fprintf(stderr, "错误: 未提供 superkey\n");
            fprintf(stderr, "用法: %s <superkey> <command> ... 或设置 SUPERKEY 环境变量\n", argv[0]);
            return 1;
        }
        cmd_idx = 1;
    } else {
        key = argv[1];
        cmd_idx = 2;
    }

    if (cmd_idx >= argc) {
        usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[cmd_idx];

    /* hello */
    if (strcmp(cmd, "hello") == 0) {
        long ret = sc_hello(key);
        if (ret == SUPERCALL_HELLO_MAGIC) {
            printf("KernelPatch 已就绪 (magic=0x%lx)\n", ret);
            long nums = sc_kpm_nums(key);
            printf("已加载 %ld 个模块\n", nums);
            return 0;
        } else {
            fprintf(stderr, "KernelPatch 未响应 (ret=%ld, errno=%d)\n", ret, errno);
            return 1;
        }
    }

    /* load <path> [args] */
    if (strcmp(cmd, "load") == 0) {
        if (cmd_idx + 1 >= argc) {
            fprintf(stderr, "用法: load <path> [args]\n");
            return 1;
        }
        const char *path = argv[cmd_idx + 1];
        const char *args = (cmd_idx + 2 < argc) ? argv[cmd_idx + 2] : NULL;

        printf("加载: %s", path);
        if (args) printf("  参数: \"%s\"", args);
        printf("\n");

        long ret = sc_kpm_load(key, path, args);
        if (ret == 0) {
            printf("加载成功\n");
            return 0;
        } else {
            fprintf(stderr, "加载失败: ret=%ld, errno=%d\n", ret, errno);
            return 1;
        }
    }

    /* unload <name> */
    if (strcmp(cmd, "unload") == 0) {
        if (cmd_idx + 1 >= argc) {
            fprintf(stderr, "用法: unload <name>\n");
            return 1;
        }
        const char *name = argv[cmd_idx + 1];
        long ret = sc_kpm_unload(key, name);
        if (ret == 0) {
            printf("卸载成功: %s\n", name);
            return 0;
        } else {
            fprintf(stderr, "卸载失败: ret=%ld, errno=%d\n", ret, errno);
            return 1;
        }
    }

    /* ctl <name> <ctl_args> */
    if (strcmp(cmd, "ctl") == 0) {
        if (cmd_idx + 2 >= argc) {
            fprintf(stderr, "用法: ctl <name> <ctl_args>\n");
            return 1;
        }
        const char *name = argv[cmd_idx + 1];
        const char *ctl_args = argv[cmd_idx + 2];
        char out_msg[4096] = {0};

        long ret = sc_kpm_control(key, name, ctl_args, out_msg, sizeof(out_msg));
        if (out_msg[0]) {
            printf("%s", out_msg);
        }
        if (ret == 0) {
            printf("控制命令成功: %s %s\n", name, ctl_args);
            return 0;
        } else {
            fprintf(stderr, "控制命令失败: ret=%ld, errno=%d\n", ret, errno);
            return 1;
        }
    }

    /* list */
    if (strcmp(cmd, "list") == 0) {
        long nums = sc_kpm_nums(key);
        printf("已加载 %ld 个模块:\n", nums);

        char buf[4096] = {0};
        long ret = sc_kpm_list(key, buf, sizeof(buf));
        if (ret > 0) {
            printf("%s", buf);
        } else if (nums == 0) {
            printf("(无)\n");
        } else {
            fprintf(stderr, "列表获取失败: ret=%ld\n", ret);
        }
        return 0;
    }

    /* info <name> */
    if (strcmp(cmd, "info") == 0) {
        if (cmd_idx + 1 >= argc) {
            fprintf(stderr, "用法: info <name>\n");
            return 1;
        }
        const char *name = argv[cmd_idx + 1];
        char buf[4096] = {0};
        long ret = sc_kpm_info(key, name, buf, sizeof(buf));
        if (ret > 0) {
            printf("%s\n", buf);
            return 0;
        } else {
            fprintf(stderr, "获取模块信息失败: ret=%ld\n", ret);
            return 1;
        }
    }

    fprintf(stderr, "未知命令: %s\n", cmd);
    usage(argv[0]);
    return 1;
}
