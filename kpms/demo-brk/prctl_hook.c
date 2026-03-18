/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * prctl_hook.c — Agent ↔ KPM 通信 (prctl syscall hook)
 *
 * 职责:
 *   1. Hook prctl syscall, 拦截 0x4B4442xx 命令码
 *   2. 分发 Spawn/Mmap/断点/内存/寄存器 等命令
 *   3. Hook __set_task_comm, 检测目标进程启动
 *
 * Agent 调用 prctl(KDBG_PRCTL_xxx, arg2, ...) → KPM 拦截 → 处理 → 返回
 * 无需注入, 无需 /dev 设备, 无需 root 文件系统修改
 */

#include "demo-brk.h"
#include "prctl_cmd.h"
#include <syscall.h>
#include <kputils.h>

/* ========== Spawn 监控状态 ========== */

char g_spawn_target_comm[256]      = {0};
volatile int g_spawn_watching      = 0;
volatile int g_spawn_found         = 0;
int g_spawn_pid                    = 0;

/* ========== 额外内核函数指针 ========== */

/* 这些在 kpm_entry.c 中解析, 这里 extern 引用 */
extern unsigned long (*kfn__copy_from_user)(void *to, const void __user *from,
                                             unsigned long n);
extern void (*kfn___set_task_comm)(void *tsk, const char *buf, int exec);
extern void *(*kfn_fget)(unsigned int fd);
extern void (*kfn_fput)(void *file);

/* ========== __set_task_comm Hook (Spawn 进程名检测) ========== */

/*
 * void __set_task_comm(struct task_struct *tsk, const char *buf, bool exec)
 *
 * Android 进程启动时 zygote fork → prctl(PR_SET_NAME) → __set_task_comm
 * buf 是内核内存中的字符串, 无需 copy_from_user
 */
void prctl_on_set_task_comm(hook_fargs3_t *args, void *udata)
{
    if (!g_spawn_watching)
        return;

    void *tsk = (void *)args->arg0;
    const char *name = (const char *)args->arg1;

    if (!name || !tsk)
        return;

    pid_t tgid = kfn___task_pid_nr_ns
        ? kfn___task_pid_nr_ns(tsk, PIDTYPE_TGID, 0) : -1;

    pr_info(TAG ": set_task_comm pid=%d name='%s' target='%s'\n",
            tgid, name, g_spawn_target_comm);

    if (!g_spawn_found && strstr(g_spawn_target_comm, name)) {
        g_spawn_pid = tgid;
        g_spawn_found = 1;
        g_spawn_watching = 0;

        /* SIGSTOP 冻结进程 */
        if (kfn_send_sig)
            kfn_send_sig(19 /* SIGSTOP */, tsk, 1);

        pr_info(TAG ": spawn detected: '%s' pid=%d, frozen\n",
                name, g_spawn_pid);
    }
}

/* ========== prctl Syscall Hook ========== */

