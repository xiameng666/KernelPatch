/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * process.c — SO 加载监控 + 进程内存操作
 *
 * 职责:
 *   1. Hook vm_mmap_pgoff, 监控目标 SO 的 PROT_EXEC mmap
 *   2. SO 加载检测后 SIGSTOP 冻结进程 (init_array/JNI_OnLoad 之前)
 *   3. 封装 access_process_vm 读写目标进程内存
 *
 * 工作模式:
 *   - 被动模式: prctl MMAP_WATCH 设置 pid + so_name → hook 过滤 → SIGSTOP
 *   - 手动模式: kpmctl load "libfoo.so" → hook 过滤所有进程 → busy-wait 冻结
 *
 * 原理:
 *   Android dlopen → linker → vm_mmap_pgoff 多次:
 *     - r-xp (代码段, PROT_EXEC)  ← 我们拦截这个
 *     - r--p (只读数据)
 *     - rw-p (可写数据)
 *   拦截第一个 PROT_EXEC mmap 时, SO 代码已映射但 init_array/JNI_OnLoad 未执行,
 *   是设置断点的最佳时机。
 */

#include "demo-brk.h"

/* ========== 全局状态 ========== */

/* 目标进程 */
int g_target_pid               = 0;     /* 目标进程 PID (0=不过滤) */
void *g_target_task            = 0;     /* 目标进程 task_struct */

/* mmap 监控 (prctl 驱动) */
char g_mmap_target_so[256]     = {0};   /* 目标 SO 名 */
volatile int g_mmap_watching   = 0;     /* 是否在监控 */
volatile int g_mmap_so_found   = 0;     /* SO 是否已检测到 */
uint64_t g_mmap_so_base        = 0;     /* SO 代码段基址 */
uint64_t g_mmap_so_size        = 0;     /* 映射大小 */
char g_mmap_so_path[256]       = {0};   /* 完整路径 */

/* 兼容: 手动模式 (kpmctl load "xxx.so" + kpmctl ctl release) */
volatile int g_mmap_frozen     = 0;     /* 手动模式冻结中 */
char g_manual_target_so[256]   = {0};   /* 手动指定的 SO 名 */

/* ========== mmap before hook ========== */

/*
 * vm_mmap_pgoff(file, addr, len, prot, flag, pgoff) before hook
 *
 * 在 mmap 执行前检查:
 *   1. file 是否为我们的目标 SO
 *   2. prot 是否包含 PROT_EXEC (代码段)
 *
 * 两种模式并行:
 *   - prctl 模式: g_mmap_watching=1, 按 PID 过滤
 *   - 手动模式: g_manual_target_so[0] 非空, 不过滤 PID
 */
void mmap_before_hook(hook_fargs6_t *args, void *udata)
{
    /* 必须先清零标记 (hook 框架不保证 local.data0 初始化) */
    args->local.data0 = 0;

    int prctl_mode = g_mmap_watching && g_mmap_target_so[0];
    int manual_mode = !g_mmap_frozen && g_manual_target_so[0];

    if (!prctl_mode && !manual_mode)
        return;

    void *file = (void *)args->arg0;
    unsigned long prot = args->arg3;

    /* 必须是文件映射 + PROT_EXEC */
    if (!file || !(prot & PROT_EXEC))
        return;

    /* prctl 模式: PID 过滤 */
    if (prctl_mode && g_target_pid) {
        void *task = get_current_task();
        pid_t tgid = kfn___task_pid_nr_ns
            ? kfn___task_pid_nr_ns(task, PIDTYPE_TGID, 0) : -1;
        if (tgid != g_target_pid)
            return;
    }

    /* 从 file->f_path 提取文件名 */
    void *path_ptr = (void *)((char *)file + FILE_F_PATH_OFFSET);

    char buf[256];
    char *name = kfn_d_path(path_ptr, buf, sizeof(buf));
    if (!name || (unsigned long)name < (unsigned long)buf ||
        (unsigned long)name >= (unsigned long)(buf + sizeof(buf)))
        return;

    /* 检查文件名是否包含目标 SO */
    const char *target = prctl_mode ? g_mmap_target_so : g_manual_target_so;
    if (!strstr(name, target))
        return;

    /* 标记匹配 + 保存路径, after hook 中处理 */
    args->local.data0 = prctl_mode ? 1 : 2;  /* 1=prctl, 2=manual */

    /* 保存完整路径 */
    int plen = strlen(name);
    if (plen >= (int)sizeof(g_mmap_so_path))
        plen = sizeof(g_mmap_so_path) - 1;
    memcpy(g_mmap_so_path, name, plen);
    g_mmap_so_path[plen] = 0;

    pr_info(TAG ": mmap_before: MATCH '%s' (prot=0x%lx)\n", name, prot);
}

