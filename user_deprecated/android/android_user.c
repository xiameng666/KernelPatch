// Android用户空间管理器

#include <fcntl.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

#include "../supercall.h"
#include "android_user.h"

#define PKG_NAME_LEN 256

// 允许的包信息结构
struct allow_pkg_info
{
    const char pkg[PKG_NAME_LEN];  // 包名
    uid_t uid;  // 用户ID
    uid_t to_uid;  // 目标用户ID
    const char sctx[SUPERCALL_SCONTEXT_LEN];  // SELinux上下文
};

// 文件路径定义
static char magiskpolicy_path[] = AP_BIN_DIR "magiskpolicy";
static char pkg_cfg_path[] = AP_DIR "package_config";
static char su_path_path[] = AP_DIR "su_path";

extern const char *key;
static bool from_kernel = false;  // 是否来自内核调用

/**
 * 去除字符串首尾空格
 * @param p 输入字符串
 * @return 去除空格后的字符串指针
 */
static char *trim(char *p)
{
    if (!p || !p[0]) return p;

    // 跳过开头的空格
    while (isspace(*p))
        p++;

    // 去除末尾的空格
    char *e = p + strlen(p) - 1;
    while (e > p && isspace(*e))
        *e-- = '\0';
    return p;
}

/**
 * 向内核日志记录信息
 * @param fmt 格式化字符串
 * @param ... 可变参数
 * @return 调用结果
 */
static int log_kernel(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return sc_klog(key, buf);
}

/**
 * 从CSV行中提取指定键的值
 * @param header CSV头部行
 * @param line CSV数据行
 * @param key 要提取的键名
 * @return 提取的值字符串，需要调用者释放内存
 */
static char *csv_val(const char *header, const char *line, const char *key)
{
    const char *kpos = strstr(header, key);
    if (!kpos) return 0;
    
    // 计算键在CSV中的列索引
    int kidx = 0;
    const char *c = 0;
    for (c = header; c < kpos; c++) {
        if (*c == ',') kidx++;
    }
    
    // 在数据行中找到对应列的值
    for (c = line; kidx; c++) {
        if (*c == ',') kidx--;
    }
    
    // 找到值的结束位置
    const char *e = c;
    for (; *e && *e != ','; e++) {
    };

    return strndup(c, e - c);
}

/**
 * 加载允许的UID配置
 * 从配置文件中读取包权限设置并应用到内核
 */
static void load_config_allow_uids()
{
    char linebuf[1024], header[1024] = { '\0' };
    char *line = 0;

    FILE *fallow = fopen(pkg_cfg_path, "r");
    if (fallow == NULL) {
        log_kernel("%d open %s error: %s\n", getpid(), pkg_cfg_path, strerror(errno));
        return;
    }

    // 如果从内核调用且文件存在，则移除默认配置
    if (from_kernel) sc_su_revoke_uid(key, 2000);

    // 读取CSV头部行
    fgets(header, sizeof(header) - 1, fallow);
    if (!strlen(header)) goto out;

    // 逐行处理配置数据
    while ((line = fgets(linebuf, sizeof(linebuf) - 1, fallow))) {
        line = trim(line);
        if (!line || line[0] == '#') continue;  // 跳过注释行
        log_kernel("pkg config line: %s\n", line);

        // 检查是否允许该包
        char *sallow = csv_val(header, line, "allow");
        if (!sallow) continue;

        if (!atol(sallow)) {  // 如果不允许则跳过
            free(sallow);
            continue;
        }

        // 提取配置参数
        char *spkg = csv_val(header, line, "pkg");
        char *suid = csv_val(header, line, "uid");
        char *sto_uid = csv_val(header, line, "to_uid");
        char *ssctx = csv_val(header, line, "sctx");

        if (!spkg || !suid || !sto_uid || !ssctx) continue;

        log_kernel("grant pkg: %s, uid: %s, to_uid: %s, sctx: %s\n", spkg, suid, sto_uid, ssctx);

        // 创建并设置权限配置
        uid_t to_uid = atol(sto_uid);
        struct su_profile profile = { 0 };
        profile.uid = atol(suid);
        profile.to_uid = to_uid;
        if (ssctx) strncpy(profile.scontext, ssctx, sizeof(profile.scontext) - 1);

        // 应用权限设置到内核
        sc_su_grant_uid(key, profile.uid, &profile);

        // 释放内存
        free(spkg);
        free(suid);
        free(sto_uid);
        free(ssctx);
    }

out:
    fclose(fallow);
}

