/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * kdbg.c - Kernel Debugger KPM (KernelPatch Module)
 *
 * 【原理】
 * 1. 通过 KP inline hook 拦截 do_debug_exception, 接管所有调试异常
 * 2. R3 通过 prctl syscall hook 下发命令 (设置断点/单步/读写内存)
 * 3. 断点命中 → 暂停目标 CPU → 通知 R3 → 等待 R3 指令 → 恢复
 *
 * 【通信方式】
 * R3 调用 prctl(KDBG_PRCTL_xxx, arg2, ...) → KPM 拦截 → 处理 → 返回
 * 无需注入, 无需 /dev 设备, 无需 root 文件系统修改
 *
 * 【断点实现】
 * 软件断点: 用 KP 的 hook 机制修改指令 (trampoline)
 * 硬件断点: 直接操作 DBGBCR/DBGBVR 寄存器
 * 单步: 操作 MDSCR_EL1.SS + PSTATE.SS
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <hook.h>
#include <ksyms.h>
#include <syscall.h>
#include <kputils.h>
#include <stdint.h>

#include "kdbg.h"

/* pid_type 枚举 (KPM 不能使用 KP core 的 kfunc, 需自行定义) */
enum pid_type {
    PIDTYPE_PID,
    PIDTYPE_TGID,
    PIDTYPE_PGID,
    PIDTYPE_SID,
    PIDTYPE_MAX,
};
struct pid_namespace;
typedef int pid_t;

KPM_NAME("kdbg");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("XiaM");
KPM_DESCRIPTION("Kernel Debugger - ARM64 debug exception hook with R3 control");

/* ========== 内核函数指针 (kallsyms 解析) ========== */

static void *(*kfn_do_debug_exception)(unsigned long addr_if_watchpoint,
                                        unsigned int esr,
                                        struct pt_regs *regs);
static int (*kfn_copy_from_kernel_nofault)(void *dst, const void *src,
                                            unsigned long size);
static int (*kfn_copy_to_kernel_nofault)(void *dst, const void *src,
                                          unsigned long size);

/* 用于 R3 ↔ R0 数据交换 (compat_strncpy_from_user 遇 0x00 截断, 不能用于结构体!) */
static unsigned long (*kfn__copy_from_user)(void *to, const void __user *from, unsigned long n);

/* 信号: send_sig(int sig, struct task_struct *p, int priv) — 3 个参数 */
static int (*kfn_send_sig)(int sig, void *p, int priv);
static char *(*kfn_d_path)(const void *path, char *buf, int buflen);
static void *(*kfn_fget)(unsigned int fd);
static void (*kfn_fput)(void *file);

/* PID 获取: 通过 kallsyms 解析, 替代硬编码 task_struct 偏移 */
static pid_t (*kfn___task_pid_nr_ns)(struct task_struct *task, enum pid_type type, struct pid_namespace *ns);

/* comm 修改: 所有路径 (prctl PR_SET_NAME / /proc/comm / execve) 最终调用此函数 */
static void (*kfn___set_task_comm)(struct task_struct *tsk, const char *buf, int exec);

/* ========== 软件断点管理 ========== */

struct sw_bp {
    uint64_t addr;           /* 断点地址 */
    uint32_t saved_insn;     /* 被替换的原始指令 */
    int      active;         /* 是否激活 */
};

static struct sw_bp g_sw_bps[KDBG_MAX_SW_BP];
static int g_num_sw_bp = 0;

/* ========== 调试器状态 ========== */

static volatile int g_stopped = 0;          /* 是否停在断点 */
static volatile int g_stopped_cpu = -1;     /* 停止的 CPU */
static volatile int g_step_pending = 0;     /* 单步请求 */
static volatile int g_continue_flag = 0;    /* 继续执行标志 */
static struct pt_regs *g_stopped_regs = 0;  /* 当前中断的 pt_regs */

/* 调试事件缓冲 */
static struct kdbg_event g_pending_event;
static volatile int g_event_ready = 0;

/* 目标进程 PID (用于异常处理器 PID 过滤) */
static int g_target_pid = 0;

/* ========== Spawn 监控状态 ========== */

static char g_spawn_target_comm[256];       /* 目标包名 (完整, 不截断) */
static volatile int g_spawn_watching = 0;   /* 是否在监控 */
static volatile int g_spawn_found = 0;      /* 是否已检测到 */
static int g_spawn_pid = 0;                 /* 检测到的 PID */

/* ========== Mmap SO 监控状态 ========== */

static char g_mmap_target_so[256];          /* 目标 SO 名 */
static volatile int g_mmap_watching = 0;    /* 是否在监控 */
static volatile int g_mmap_matched = 0;     /* before_mmap 设置的匹配标志 */
static volatile int g_mmap_so_found = 0;    /* SO 是否已检测到 */
static uint64_t g_mmap_so_base = 0;         /* SO 基地址 */
static uint64_t g_mmap_so_size = 0;         /* 映射大小 */
static char g_mmap_so_path[256];            /* 完整路径 */

/* ========== CPU ID ========== */

static inline int kdbg_cpu_id(void)
{
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    int id = (int)(((mpidr >> 8) & 0xFF) * 4 + (mpidr & 0xFF));
    return id & 0xF;
}

