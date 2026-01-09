/* KernelPatch超级命令处理模块 - 提供完整的命令系统接口 */

#include <kputils.h>
#include <stdarg.h>
#include <sucompat.h>
#include <linux/string.h>
#include <linux/syscall.h>
#include <ktypes.h>
#include <stdbool.h>
#include <uapi/scdefs.h>
#include <syscall.h>
#include <predata.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/ptrace.h>
#include <accctl.h>
#include <linux/slab.h>
#include <module.h>
#include <user_event.h>

/**
 * 将字符串复制到用户栈空间
 * 在用户栈上分配空间并复制字符串数据，确保8字节对齐
 * @param data 要复制的字符串数据
 * @param sp 用户栈指针，会被修改指向新分配的空间
 * @return 成功返回用户空间字符串指针，失败返回0
 */
static char *__user supercmd_str_to_user_sp(const char *data, uintptr_t *sp)
{
    int len = strlen(data) + 1;                     // 计算字符串长度（包含结束符）
    *sp -= len;                                     // 在栈上向下分配空间
    *sp &= 0xFFFFFFFFFFFFFFF8;                     // 8字节对齐
    int cplen = compat_copy_to_user((void *)*sp, data, len);  // 复制数据到用户空间
    if (cplen > 0) return (char *__user) * sp;     // 成功返回用户空间指针
    return 0;                                       // 失败返回NULL
}

/**
 * 重定向执行命令的文件名
 * 将用户要执行的命令路径替换为指定的命令路径
 * @param u_filename_p 指向用户空间文件名指针的指针
 * @param cmd 要执行的目标命令路径
 * @param sp 用户栈指针，用于栈空间分配
 */
static void supercmd_exec(char **__user u_filename_p, const char *cmd, uintptr_t *sp)
{
    int cplen = 0;
#if 1
    // 尝试直接在原位置替换文件名
    cplen = compat_copy_to_user(*u_filename_p, cmd, strlen(cmd) + 1);
#endif
    // 如果直接替换失败，使用栈空间分配新位置
    if (cplen <= 0) *u_filename_p = supercmd_str_to_user_sp(cmd, sp);
}

/**
 * 重定向为echo命令并格式化输出内容
 * 将当前执行重定向为echo命令，并设置格式化的输出内容作为参数
 * @param u_filename_p 指向用户空间文件名指针的指针
 * @param uargv 用户空间参数数组指针
 * @param sp 用户栈指针，用于栈空间分配
 * @param fmt 格式化字符串
 * @param ... 格式化参数
 */
static void supercmd_echo(char **__user u_filename_p, char **__user uargv, uintptr_t *sp, const char *fmt, ...)
{
    // 重定向执行文件为echo命令
    supercmd_exec(u_filename_p, ECHO_PATH, sp);

    // 格式化输出内容到缓冲区
    char buffer[4096];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    va_end(va);

    // 在用户栈上分配参数字符串
    const char *__user cmd = supercmd_str_to_user_sp(ECHO_PATH, sp);     // argv[0] - 命令名
    const char *__user argv1 = supercmd_str_to_user_sp(buffer, sp);     // argv[1] - 输出内容

    // 设置用户空间参数数组
    set_user_arg_ptr(0, *uargv, 0, (uintptr_t)cmd);    // 设置argv[0]
    set_user_arg_ptr(0, *uargv, 1, (uintptr_t)argv1);  // 设置argv[1]
    set_user_arg_ptr(0, *uargv, 2, 0);                 // 设置argv[2]为NULL（参数结束标记）
}

