/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * breakpoint.c — 断点管理器
 *
 * 支持多断点、多类型 (软件 BRK / 硬件 BP / 硬件 WP)。
 * 当前实现: 软件 BRK。硬件断点为预留接口, 后续 demo 逐步填充。
 *
 * 每个软件断点使用不同的 BRK #imm (BRK_IMM_BASE + slot_index),
 * 在 do_debug_exception hook 中通过 ESR 的 comment 字段区分是哪个断点。
 *
 * 生命周期:
 *   bp_add()  → PENDING
 *   bp_arm()  → ARMED  (写入 BRK 指令)
 *   命中      → HIT    (寄存器快照 + 冻结)
 *   bp_continue() → 恢复原指令, 回到 FREE 或 re-arm
 */

#include "demo-brk.h"

/* ========== 全局断点管理器 ========== */

struct bp_manager g_bpm = {0};

/* ========== 内部辅助 ========== */

static struct breakpoint *bp_find(int id)
{
    if (id < 1 || id > BP_MAX_SLOTS)
        return 0;
    struct breakpoint *bp = &g_bpm.slots[id - 1];
    if (bp->type == BP_NONE)
        return 0;
    return bp;
}

/* 通过 BRK immediate 查找断点 */
static struct breakpoint *bp_find_by_imm(uint16_t imm)
{
    int i;
    for (i = 0; i < BP_MAX_SLOTS; i++) {
        struct breakpoint *bp = &g_bpm.slots[i];
        if (bp->type == BP_SW_BRK && bp->state == BP_ARMED && bp->brk_imm == imm)
            return bp;
    }
    return 0;
}

/* 通过地址查找 armed 的断点 */
static struct breakpoint *bp_find_by_addr(unsigned long addr)
{
    int i;
    for (i = 0; i < BP_MAX_SLOTS; i++) {
        struct breakpoint *bp = &g_bpm.slots[i];
        if (bp->state == BP_ARMED && bp->addr == addr)
            return bp;
    }
    return 0;
}

/* ========== 初始化 ========== */

void bp_init(void)
{
    memset(&g_bpm, 0, sizeof(g_bpm));
    pr_info(TAG ": breakpoint manager initialized (%d slots)\n", BP_MAX_SLOTS);
}

/* ========== bp_add: 创建断点 ========== */

int bp_add(enum bp_type type, unsigned long addr)
{
    if (!addr) {
        pr_err(TAG ": bp_add: invalid address 0\n");
        return -1;
    }

    /* 查找空槽位 */
    int i;
    struct breakpoint *bp = 0;
    for (i = 0; i < BP_MAX_SLOTS; i++) {
        if (g_bpm.slots[i].type == BP_NONE) {
            bp = &g_bpm.slots[i];
            break;
        }
    }
    if (!bp) {
        pr_err(TAG ": bp_add: no free slots (max=%d)\n", BP_MAX_SLOTS);
        return -1;
    }

    /* 检查地址是否已有断点 */
    int j;
    for (j = 0; j < BP_MAX_SLOTS; j++) {
        if (g_bpm.slots[j].type != BP_NONE && g_bpm.slots[j].addr == addr) {
            pr_warn(TAG ": bp_add: address 0x%lx already has bp #%d\n",
                    addr, g_bpm.slots[j].id);
            return g_bpm.slots[j].id;
        }
    }

    memset(bp, 0, sizeof(*bp));
    bp->id = i + 1;
    bp->type = type;
    bp->state = BP_PENDING;
    bp->addr = addr;

    if (type == BP_SW_BRK) {
        /* 每个软件断点用不同的 BRK immediate */
        bp->brk_imm = BRK_IMM_BASE + (uint16_t)i;
    }

    g_bpm.count++;
    pr_info(TAG ": bp #%d created: type=%d addr=0x%lx\n", bp->id, type, addr);
    return bp->id;
}

/* ========== bp_remove: 删除断点 ========== */