/* ========== MDSCR_EL1 操作 (单步控制) ========== */

#define DBG_MDSCR_SS    (1U << 0)
#define DBG_MDSCR_KDE   (1U << 13)
#define DBG_MDSCR_MDE   (1U << 15)
#define DBG_SPSR_SS     (1U << 21)

static inline uint32_t read_mdscr(void)
{
    uint32_t val;
    asm volatile("mrs %0, mdscr_el1" : "=r"(val));
    return val;
}

static inline void write_mdscr(uint32_t val)
{
    asm volatile("msr mdscr_el1, %0" :: "r"(val));
    asm volatile("isb");
}

/*
 * 使能内核态单步:
 *   MDSCR_EL1.SS = 1   → 软件步进使能
 *   MDSCR_EL1.KDE = 1  → 内核态调试异常使能
 *   PSTATE.SS = 1       → armed 状态, ERET 后执行 1 条指令即触发异常
 */
static void kdbg_enable_single_step(struct pt_regs *regs)
{
    uint32_t mdscr = read_mdscr();
    mdscr |= DBG_MDSCR_SS;
    mdscr |= DBG_MDSCR_KDE;
    mdscr |= DBG_MDSCR_MDE;
    write_mdscr(mdscr);

    regs->pstate |= DBG_SPSR_SS;
}

static void kdbg_disable_single_step(void)
{
    uint32_t mdscr = read_mdscr();
    mdscr &= ~DBG_MDSCR_SS;
    write_mdscr(mdscr);
}

/* ========== 硬件断点寄存器操作 ========== */

/*
 * ARM64 最多支持 16 组 DBGBCR/DBGBVR (执行断点)
 *                 16 组 DBGWCR/DBGWVR (数据watchpoint)
 * 实际数量由 ID_AA64DFR0_EL1 决定, 通常 4-6 组
 */

/* 读取硬件断点/watchpoint 数量 */
static int kdbg_get_num_brps(void)
{
    uint64_t dfr0;
    asm volatile("mrs %0, id_aa64dfr0_el1" : "=r"(dfr0));
    return (int)((dfr0 >> 12) & 0xF) + 1;  /* BRPs 字段 */
}

static int kdbg_get_num_wrps(void)
{
    uint64_t dfr0;
    asm volatile("mrs %0, id_aa64dfr0_el1" : "=r"(dfr0));
    return (int)((dfr0 >> 20) & 0xF) + 1;  /* WRPs 字段 */
}

/* 写硬件断点: 将地址写入 DBGBVR, 控制位写入 DBGBCR */
static void kdbg_set_hw_breakpoint(int idx, uint64_t addr)
{
    /*
     * DBGBCR 控制字段:
     *   E   [0]    = 1     使能
     *   PMC [2:1]  = 0b11  EL1+EL0 都匹配
     *   BAS [8:5]  = 0xF   地址匹配 (4字节对齐指令)
     */
    uint32_t bcr = (0xFU << 5) | (0x3U << 1) | 1U;

    switch (idx) {
    case 0:
        asm volatile("msr dbgbvr0_el1, %0" :: "r"(addr));
        asm volatile("msr dbgbcr0_el1, %0" :: "r"((uint64_t)bcr));
        break;
    case 1:
        asm volatile("msr dbgbvr1_el1, %0" :: "r"(addr));
        asm volatile("msr dbgbcr1_el1, %0" :: "r"((uint64_t)bcr));
        break;
    case 2:
        asm volatile("msr dbgbvr2_el1, %0" :: "r"(addr));
        asm volatile("msr dbgbcr2_el1, %0" :: "r"((uint64_t)bcr));
        break;
    case 3:
        asm volatile("msr dbgbvr3_el1, %0" :: "r"(addr));
        asm volatile("msr dbgbcr3_el1, %0" :: "r"((uint64_t)bcr));
        break;
    }
    asm volatile("isb");

    /* 确保调试监视器使能 */
    uint32_t mdscr = read_mdscr();
    mdscr |= DBG_MDSCR_MDE | DBG_MDSCR_KDE;
    write_mdscr(mdscr);
}

static void kdbg_clear_hw_breakpoint(int idx)
{
    switch (idx) {
    case 0: asm volatile("msr dbgbcr0_el1, xzr"); break;
    case 1: asm volatile("msr dbgbcr1_el1, xzr"); break;
    case 2: asm volatile("msr dbgbcr2_el1, xzr"); break;
    case 3: asm volatile("msr dbgbcr3_el1, xzr"); break;
    }
    asm volatile("isb");
}

