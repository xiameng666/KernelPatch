/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * kdbg.h - Kernel Debugger KPM 共享头文件
 *
 * R3 (用户态) 和 R0 (内核态/KPM) 共享的命令码、数据结构。
 * R3 通过 prctl(KDBG_PRCTL_xxx, ...) 与 KPM 通信, 无需注入。
 */

#ifndef _KDBG_H_
#define _KDBG_H_

#ifdef __KERNEL__
#include <stdint.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif

/* ========== prctl 命令码 (通过 hook prctl syscall 拦截) ========== */
/*
 * 编号空间: 0x4B444200 ~ 0x4B4442FF ("KDB\x00" ~ "KDB\xFF")
 * R3 调用: prctl(KDBG_PRCTL_xxx, arg2, arg3, arg4, arg5)
 */
#define KDBG_PRCTL_SET_BP       0x4B444201  /* 设置软件断点 */
#define KDBG_PRCTL_REMOVE_BP    0x4B444202  /* 移除软件断点 */
#define KDBG_PRCTL_SET_HW_BP    0x4B444203  /* 设置硬件断点 */
#define KDBG_PRCTL_SET_WP       0x4B444204  /* 设置 watchpoint */
#define KDBG_PRCTL_REMOVE_HW    0x4B444205  /* 移除硬件断点/watchpoint */
#define KDBG_PRCTL_SINGLE_STEP  0x4B444210  /* 单步执行 */
#define KDBG_PRCTL_CONTINUE     0x4B444211  /* 继续执行 */
#define KDBG_PRCTL_READ_MEM     0x4B444220  /* 读内核内存 */
#define KDBG_PRCTL_WRITE_MEM    0x4B444221  /* 写内核内存 */
#define KDBG_PRCTL_READ_REGS    0x4B444230  /* 读寄存器 */
#define KDBG_PRCTL_WRITE_REGS   0x4B444231  /* 写寄存器 */
#define KDBG_PRCTL_WAIT_EVENT   0x4B444240  /* 等待调试事件 */
#define KDBG_PRCTL_QUERY_STATE  0x4B444250  /* 查询调试器状态 */

/* Spawn 调试 */
#define KDBG_PRCTL_SPAWN_WATCH  0x4B444260  /* 开始监控目标进程名, arg2=用户态字符串 */
#define KDBG_PRCTL_WAIT_SPAWN   0x4B444261  /* 自旋等待 spawn, 返回 PID */
#define KDBG_PRCTL_CANCEL_SPAWN 0x4B444262  /* 取消 spawn 监控 */

/* SO 加载监控 */
#define KDBG_PRCTL_MMAP_WATCH   0x4B444270  /* 监控 SO 加载, arg2=用户态 kdbg_mmap_args */
#define KDBG_PRCTL_WAIT_MMAP    0x4B444271  /* 自旋等待 SO 加载, 返回 base 到 arg2 */
#define KDBG_PRCTL_RESUME       0x4B444272  /* SIGCONT 恢复目标进程, arg2=PID */

#define KDBG_PRCTL_DETACH       0x4B4442FF  /* 分离调试器 */

/* prctl syscall number on ARM64 */
#define PRCTL_NR  167

/* ========== 调试事件类型 ========== */
enum kdbg_event_type {
    KDBG_EVT_NONE       = 0,
    KDBG_EVT_BREAKPOINT = 1,   /* 软件/硬件断点命中 */
    KDBG_EVT_STEP       = 2,   /* 单步完成 */
    KDBG_EVT_WATCHPOINT = 3,   /* watchpoint 触发 */
    KDBG_EVT_EXCEPTION  = 4,   /* 其他异常 */
};

/* ========== 断点类型 ========== */
enum kdbg_bp_type {
    KDBG_BP_SW_KERNEL  = 0,    /* 内核态软件断点 (BRK 指令) */
    KDBG_BP_HW_EXEC   = 1,    /* 硬件执行断点 (DBGBCR/BVR) */
    KDBG_BP_HW_READ   = 2,    /* 硬件读 watchpoint */
    KDBG_BP_HW_WRITE  = 3,    /* 硬件写 watchpoint */
    KDBG_BP_HW_RW     = 4,    /* 硬件读写 watchpoint */
};

/* ========== R3 ↔ R0 共享数据结构 ========== */

/*
 * 调试事件 (R0 → R3, 通过 WAIT_EVENT 返回)
 */
struct kdbg_event {
    uint32_t type;          /* kdbg_event_type */
    uint32_t cpu;           /* 触发 CPU */
    uint64_t addr;          /* 触发地址 (断点地址 / watchpoint 地址) */
    uint64_t pc;            /* 当前 PC */
    uint32_t bp_id;         /* 命中的断点 ID */
    uint32_t reserved;
};

/*
 * 寄存器快照 (R3 ↔ R0)
 * 对应 ARM64 pt_regs 布局
 */
struct kdbg_regs {
    uint64_t regs[31];      /* x0-x30 */
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
};

/*
 * 设置断点参数 (R3 → R0)
 * prctl(KDBG_PRCTL_SET_BP, &args, 0, 0, 0)
 */
struct kdbg_bp_args {
    uint64_t addr;          /* 断点地址 */
    uint32_t type;          /* kdbg_bp_type */
    uint32_t len;           /* watchpoint 长度: 1/2/4/8 */
    int32_t  id;            /* [out] 分配的断点 ID, [in] 操作目标 ID */
    uint32_t reserved;
};

/*
 * 内存读写参数 (R3 → R0)
 * prctl(KDBG_PRCTL_READ_MEM, &args, 0, 0, 0)
 */
struct kdbg_mem_args {
    uint64_t kaddr;         /* 内核虚拟地址 */
    uint64_t ubuf;          /* R3 用户态缓冲区指针 */
    uint32_t len;           /* 长度 */
    int32_t  result;        /* [out] 0=成功, 负数=错误码 */
};

/*
 * 调试器状态查询 (R3 ← R0)
 */
struct kdbg_state {
    uint32_t stopped;       /* 1=内核已停在断点 */
    uint32_t stopped_cpu;   /* 停止的 CPU */
    uint64_t stopped_pc;    /* 停止时的 PC */
    uint32_t num_sw_bp;     /* 活跃软件断点数 */
    uint32_t num_hw_bp;     /* 活跃硬件断点数 */
};

/*
 * SO 加载监控参数 (R3 → R0)
 * prctl(KDBG_PRCTL_MMAP_WATCH, &args, 0, 0, 0)
 */
struct kdbg_mmap_args {
    int32_t  pid;           /* 目标进程 PID */
    char     so_name[256];  /* 目标 SO 文件名 (如 "libnative.so") */
};

/*
 * SO 加载结果 (R0 → R3)
 * prctl(KDBG_PRCTL_WAIT_MMAP, &result, 0, 0, 0)
 */
struct kdbg_mmap_result {
    uint64_t base;          /* SO mmap 基地址 */
    uint64_t size;          /* 映射大小 */
    char     path[256];     /* 完整路径 */
};

/* ========== 常量 ========== */
#define KDBG_MAX_SW_BP  16      /* 最大软件断点数 */
#define KDBG_MAX_HW_BP  6       /* 最大硬件断点数 (ARM64 通常 4-6) */
#define KDBG_MAX_WP     4       /* 最大 watchpoint 数 */
#define KDBG_MEM_MAX    4096    /* 单次读写最大字节 */

#endif /* _KDBG_H_ */