int bp_remove(int id)
{
    struct breakpoint *bp = bp_find(id);
    if (!bp) {
        pr_err(TAG ": bp_remove: #%d not found\n", id);
        return -1;
    }

    /* 如果还 armed, 先 disarm */
    if (bp->state == BP_ARMED)
        bp_disarm(id);

    pr_info(TAG ": bp #%d removed (addr=0x%lx, hits=%u)\n",
            id, bp->addr, bp->hit_count);

    memset(bp, 0, sizeof(*bp));
    g_bpm.count--;
    return 0;
}

/* ========== bp_arm: 激活断点 ========== */

static int bp_arm_sw(struct breakpoint *bp)
{
    if (!kfn_access_process_vm) {
        pr_err(TAG ": access_process_vm not available\n");
        return -1;
    }

    void *task = g_target_task;
    if (!task) {
        pr_err(TAG ": no target task (must arm while frozen)\n");
        return -1;
    }

    /* 读取原指令 */
    uint32_t orig = 0;
    int n = proc_read_mem(task, bp->addr, &orig, 4);
    if (n != 4) {
        pr_err(TAG ": bp #%d: failed to read insn at 0x%lx (n=%d)\n",
               bp->id, bp->addr, n);
        return -1;
    }
    bp->orig_insn = orig;

    /* 写入 BRK #imm */
    uint32_t brk = BRK_INSN(bp->brk_imm);
    n = proc_write_mem(task, bp->addr, &brk, 4);
    if (n != 4) {
        pr_err(TAG ": bp #%d: failed to write BRK at 0x%lx (n=%d)\n",
               bp->id, bp->addr, n);
        return -1;
    }

    /* 刷新指令缓存 */
    if (kfn___flush_icache_range)
        kfn___flush_icache_range(bp->addr, bp->addr + 4);

    /* 验证 */
    uint32_t verify = 0;
    proc_read_mem(task, bp->addr, &verify, 4);

    bp->state = BP_ARMED;
    pr_info(TAG ": bp #%d ARMED: addr=0x%lx orig=0x%08x brk=0x%08x (verify=0x%08x)\n",
            bp->id, bp->addr, orig, brk, verify);
    return 0;
}

int bp_arm(int id)
{
    /* id == -1: arm all pending */
    if (id == -1) {
        int i, armed = 0;
        for (i = 0; i < BP_MAX_SLOTS; i++) {
            struct breakpoint *bp = &g_bpm.slots[i];
            if (bp->type != BP_NONE && bp->state == BP_PENDING) {
                if (bp_arm(bp->id) == 0)
                    armed++;
            }
        }
        pr_info(TAG ": armed %d breakpoints\n", armed);
        return armed;
    }

    struct breakpoint *bp = bp_find(id);
    if (!bp) {
        pr_err(TAG ": bp_arm: #%d not found\n", id);
        return -1;
    }
    if (bp->state == BP_ARMED) {
        pr_info(TAG ": bp #%d already armed\n", id);
        return 0;
    }
    if (bp->state != BP_PENDING && bp->state != BP_DISABLED) {
        pr_err(TAG ": bp #%d: cannot arm from state %d\n", id, bp->state);
        return -1;
    }

    switch (bp->type) {
    case BP_SW_BRK:
        return bp_arm_sw(bp);
    case BP_HW_EXEC:
    case BP_HW_WATCH_R:
    case BP_HW_WATCH_W:
    case BP_HW_WATCH_RW:
        pr_err(TAG ": hardware breakpoints not yet implemented\n");
        return -1;
    default:
        return -1;
    }
}

/* ========== bp_disarm: 停用断点 (恢复原指令) ========== */

static int bp_disarm_sw(struct breakpoint *bp, void *task)
{
    if (!task) {
        pr_warn(TAG ": bp #%d: no task for disarm, insn not restored\n", bp->id);
        bp->state = BP_DISABLED;
        return 0;
    }

    int n = proc_write_mem(task, bp->addr, &bp->orig_insn, 4);
    if (n == 4) {
        if (kfn___flush_icache_range)
            kfn___flush_icache_range(bp->addr, bp->addr + 4);
        pr_info(TAG ": bp #%d disarmed: restored insn 0x%08x at 0x%lx\n",
                bp->id, bp->orig_insn, bp->addr);
    } else {
        pr_err(TAG ": bp #%d: failed to restore insn (n=%d)\n", bp->id, n);
    }

    bp->state = BP_DISABLED;
    return 0;
}