/* ========== mmap after hook ========== */

void mmap_after_hook(hook_fargs6_t *args, void *udata)
{
    int mode = (int)args->local.data0;
    if (!mode)
        return;

    unsigned long base = (unsigned long)args->ret;

    /* mmap 失败检查 */
    if ((long)base <= 0) {
        pr_warn(TAG ": mmap_after: mmap returned error %ld\n", (long)base);
        return;
    }

    /* 获取当前进程信息 */
    void *task = get_current_task();
    pid_t pid = kfn___task_pid_nr_ns
        ? kfn___task_pid_nr_ns(task, PIDTYPE_TGID, 0) : -1;

    unsigned long len = args->arg2;

    pr_info(TAG ": *** SO LOADED ***\n");
    pr_info(TAG ":   path = '%s'\n", g_mmap_so_path);
    pr_info(TAG ":   base = 0x%lx\n", base);
    pr_info(TAG ":   len  = 0x%lx\n", len);
    pr_info(TAG ":   pid  = %d\n", pid);

    if (mode == 1) {
        /* ---- prctl 模式: SIGSTOP + 设标志, 不 busy-wait ---- */
        g_mmap_so_base = (uint64_t)base;
        g_mmap_so_size = (uint64_t)len;
        g_target_pid = pid;
        g_target_task = task;

        /* SIGSTOP 冻结 */
        if (kfn_send_sig)
            kfn_send_sig(19 /* SIGSTOP */, task, 1);

        g_mmap_so_found = 1;
        g_mmap_watching = 0;  /* 停止监控 */

        pr_info(TAG ":   FROZEN (prctl mode), Agent will set breakpoints\n");

    } else {
        /* ---- 手动模式: SIGSTOP + busy-wait ---- */
        g_mmap_so_base = (uint64_t)base;
        g_mmap_so_size = (uint64_t)len;
        g_target_pid = pid;
        g_target_task = task;

        if (kfn_send_sig)
            kfn_send_sig(19 /* SIGSTOP */, task, 1);

        g_mmap_frozen = 1;

        pr_info(TAG ":   FROZEN (manual mode), use 'ctl release' to continue\n");
        pr_info(TAG ":   base=0x%lx, set breakpoints then release\n", base);

        /* busy-wait 直到手动 release */
        while (g_mmap_frozen)
            asm volatile("yield");

        pr_info(TAG ":   pid=%d released, SO init continues\n", pid);
    }
}

/* ========== proc_release: 放行手动模式冻结 ========== */

int proc_release(void)
{
    if (!g_mmap_frozen) {
        pr_info(TAG ": nothing frozen (manual mode)\n");
        return 0;
    }

    pr_info(TAG ": releasing pid=%d...\n", g_target_pid);

    if (kfn_send_sig && g_target_task)
        kfn_send_sig(18 /* SIGCONT */, g_target_task, 1);

    g_mmap_frozen = 0;   /* 最后清, 让 busy-wait 退出 */

    pr_info(TAG ": pid=%d RESUMED\n", g_target_pid);
    return 0;
}

/* ========== 进程内存读写 ========== */

int proc_read_mem(void *task, unsigned long addr, void *buf, int len)
{
    if (!kfn_access_process_vm || !task)
        return -1;
    return kfn_access_process_vm(task, addr, buf, len, FOLL_FORCE);
}

int proc_write_mem(void *task, unsigned long addr, void *buf, int len)
{
    if (!kfn_access_process_vm || !task)
        return -1;
    return kfn_access_process_vm(task, addr, buf, len, FOLL_FORCE | FOLL_WRITE);
}
