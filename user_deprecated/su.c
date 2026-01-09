/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 超级用户切换实现 - 通过KernelPatch的supercall机制提供su功能

#include "su.h"

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>
#include <error.h>

#include "supercall.h"

// 程序退出状态码定义
enum
{
    EXIT_TIMEDOUT = 124,     /* 子进程执行超时 */
    EXIT_CANCELED = 125,     /* 执行前内部错误 */
    EXIT_CANNOT_INVOKE = 126, /* 程序找到但不可执行 */
    EXIT_ENOENT = 127        /* 找不到要执行的程序 */
};

// 根据编译环境设置默认值
#ifdef ANDROID
#define DEFAULT_SHELL "/system/bin/sh"  // Android默认shell
#define DEFAULT_PATH "/product/bin:/apex/com.android.runtime/bin:/system/bin:/odm/bin:/vendor/bin:/usr/bin"
#define DEFAULT_ROOT_PATH \
    AP_BIN_DIR            \
    ":" ADB_DIR           \
    ":/sbin:/system/sbin:/product/bin:/apex/com.android.runtime/bin:/system/bin:/system/xbin:/odm/bin:/vendor/bin:/vendor/xbin:/usr/bin:/user/sbin"

#else
#define DEFAULT_SHELL "/bin/sh"         // 标准Linux默认shell
#define DEFAULT_PATH ":/bin:/usr/bin"
#define DEFAULT_ROOT_PATH ":/usr/ucb:/bin:/usr/bin:/etc"
#endif

#define DEFAULT_USER "root"  // 默认目标用户
#define PROGRAM_NAME "su"    // 程序名称

// 函数声明
static void run_shell(char const *, char const *, char **, size_t);
extern const char program_name[];
extern const char *key;

int setns(int __fd, int __ns_type);
int unshare(int __flags);

/**
 * 提取路径的最后一个组件（文件名部分）
 * @param name 完整路径字符串
 * @return 指向最后一个路径组件的指针
 */
char *last_component(char const *name)
{
    char const *base = name;
    char const *p;
    bool last_was_slash = false;

    // 跳过开头的斜杠
    while (*base == '/')
        base++;

    // 遍历路径，找到最后一个非斜杠的组件
    for (p = base; *p; p++) {
        if (*p == '/')
            last_was_slash = true;
        else if (last_was_slash) {
            base = p;
            last_was_slash = false;
        }
    }

    return (char *)base;
}

/**
 * 向环境变量添加NAME=VAL，检查内存分配错误
 * @param name 环境变量名
 * @param val 环境变量值
 */
static void xsetenv(char const *name, char const *val)
{
    size_t namelen = strlen(name);
    size_t vallen = strlen(val);
    char *string = malloc(namelen + 1 + vallen + 1);  // 分配内存：name + '=' + val + '\0'
    strcpy(string, name);
    string[namelen] = '=';
    strcpy(string + namelen + 1, val);
    putenv(string);  // 设置环境变量
}

/**
 * 切换到指定进程的挂载命名空间
 * @param pid 目标进程ID
 * @return 成功返回0，失败返回错误码
 */
static int switch_mnt_ns(int pid)
{
    int rc = 0;
    char mnt[32];
    snprintf(mnt, sizeof(mnt), "/proc/%d/ns/mnt", pid);  // 构造挂载命名空间路径
    if ((rc = access(mnt, R_OK)) < 0) {
        error(0, errno, "access %s error\n", mnt);
        return rc;
    }
    int fd = open(mnt, O_RDONLY);
    if (fd < 0) {
        error(0, errno, "access %s\n", mnt);
        rc = fd;
        return rc;
    }
    // 切换到目标命名空间
    if ((rc = setns(fd, 0)) < 0) error(0, errno, "setns %d error\n", fd);
    close(fd);

    return rc;
}

/**
 * 设置进程的用户和组身份
 * @param uid 目标用户ID
 * @param gids 组ID数组
 * @param gids_num 组数量
 */
static void set_identity(uid_t uid, gid_t *gids, int gids_num)
{
    gid_t gid;
    if (gids_num > 0) {
        // 设置补充组
        if (setgroups(gids_num, gids)) error(EXIT_CANCELED, errno, "cannot set groups");
        gid = gids[0];  // 使用第一个组作为主组
    } else {
        gid = uid;  // 如果没有指定组，使用UID作为GID
    }
    // 设置实际、有效和保存的组ID
    if (setresgid(gid, gid, gid)) error(EXIT_CANCELED, errno, "cannot set gids");
    // 设置实际、有效和保存的用户ID
    if (setresuid(uid, uid, uid)) error(EXIT_CANCELED, errno, "cannot set uids");
}