int bp_disarm(int id)
{
    struct breakpoint *bp = bp_find(id);
    if (!bp) return -1;
    if (bp->state != BP_ARMED) return 0;

    void *task = g_target_task ? g_target_task : get_current_task();

    switch (bp->type) {
    case BP_SW_BRK:
        return bp_disarm_sw(bp, task);
    default:
        bp->state = BP_DISABLED;
        return 0;
    }
}

/* ========== do_debug_exception hook ========== */

/*
 * do_debug_exception(addr_if_watchpoint, esr, regs) before hook
 *
 * ESR 解析:
 *   EC    = bits[31:26]  → 0x3C = BRK from AArch64
 *   ISS   = bits[24:0]
 *   BRK comment = bits[20:5] of ISS  (即 BRK #imm 的值)
 */
void bp_on_debug_exception(hook_fargs3_t *args, void *udata)
{
    /* 没有活跃断点, 快速退出 */
    if (g_bpm.count == 0)
        return;

    unsigned int esr = (unsigned int)args->arg1;

    /* 只处理 BRK (EC = 0x3C) */
    unsigned int ec = (esr >> 26) & 0x3F;
    if (ec != 0x3C)
        return;

    /* 只处理目标进程 */
    void *task = get_current_task();
    pid_t cur_pid = kfn___task_pid_nr_ns
        ? kfn___task_pid_nr_ns(task, PIDTYPE_TGID, 0) : -1;
    if (cur_pid != g_target_pid)
        return;

    /* 从 ESR 提取 BRK immediate: ISS bits[20:5] */
    uint16_t imm = (uint16_t)((esr >> 5) & 0xFFFF);

    /* 优先用 imm 查找, 兜底用 PC 查找 */
    struct breakpoint *bp = bp_find_by_imm(imm);
    if (!bp) {
        void *regs = (void *)args->arg2;
        unsigned long pc = *(unsigned long *)((char *)regs + PT_REGS_PC);
        bp = bp_find_by_addr(pc);
    }
    if (!bp)
        return;

    void *regs = (void *)args->arg2;
    unsigned long pc = *(unsigned long *)((char *)regs + PT_REGS_PC);

    /* =============== BREAKPOINT HIT =============== */

    bp->hit_count++;
    bp->state = BP_HIT;

    /* 保存完整寄存器快照 */
    memcpy(bp->saved_regs, regs, PT_REGS_COUNT * sizeof(unsigned long));

    pr_info(TAG ": *** BP #%d HIT (count=%u) ***\n", bp->id, bp->hit_count);
    pr_info(TAG ":   PC = 0x%lx  PID = %d\n", pc, cur_pid);

    /* 打印关键寄存器 */
    int i;
    for (i = 0; i <= 7; i++)
        pr_info(TAG ":   X%-2d = 0x%016lx\n", i, bp->saved_regs[i]);
    pr_info(TAG ":   X30(LR) = 0x%016lx\n", bp->saved_regs[30]);
    pr_info(TAG ":   SP  = 0x%016lx\n", bp->saved_regs[31]);

    /* 恢复原指令 */
    int n = proc_write_mem(task, bp->addr, &bp->orig_insn, 4);
    if (n == 4) {
        if (kfn___flush_icache_range)
            kfn___flush_icache_range(bp->addr, bp->addr + 4);
        pr_info(TAG ":   insn restored: 0x%08x\n", bp->orig_insn);
    } else {
        pr_err(TAG ":   FAILED to restore insn (n=%d)\n", n);
    }

    /* 跳过原始 do_debug_exception (防止 SIGTRAP) */
    args->skip_origin = 1;

    /* 标记全局状态 */
    g_bpm.any_hit = 1;
    g_bpm.active_hit_id = bp->id;

    pr_info(TAG ":   FROZEN at bp #%d, use 'ctl cont' to resume\n", bp->id);

    /* 冻结等待 */
    while (g_bpm.any_hit)
        asm volatile("yield");

    /*
     * cont 后:
     *   - 原指令已恢复
     *   - skip_origin=1 跳过了 do_debug_exception
     *   - 异常返回 eret → ELR_EL1 = 断点地址 → 执行原指令
     */
    pr_info(TAG ":   bp #%d continuing...\n", bp->id);
}