/* 写 watchpoint: DBGWVR + DBGWCR */
static void kdbg_set_hw_watchpoint(int idx, uint64_t addr, int type, int len)
{
    /*
     * DBGWCR 控制字段:
     *   E    [0]    = 1     使能
     *   LSC  [4:3]  = type  01=load, 10=store, 11=both
     *   BAS  [12:5] = mask  字节选择 (根据 len)
     *   PAC  [2:1]  = 0b11  EL1+EL0
     */
    uint32_t lsc;
    switch (type) {
    case KDBG_BP_HW_READ:  lsc = 0x1; break;
    case KDBG_BP_HW_WRITE: lsc = 0x2; break;
    default:               lsc = 0x3; break;
    }

    uint32_t bas = (1U << len) - 1;  /* len=4 → bas=0xF */
    uint32_t wcr = (bas << 5) | (lsc << 3) | (0x3U << 1) | 1U;

    switch (idx) {
    case 0:
        asm volatile("msr dbgwvr0_el1, %0" :: "r"(addr));
        asm volatile("msr dbgwcr0_el1, %0" :: "r"((uint64_t)wcr));
        break;
    case 1:
        asm volatile("msr dbgwvr1_el1, %0" :: "r"(addr));
        asm volatile("msr dbgwcr1_el1, %0" :: "r"((uint64_t)wcr));
        break;
    case 2:
        asm volatile("msr dbgwvr2_el1, %0" :: "r"(addr));
        asm volatile("msr dbgwcr2_el1, %0" :: "r"((uint64_t)wcr));
        break;
    case 3:
        asm volatile("msr dbgwvr3_el1, %0" :: "r"(addr));
        asm volatile("msr dbgwcr3_el1, %0" :: "r"((uint64_t)wcr));
        break;
    }
    asm volatile("isb");

    uint32_t mdscr = read_mdscr();
    mdscr |= DBG_MDSCR_MDE | DBG_MDSCR_KDE;
    write_mdscr(mdscr);
}

/* ========== do_debug_exception Hook ========== */

/*
 * do_debug_exception 是所有内核态调试异常的入口:
 *   ESR_ELx_EC_BREAKPT_CUR → 硬件断点
 *   ESR_ELx_EC_SOFTSTP_CUR → 单步
 *   ESR_ELx_EC_WATCHPT_CUR → watchpoint
 *   ESR_ELx_EC_BRK64       → BRK 指令
 *
 * 我们在 before hook 中检查 ESR, 判断是否是我们的断点/单步
 * 如果是, 阻塞执行等待 R3 命令, 然后修改 args 跳过原始处理
 */

/* ESR 异常类提取 */
#define ESR_ELx_EC_SHIFT    26
#define ESR_ELx_EC_MASK     (0x3FUL << ESR_ELx_EC_SHIFT)
#define ESR_ELx_EC(esr)     (((esr) & ESR_ELx_EC_MASK) >> ESR_ELx_EC_SHIFT)

#define ESR_EC_BREAKPT_CUR  0x31
#define ESR_EC_SOFTSTP_CUR  0x33
#define ESR_EC_WATCHPT_CUR  0x35
#define ESR_EC_BRK64        0x3C

static void fill_event(struct kdbg_event *evt, uint32_t type, uint32_t bp_id,
                        struct pt_regs *regs)
{
    evt->type   = type;
    evt->cpu    = kdbg_cpu_id();
    evt->pc     = regs->pc;
    evt->addr   = regs->pc;
    evt->bp_id  = bp_id;
}

/*
 * 暂停 CPU 并等待 R3 的 CONTINUE/STEP 命令
 * 这是最核心的调试"暂停"语义实现
 */
static void kdbg_wait_for_r3(struct pt_regs *regs)
{
    g_stopped = 1;
    g_stopped_regs = regs;
    g_stopped_cpu = kdbg_cpu_id();
    g_continue_flag = 0;
    g_event_ready = 1;

    pr_info("kdbg: stopped at PC=%llx CPU=%d, waiting for R3...\n",
            regs->pc, g_stopped_cpu);

    /*
     * 自旋等待 R3 发送 continue/step 命令
     *
     * 注意: 在异常上下文中不能 schedule, 只能 busy-wait
     * 这会冻结当前 CPU, 但其他 CPU 仍在运行
     * R3 在其他 CPU 上通过 prctl 设置 g_continue_flag = 1
     */
    while (!g_continue_flag) {
        asm volatile("yield");  /* 省电的自旋提示 */
    }

    /* R3 可能请求了单步 */
    if (g_step_pending) {
        kdbg_enable_single_step(regs);
        g_step_pending = 0;
    }

    g_stopped = 0;
    g_stopped_regs = 0;
    g_stopped_cpu = -1;
    g_event_ready = 0;

    pr_info("kdbg: resumed from PC=%llx\n", regs->pc);
}

/*
 * do_debug_exception 的 before hook
 *
 * 函数原型: void do_debug_exception(unsigned long addr, unsigned int esr,
 *                                     struct pt_regs *regs)
 */
#define ESR_BRK64_ISS_COMMENT_MASK  0xFFFFUL