/**
 * 运行shell或指定命令
 * @param shell shell程序路径
 * @param command 要执行的命令（可为NULL）
 * @param additional_args 额外的命令行参数
 * @param n_additional_args 额外参数的数量
 */
static void __attribute__((noreturn))
run_shell(char const *shell, char const *command, char **additional_args, size_t n_additional_args)
{
    // 计算总参数数量：程序名 + 可选的-c和命令 + 额外参数 + NULL结尾
    size_t n_args = 1 + 2 * !!command + n_additional_args + 1;
    char const **args = malloc(n_args * sizeof *args);
    size_t argno = 1;

    args[0] = last_component(shell);  // 设置程序名（去掉路径的文件名）
    if (command) {
        // 如果有命令，添加-c参数和命令字符串
        args[argno++] = "-c";
        args[argno++] = command;
    }
    // 复制额外的参数
    memcpy(args + argno, additional_args, n_additional_args * sizeof *args);
    args[argno + n_additional_args] = NULL;  // NULL结尾
    execv(shell, (char **)args);  // 执行shell

    {
        // 如果执行失败，设置相应的退出状态
        int exit_status = (errno == ENOENT ? -EXIT_ENOENT : EXIT_CANNOT_INVOKE);
        error(0, errno, "%s", shell);
        exit(exit_status);
    }
}

/**
 * 显示使用帮助信息
 * @param status 退出状态码
 */
static void usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s help' for more information.\n", program_name);
    else {
        fprintf(stdout, "Change the user id, group id and security context.\n"
                        "If USER not given, assume root.\n\n");
        fprintf(stdout, "Usage: %s [OPTION]... [USER [ARG]...]\n\n", program_name);
        fprintf(
            stdout,
            "-h, --help                         Print this help message. \n"
            "-c, --command=COMMAND              pass a single COMMAND to the shell with -c\n"
            "-m, -p, --preserve-environment     do not reset environment variables\n"
            "-g, --group GROUP                  Specify the primary group\n"
            "-G, --supp-group GROUP             Specify a supplementary group.\n"
            "                                       The first specified supplementary group is also used\n"
            "                                       as a primary group if the option -g is not specified.\n"
            "-t, --target PID                   PID to take mount namespace from\n "
            "-i, --target-isolate               Use new isolated namespace if -t is specified.\n "
            "-s, --shell SHELL                  use SHELL instead of the default\n"
            "-, -l, --login                     Pretend the shell to be a login shell\n"
            "-Z, --context SCONTEXT             Switch security context to SCONTEXT, If SCONTEXT is not specified\n"
            "                                   or specified with a non-existent value, bypass all selinux permission\n"
            "                                   checks for all calls initiated by this task using hooks, \n"
            "                                   but the permission determined by other task remain unchanged. \n"
            "-M, --mount-master                 force run in the global mount namespace\n"
            "");
    }
    exit(status);
}

static struct option const longopts[] = { { "command", required_argument, 0, 'c' },
                                          { "help", no_argument, 0, 'h' },
                                          { "login", no_argument, 0, 'l' },
                                          { "preserve-environment", no_argument, 0, 'p' },
                                          { "shell", required_argument, 0, 's' },
                                          { "version", no_argument, 0, 'v' },
                                          { "context", required_argument, 0, 'Z' },
                                          { "mount-master", no_argument, 0, 'M' },
                                          { "target", required_argument, 0, 't' },
                                          { "target-isolate", required_argument, 0, 'i' },
                                          { "group", required_argument, 0, 'g' },
                                          { "supp-group", required_argument, 0, 'G' },
                                          { 0, 0, 0, 0 },
                                          { NULL, 0, NULL, 0 } };

// 全局变量定义
uid_t uid = 0;              // 目标用户ID
bool login = false;         // 是否为登录shell
bool keepenv = false;       // 是否保持环境变量
bool isolated = false;      // 是否使用隔离命名空间
pid_t target = -1;          // 目标PID用于挂载命名空间

char *command = NULL;       // 要执行的命令
char *shell = NULL;         // 指定的shell
char *scontext = NULL;      // SELinux安全上下文

gid_t gids_num = 0;         // 补充组数量
gid_t gids[128] = { -1 };   // 补充组ID数组

const char *new_user = DEFAULT_USER;  // 目标用户，默认为root

/**
 * su命令主函数
 * 实现通过KernelPatch机制的超级用户切换
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 退出状态码
 */
