/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * demo-spawn.c — 最小 spawn 拦截 demo
 *
 * 功能:
 *   1. hook __set_task_comm, 检测目标包名
 *   2. 检测到 → SIGSTOP 冻结 → dmesg 输出 PID
 *   3. ctl "release" → SIGCONT 放行
 *
 * 用法:
 *   加载: kpatch load demo-spawn.kpm "com.example.app"
 *   放行: kpatch ctl demo-spawn release
 *   卸载: kpatch unload demo-spawn
 *
 * 原理:
 *   Android 上 zygote fork 子进程后, 调用 prctl(PR_SET_NAME) 设置进程名,
 *   最终走到内核的 __set_task_comm(task, name, exec).
 *   我们 hook 这个函数, 在 name 匹配目标包名时, 对该进程发 SIGSTOP.
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <hook.h>
#include <ksyms.h>
#include <stdint.h>

KPM_NAME("demo-spawn");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("kdbg");
KPM_DESCRIPTION("Demo: spawn detection — detect app launch, freeze, then release");

/* ========== pid_type (内核定义, KPM 无法直接引用) ========== */
enum pid_type {
    PIDTYPE_PID,
    PIDTYPE_TGID,
    PIDTYPE_PGID,
    PIDTYPE_SID,
    PIDTYPE_MAX,
};
struct pid_namespace;
typedef int pid_t;

/* ========== 内核函数指针 ========== */

/* void __set_task_comm(struct task_struct *tsk, const char *buf, int exec) */
static void (*kfn___set_task_comm)(void *tsk, const char *buf, int exec);

/* int send_sig(int sig, struct task_struct *p, int priv) */
static int (*kfn_send_sig)(int sig, void *p, int priv);

/* pid_t __task_pid_nr_ns(struct task_struct *task, enum pid_type type, struct pid_namespace *ns) */
static pid_t (*kfn___task_pid_nr_ns)(void *task, enum pid_type type, void *ns);

/* ========== 状态 ========== */

static char g_target[256];           /* 目标包名 */
static volatile int g_frozen = 0;    /* 是否已冻结 */
static int g_frozen_pid = 0;         /* 被冻结的进程 PID */
static void *g_frozen_task = 0;      /* 被冻结的 task_struct (用于 SIGCONT) */

/* ========== Hook 回调 ========== */

/*
 * __set_task_comm 的 before hook
 *
 * 参数映射 (hook_fargs3_t):
 *   arg0 = struct task_struct *tsk
 *   arg1 = const char *buf        ← 新的进程名
 *   arg2 = int exec
 */
static void before_set_task_comm(hook_fargs3_t *args, void *udata)
{
    /* 已经冻住了一个, 不再检测 */
    if (g_frozen)
        return;

    const char *name = (const char *)args->arg1;
    void *tsk = (void *)args->arg0;

    if (!name || !tsk)
        return;

    /* 检查是否包含目标包名 (用 strstr 子串匹配) */
    if (!strstr(g_target, name))
        return;

    /* 匹配! 获取 PID */
    pid_t pid = kfn___task_pid_nr_ns ? kfn___task_pid_nr_ns(tsk, PIDTYPE_TGID, 0) : -1;

    pr_info("demo-spawn: *** TARGET DETECTED ***\n");
    pr_info("demo-spawn:   name = '%s'\n", name);
    pr_info("demo-spawn:   pid  = %d\n", pid);

    /* SIGSTOP (19) 冻结进程 */
    if (kfn_send_sig) {
        kfn_send_sig(19, tsk, 1);
        pr_info("demo-spawn:   SIGSTOP sent → process FROZEN\n");
    } else {
        pr_warn("demo-spawn:   send_sig not found, cannot freeze!\n");
        return;
    }

    g_frozen_pid = pid;
    g_frozen_task = tsk;
    g_frozen = 1;

    pr_info("demo-spawn: use 'ctl release' to SIGCONT and resume\n");
}

/* ========== KPM 生命周期 ========== */