// KernelPatch超级命令帮助信息文本
static const char supercmd_help[] =
    ""
    "KernelPatch supercmd:\n"
    "Usage: truncate <superkey|su> [-uZc] [Command [[SubCommand]...]]\n"
    "superkey|su:                   Authentication.\n"
    "Options:\n"
    "  -u <UID>                     Change user id to UID.\n"
    "  -Z <SCONTEXT>                Change security context to SCONTEXT.\n"
    "\n"
    "Command:\n"
    "  help:                        Print this help message.\n"
    "  version:                     Print Kernel version and KernelPatch version.\n"
    "  buildtime:                   Print KernelPatch build time.\n "
    "    eg: 50a0a,a06 means kernel version 5.10.10, KernelPatch version 0.10.6.\n"
    "  -c <COMMAND> [...]:          Pass a single COMMAND to the default shell.\n"
    "  exec <PATH> [...]:           Execute command with full PATH.\n"
    "  sumgr <SubCommand> [...]:    SU permission manager\n"
    "    The default command obtain a shell with the specified TO_UID and SCONTEXT is 'kp',\n"
    "    whose full PATH is '/system/bin/kp'. This can avoid conflicts with the existing 'su' command.\n"
    "    If you wish to modify this PATH, you can use the 'reset' command.\n"
    "    SubCommand:\n"
    "      grant <UID> [TO_UID [SCONTEXT]]  Grant su permission to UID.\n"
    "      revoke                           Revoke su permission to UID.\n"
    "      num                              Get the number of uids with the aforementioned permissions.\n"
    "      list                             List all su allowed uids.\n"
    "      profile <UID>                    Get the profile of the uid configuration.\n"
    "      path [PATH]                      Get or Reset current su path. The length of PATH must 2-127.\n"
    "      sctx [SCONTEXT]                  Get or Reset current all allowed security context.\n"
#ifdef ANDROID
    "      exclude_list                     List all exclude UIDs.\n"
    "      exclude <UID> [1|0]              Get or Reset exclude policy for UID.\n"
#endif
    "  event <EVENT>                        Report EVENT.\n"
    "\n"
    "The command below requires superkey authentication.\n"
    "  module <SubCommand> [...]:   KernelPatch Module manager\n"
    "    SubCommand:\n"
    "      load <KPM_PATH> [KPM_ARGS]       Load module with KPM_PATH and KPM_ARGS.\n"
    "      ctl0 <KPM_NAME> <CTL_ARGS>       Control module named KPM_PATH with CTL_ARGS.\n"
    "      unload <KPM_NAME>                Unload module named KPM_NAME.\n"
    "      num                              Get the number of modules that have been loaded.\n"
    "      list                             List names of all loaded modules.\n"
    "      info <KPM_NAME>                  Get detailed information about module named KPM_NAME.\n"
    "  key <SubCommand> [...]:      Superkey manager\n"
    "    SubCommand:\n"
    "      key [SUPERKEY]:                  Get or Reset current superkey\n"
    "      hash <enable|disable>:           Whether to use hash to verify the root superkey.\n"
    "";

// 命令执行结果结构体
struct cmd_res
{
    const char *msg;        // 成功消息
    const char *err_msg;    // 错误消息
    int rc;                 // 返回码
};

/**
 * 处理SU管理器相关命令
 * 处理用户权限管理、路径设置、上下文配置等SU相关操作
 * @param u_filename_p 指向用户空间文件名指针的指针
 * @param carr 命令参数数组
 * @param buffer 用于格式化输出的缓冲区
 * @param buflen 缓冲区长度
 * @param cmd_res 命令结果结构体
 */
static void handle_cmd_sumgr(char **__user u_filename_p, const char **carr, char *buffer, int buflen,
                             struct cmd_res *cmd_res)
{
    const char *sub_cmd = carr[1];
    if (!sub_cmd) sub_cmd = "";

