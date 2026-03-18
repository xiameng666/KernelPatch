/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * kpm_entry.c — Demo BRK: KPM 生命周期 + 符号解析 + ctl 分发
 *
 * 工作流程:
 *   1. load demo-brk.kpm (SO 名可选, 可后续通过 prctl 动态设置)
 *   2. Agent 通过 prctl 设置 spawn_watch → wait_spawn → mmap_watch → wait_mmap
 *   3. 冻结期间: Agent 通过 prctl SET_BP 设置断点
 *   4. Agent 发 SIGCONT 放行 → 进程运行 → 命中 BRK → 寄存器快照 → 冻结
 *   5. Agent 通过 prctl CONTINUE 恢复执行
 *
 * 也支持手动模式 (kpmctl ctl):
 *   load  demo-brk.kpm "libfoo.so"  → 监控所有进程的 mmap
 *   ctl release / brk 0x<addr> / cont / regs / status
 */

#include "demo-brk.h"
#include "prctl_cmd.h"
#include <syscall.h>

KPM_NAME("demo-brk");
KPM_VERSION("2.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("kdbg");
KPM_DESCRIPTION("SO mmap hook + BRK breakpoint + Agent prctl communication");

/* ========== 内核函数指针定义 ========== */

pid_t (*kfn___task_pid_nr_ns)(void *task, enum pid_type type, void *ns);
int   (*kfn_send_sig)(int sig, void *p, int priv);
int   (*kfn_access_process_vm)(void *tsk, unsigned long addr,
                               void *buf, int len, unsigned int gup_flags);
void  (*kfn___flush_icache_range)(unsigned long start, unsigned long end);
void  (*kfn_do_debug_exception)(unsigned long addr, unsigned int esr, void *regs);

unsigned long (*kfn_vm_mmap_pgoff)(void *file, unsigned long addr,
                                    unsigned long len, unsigned long prot,
                                    unsigned long flag, unsigned long pgoff);
char *(*kfn_d_path)(void *path, char *buf, int buflen);

/* prctl_hook.c 需要的额外符号 */
unsigned long (*kfn__copy_from_user)(void *to, const void __user *from,
                                     unsigned long n);
void (*kfn___set_task_comm)(void *tsk, const char *buf, int exec);
void *(*kfn_fget)(unsigned int fd);
void (*kfn_fput)(void *file);

/* ========== 符号解析辅助 ========== */

#define RESOLVE_SYM(var, name) \
    var = (typeof(var))kallsyms_lookup_name(name)

#define REQUIRE_SYM(var, name) do { \
    RESOLVE_SYM(var, name); \
    if (!(var)) { \
        pr_err(TAG ": symbol '%s' not found!\n", name); \
        return -1; \
    } \
} while (0)

#define OPTIONAL_SYM(var, name) do { \
    RESOLVE_SYM(var, name); \
    if (!(var)) \
        pr_warn(TAG ": symbol '%s' not found (optional)\n", name); \
} while (0)

/* ========== 十六进制字符串解析 ========== */

static unsigned long parse_hex(const char *s)
{
    unsigned long val = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    while (*s) {
        char c = *s++;
        unsigned int d;
        if (c >= '0' && c <= '9')      d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else break;
        val = (val << 4) | d;
    }
    return val;
}

static int parse_int(const char *s)
{
    int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val;
}