static void before_do_debug_exception(hook_fargs3_t *args, void *udata)
{
    unsigned long addr = (unsigned long)args->arg0;
    unsigned int esr   = (unsigned int)args->arg1;
    struct pt_regs *regs = (struct pt_regs *)args->arg2;
    unsigned int ec = ESR_ELx_EC(esr);

    /*
     * 安全守卫: 没有设置目标进程时, 不拦截任何调试异常
     * 否则内核自身的 BRK (BUG/WARN/KASAN/kprobes) 会进入
     * kdbg_wait_for_r3() 无限自旋 → CPU 死锁 → 黑屏
     */
    if (!g_target_pid)
        return;

    /* PID 过滤: 仅处理目标进程的调试异常 */
    {
        struct task_struct *tsk = (struct task_struct *)current;
        pid_t pid = kfn___task_pid_nr_ns ? kfn___task_pid_nr_ns(tsk, PIDTYPE_PID, 0) : -1;
        pid_t tgid = kfn___task_pid_nr_ns ? kfn___task_pid_nr_ns(tsk, PIDTYPE_TGID, 0) : -1;
        if (pid != g_target_pid && tgid != g_target_pid)
            return;
    }

    switch (ec) {
    case ESR_EC_BRK64: {
        /*
         * 只拦截 kdbg 自己的 BRK 立即数 (KDBG_BRK_IMM)
         * 放行内核的 BRK: 0x800(BUG/WARN) 0x004(kprobes) 0x900+(KASAN)
         */
        uint16_t brk_imm = (uint16_t)(esr & ESR_BRK64_ISS_COMMENT_MASK);
        if (brk_imm != KDBG_BRK_IMM)
            break;  /* 不是我们的 BRK, 放行给原始 handler */

        pr_info("kdbg: BRK exception at %llx ESR=%x\n", regs->pc, esr);
        fill_event(&g_pending_event, KDBG_EVT_BREAKPOINT, 0, regs);
        kdbg_wait_for_r3(regs);
        args->skip_origin = 1;
        break;
    }

    case ESR_EC_SOFTSTP_CUR:
        /* 单步异常 */
        kdbg_disable_single_step();
        pr_info("kdbg: single step at %llx\n", regs->pc);
        fill_event(&g_pending_event, KDBG_EVT_STEP, 0, regs);
        kdbg_wait_for_r3(regs);
        args->skip_origin = 1;
        break;

    case ESR_EC_BREAKPT_CUR:
        /* 硬件执行断点 */
        pr_info("kdbg: HW breakpoint at %llx\n", regs->pc);
        fill_event(&g_pending_event, KDBG_EVT_BREAKPOINT, 0, regs);
        kdbg_wait_for_r3(regs);
        args->skip_origin = 1;
        break;

    case ESR_EC_WATCHPT_CUR:
        /* 硬件 watchpoint */
        pr_info("kdbg: watchpoint at addr=%lx, PC=%llx\n", addr, regs->pc);
        g_pending_event.addr = addr;
        fill_event(&g_pending_event, KDBG_EVT_WATCHPOINT, 0, regs);
        g_pending_event.addr = addr;  /* watchpoint 地址, 不是 PC */
        kdbg_wait_for_r3(regs);
        args->skip_origin = 1;
        break;

    default:
        /* 不是我们关心的异常, 放行给原始 handler */
        break;
    }
}

/* ========== prctl Syscall Hook (R3 通信入口) ========== */

/*
 * R3 调用: prctl(KDBG_PRCTL_xxx, arg2, arg3, arg4, arg5)
 *
 * hook prctl 的 before 回调:
 *   arg0 = option (我们的命令码)
 *   arg1 = arg2
 *   arg2 = arg3
 *   arg3 = arg4
 *   arg4 = arg5
 *
 * 使用 syscall_argn() 提取参数 (兼容 syscall wrapper)
 */
/* ========== mmap syscall hook (Spawn SO 加载检测) ========== */

/*
 * ARM64 mmap: NR=222
 * long mmap(addr, len, prot, flags, fd, off)
 *
 * 在目标进程 mmap 带 PROT_EXEC 的文件时检测 SO 名称
 */
#define PROT_EXEC  0x4
#define TASK_COMM_LEN 16

/* 从 file 结构体获取 f_path 的偏移 (struct file 中 f_path 的位置) */
#define FILE_F_PATH_OFFSET 16  /* offsetof(struct file, f_path) */

static void before_mmap(hook_fargs6_t *args, void *udata)
{
    g_mmap_matched = 0;

    if (!g_mmap_watching || !g_target_pid)
        return;

    /* 检查是否是目标进程 */
    struct task_struct *tsk = (struct task_struct *)current;
    pid_t tgid = kfn___task_pid_nr_ns ? kfn___task_pid_nr_ns(tsk, PIDTYPE_TGID, 0) : -1;
    if (tgid != g_target_pid)
        return;

    unsigned long prot = (unsigned long)syscall_argn(args, 2);
    unsigned long fd   = (unsigned long)syscall_argn(args, 4);

    /* 只关心带 PROT_EXEC 的文件映射 */
    if (!(prot & PROT_EXEC) || (int)fd < 0)
        return;

    /* 从 fd 获取 struct file, 然后用 d_path 取路径 */
    if (!kfn_fget || !kfn_d_path || !kfn_fput)
        return;

    void *file = kfn_fget((unsigned int)fd);
    if (!file)
        return;

    char pathbuf[256];
    void *fpath = (char *)file + FILE_F_PATH_OFFSET;
    char *path = kfn_d_path(fpath, pathbuf, sizeof(pathbuf));

    kfn_fput(file);

    if (!path || (unsigned long)path >= (unsigned long)(pathbuf + sizeof(pathbuf)))
        return;

    /* 检查路径是否包含目标 SO 名 */
    if (strstr(path, g_mmap_target_so)) {
        g_mmap_matched = 1;
        /* 保存完整路径 */
        int plen = strlen(path);
        if (plen >= (int)sizeof(g_mmap_so_path))
            plen = sizeof(g_mmap_so_path) - 1;
        memcpy(g_mmap_so_path, path, plen);
        g_mmap_so_path[plen] = 0;

        unsigned long len = (unsigned long)syscall_argn(args, 1);
        g_mmap_so_size = len;

        pr_info("kdbg: mmap match: %s (len=%lx) for pid %d\n",
                g_mmap_so_path, len, tgid);
    }
}