    if (!strcmp(sub_cmd, "grant")) {
        // 授予用户SU权限
        unsigned long long uid = 0, to_uid = 0;
        const char *scontext = "";
        if (!carr[2] || kstrtoull(carr[2], 10, &uid)) {
            sprintf(buffer, "illegal uid: %s", carr[2]);
            cmd_res->err_msg = buffer;
            return;
        }
        if (carr[3]) kstrtoull(carr[3], 10, &to_uid);      // 目标UID（可选）
        if (carr[4]) scontext = carr[4];                   // SELinux上下文（可选）
        su_add_allow_uid(uid, to_uid, scontext);           // 添加到允许列表
        sprintf(buffer, "grant %d, %d, %s", uid, to_uid, scontext);
        cmd_res->msg = buffer;
    } else if (!strcmp(sub_cmd, "revoke")) {
        // 撤销用户SU权限
        const char *suid = carr[2];
        unsigned long long uid;
        if (!suid || kstrtoull(suid, 10, &uid)) {
            sprintf(buffer, "illegal uid: %s\n", suid);
            cmd_res->err_msg = buffer;
            return;
        }
        su_remove_allow_uid(uid);                          // 从允许列表移除
        cmd_res->msg = suid;
    } else if (!strcmp(sub_cmd, "num")) {
        // 获取允许SU的UID数量
        int num = su_allow_uid_nums();
        sprintf(buffer, "%d", num);
        cmd_res->msg = buffer;
    } else if (!strcmp(sub_cmd, "list")) {
        // 列出所有允许SU的UID
        uid_t uids[128]; // 默认最大128个
        int offset = 0;
        buffer[0] = '\0';
        int num = su_allow_uids(0, uids, sizeof(uids) / sizeof(uids[0]));
        for (int i = 0; i < num; i++) {
            offset += sprintf(buffer + offset, "%d\n", uids[i]);
        };
        if (offset > 0) buffer[offset - 1] = '\0';         // 移除最后的换行符
        cmd_res->msg = buffer;

    } else if (!strcmp(sub_cmd, "profile")) {
        // 获取指定UID的配置信息
        unsigned long long uid;
        if (!carr[2] || kstrtoull(carr[2], 10, &uid)) {
            cmd_res->err_msg = "invalid uid";
            return;
        }
        struct su_profile profile;
        cmd_res->rc = su_allow_uid_profile(0, uid, &profile);
        if (cmd_res->rc) return;

        sprintf(buffer, "uid: %d, to_uid: %d, scontext: %s", profile.uid, profile.to_uid, profile.scontext);
        cmd_res->msg = buffer;

    } else if (!strcmp(sub_cmd, "path")) {
        // 获取或设置SU路径
        if (carr[2]) {
            cmd_res->rc = su_reset_path(carr[2]);          // 设置新路径
            if (cmd_res->rc) return;
            cmd_res->msg = carr[2];
            carr[2] = 0; // 标记不释放内存
        } else {
            cmd_res->msg = su_get_path();                  // 获取当前路径
        }
    } else if (!strcmp(sub_cmd, "sctx")) {
        // 获取或设置安全上下文
        if (carr[2]) {
            cmd_res->rc = set_all_allow_sctx(carr[2]);     // 设置新的安全上下文
            if (!cmd_res->rc) cmd_res->msg = carr[2];
        } else {
            cmd_res->msg = all_allow_sctx;                 // 获取当前安全上下文
        }
    }
#ifdef ANDROID
    else if (!strcmp(sub_cmd, "exclude")) {
        // 管理排除列表（Android特有）
        unsigned long long uid;
        if (!carr[2] || kstrtoull(carr[2], 10, &uid)) {
            cmd_res->err_msg = "invalid uid";
            return;
        } else {
            if (!carr[3]) {
                // 获取排除状态
                int exclude = get_ap_mod_exclude(uid);
                sprintf(buffer, "%d", exclude);
                cmd_res->msg = buffer;
            } else {
                // 设置排除状态
                if (carr[3][0] == '0') {
                    set_ap_mod_exclude(uid, 0);            // 移出排除列表
                    cmd_res->msg = "0";
                } else {
                    set_ap_mod_exclude(uid, 1);            // 加入排除列表
                    cmd_res->msg = "1";
                }
            }
        }
    } else if (!strcmp(sub_cmd, "exclude_list")) {
        // 列出所有排除的UID
        uid_t uids[128];
        int offset = 0;
        buffer[0] = '\0';
        int cnt = list_ap_mod_exclude(uids, sizeof(uids) / sizeof(uids[0]));
        if (cnt < 0) {
            cmd_res->rc = cnt;
        } else {
            for (int i = 0; i < cnt; i++) {
                offset += sprintf(buffer + offset, "%d\n", uids[i]);
            };
            if (offset > 0) buffer[offset - 1] = '\0';     // 移除最后的换行符
            cmd_res->msg = buffer;
        }
    }
#endif
    else {
        cmd_res->err_msg = "invalid subcommand";            // 无效的子命令
    }
}