static const char *skip_ws(const char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/* ========== KPM 初始化 ========== */

static long demo_init(const char *kpm_args, const char *event, void *__user reserved)
{
    pr_info(TAG ": ==========================================\n");
    pr_info(TAG ":   Demo BRK v2 — SO Hook + BRK + prctl\n");
    pr_info(TAG ": ==========================================\n");

    if (!kpm_args || !kpm_args[0]) {
        pr_info(TAG ": no SO specified, waiting for Agent prctl\n");
    } else {
        int len = strlen(kpm_args);
        if (len >= (int)sizeof(g_manual_target_so))
            len = sizeof(g_manual_target_so) - 1;
        memcpy(g_manual_target_so, kpm_args, len);
        g_manual_target_so[len] = 0;
        pr_info(TAG ": manual target SO = '%s'\n", g_manual_target_so);
    }

    /* ---- 解析内核符号 ---- */
    OPTIONAL_SYM(kfn___task_pid_nr_ns,     "__task_pid_nr_ns");
    OPTIONAL_SYM(kfn_send_sig,             "send_sig");
    REQUIRE_SYM(kfn_access_process_vm,     "access_process_vm");
    REQUIRE_SYM(kfn_do_debug_exception,    "do_debug_exception");
    REQUIRE_SYM(kfn_vm_mmap_pgoff,         "vm_mmap_pgoff");
    REQUIRE_SYM(kfn_d_path,               "d_path");
    OPTIONAL_SYM(kfn___flush_icache_range, "__flush_icache_range");

    /* prctl 通信需要的额外符号 */
    OPTIONAL_SYM(kfn__copy_from_user,      "_copy_from_user");
    OPTIONAL_SYM(kfn___set_task_comm,      "__set_task_comm");
    OPTIONAL_SYM(kfn_fget,                 "fget");
    OPTIONAL_SYM(kfn_fput,                 "fput");

    if (!kfn__copy_from_user)
        pr_warn(TAG ": _copy_from_user not found, prctl struct commands won't work\n");
    if (!kfn___set_task_comm)
        pr_warn(TAG ": __set_task_comm not found, spawn detection won't work\n");

    /* ---- 初始化断点管理器 ---- */
    bp_init();

    /* ---- Hook vm_mmap_pgoff (SO 加载监控) ---- */
    hook_err_t err;
    err = hook_wrap6((void *)kfn_vm_mmap_pgoff,
                     (hook_chain6_callback)mmap_before_hook,
                     (hook_chain6_callback)mmap_after_hook, 0);
    if (err) {
        pr_err(TAG ": hook vm_mmap_pgoff failed: %d\n", err);
        return -1;
    }
    pr_info(TAG ": hooked vm_mmap_pgoff\n");

    /* ---- Hook do_debug_exception (BRK 捕获) ---- */
    err = hook_wrap3((void *)kfn_do_debug_exception,
                     (hook_chain3_callback)bp_on_debug_exception, 0, 0);
    if (err) {
        pr_err(TAG ": hook do_debug_exception failed: %d\n", err);
        hook_unwrap((void *)kfn_vm_mmap_pgoff,
                    (void *)mmap_before_hook, (void *)mmap_after_hook);
        return -1;
    }
    pr_info(TAG ": hooked do_debug_exception\n");

    /* ---- Hook prctl syscall (Agent 通信) ---- */
    err = hook_syscalln(PRCTL_NR, 4,
                        (hook_chain4_callback)prctl_before_hook, 0, 0);
    if (err) {
        pr_err(TAG ": hook prctl failed: %d\n", err);
        hook_unwrap((void *)kfn_do_debug_exception,
                    (void *)bp_on_debug_exception, 0);
        hook_unwrap((void *)kfn_vm_mmap_pgoff,
                    (void *)mmap_before_hook, (void *)mmap_after_hook);
        return -1;
    }
    pr_info(TAG ": hooked prctl (Agent communication ready)\n");

    /* ---- Hook __set_task_comm (Spawn 进程名检测) ---- */
    if (kfn___set_task_comm) {
        err = hook_wrap3((void *)kfn___set_task_comm,
                         (hook_chain3_callback)prctl_on_set_task_comm, 0, 0);
        if (err) {
            pr_warn(TAG ": hook __set_task_comm failed: %d, spawn detection disabled\n", err);
        } else {
            pr_info(TAG ": hooked __set_task_comm (spawn detection ready)\n");
        }
    }

    if (g_manual_target_so[0])
        pr_info(TAG ": manual mode: waiting for '%s' to be loaded...\n",
                g_manual_target_so);
    else
        pr_info(TAG ": all hooks active, waiting for Agent prctl commands\n");

    return 0;
}

/* ========== KPM 控制命令 (kpmctl ctl, 手动模式) ========== */

static long demo_control(const char *ctl_args, char *__user out_msg, int outlen)
{
    if (!ctl_args)
        return 0;

    const char *args = ctl_args;

    /* ---- release ---- */
    if (!strcmp(args, "release"))
        return proc_release();

    /* ---- cont ---- */
    if (!strcmp(args, "cont"))
        return bp_continue();

    /* ---- regs [id] ---- */
    if (!strncmp(args, "regs", 4)) {
        args = skip_ws(args + 4);
        int id = *args ? parse_int(args) : -1;
        bp_dump_regs(id);
        return 0;
    }

    /* ---- brk 0x<addr>: 快捷命令 — 创建 + 激活 ---- */
    if (!strncmp(args, "brk ", 4)) {
        args = skip_ws(args + 4);
        unsigned long addr = parse_hex(args);
        int id = bp_add(BP_SW_BRK, addr);
        if (id < 0) return -1;
        return bp_arm(id);
    }

    /* ---- bp add sw|hw 0x<addr> ---- */
    if (!strncmp(args, "bp add ", 7)) {
        args = skip_ws(args + 7);
        enum bp_type type = BP_SW_BRK;
        if (!strncmp(args, "sw ", 3)) {
            type = BP_SW_BRK;
            args = skip_ws(args + 3);
        } else if (!strncmp(args, "hw ", 3)) {
            type = BP_HW_EXEC;
            args = skip_ws(args + 3);
        }
        unsigned long addr = parse_hex(args);
        int id = bp_add(type, addr);
        return (id > 0) ? 0 : -1;
    }

    /* ---- bp del <id> ---- */
    if (!strncmp(args, "bp del ", 7)) {
        int id = parse_int(skip_ws(args + 7));
        return bp_remove(id);
    }

    /* ---- bp arm <id|all> ---- */
    if (!strncmp(args, "bp arm", 6)) {
        args = skip_ws(args + 6);
        if (!*args || !strncmp(args, "all", 3))
            return bp_arm(-1);
        return bp_arm(parse_int(args));
    }

    /* ---- bp disarm <id> ---- */
    if (!strncmp(args, "bp disarm ", 10)) {
        int id = parse_int(skip_ws(args + 10));
        return bp_disarm(id);
    }

    /* ---- bp list ---- */
    if (!strncmp(args, "bp list", 7) || !strcmp(args, "bp")) {
        bp_list();
        return 0;
    }

    /* ---- status ---- */
    if (!strcmp(args, "status")) {
        pr_info(TAG ": === Status ===\n");
        pr_info(TAG ":   target_pid=%d task=%p\n", g_target_pid, g_target_task);
        pr_info(TAG ":   manual_so='%s' frozen=%d\n",
                g_manual_target_so, g_mmap_frozen);
        pr_info(TAG ":   prctl_so='%s' watching=%d found=%d\n",
                g_mmap_target_so, g_mmap_watching, g_mmap_so_found);
        pr_info(TAG ":   so_base=0x%llx size=0x%llx\n",
                g_mmap_so_base, g_mmap_so_size);
        pr_info(TAG ":   spawn_watching=%d found=%d pid=%d target='%s'\n",
                g_spawn_watching, g_spawn_found, g_spawn_pid,
                g_spawn_target_comm);
        bp_list();
        return 0;
    }

    pr_info(TAG ": unknown command '%s'\n", args);
    pr_info(TAG ": commands:\n");
    pr_info(TAG ":   release              — 放行 mmap 冻结\n");
    pr_info(TAG ":   brk 0x<addr>         — 快捷设断点 (创建+激活)\n");
    pr_info(TAG ":   cont                 — BRK 命中后继续\n");
    pr_info(TAG ":   regs [id]            — 查看寄存器快照\n");
    pr_info(TAG ":   bp add sw 0x<addr>   — 添加软件断点\n");
    pr_info(TAG ":   bp del <id>          — 删除断点\n");
    pr_info(TAG ":   bp arm <id|all>      — 激活断点\n");
    pr_info(TAG ":   bp disarm <id>       — 停用断点\n");
    pr_info(TAG ":   bp list              — 列出断点\n");
    pr_info(TAG ":   status               — 总览\n");
    return 0;
}

/* ========== KPM 卸载 ========== */

static long demo_exit(void *__user reserved)
{
    pr_info(TAG ": unloading...\n");

    /* 清理断点 + 释放 HIT 等待 */
    bp_cleanup();

    /* 释放手动模式冻结 */
    if (g_mmap_frozen) {
        pr_info(TAG ": auto-release pid=%d on unload\n", g_target_pid);
        if (kfn_send_sig && g_target_task)
            kfn_send_sig(18, g_target_task, 1);
        g_mmap_frozen = 0;
    }

    /* 取消 spawn/mmap 监控 (解除 busy-wait) */
    g_spawn_watching = 0;
    g_mmap_watching = 0;

    /* 卸载 hooks (逆序) */
    if (kfn___set_task_comm)
        hook_unwrap((void *)kfn___set_task_comm,
                    (void *)prctl_on_set_task_comm, 0);

    unhook_syscalln(PRCTL_NR, (void *)prctl_before_hook, 0);

    hook_unwrap((void *)kfn_do_debug_exception,
                (void *)bp_on_debug_exception, 0);

    hook_unwrap((void *)kfn_vm_mmap_pgoff,
                (void *)mmap_before_hook, (void *)mmap_after_hook);

    pr_info(TAG ": unloaded\n");
    return 0;
}

KPM_INIT(demo_init);
KPM_CTL0(demo_control);
KPM_EXIT(demo_exit);