static void after_mmap(hook_fargs6_t *args, void *udata)
{
    if (!g_mmap_matched)
        return;
    g_mmap_matched = 0;

    long ret = (long)args->ret;
    if (ret < 0)  /* mmap 失败 */
        return;

    g_mmap_so_base = (uint64_t)ret;
    g_mmap_so_found = 1;
    g_mmap_watching = 0;  /* 停止监控 */

    /* SIGSTOP 冻结目标进程 */
    if (kfn_send_sig) {
        struct task_struct *tsk = (struct task_struct *)current;
        kfn_send_sig(19 /* SIGSTOP */, tsk, 1 /* priv */);
    }

    pr_info("kdbg: SO loaded at base=%llx size=%llx, process frozen\n",
            g_mmap_so_base, g_mmap_so_size);
}

/* ========== __set_task_comm Hook (Spawn 进程名检测) ========== */

/*
 * void __set_task_comm(struct task_struct *tsk, const char *buf, bool exec)
 *
 * 所有 comm 修改路径最终调用此函数:
 *   - prctl(PR_SET_NAME) → set_task_comm → __set_task_comm(tsk, buf, false)
 *   - execve → setup_new_exec → __set_task_comm(tsk, basename, true)
 *   - /proc/self/task/<tid>/comm 写入 → __set_task_comm(tsk, buf, false)
 *
 * buf 已在内核内存中, 无需 compat_strncpy_from_user
 */
static void before_set_task_comm(hook_fargs3_t *args, void *udata)
{
    if (!g_spawn_watching)
        return;

    struct task_struct *tsk = (struct task_struct *)args->arg0;
    const char *name = (const char *)args->arg1;

    if (!name || !tsk)
        return;

    /* 调试日志: 打印每个 comm 变更 (仅监控期间) */
    pid_t tgid = kfn___task_pid_nr_ns ? kfn___task_pid_nr_ns(tsk, PIDTYPE_TGID, 0) : -1;
    pr_info("kdbg: set_task_comm pid=%d name='%s' target='%s'\n",
            tgid, name, g_spawn_target_comm);

    if (!g_spawn_found && strstr(g_spawn_target_comm, name)) {
        g_spawn_pid = tgid;
        g_spawn_found = 1;
        g_spawn_watching = 0;

        /* SIGSTOP 冻结进程 */
        if (kfn_send_sig)
            kfn_send_sig(19 /* SIGSTOP */, tsk, 1 /* priv */);

        pr_info("kdbg: spawn detected: '%s' pid=%d, frozen\n",
                name, g_spawn_pid);
    }
}

/* ========== prctl Syscall Hook (R3 通信入口) ========== */