void prctl_before_hook(hook_fargs4_t *args, void *udata)
{
    int option  = (int)syscall_argn(args, 0);
    long uarg2  = (long)syscall_argn(args, 1);
    long uarg3  = (long)syscall_argn(args, 2);

    /* 快速过滤: 不是我们的命令码就放行 */
    if ((option & 0xFFFFFF00) != KDBG_PRCTL_PREFIX)
        return;

    /* 拦截, 不再调用原始 prctl */
    args->skip_origin = 1;

    switch (option) {

    /* ========== Spawn 监控 ========== */

    case KDBG_PRCTL_SPAWN_WATCH: {
        /* arg2 = 用户态字符串 (目标包名) */
        if (compat_strncpy_from_user(g_spawn_target_comm, (const char *)uarg2,
                                      sizeof(g_spawn_target_comm) - 1) <= 0) {
            pr_err(TAG ": SPAWN_WATCH: read user string failed\n");
            args->ret = -1; return;
        }
        g_spawn_target_comm[sizeof(g_spawn_target_comm) - 1] = 0;
        g_spawn_found = 0;
        g_spawn_pid = 0;
        g_spawn_watching = 1;
        pr_info(TAG ": spawn watch started for '%s'\n", g_spawn_target_comm);
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WAIT_SPAWN: {
        /* 自旋等待目标进程出现 */
        while (!g_spawn_found && g_spawn_watching)
            asm volatile("yield");

        if (g_spawn_found) {
            args->ret = g_spawn_pid;
            pr_info(TAG ": spawn found pid=%d\n", g_spawn_pid);
        } else {
            args->ret = -1;  /* 被取消 */
        }
        return;
    }

    case KDBG_PRCTL_CANCEL_SPAWN: {
        g_spawn_watching = 0;
        g_spawn_found = 0;
        g_spawn_pid = 0;
        g_mmap_watching = 0;  /* 同时取消 mmap 监控 */
        pr_info(TAG ": spawn/mmap watch cancelled\n");
        args->ret = 0;
        return;
    }

    /* ========== Mmap SO 监控 ========== */

    case KDBG_PRCTL_MMAP_WATCH: {
        /* arg2 = &kdbg_mmap_args (用户态) */
        struct kdbg_mmap_args ma;
        if (!kfn__copy_from_user ||
            kfn__copy_from_user(&ma, (void *)uarg2, sizeof(ma))) {
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
        g_mmap_watching = 1;

        pr_info(TAG ": mmap watch started for '%s' pid=%d\n",
                g_mmap_target_so, g_target_pid);
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WAIT_MMAP: {
        /* 自旋等待 SO 加载 */
        while (!g_mmap_so_found && g_mmap_watching)
            asm volatile("yield");

        if (!g_mmap_so_found) {
            args->ret = -1; return;
        }

        /* arg2 = &kdbg_mmap_result (用户态, 输出) */
        struct kdbg_mmap_result mr;
        mr.base = g_mmap_so_base;
        mr.size = g_mmap_so_size;
        memcpy(mr.path, g_mmap_so_path, sizeof(mr.path));
        if (compat_copy_to_user((void *)uarg2, &mr, sizeof(mr))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        pr_info(TAG ": mmap result: base=0x%llx size=0x%llx\n",
                mr.base, mr.size);
        return;
    }

    case KDBG_PRCTL_RESUME: {
        /* arg2 = PID, 发 SIGCONT */
        int target_pid = (int)uarg2;
        pr_info(TAG ": resume request for pid=%d\n", target_pid);
        /* Agent 端通过 sys_kill(pid, SIGCONT) 做, 这里只是确认 */
        args->ret = 0;
        return;
    }

    /* ========== 断点操作 ========== */

    case KDBG_PRCTL_SET_BP: {
        /* arg2 = &kdbg_bp_args (用户态) */
        struct kdbg_bp_args ba;
        if (!kfn__copy_from_user ||
            kfn__copy_from_user(&ba, (void *)uarg2, sizeof(ba))) {
            args->ret = -1; return;
        }

        enum bp_type type;
        switch (ba.bp_type) {
        case 0: type = BP_SW_BRK;      break;
        case 1: type = BP_HW_EXEC;     break;
        case 2: type = BP_HW_WATCH_R;  break;
        case 3: type = BP_HW_WATCH_W;  break;
        case 4: type = BP_HW_WATCH_RW; break;
        default: args->ret = -1; return;
        }

        int id = bp_add(type, (unsigned long)ba.addr);
        if (id < 0) { args->ret = -1; return; }

        /* 自动 arm */
        if (bp_arm(id) < 0) {
            bp_remove(id);
            args->ret = -1; return;
        }

        /* 写回 id */
        ba.id = id;
        compat_copy_to_user((void *)uarg2, &ba, sizeof(ba));

        pr_info(TAG ": bp #%d set at 0x%llx (type=%d)\n", id, ba.addr, ba.bp_type);
        args->ret = id;
        return;
    }

    case KDBG_PRCTL_REMOVE_BP: {
        int bp_id = (int)uarg2;
        args->ret = bp_remove(bp_id);
        return;
    }

    case KDBG_PRCTL_SINGLE_STEP: {
        /* TODO: 单步需要 do_debug_exception 支持 */
        pr_info(TAG ": single step requested (not yet implemented)\n");
        if (!g_bpm.any_hit) { args->ret = -1; return; }
        /* 暂时当作 continue */
        args->ret = bp_continue();
        return;
    }

    case KDBG_PRCTL_CONTINUE: {
        args->ret = bp_continue();
        return;
    }

    /* ========== 内存读写 ========== */

    case KDBG_PRCTL_READ_MEM: {
        /* arg2 = &kdbg_mem_args */
        struct kdbg_mem_args ma;
        if (!kfn__copy_from_user ||
            kfn__copy_from_user(&ma, (void *)uarg2, sizeof(ma))) {
            args->ret = -1; return;
        }
        if (ma.len > KDBG_MEM_MAX) ma.len = KDBG_MEM_MAX;

        if (!g_target_task) { args->ret = -1; return; }

        char tmp[KDBG_MEM_MAX];
        int n = proc_read_mem(g_target_task, (unsigned long)ma.kaddr, tmp, ma.len);
        if (n != (int)ma.len) {
            pr_warn(TAG ": read_mem: got %d, wanted %d\n", n, ma.len);
            args->ret = -1; return;
        }

        if (compat_copy_to_user((void *)ma.ubuf, tmp, ma.len)) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WRITE_MEM: {
        struct kdbg_mem_args ma;
        if (!kfn__copy_from_user ||
            kfn__copy_from_user(&ma, (void *)uarg2, sizeof(ma))) {
            args->ret = -1; return;
        }
        if (ma.len > KDBG_MEM_MAX) ma.len = KDBG_MEM_MAX;

        if (!g_target_task) { args->ret = -1; return; }

        char tmp[KDBG_MEM_MAX];
        if (kfn__copy_from_user(tmp, (void *)ma.ubuf, ma.len)) {
            args->ret = -1; return;
        }

        int n = proc_write_mem(g_target_task, (unsigned long)ma.kaddr, tmp, ma.len);
        args->ret = (n == (int)ma.len) ? 0 : -1;
        return;
    }

    /* ========== 寄存器读写 ========== */

    case KDBG_PRCTL_READ_REGS: {
        if (!g_bpm.any_hit) { args->ret = -1; return; }

        int hit_id = g_bpm.active_hit_id;
        struct breakpoint *bp = (hit_id >= 1 && hit_id <= BP_MAX_SLOTS)
            ? &g_bpm.slots[hit_id - 1] : 0;
        if (!bp || bp->hit_count == 0) { args->ret = -1; return; }

        /* 从 saved_regs 构建 kdbg_regs */
        struct kdbg_regs kr;
        int i;
        for (i = 0; i < 31; i++)
            kr.regs[i] = bp->saved_regs[i];
        kr.sp     = bp->saved_regs[31];
        kr.pc     = bp->saved_regs[32];
        kr.pstate = bp->saved_regs[33];

        if (compat_copy_to_user((void *)uarg2, &kr, sizeof(kr))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_WRITE_REGS: {
        /* TODO: 写回寄存器到断点上下文 */
        pr_info(TAG ": write_regs not yet supported\n");
        args->ret = -1;
        return;
    }

    /* ========== 事件/状态查询 ========== */

    case KDBG_PRCTL_WAIT_EVENT: {
        /* 自旋等待断点命中 */
        while (!g_bpm.any_hit)
            asm volatile("yield");

        struct kdbg_event evt;
        evt.type = 1;  /* BREAKPOINT */
        evt.cpu = 0;
        evt.bp_id = g_bpm.active_hit_id;

        int hit_id = g_bpm.active_hit_id;
        struct breakpoint *bp = (hit_id >= 1 && hit_id <= BP_MAX_SLOTS)
            ? &g_bpm.slots[hit_id - 1] : 0;
        evt.pc = bp ? bp->saved_regs[32] : 0;
        evt.addr = bp ? bp->addr : 0;
        evt.reserved = 0;

        if (compat_copy_to_user((void *)uarg2, &evt, sizeof(evt))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_QUERY_STATE: {
        struct kdbg_state st;
        st.stopped = g_bpm.any_hit ? 1 : 0;
        st.stopped_cpu = 0;
        st.stopped_pc = 0;
        st.num_sw_bp = g_bpm.count;
        st.num_hw_bp = 0;

        if (g_bpm.any_hit) {
            int hit_id = g_bpm.active_hit_id;
            struct breakpoint *bp = (hit_id >= 1 && hit_id <= BP_MAX_SLOTS)
                ? &g_bpm.slots[hit_id - 1] : 0;
            if (bp) st.stopped_pc = bp->saved_regs[32];
        }

        if (compat_copy_to_user((void *)uarg2, &st, sizeof(st))) {
            args->ret = -1; return;
        }
        args->ret = 0;
        return;
    }

    case KDBG_PRCTL_DETACH: {
        /* 清理所有断点, 恢复执行 */
        if (g_bpm.any_hit)
            bp_continue();
        bp_cleanup();
        g_target_pid = 0;
        g_target_task = 0;
        g_spawn_watching = 0;
        g_mmap_watching = 0;
        pr_info(TAG ": detached\n");
        args->ret = 0;
        return;
    }

    default:
        pr_warn(TAG ": unknown prctl command 0x%x\n", option);
        args->ret = -1;
        return;
    }
}