/**
 * 处理需要超级密钥认证的命令
 * 包括密钥管理和模块管理等高权限操作
 * @param u_filename_p 指向用户空间文件名指针的指针
 * @param cmd 主命令名称
 * @param carr 命令参数数组
 * @param buffer 用于格式化输出的缓冲区
 * @param buflen 缓冲区长度
 * @param cmd_res 命令结果结构体
 */
static void handle_cmd_key_auth(char **__user u_filename_p, const char *cmd, const char **carr, char *buffer,
                                int buflen, struct cmd_res *cmd_res)
{
    if (!strcmp("key", cmd)) {
        // 超级密钥管理命令
        const char *sub_cmd = carr[1];
        if (!sub_cmd) sub_cmd = "";
        if (!strcmp("get", sub_cmd)) {
            // 获取当前超级密钥
            cmd_res->msg = get_superkey();
        } else if (!strcmp("set", sub_cmd)) {
            // 设置新的超级密钥
            const char *key = carr[2];
            if (!key) {
                cmd_res->err_msg = "invalid new key";
                return;
            }
            cmd_res->msg = key;
            reset_superkey(key);                            // 重置超级密钥
        } else if (!strcmp("hash", sub_cmd)) {
            // 控制是否使用哈希验证根超级密钥
            const char *able = carr[2];
            if (able && !strcmp("enable", able)) {
                cmd_res->msg = able;
                enable_auth_root_key(true);                 // 启用哈希验证
            } else if (able && !strcmp("disable", able)) {
                cmd_res->msg = able;
                enable_auth_root_key(false);                // 禁用哈希验证
            } else {
                cmd_res->err_msg = "invalid enable or disable";
                return;
            }
        } else {
            cmd_res->err_msg = "invalid subcommand";
            return;
        }
    } else if (!strcmp("module", cmd)) {
        // 模块管理命令
        const char *sub_cmd = carr[1];
        if (!sub_cmd) sub_cmd = "";
        if (!strcmp("num", sub_cmd)) {
            // 获取已加载模块数量
            int num = get_module_nums();
            sprintf(buffer, "%d\n", num);
            cmd_res->msg = buffer;
        } else if (!strcmp("list", sub_cmd)) {
            // 列出所有已加载模块
            list_modules(buffer, buflen);
            cmd_res->msg = buffer;
        } else if (!strcmp("load", sub_cmd)) {
            // 加载新模块
            const char *path = carr[2];
            if (!path) {
                cmd_res->err_msg = "invalid module path";
                return;
            }
            cmd_res->rc = load_module_path(path, carr[3], 0); // 加载指定路径的模块
            if (!cmd_res->rc) cmd_res->msg = path;
        } else if (!strcmp("ctl0", sub_cmd)) {
            // 控制模块（方法0）
            const char *name = carr[2];
            if (!name) {
                cmd_res->err_msg = "invalid module name";
                return;
            }
            const char *mod_args = carr[3];
            if (!mod_args) {
                cmd_res->err_msg = "invalid control arguments";
                return;
            }
            buffer[0] = '\0';
            cmd_res->rc = module_control0(name, mod_args, buffer, buflen); // 执行模块控制
            cmd_res->msg = buffer;
        } else if (!strcmp("ctl1", sub_cmd)) {
            // 控制模块（方法1）- 未实现
            cmd_res->err_msg = "not implement";
        } else if (!strcmp("unload", sub_cmd)) {
            // 卸载模块
            const char *name = carr[2];
            if (!name) {
                cmd_res->err_msg = "invalid module name";
                return;
            }
            cmd_res->rc = unload_module(name, 0);           // 卸载指定模块
            if (!cmd_res->rc) cmd_res->msg = name;
        } else if (!strcmp("info", sub_cmd)) {
            // 获取模块信息
            const char *name = carr[2];
            if (!name) {
                cmd_res->err_msg = "invalid module name";
                return;
            }
            int sz = get_module_info(name, buffer, buflen);  // 获取模块详细信息
            if (sz <= 0) cmd_res->rc = sz;
            cmd_res->msg = buffer;
        } else {
            cmd_res->err_msg = "invalid subcommand";
            return;
        }
    } else {
        cmd_res->err_msg = "invalid command";
        return;
    }
}