static void before_prctl(hook_fargs4_t *args, void *udata)
{
    int option  = (int)syscall_argn(args, 0);
    long uarg2  = (long)syscall_argn(args, 1);
    long uarg3  = (long)syscall_argn(args, 2);
    /* long uarg4 = (long)syscall_argn(args, 3); */
    /* long uarg5 = (long)syscall_argn(args, 4); */

    /* 快速过滤: 不是我们的命令码就放行 */
    if ((option & 0xFFFFFF00) != 0x4B444200)
        return;

    /* 拦截, 不再调用原始 prctl */
    args->skip_origin = 1;

    switch (option) {

    case KDBG_PRCTL_SET_BP: {
        /* arg2 = uint64_t 目标内核地址 */
        uint64_t bp_addr = (uint64_t)uarg2;
        int idx = -1;
        for (int i = 0; i < KDBG_MAX_SW_BP; i++) {
            if (!g_sw_bps[i].active) { idx = i; break; }
        }
        if (idx < 0) { args->ret = -1; return; }

        /* 使用 KP 的 hook_wrap 在目标函数设置 trampoline
         * 或者用硬件断点实现 (更简单, 无需修改指令) */
        kdbg_set_hw_breakpoint(idx % kdbg_get_num_brps(), bp_addr);

        g_sw_bps[idx].addr = bp_addr;
        g_sw_bps[idx].active = 1;
        g_num_sw_bp++;

        pr_info("kdbg: breakpoint %d set at %llx\n", idx, bp_addr);
        args->ret = idx;
        return;
    }

    case KDBG_PRCTL_REMOVE_BP: {
        int bp_id = (int)uarg2;
        if (bp_id < 0 || bp_id >= KDBG_MAX_SW_BP || !g_sw_bps[bp_id].active) {
            args->ret = -1; return;
        }
        kdbg_clear_hw_breakpoint(bp_id % kdbg_get_num_brps());
        g_sw_bps[bp_id].active = 0;
        g_num_sw_bp--;
        pr_info("kdbg: breakpoint %d removed\n", bp_id);
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_SET_WP: {
        /* arg2 = addr, arg3 = (type << 32 | len) */
        uint64_t wp_addr = (uint64_t)uarg2;
        int type = (int)(uarg3 >> 32);
        int len  = (int)(uarg3 & 0xFFFFFFFF);
        if (len <= 0 || len > 8) len = 4;
        /* 用第一个空闲的 watchpoint 寄存器 */
        int idx = 0; /* TODO: 分配管理 */
        kdbg_set_hw_watchpoint(idx, wp_addr, type, len);
        pr_info("kdbg: watchpoint set at %llx type=%d len=%d\n", wp_addr, type, len);
        args->ret = idx;
        return;
    }

    case KDBG_PRCTL_SINGLE_STEP: {
        if (!g_stopped || !g_stopped_regs) {
            args->ret = -1; return;
        }
        g_step_pending = 1;
        g_continue_flag = 1;   /* 释放自旋等待 */
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_CONTINUE: {
        if (!g_stopped) {
            args->ret = -1; return;
        }
        g_step_pending = 0;
        g_continue_flag = 1;
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_READ_MEM: {
        /* arg2 = &kdbg_mem_args (用户态指针) */
        struct kdbg_mem_args ma;
        if (!kfn__copy_from_user || kfn__copy_from_user(&ma, (void *)uarg2,
                                      sizeof(ma))) {
            args->ret = -1; return;
        }
        if (ma.len > KDBG_MEM_MAX) ma.len = KDBG_MEM_MAX;

        char tmp[KDBG_MEM_MAX];
        int err = kfn_copy_from_kernel_nofault(tmp, (void *)ma.kaddr, ma.len);
        if (err) { args->ret = err; return; }

        if (compat_copy_to_user((void *)ma.ubuf, tmp, ma.len)) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WRITE_MEM: {
        struct kdbg_mem_args ma;
        if (!kfn__copy_from_user || kfn__copy_from_user(&ma, (void *)uarg2,
                                      sizeof(ma))) {
            args->ret = -1; return;
        }
        if (ma.len > KDBG_MEM_MAX) ma.len = KDBG_MEM_MAX;

        char tmp[KDBG_MEM_MAX];
        if (kfn__copy_from_user(tmp, (void *)ma.ubuf, ma.len)) {
            args->ret = -1; return;
        }

        int err = kfn_copy_to_kernel_nofault((void *)ma.kaddr, tmp, ma.len);
        args->ret = err;
        return;
    }

    case KDBG_PRCTL_READ_REGS: {
        /* arg2 = &kdbg_regs (用户态指针, 用于写出寄存器) */
        if (!g_stopped || !g_stopped_regs) {
            args->ret = -1; return;
        }
        struct kdbg_regs kr;
        for (int i = 0; i < 31; i++)
            kr.regs[i] = g_stopped_regs->regs[i];
        kr.sp     = g_stopped_regs->sp;
        kr.pc     = g_stopped_regs->pc;
        kr.pstate = g_stopped_regs->pstate;

        if (compat_copy_to_user((void *)uarg2, &kr, sizeof(kr))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WRITE_REGS: {
        if (!g_stopped || !g_stopped_regs) {
            args->ret = -1; return;
        }
        struct kdbg_regs kr;
        if (!kfn__copy_from_user || kfn__copy_from_user(&kr, (void *)uarg2,
                                      sizeof(kr))) {
            args->ret = -1; return;
        }
        for (int i = 0; i < 31; i++)
            g_stopped_regs->regs[i] = kr.regs[i];
        g_stopped_regs->sp     = kr.sp;
        g_stopped_regs->pc     = kr.pc;
        g_stopped_regs->pstate = kr.pstate;
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WAIT_EVENT: {
        /* arg2 = &kdbg_event (用户态指针) */
        /* 自旋等待事件 (非阻塞轮询也可以) */
        while (!g_event_ready) {
            asm volatile("yield");
        }
        if (compat_copy_to_user((void *)uarg2, &g_pending_event,
                                sizeof(g_pending_event))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_QUERY_STATE: {
        struct kdbg_state st = {
            .stopped     = g_stopped,
            .stopped_cpu = g_stopped_cpu,
            .stopped_pc  = g_stopped_regs ? g_stopped_regs->pc : 0,
            .num_sw_bp   = g_num_sw_bp,
            .num_hw_bp   = 0,
        };
        if (compat_copy_to_user((void *)uarg2, &st, sizeof(st))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    /* ========== Spawn 调试命令 ========== */

    case KDBG_PRCTL_SPAWN_WATCH: {
        /* arg2 = 用户态字符串 (目标包名) */
        if (compat_strncpy_from_user(g_spawn_target_comm, (const char *)uarg2,
                                      sizeof(g_spawn_target_comm) - 1) <= 0) {
            pr_err("kdbg: SPAWN_WATCH: 读取用户态字符串失败\n");
            args->ret = -1; return;
        }
        g_spawn_target_comm[sizeof(g_spawn_target_comm) - 1] = 0;
        g_spawn_found = 0;
        g_spawn_pid = 0;
        g_spawn_watching = 1;
        pr_info("kdbg: spawn watch started for '%s' (len=%d)\n",
                g_spawn_target_comm, (int)strlen(g_spawn_target_comm));
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WAIT_SPAWN: {
        /* 自旋等待目标进程出现, 同时检查 watching 标志以支持取消 */
        while (!g_spawn_found && g_spawn_watching) {
            asm volatile("yield");
        }
        if (g_spawn_found) {
            args->ret = g_spawn_pid;
        } else {
            /* 被 CANCEL_SPAWN 取消 */
            args->ret = -1;
        }
        return;
    }

    case KDBG_PRCTL_CANCEL_SPAWN: {
        g_spawn_watching = 0;  /* 先清 watching, 让 WAIT_SPAWN 自旋退出 */
        g_spawn_found = 0;
        g_spawn_pid = 0;
        /* 同时取消 mmap 监控 (解除 WAIT_MMAP 自旋) */
        g_mmap_watching = 0;
        pr_info("kdbg: spawn/mmap watch cancelled\n");
        args->ret = 0;
        return;
    }

    /* ========== Mmap SO 监控命令 ========== */

    case KDBG_PRCTL_MMAP_WATCH: {
        /* arg2 = &kdbg_mmap_args (用户态) */
        struct kdbg_mmap_args ma;
        if (!kfn__copy_from_user || kfn__copy_from_user(&ma, (void *)uarg2,
                                      sizeof(ma))) {
            args->ret = -1; return;
        }
        g_target_pid = ma.pid;
        int slen = strlen(ma.so_name);
        if (slen >= (int)sizeof(g_mmap_target_so))
            slen = sizeof(g_mmap_target_so) - 1;
        memcpy(g_mmap_target_so, ma.so_name, slen);
        g_mmap_target_so[slen] = 0;

        g_mmap_so_found = 0;
        g_mmap_so_base = 0;
        g_mmap_matched = 0;
        g_mmap_watching = 1;

        pr_info("kdbg: mmap watch started for '%s' pid=%d\n",
                g_mmap_target_so, g_target_pid);
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WAIT_MMAP: {
        /* 自旋等待 SO 加载, arg2 = &kdbg_mmap_result (用户态) */
        while (!g_mmap_so_found && g_mmap_watching) {
            asm volatile("yield");
        }
        if (!g_mmap_so_found) {
            /* 被取消 */
            args->ret = -1;
            return;
        }
        struct kdbg_mmap_result mr;
        mr.base = g_mmap_so_base;
        mr.size = g_mmap_so_size;
        memcpy(mr.path, g_mmap_so_path, sizeof(mr.path));
        if (compat_copy_to_user((void *)uarg2, &mr, sizeof(mr))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_RESUME: {
        /* arg2 = PID, 发送 SIGCONT 恢复目标进程 */
        int target_pid = (int)uarg2;
        /* 通过 pid 查找 task_struct 并发送 SIGCONT */
        /* 简化: 直接用 kill_pid 或 kill syscall, 这里通过 R3 agent 来做 */
        pr_info("kdbg: resume request for pid=%d\n", target_pid);
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_DETACH: {
        /* 清理所有断点, 恢复执行 */
        for (int i = 0; i < KDBG_MAX_SW_BP; i++) {
            if (g_sw_bps[i].active) {
                kdbg_clear_hw_breakpoint(i % kdbg_get_num_brps());
                g_sw_bps[i].active = 0;
            }
        }
        g_num_sw_bp = 0;
        g_target_pid = 0;
        if (g_stopped) {
            g_step_pending = 0;
            g_continue_flag = 1;
        }
        pr_info("kdbg: detached\n");
        args->ret = 0;
        return;
    }

    default:
        /* 未知的 KDBG 命令, 返回错误 */
        args->ret = -1;
        return;
    }
}

/* ========== KPM 生命周期 ========== */

static long kdbg_init(const char *kpm_args, const char *event, void *__user reserved)
{
    pr_info("kdbg: initializing kernel debugger...\n");

    /* 解析内核符号 */
    kfn_do_debug_exception = (typeof(kfn_do_debug_exception))
        kallsyms_lookup_name("do_debug_exception");
    kfn_copy_from_kernel_nofault = (typeof(kfn_copy_from_kernel_nofault))
        kallsyms_lookup_name("copy_from_kernel_nofault");
    kfn_copy_to_kernel_nofault = (typeof(kfn_copy_to_kernel_nofault))
        kallsyms_lookup_name("copy_to_kernel_nofault");

    if (!kfn_do_debug_exception) {
        pr_err("kdbg: failed to find do_debug_exception\n");
        return -1;
    }
    pr_info("kdbg: do_debug_exception = %llx\n",
            (uint64_t)kfn_do_debug_exception);

    if (!kfn_copy_from_kernel_nofault)
        pr_warn("kdbg: copy_from_kernel_nofault not found, mem read may crash\n");

    /* Spawn/mmap 监控需要的内核函数 */
    kfn_send_sig = (typeof(kfn_send_sig))
        kallsyms_lookup_name("send_sig");
    kfn_d_path = (typeof(kfn_d_path))
        kallsyms_lookup_name("d_path");
    kfn_fget = (typeof(kfn_fget))
        kallsyms_lookup_name("fget");
    kfn_fput = (typeof(kfn_fput))
        kallsyms_lookup_name("fput");
    kfn___task_pid_nr_ns = (typeof(kfn___task_pid_nr_ns))
        kallsyms_lookup_name("__task_pid_nr_ns");
    kfn__copy_from_user = (typeof(kfn__copy_from_user))
        kallsyms_lookup_name("_copy_from_user");
    kfn___set_task_comm = (typeof(kfn___set_task_comm))
        kallsyms_lookup_name("__set_task_comm");

    if (!kfn_send_sig)
        pr_warn("kdbg: send_sig not found, spawn freeze won't work\n");
    if (!kfn___task_pid_nr_ns)
        pr_warn("kdbg: __task_pid_nr_ns not found, PID filtering won't work\n");
    if (!kfn__copy_from_user)
        pr_warn("kdbg: _copy_from_user not found, struct prctl commands broken\n");
    if (!kfn___set_task_comm)
        pr_warn("kdbg: __set_task_comm not found, spawn detection won't work\n");
    if (!kfn_d_path || !kfn_fget || !kfn_fput)
        pr_warn("kdbg: d_path/fget/fput not found, mmap SO detection won't work\n");

    /* Hook do_debug_exception (3 个参数) */
    hook_err_t err = hook_wrap3(
        (void *)kfn_do_debug_exception,
        (hook_chain3_callback)before_do_debug_exception,
        0,  /* no after hook */
        0   /* no udata */
    );
    if (err) {
        pr_err("kdbg: hook do_debug_exception failed: %d\n", err);
        return -1;
    }
    pr_info("kdbg: do_debug_exception hooked\n");

    /* Hook prctl syscall 用于 R3 通信 */
    err = hook_syscalln(PRCTL_NR, 4, before_prctl, 0, 0);
    if (err) {
        pr_err("kdbg: hook prctl failed: %d\n", err);
        hook_unwrap((void *)kfn_do_debug_exception,
                    (void *)before_do_debug_exception, 0);
        return -1;
    }
    pr_info("kdbg: prctl hooked, R3 communication ready\n");

    /* Hook mmap syscall 用于 SO 加载检测 (NR=222 on ARM64) */
    err = hook_syscalln(222, 6, (hook_chain6_callback)before_mmap,
                        (hook_chain6_callback)after_mmap, 0);
    if (err) {
        pr_warn("kdbg: hook mmap failed: %d, SO detection disabled\n", err);
        /* 非致命, 继续 */
    } else {
        pr_info("kdbg: mmap hooked, SO load detection ready\n");
    }

    /* Hook __set_task_comm 用于 spawn 进程名检测 (3 个参数) */
    if (kfn___set_task_comm) {
        err = hook_wrap3(
            (void *)kfn___set_task_comm,
            (hook_chain3_callback)before_set_task_comm,
            0,  /* no after hook */
            0   /* no udata */
        );
        if (err) {
            pr_warn("kdbg: hook __set_task_comm failed: %d, spawn detection disabled\n", err);
        } else {
            pr_info("kdbg: __set_task_comm hooked, spawn detection ready\n");
        }
    }

    /* 初始化断点表 */
    for (int i = 0; i < KDBG_MAX_SW_BP; i++)
        g_sw_bps[i].active = 0;

    int num_brps = kdbg_get_num_brps();
    int num_wrps = kdbg_get_num_wrps();
    pr_info("kdbg: CPU has %d HW breakpoints, %d watchpoints\n",
            num_brps, num_wrps);

    pr_info("kdbg: kernel debugger ready! Use prctl(0x4B4442xx) from R3\n");
    return 0;
}

static long kdbg_control(const char *ctl_args, char *__user out_msg, int outlen)
{
    pr_info("kdbg: control cmd: %s\n", ctl_args);

    /* 支持通过 sc_kpm_control 发送简单命令 */
    if (ctl_args && !strcmp(ctl_args, "status")) {
        pr_info("kdbg: stopped=%d cpu=%d sw_bp=%d\n",
                g_stopped, g_stopped_cpu, g_num_sw_bp);
    }
    return 0;
}

static long kdbg_exit(void *__user reserved)
{
    pr_info("kdbg: unloading...\n");

    /* 先确保不在停止状态 */
    if (g_stopped) {
        g_step_pending = 0;
        g_continue_flag = 1;
        /* 等待目标 CPU 恢复 */
        while (g_stopped)
            asm volatile("yield");
    }

    /* 清理所有硬件断点 */
    for (int i = 0; i < KDBG_MAX_SW_BP; i++) {
        if (g_sw_bps[i].active) {
            kdbg_clear_hw_breakpoint(i % kdbg_get_num_brps());
            g_sw_bps[i].active = 0;
        }
    }

    /* 卸载 hook */
    if (kfn___set_task_comm)
        hook_unwrap((void *)kfn___set_task_comm,
                    (void *)before_set_task_comm, 0);
    unhook_syscalln(222, (hook_chain6_callback)before_mmap,
                    (hook_chain6_callback)after_mmap);
    unhook_syscalln(PRCTL_NR, before_prctl, 0);
    hook_unwrap((void *)kfn_do_debug_exception,
                (void *)before_do_debug_exception, 0);

    pr_info("kdbg: unloaded\n");
    return 0;
}

KPM_INIT(kdbg_init);
KPM_CTL0(kdbg_control);
KPM_EXIT(kdbg_exit);