int su_main(int argc, char **argv)
{
    int optc, c;

    struct passwd *pw;
    struct passwd pw_copy;

    pid_t origin_pid = getpid();  // 获取原始进程ID

    // 解析命令行参数
    while ((c = getopt_long(argc, argv, "c:hlmps:VvuZ:Mt:g:G:", longopts, 0)) != -1) {
        switch (c) {
        case 'c':
            command = optarg;  // 指定要执行的命令
            break;
        case 'h':
            usage(EXIT_SUCCESS);  // 显示帮助信息
        case 'l':
            login = true;  // 登录shell模式
            break;
        case 'm':
        case 'p':
            keepenv = true;  // 保持环境变量
            break;
        case 's':
            shell = optarg;  // 指定shell
            break;
        case 'Z':
            scontext = optarg;  // 指定SELinux安全上下文
            break;
        case 'M':  // 强制使用全局挂载命名空间
        case 't':  // 指定目标PID的挂载命名空间
            if (target != -1) {
                error(-EINVAL, 0, "Can't use -M and -t at the same time\n");
            }
            if (optarg == 0) {
                target = 0;  // 全局命名空间
            } else {
                target = atol(optarg);  // 解析PID
                if (*optarg == '-' || target == -1) {
                    error(-EINVAL, 0, "Invalid PID: %s\n", optarg);
                }
            }
            break;
        case 'i':
            isolated = true;  // 隔离命名空间模式
            break;
        case 'g':  // 主组
        case 'G':  // 补充组
            if (atol(optarg) >= 0) {
                if (gids_num >= sizeof(gids) / sizeof(gids[0])) break;  // 防止溢出
                gids[gids_num++] = atol(optarg);  // 添加组ID
            } else {
                error(-EINVAL, 0, "Invalid GID: %s\n", optarg);
            }
            break;
        default:
            usage(EXIT_FAILURE);  // 无效参数
        }
    }

    // 处理登录shell标识（"-"参数）
    if (optind < argc && strcmp(argv[optind], "-") == 0) {
        login = true;
        optind++;
    }

    // 获取目标用户名/UID
    if (optind < argc) new_user = argv[optind++];

    // 查找用户信息并获取UID
    pw = getpwnam(new_user);
    if (pw)
        uid = pw->pw_uid;  // 通过用户名获取UID
    else
        uid = atol(new_user);  // 直接解析为UID
    optind++;

    // 环境和shell设置
    if (!shell && keepenv) shell = getenv("SHELL");  // 保持环境时使用原shell
    if (!shell) shell = DEFAULT_SHELL;  // 使用默认shell

    // 通过KernelPatch进行超级用户切换
    struct su_profile profile = { 0 };
    profile.uid = getuid();      // 当前用户ID
    profile.to_uid = 0;          // 目标用户ID（root）
    if (scontext) strncpy(profile.scontext, scontext, sizeof(profile.scontext) - 1);  // 设置SELinux上下文
    if (sc_su(key, &profile)) error(-EACCES, 0, "incorrect super key");  // 执行supercall切换

    // 会话领导者设置（已注释）
    // setsid();

    // 命名空间处理
    if (target > 0) { // 使用指定PID的命名空间
        if (switch_mnt_ns(target)) {
            error(0, errno, "switch_mnt_ns failed, fallback to global\n");
        } else {
            if (isolated) { // 创建新的隔离命名空间
                if (unshare(CLONE_NEWNS) < 0) error(0, errno, "unshare");
                if (mount(0, "/", 0, MS_PRIVATE | MS_REC, 0) < 0) error(0, errno, "mount");
            }
        }
    }

    // 环境变量设置（如果不保持原环境）
    if (!keepenv) {
        xsetenv("HOME", pw->pw_dir);                                         // 设置家目录
        xsetenv("SHELL", shell);                                             // 设置shell
        xsetenv("PATH", pw->pw_uid ? DEFAULT_PATH : DEFAULT_ROOT_PATH);      // 设置PATH
        if (pw->pw_uid) {  // 非root用户时设置用户名
            xsetenv("USER", pw->pw_name);
            xsetenv("LOGNAME", pw->pw_name);
        }
    }

    // 设置用户身份（UID/GID）
    set_identity(uid, gids, gids_num);

    // 切换到用户家目录
    if (chdir(pw->pw_dir) != 0) error(0, errno, "cannot change directory: %s", pw->pw_dir);

    // 检查stderr错误状态
    if (ferror(stderr)) exit(EXIT_CANCELED);

    // 运行shell或指定命令
    run_shell(shell, command, argv + optind, argc - optind > 0 ?: 0);
}