/**
 * 加载su路径配置
 * 从配置文件中读取自定义su路径并设置
 */
static void load_config_su_path()
{
    FILE *file = fopen(su_path_path, "rb");
    if (file == NULL) {
        log_kernel("%d open %s error: %s\n", getpid(), su_path_path, strerror(errno));
        return;
    }
    
    char linebuf[SU_PATH_MAX_LEN] = { '\0' };
    char *path = fgets(linebuf, sizeof(linebuf), file);
    if (path) path = trim(path);
    if (path) sc_su_reset_path(key, path);  // 设置新的su路径
    fclose(file);
}

/**
 * 创建子进程执行命令并等待结果
 * @param exec 可执行文件路径
 * @param argv 命令参数数组
 */
static void fork_for_result(const char *exec, char *const *argv)
{
    char cmd[4096] = { '\0' };
    
    // 构建完整命令字符串用于日志
    for (int i = 0;; i++) {
        if (!argv[i]) break;
        strncat(cmd, argv[i], sizeof(cmd) - strlen(cmd) - 1);
        strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        log_kernel("%d fork %s error: %d\n", getpid(), exec, pid);
    } else if (pid == 0) {
        // 子进程：设置环境变量并执行命令
        setenv("KERNELPATCH", "true", 1);
        char kpver[16] = { '\0' }, kver[16] = { '\0' };
        sprintf(kpver, "%x", sc_kp_ver(key));
        setenv("KERNELPATCH_VERSION", kpver, 1);
        sprintf(kver, "%x", sc_k_ver(key));
        setenv("KERNEL_VERSION", kver, 1);
        setenv("SUPERKEY", key, 1);
        int rc = execv(exec, argv);
        log_kernel("%d exec %s error: %s\n", getpid(), cmd, strerror(errno));
    } else {
        // 父进程：等待子进程完成
        int status;
        wait(&status);
        log_kernel("%d wait %s status: 0x%x\n", getpid(), cmd, status);
    }
}

/**
 * 保存日志到文件
 * @param argv 执行的命令参数
 * @param file 输出文件路径
 */
static void save_log(char **argv, const char *file)
{
    pid_t pid = fork();

    if (pid < 0) {
        log_kernel("%d fork for dmesg error: %d\n", getpid(), pid);
    } else if (pid == 0) {
        // 子进程：重定向标准输出到文件并执行命令
        int fd = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
        dup2(fd, 1);  // 重定向stdout
        dup2(fd, 2);  // 重定向stderr
        close(fd);
        int rc = execv(argv[0], argv);
        log_kernel("%d save log > %s error: %s\n", getpid(), file, strerror(errno));
    } else {
        // 父进程：等待子进程完成
        int status;
        wait(&status);
        log_kernel("%d save log status: 0x%x\n", getpid(), status);
    }
}

/**
 * 保存dmesg日志到文件
 * @param file 输出文件路径
 */
static void save_dmegs(const char *file)
{
    char *dmesg_argv[] = {
        "/system/bin/dmesg",
        NULL,
    };
    save_log(dmesg_argv, file);
}

/**
 * 早期初始化阶段处理
 * Android启动早期阶段的初始化工作
 */
