/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * demo-brk.h — 共享头文件
 *
 * 架构:
 *   kpm_entry.c   — KPM 生命周期, 符号解析, ctl 分发
 *   process.c     — SO 加载监控 (hook vm_mmap_pgoff) + 进程内存读写
 *   breakpoint.c  — 断点管理器 (SW BRK / HW BP / HW WP)
 *   prctl_hook.c  — Agent 通信 (prctl hook) + Spawn 检测 (__set_task_comm hook)
 */
#ifndef DEMO_BRK_H
#define DEMO_BRK_H

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <hook.h>
#include <ksyms.h>
#include <stdint.h>

#define TAG "demo-brk"

/* ========== 内核类型 ========== */

enum pid_type { PIDTYPE_PID, PIDTYPE_TGID, PIDTYPE_PGID, PIDTYPE_SID, PIDTYPE_MAX };
typedef int pid_t;

#define FOLL_WRITE   0x01
#define FOLL_FORCE   0x10
#define PROT_EXEC    0x04

/*
 * struct file::f_path 在 GKI 5.10 的偏移
 *   f_u (union: llist_node 8B / rcu_head 16B) → 16 字节
 *   f_path 紧随其后 → offset 16
 */
#define FILE_F_PATH_OFFSET  16

/* ========== ARM64 指令编码 ========== */

#define BRK_IMM_BASE   0xDB60u
#define BRK_INSN(imm)  (0xD4200000u | (((imm) & 0xFFFF) << 5))

/* pt_regs 布局 */
#define PT_REGS_SP     (31 * 8)
#define PT_REGS_PC     (32 * 8)
#define PT_REGS_PSTATE (33 * 8)
#define PT_REGS_COUNT  34

/* ========== 内核函数指针 (kpm_entry.c 定义) ========== */

extern pid_t (*kfn___task_pid_nr_ns)(void *task, enum pid_type type, void *ns);
extern int   (*kfn_send_sig)(int sig, void *p, int priv);
extern int   (*kfn_access_process_vm)(void *tsk, unsigned long addr,
                                      void *buf, int len, unsigned int gup_flags);
extern void  (*kfn___flush_icache_range)(unsigned long start, unsigned long end);
extern void  (*kfn_do_debug_exception)(unsigned long addr, unsigned int esr, void *regs);

/* vm_mmap_pgoff(file, addr, len, prot, flag, pgoff) */
extern unsigned long (*kfn_vm_mmap_pgoff)(void *file, unsigned long addr,
                                          unsigned long len, unsigned long prot,
                                          unsigned long flag, unsigned long pgoff);
/* d_path(path, buf, buflen) → char* */
extern char *(*kfn_d_path)(void *path, char *buf, int buflen);

/* ========== get_current_task ========== */

static inline void *get_current_task(void)
{
    void *task;
    asm volatile("mrs %0, sp_el0" : "=r"(task));
    return task;
}

/* ========== 进程/SO 监控状态 (process.c) ========== */

extern int g_target_pid;               /* 目标进程 PID */
extern void *g_target_task;            /* 目标进程 task_struct */

/* prctl 模式: Agent 通过 MMAP_WATCH/WAIT_MMAP 控制 */
extern char g_mmap_target_so[256];     /* 目标 SO 名 */
extern volatile int g_mmap_watching;   /* 是否在监控 */
extern volatile int g_mmap_so_found;   /* SO 是否已检测到 */
extern uint64_t g_mmap_so_base;        /* SO 代码段基址 */
extern uint64_t g_mmap_so_size;        /* 映射大小 */
extern char g_mmap_so_path[256];       /* 完整路径 */

/* 手动模式: kpmctl load "xxx.so" + kpmctl ctl release */
extern volatile int g_mmap_frozen;     /* 手动模式冻结中 */
extern char g_manual_target_so[256];   /* 手动指定的 SO 名 */

/* process.c 接口 */
void mmap_before_hook(hook_fargs6_t *args, void *udata);
void mmap_after_hook(hook_fargs6_t *args, void *udata);
int  proc_release(void);
int  proc_read_mem(void *task, unsigned long addr, void *buf, int len);
int  proc_write_mem(void *task, unsigned long addr, void *buf, int len);

/* ========== Spawn 监控状态 (prctl_hook.c) ========== */

extern char g_spawn_target_comm[256];
extern volatile int g_spawn_watching;
extern volatile int g_spawn_found;
extern int g_spawn_pid;

/* prctl_hook.c 接口 */
void prctl_before_hook(hook_fargs4_t *args, void *udata);
void prctl_on_set_task_comm(hook_fargs3_t *args, void *udata);

/* ========== 断点管理器 (breakpoint.c) ========== */

#define BP_MAX_SLOTS   16

enum bp_type {
    BP_NONE = 0,
    BP_SW_BRK,
    BP_HW_EXEC,
    BP_HW_WATCH_R,
    BP_HW_WATCH_W,
    BP_HW_WATCH_RW,
};

enum bp_state {
    BP_FREE = 0,
    BP_PENDING,
    BP_ARMED,
    BP_HIT,
    BP_DISABLED,
};

struct breakpoint {
    int              id;
    enum bp_type     type;
    enum bp_state    state;
    unsigned long    addr;
    uint32_t         orig_insn;
    uint16_t         brk_imm;
    int              hw_reg;
    unsigned int     hw_len;
    unsigned int     hit_count;
    unsigned long    saved_regs[PT_REGS_COUNT];
};

struct bp_manager {
    struct breakpoint slots[BP_MAX_SLOTS];
    int              count;
    volatile int     any_hit;
    int              active_hit_id;
};

extern struct bp_manager g_bpm;

void bp_init(void);
int  bp_add(enum bp_type type, unsigned long addr);
int  bp_remove(int id);
int  bp_arm(int id);
int  bp_disarm(int id);
int  bp_continue(void);
void bp_dump_regs(int id);
void bp_list(void);
void bp_cleanup(void);
void bp_on_debug_exception(hook_fargs3_t *args, void *udata);

#endif /* DEMO_BRK_H */
