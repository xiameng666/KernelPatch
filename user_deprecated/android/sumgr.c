// 超级用户权限管理器

#include "sumgr.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

#include "../supercall.h"

/**
 * 授予用户超级用户权限
 * @param key 超级调用密钥
 * @param uid 源用户ID
 * @param to_uid 目标用户ID
 * @param scontext SELinux安全上下文
 * @return 成功返回0，失败返回错误码
 */
int su_grant(const char *key, uid_t uid, uid_t to_uid, const char *scontext)
{
    struct su_profile profile = { 0 };
    profile.uid = uid;
    profile.to_uid = to_uid;
    if (scontext) {
        strncpy(profile.scontext, scontext, sizeof(profile.scontext) - 1);
    }
    profile.scontext[sizeof(profile.scontext) - 1] = '\0';
    int rc = sc_su_grant_uid(key, uid, &profile);
    return rc;
}

/**
 * 撤销用户的超级用户权限
 * @param key 超级调用密钥
 * @param uid 用户ID
 * @return 成功返回0，失败返回错误码
 */
int su_revoke(const char *key, uid_t uid)
{
    int rc = sc_su_revoke_uid(key, uid);
    return rc;
}

/**
 * 获取拥有超级用户权限的用户数量
 * @param key 超级调用密钥
 * @return 成功返回用户数量，失败返回错误码
 */
int su_nums(const char *key)
{
    int nums = sc_su_uid_nums(key);
    fprintf(stdout, "%d\n", nums);
    return 0;
}

/**
 * 列出所有拥有超级用户权限的用户ID
 * @param key 超级调用密钥
 * @return 成功返回0，失败返回错误码
 */
int su_list(const char *key)
{
    uid_t uids[256];
    int rc = sc_su_allow_uids(key, uids, sizeof(uids) / sizeof(uids[0]));
    if (rc > 0) {
        for (int i = 0; i < rc; i++) {
            fprintf(stdout, "%d\n", uids[i]);
        }
        return 0;
    }
    return rc;
}

/**
 * 获取指定用户的超级用户权限配置
 * @param key 超级调用密钥
 * @param uid 用户ID
 * @return 成功返回0，失败返回错误码
 */
int su_profile(const char *key, uid_t uid)
{
    struct su_profile profile = { 0 };
    long rc = sc_su_uid_profile(key, (uid_t)uid, &profile);
    if (rc < 0) return rc;
    fprintf(stdout, "uid: %d, to_uid: %d, scontext: %s\n", profile.uid, profile.to_uid, profile.scontext);
    return 0;
}

/**
 * 重置su命令路径
 * @param key 超级调用密钥
 * @param path 新的su命令路径
 * @return 成功返回0，失败返回错误码
 */
int su_reset_path(const char *key, const char *path)
{
    int rc = sc_su_reset_path(key, path);
    return rc;
}

/**
 * 获取当前su命令路径
 * @param key 超级调用密钥
 * @return 成功返回0，失败返回错误码
 */
int su_get_path(const char *key)
{
    char buf[SU_PATH_MAX_LEN];
    int rc = sc_su_get_path(key, buf, sizeof(buf));
    if (rc > 0) {
        fprintf(stdout, "%s\n", buf);
        return 0;
    }
    return rc;
}

extern const char program_name[];
extern const char *key;

/**
 * 显示使用说明
 * @param status 退出状态码
 */
void usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s help' for more information.\n", program_name);
    else {
        printf("Usage: %s <COMMAND> [ARG]...\n\n", program_name);
        fprintf(
            stdout,
            ""
            "Android Root permission manager command set.\n"
            "    The default command obtain a shell with the specified TO_UID and SCONTEXT is 'kp',\n"
            "    whose full PATH is '/system/bin/kp'. This can avoid conflicts with the existing 'su' command.\n"
            "    If you wish to modify this PATH, you can use the 'reset' command. \n"
            "\n"
            "help                              Print this help message. \n"
            "grant <UID> [TO_UID] [SCONTEXT]   Grant access permission to UID.\n"
            "revoke                            Revoke access permission to UID.\n"
            "num                               Get the number of uids with the aforementioned permissions.\n"
            "list                              List aforementioned uids.\n"
            "profile <UID>                     Get the profile of the uid configuration.\n"
            "reset <PATH>                      Reset '/system/bin/kp' to PATH. The length of PATH must be between 1-127.\n"
            "path                              Get current su PATH.\n"
            "");
    }
    exit(status);
}

/**
 * 超级用户管理器主函数
 * @param argc 参数个数
 * @param argv 参数数组
 * @return 执行结果
 */
int sumgr_main(int argc, char **argv)
{
    if (argc < 2) usage(EXIT_FAILURE);

    const char *scmd = argv[1];
    int cmd = -1;

    // 命令映射表
    struct
    {
        const char *scmd;
        int cmd;
    } cmd_arr[] = { { "grant", SUPERCALL_SU_GRANT_UID }, { "revoke", SUPERCALL_SU_REVOKE_UID },
                    { "num", SUPERCALL_SU_NUMS },        { "list", SUPERCALL_SU_LIST },
                    { "profile", SUPERCALL_SU_PROFILE }, { "reset", SUPERCALL_SU_RESET_PATH },
                    { "path", SUPERCALL_SU_GET_PATH },   { "help", 0 } };

    // 查找匹配的命令
    for (int i = 0; i < sizeof(cmd_arr) / sizeof(cmd_arr[0]); i++) {
        if (strcmp(scmd, cmd_arr[i].scmd)) continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    if (cmd < 0) usage(EXIT_FAILURE);
    if (cmd == 0) usage(EXIT_SUCCESS);

    uid_t uid = 0;
    uid_t to_uid = 0;
    const char *sctx = NULL;
    const char *path = NULL;

    // 根据命令执行相应操作
    switch (cmd) {
    case SUPERCALL_SU_GRANT_UID:  // 授予权限
        if (argc < 3) error(-EINVAL, 0, "uid does not exist");
        uid = (uid_t)atoi(argv[2]);
        if (argc >= 4) to_uid = (uid_t)atoi(argv[3]);
        if (argc >= 5) sctx = argv[4];
        return su_grant(key, uid, to_uid, sctx);
    case SUPERCALL_SU_REVOKE_UID:  // 撤销权限
        if (argc < 3) error(-EINVAL, 0, "uid does not exist");
        uid = (uid_t)atoi(argv[2]);
        return su_revoke(key, uid);
    case SUPERCALL_SU_NUMS:  // 获取数量
        return su_nums(key);
    case SUPERCALL_SU_LIST:  // 列出用户
        return su_list(key);
    case SUPERCALL_SU_PROFILE:  // 获取配置
        if (argc < 3) error(-EINVAL, 0, "uid does not exist");
        uid = (uid_t)atoi(argv[2]);
        return su_profile(key, uid);
    case SUPERCALL_SU_RESET_PATH:  // 重置路径
        if (argc < 3) error(-EINVAL, 0, "path does not exist");
        path = argv[2];
        return su_reset_path(key, path);
    case SUPERCALL_SU_GET_PATH:  // 获取路径
        return su_get_path(key);
    case 0:  // 帮助
        usage(EXIT_SUCCESS);
    default:
        usage(EXIT_FAILURE);
    }

    return 0;
}