static void early_init()
{
    struct su_profile profile = { .uid = getuid() };
    sc_su(key, &profile);  // 获取su权限

    log_kernel("%d starting android user early-init\n", getpid());

    // 保存早期启动日志
    save_dmegs(EARLY_INIT_LOG_0);

    // TODO: 添加其他早期初始化操作

    save_dmegs(EARLY_INIT_LOG_1);
}

/**
 * post-fs-data阶段初始化
 * Android启动后文件系统挂载完成后的初始化工作
 */
static void post_fs_data_init()
{
    struct su_profile profile = { .uid = getuid() };
    sc_su(key, &profile);  // 获取su权限

    char current_exe[256] = { '\0' };
    readlink("/proc/self/exe", current_exe, sizeof(current_exe) - 1);

    log_kernel("%d starting android user post-fs-data-init, exec: %s\n", getpid(), current_exe);

    // 如果当前执行文件是开发路径，则复制到数据目录
    if (!strcmp(current_exe, KPATCH_DEV_PATH)) {
        char *const args[] = { "/system/bin/cp", "-f", current_exe, KPATCH_DATA_PATH, NULL };
        fork_for_result(args[0], args);
        return;
    }

    // 创建必要的目录
    if (access(AP_DIR, F_OK)) mkdir(AP_DIR, 0700);
    if (access(APATCH_LOG_FLODER, F_OK)) mkdir(APATCH_LOG_FLODER, 0700);

    // 复制早期初始化日志到日志目录
    char *log_args[] = { "/system/bin/cp", "-f", EARLY_INIT_LOG_0, APATCH_LOG_FLODER, NULL };
    fork_for_result(log_args[0], log_args);

    log_args[2] = EARLY_INIT_LOG_1;
    fork_for_result(log_args[0], log_args);

    // 执行magisk策略设置
    char *argv[] = { magiskpolicy_path, "--magisk", "--live", NULL };
    fork_for_result(magiskpolicy_path, argv);

    // 加载配置
    load_config_su_path();
    load_config_allow_uids();

    // 加载模块（待实现）

    log_kernel("%d finished android user post-fs-data-init.\n", getpid());
}

// 命令行选项定义
static struct option const longopts[] = {
    { "kernel", no_argument, NULL, 'k' },
    { NULL, 0, NULL, 0 },
};

/**
 * Android用户空间管理器主函数
 * @param argc 参数个数
 * @param argv 参数数组
 * @return 执行结果
 */
int android_user(int argc, char **argv)
{
    if (!sc_ready(key)) return -EFAULT;

    char *scmd = argv[1];
    if (scmd == NULL) return -EINVAL;

    // 解析命令行选项
    int optc;
    while ((optc = getopt_long(argc, argv, "k", longopts, NULL)) != -1) {
        switch (optc) {
        case 'k':
            from_kernel = true;  // 标记为内核调用
            break;
        default:
            break;
        }
    }

    // 根据命令执行相应操作
    if (!strcmp("early-init", scmd)) {
        early_init();
    } else if (!strcmp("post-fs-data-init", scmd)) {
        post_fs_data_init();
    } else if (!strcmp("post-fs-data", scmd) || !strcmp("services", scmd) || !strcmp("boot-completed", scmd)) {
        // TODO: 迁移到apd进程
        struct su_profile profile = {
            .uid = getuid(),
            .to_uid = 0,
            .scontext = ALL_ALLOW_SCONTEXT,
        };
        sc_su(key, &profile);

        // 启动apd进程处理后续阶段
        char *apd_argv[] = {
            APD_PATH,
            scmd,
            NULL,
        };

        fork_for_result(APD_PATH, apd_argv);

        // 创建日志目录
        if (access(APATCH_LOG_FLODER, F_OK)) mkdir(APATCH_LOG_FLODER, 0700);

        // 保存触发器执行的日志
        char log_path[256] = { '\0' };
        sprintf(log_path, APATCH_LOG_FLODER "trigger_%s.dmesg.log", scmd);
        save_dmegs(log_path);

    } else {
        log_kernel("invalid android user cmd: %s\n", scmd);
    }

    return 0;
}