/**
 * 超级命令处理主函数
 * 解析命令参数，进行身份认证，并分发到相应的处理函数
 * @param u_filename_p 指向用户空间文件名指针的指针
 * @param uargv 用户空间参数数组指针
 */
void handle_supercmd(char **__user u_filename_p, char **__user uargv)
{
    int is_key_auth = 0;                                    // 超级密钥认证标志

    // 获取第一个参数（认证密钥）
    const char __user *p1 = get_user_arg_ptr(0, *uargv, 1);
    if (!p1 || IS_ERR(p1)) return;

    struct su_profile profile = { .to_uid = 0, .scontext = "" };

    // 进行身份认证
    char arg1[SUPER_KEY_LEN];
    if (compat_strncpy_from_user(arg1, p1, sizeof(arg1)) <= 0) return;

    if (!auth_superkey(arg1)) {
        // 超级密钥认证成功
        is_key_auth = 1;
    } else if (!strcmp("su", arg1)) {
        // SU认证模式
        uid_t uid = current_uid();
        if (!is_su_allow_uid(uid)) return;                  // 检查UID是否被允许
        su_allow_uid_profile(0, uid, &profile);             // 获取SU配置信息
    } else {
        return;                                             // 认证失败，直接返回
    }

#define SUPERCMD_ARGS_NO 16

    // 复制参数到内核空间
    const char *parr[SUPERCMD_ARGS_NO + 4] = { 0 };

    for (int i = 2; i < SUPERCMD_ARGS_NO; i++) {
        const char __user *ua = get_user_arg_ptr(0, *uargv, i);
        if (IS_ERR(ua)) break;
        const char *a = strndup_user(ua, 512);              // 从用户空间复制字符串
        if (IS_ERR(a)) break;
        parr[i] = a;
        // 遇到-c参数后停止解析（后续参数将传递给shell）
        if (a[0] == '-' && a[1] == 'c') break;
    }

    uint64_t sp = current_user_stack_pointer();             // 获取当前用户栈指针

    // 如果没有提供命令，直接启动shell
    if (!parr[2]) {
        supercmd_exec(u_filename_p, sh_path, &sp);          // 重定向到shell
        const char *__user argv1 = supercmd_str_to_user_sp(sh_path, &sp);
        set_user_arg_ptr(0, *uargv, 1, (uintptr_t)argv1);
        *uargv += 1 * 8;                                    // 调整参数数组指针
        commit_su(profile.to_uid, profile.scontext);        // 提交SU权限变更
        return;
    }

    int pi = 2;                                             // 参数索引，从第3个参数开始

    // 解析选项参数（连续的选项）
    while (pi < SUPERCMD_ARGS_NO) {
        const char *arg = parr[pi];
        if (!arg || arg[0] != '-') break;                   // 非选项参数，停止解析
        // 忽略-c参数（后续参数将传递给shell）
        if (arg[0] == '-' && arg[1] == 'c') break;
        char o = arg[1];                                    // 获取选项字符
        pi++;
        switch (o) {
        case 'u':
            // -u选项：设置目标UID
            if (parr[pi]) {
                unsigned long long to_uid = profile.to_uid;
                kstrtoull(parr[pi++], 10, &to_uid);         // 解析UID数值
                profile.to_uid = to_uid;
            } else {
                supercmd_echo(u_filename_p, uargv, &sp, "supercmd error: invalid to_uid");
                goto free;
            }
            break;
        case 'Z':
            // -Z选项：设置SELinux安全上下文
            if (parr[pi]) {
                strncpy(profile.scontext, parr[pi++], sizeof(profile.scontext) - 1);
                profile.scontext[sizeof(profile.scontext) - 1] = '\0';
            } else {
                supercmd_echo(u_filename_p, uargv, &sp, "supercmd error: invalid scontext\n");
                goto free;
            }
            break;
        default:
            break;
        }
    }

    commit_su(profile.to_uid, profile.scontext);            // 应用SU权限设置

    struct cmd_res cmd_res = { 0 };                         // 初始化命令结果

    char buffer[4096];
    buffer[0] = '\0';

    // 获取主命令
    const char **carr = parr + pi;                          // 指向命令参数数组
    const char *cmd = 0;

    if (pi < SUPERCMD_ARGS_NO - 1) {
        cmd = carr[0];                                      // 获取命令名
    } else {
        cmd_res.err_msg = "too many args\n";
        goto echo;
    }

    if (!cmd) {
        // 没有命令，启动shell
        supercmd_exec(u_filename_p, sh_path, &sp);
        *uargv += pi * 8;
        goto free;
    }

    // 分发命令处理
    if (!strcmp("help", cmd)) {
        // 显示帮助信息
        cmd_res.msg = supercmd_help;
    } else if (!strcmp("-c", cmd)) {
        // -c选项：传递命令给shell执行
        supercmd_exec(u_filename_p, sh_path, &sp);
        *uargv += (carr - parr - 1) * 8;
        goto free;
    } else if (!strcmp("exec", cmd)) {
        // exec命令：执行指定路径的程序
        if (!carr[1]) {
            cmd_res.err_msg = "invalid commmand path";
            goto echo;
        }
        supercmd_exec(u_filename_p, carr[1], &sp);          // 重定向到指定程序
        *uargv += (carr - parr + 1) * 8;
        goto free;
    } else if (!strcmp("version", cmd)) {
        // version命令：显示版本信息
        supercmd_echo(u_filename_p, uargv, &sp, "%x,%x", kver, kpver);
        goto free;
    } else if (!strcmp("buildtime", cmd)) {
        // buildtime命令：显示构建时间
        cmd_res.msg = get_build_time();
        goto echo;
    } else if (!strcmp("sumgr", cmd)) {
        // sumgr命令：SU管理器
        handle_cmd_sumgr(u_filename_p, carr, buffer, sizeof(buffer), &cmd_res);
    } else if (!strcmp("event", cmd)) {
        // event命令：报告用户事件
        if (carr[1]) {
            cmd_res.rc = report_user_event(carr[1], carr[2]);
            if (!cmd_res.rc) cmd_res.msg = "report success";
        } else {
            cmd_res.err_msg = "empty event";
        }
    } else if (!strcmp("bootlog", cmd)) {
        // bootlog命令：获取启动日志
        cmd_res.msg = get_boot_log();
    } else if (!strcmp("test", cmd)) {
        // test命令：执行测试函数
        void test();
        test();
        cmd_res.msg = "test done...";
    } else {
        // 需要超级密钥认证的命令
        if (is_key_auth) {
            handle_cmd_key_auth(u_filename_p, cmd, carr, buffer, sizeof(buffer), &cmd_res);
        } else {
            cmd_res.err_msg = "invalid command or a superkey is required";
        }
    }

echo:
    // 输出命令执行结果
    if (cmd_res.msg) supercmd_echo(u_filename_p, uargv, &sp, cmd_res.msg);
    if (cmd_res.rc) supercmd_echo(u_filename_p, uargv, &sp, "supercmd error code: %d", cmd_res.rc);
    if (cmd_res.err_msg) supercmd_echo(u_filename_p, uargv, &sp, "supercmd error message: %s", cmd_res.err_msg);

free:
    // 释放分配的参数字符串内存
    for (int i = 2; i < sizeof(parr) / sizeof(parr[0]); i++) {
        const char *a = parr[i];
        if (!a) continue;
        kfree(a);                                           // 释放从用户空间复制的字符串
    }
}
