/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * prctl_cmd.h — Agent ↔ KPM 通信协议
 *
 * Agent 通过 prctl(KDBG_PRCTL_xxx, arg2, ...) 与 KPM 通信。
 * 命令码和数据结构必须与 Rust protocol/src/lib.rs 保持一致。
 *
 * 编号空间: 0x4B4442xx ("KDB\x00" ~ "KDB\xFF")
 */
#ifndef PRCTL_CMD_H
#define PRCTL_CMD_H

#include <stdint.h>

/* ========== prctl syscall number (ARM64) ========== */

#define PRCTL_NR  167

/* ========== 命令码前缀 ========== */

#define KDBG_PRCTL_PREFIX  0x4B444200u   /* 快速过滤 mask */

/* ========== 断点/调试命令 ========== */

#define KDBG_PRCTL_SET_BP       0x4B444201  /* 设置断点, arg2=&bp_args */
#define KDBG_PRCTL_REMOVE_BP    0x4B444202  /* 移除断点, arg2=bp_id */
#define KDBG_PRCTL_SET_HW_BP    0x4B444203  /* 设置硬件断点 */
#define KDBG_PRCTL_SET_WP       0x4B444204  /* 设置 watchpoint */
#define KDBG_PRCTL_REMOVE_HW    0x4B444205  /* 移除硬件 BP/WP */
#define KDBG_PRCTL_SINGLE_STEP  0x4B444210  /* 单步执行 */
#define KDBG_PRCTL_CONTINUE     0x4B444211  /* 继续执行 */
#define KDBG_PRCTL_READ_MEM     0x4B444220  /* 读内核内存, arg2=&mem_args */
#define KDBG_PRCTL_WRITE_MEM    0x4B444221  /* 写内核内存, arg2=&mem_args */
#define KDBG_PRCTL_READ_REGS    0x4B444230  /* 读寄存器, arg2=&regs */
#define KDBG_PRCTL_WRITE_REGS   0x4B444231  /* 写寄存器, arg2=&regs */
#define KDBG_PRCTL_WAIT_EVENT   0x4B444240  /* 等待调试事件, arg2=&event */
#define KDBG_PRCTL_QUERY_STATE  0x4B444250  /* 查询状态, arg2=&state */
#define KDBG_PRCTL_DETACH       0x4B4442FF  /* 分离调试器 */

/* ========== Spawn / SO 监控命令 ========== */

#define KDBG_PRCTL_SPAWN_WATCH  0x4B444260  /* 监控进程创建, arg2=用户态字符串 */
#define KDBG_PRCTL_WAIT_SPAWN   0x4B444261  /* 自旋等待, ret=PID */
#define KDBG_PRCTL_CANCEL_SPAWN 0x4B444262  /* 取消监控 */
#define KDBG_PRCTL_MMAP_WATCH   0x4B444270  /* 监控 SO 加载, arg2=&mmap_args */
#define KDBG_PRCTL_WAIT_MMAP    0x4B444271  /* 等待 SO 加载, arg2=&mmap_result */
#define KDBG_PRCTL_RESUME       0x4B444272  /* SIGCONT 恢复进程, arg2=PID */

/* ========== C 结构体 (匹配 Rust #[repr(C)]) ========== */

/* 断点参数 (Agent → KPM) */
struct kdbg_bp_args {
    uint64_t addr;
    uint32_t bp_type;       /* 0=SW_BRK, 1=HW_EXEC, 2=HW_READ, 3=HW_WRITE, 4=HW_RW */
    uint32_t len;           /* watchpoint 长度: 1/2/4/8 */
    int32_t  id;            /* [out] 分配的 ID, [in] 操作目标 ID */
    uint32_t reserved;
};

/* 内存读写参数 (Agent → KPM) */
struct kdbg_mem_args {
    uint64_t kaddr;         /* 内核虚拟地址 */
    uint64_t ubuf;          /* 用户态缓冲区指针 */
    uint32_t len;
    int32_t  result;
};

/* SO 加载监控参数 (Agent → KPM) */
struct kdbg_mmap_args {
    int32_t  pid;
    char     so_name[256];
};

/* SO 加载结果 (KPM → Agent) */
struct kdbg_mmap_result {
    uint64_t base;
    uint64_t size;
    char     path[256];
};

/* ARM64 寄存器快照 */
struct kdbg_regs {
    uint64_t regs[31];      /* x0-x30 */
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
};

/* 调试事件 (KPM → Agent) */
struct kdbg_event {
    uint32_t type;          /* 0=none, 1=breakpoint, 2=step, 3=watchpoint */
    uint32_t cpu;
    uint64_t addr;
    uint64_t pc;
    uint32_t bp_id;
    uint32_t reserved;
};

/* 调试器状态查询 (KPM → Agent) */
struct kdbg_state {
    uint32_t stopped;
    uint32_t stopped_cpu;
    uint64_t stopped_pc;
    uint32_t num_sw_bp;
    uint32_t num_hw_bp;
};

/* ========== 常量 ========== */

#define KDBG_MEM_MAX  4096  /* 单次读写上限 */

#endif /* PRCTL_CMD_H */