static long demo_init(const char *kpm_args, const char *event, void *__user reserved)
{
    pr_info("demo-spawn: ==========================================\n");
    pr_info("demo-spawn:   Spawn Detection Demo v1.0\n");
    pr_info("demo-spawn: ==========================================\n");

    /* 解析目标包名 */
    if (!kpm_args || !kpm_args[0]) {
        pr_err("demo-spawn: no target package specified!\n");
        pr_err("demo-spawn: usage: load demo-spawn.kpm \"com.example.app\"\n");
        return -1;
    }

    int len = strlen(kpm_args);
    if (len >= (int)sizeof(g_target))
        len = sizeof(g_target) - 1;
    memcpy(g_target, kpm_args, len);
    g_target[len] = 0;

    pr_info("demo-spawn: target = '%s'\n", g_target);

    /* 解析内核符号 */
    kfn___set_task_comm = (typeof(kfn___set_task_comm))
        kallsyms_lookup_name("__set_task_comm");
    kfn_send_sig = (typeof(kfn_send_sig))
        kallsyms_lookup_name("send_sig");
    kfn___task_pid_nr_ns = (typeof(kfn___task_pid_nr_ns))
        kallsyms_lookup_name("__task_pid_nr_ns");

    if (!kfn___set_task_comm) {
        pr_err("demo-spawn: __set_task_comm not found!\n");
        return -1;
    }
    if (!kfn_send_sig) {
        pr_warn("demo-spawn: send_sig not found, freeze will not work\n");
    }
    if (!kfn___task_pid_nr_ns) {
        pr_warn("demo-spawn: __task_pid_nr_ns not found, PID will show -1\n");
    }

    /* Hook __set_task_comm (3 个参数) */
    hook_err_t err = hook_wrap3(
        (void *)kfn___set_task_comm,
        (hook_chain3_callback)before_set_task_comm,
        0,  /* no after */
        0   /* no udata */
    );
    if (err) {
        pr_err("demo-spawn: hook __set_task_comm failed: %d\n", err);
        return -1;
    }

    pr_info("demo-spawn: __set_task_comm hooked, waiting for '%s' to launch...\n", g_target);
    return 0;
}

static long demo_control(const char *ctl_args, char *__user out_msg, int outlen)
{
    if (!ctl_args)
        return 0;

    if (!strcmp(ctl_args, "release")) {
        if (!g_frozen) {
            pr_info("demo-spawn: nothing frozen, nothing to release\n");
            return 0;
        }

        /* SIGCONT (18) 恢复进程 */
        if (kfn_send_sig && g_frozen_task) {
            kfn_send_sig(18, g_frozen_task, 1);
            pr_info("demo-spawn: SIGCONT sent → pid=%d RESUMED\n", g_frozen_pid);
        }

        g_frozen = 0;
        g_frozen_task = 0;
        g_frozen_pid = 0;

        pr_info("demo-spawn: listening again for '%s'...\n", g_target);
        return 0;
    }

    if (!strcmp(ctl_args, "status")) {
        pr_info("demo-spawn: target='%s' frozen=%d pid=%d\n",
                g_target, g_frozen, g_frozen_pid);
        return 0;
    }

    pr_info("demo-spawn: unknown command '%s' (try: release, status)\n", ctl_args);
    return 0;
}

static long demo_exit(void *__user reserved)
{
    /* 先放行被冻结的进程 */
    if (g_frozen && kfn_send_sig && g_frozen_task) {
        kfn_send_sig(18, g_frozen_task, 1);
        pr_info("demo-spawn: auto-release pid=%d on unload\n", g_frozen_pid);
    }

    /* 解除 hook */
    hook_unwrap((void *)kfn___set_task_comm,
                (void *)before_set_task_comm, 0);

    pr_info("demo-spawn: unloaded\n");
    return 0;
}

KPM_INIT(demo_init);
KPM_CTL0(demo_control);
KPM_EXIT(demo_exit);