/* ========== bp_continue: 从 HIT 状态恢复 ========== */

int bp_continue(void)
{
    if (!g_bpm.any_hit) {
        pr_info(TAG ": not at any breakpoint\n");
        return 0;
    }

    int id = g_bpm.active_hit_id;
    struct breakpoint *bp = bp_find(id);

    if (bp) {
        /* one-shot: 命中后回到 DISABLED */
        bp->state = BP_DISABLED;
        pr_info(TAG ": bp #%d → cont (one-shot, now disabled)\n", id);
    }

    g_bpm.active_hit_id = 0;
    g_bpm.any_hit = 0;   /* 最后清, 解除 busy-wait */
    return 0;
}

/* ========== bp_dump_regs ========== */

void bp_dump_regs(int id)
{
    /* id == -1: 使用当前 HIT 的 */
    if (id == -1)
        id = g_bpm.active_hit_id;

    struct breakpoint *bp = bp_find(id);
    if (!bp || bp->hit_count == 0) {
        pr_info(TAG ": no register snapshot for bp #%d\n", id);
        return;
    }

    pr_info(TAG ": === BP #%d Register Snapshot (hit #%u) ===\n",
            id, bp->hit_count);
    int i;
    for (i = 0; i < 31; i += 2) {
        pr_info(TAG ":   X%-2d = 0x%016lx    X%-2d = 0x%016lx\n",
                i, bp->saved_regs[i],
                i + 1, bp->saved_regs[i + 1]);
    }
    pr_info(TAG ":   SP     = 0x%016lx\n", bp->saved_regs[31]);
    pr_info(TAG ":   PC     = 0x%016lx\n", bp->saved_regs[32]);
    pr_info(TAG ":   PSTATE = 0x%016lx\n", bp->saved_regs[33]);
    pr_info(TAG ": ==========================================\n");
}

/* ========== bp_list: 列出所有断点 ========== */

static const char *bp_type_str(enum bp_type t)
{
    switch (t) {
    case BP_SW_BRK:      return "SW-BRK";
    case BP_HW_EXEC:     return "HW-BP";
    case BP_HW_WATCH_R:  return "HW-WP-R";
    case BP_HW_WATCH_W:  return "HW-WP-W";
    case BP_HW_WATCH_RW: return "HW-WP-RW";
    default:             return "?";
    }
}

static const char *bp_state_str(enum bp_state s)
{
    switch (s) {
    case BP_FREE:     return "FREE";
    case BP_PENDING:  return "PENDING";
    case BP_ARMED:    return "ARMED";
    case BP_HIT:      return "HIT";
    case BP_DISABLED: return "DISABLED";
    default:          return "?";
    }
}

void bp_list(void)
{
    pr_info(TAG ": === Breakpoints (%d active) ===\n", g_bpm.count);
    int i;
    for (i = 0; i < BP_MAX_SLOTS; i++) {
        struct breakpoint *bp = &g_bpm.slots[i];
        if (bp->type == BP_NONE)
            continue;
        pr_info(TAG ":   #%-2d  %-8s  %-8s  addr=0x%lx  hits=%u\n",
                bp->id, bp_type_str(bp->type), bp_state_str(bp->state),
                bp->addr, bp->hit_count);
    }
    if (g_bpm.count == 0)
        pr_info(TAG ":   (empty)\n");
    pr_info(TAG ": ===============================\n");
}

/* ========== bp_cleanup ========== */

void bp_cleanup(void)
{
    int i;
    for (i = 0; i < BP_MAX_SLOTS; i++) {
        struct breakpoint *bp = &g_bpm.slots[i];
        if (bp->type == BP_NONE)
            continue;
        if (bp->state == BP_ARMED)
            bp_disarm(bp->id);
    }

    /* 释放所有 HIT 等待 */
    if (g_bpm.any_hit) {
        g_bpm.any_hit = 0;
    }

    memset(&g_bpm, 0, sizeof(g_bpm));
    pr_info(TAG ": breakpoint manager cleaned up\n");
}
