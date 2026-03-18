/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * function_utils.c - kernel symbol & dispatch table probing
 *
 * Symbol address lookup, debug_fault_info[] table verification
 */

#include "kdbg-probe.h"

/* ================================================================
 * 1. Symbol address probe
 * ================================================================ */

struct sym_entry {
    const char *name;
    const char *purpose;
};

static struct sym_entry g_symbols[] = {
/* debug exception core */

    //产生任何调试异常（断点/单步/watchpoint/BRK）都会进入这个函数
    {"do_debug_exception",       "HOOK TARGET: all debug exception entry"},

    /*
    一个 struct fault_info[] 数组。
    do_debug_exception 内部用 ESR bits[29:27] 作为索引查这个表，找到对应的 handler函数指针去调用。
    我们hook 了 do_debug_exception 入口所以直接绕过了这张表，但探测它是为了验证内核结构没被魔改
    */
    {"debug_fault_info",         "dispatch table: ESR -> handler array"},

/* 分发表里的四个 handler — 内核原生处理函数 */
    //处理硬件执行断点 (ESR EC=0x30/0x31)。遍历 DBGBVR/DBGBCR寄存器找到命中的断点，触发 perf_bp_event，然后禁用断点+启用单步来step-over
    {"breakpoint_handler",       "table slot[0]: HW breakpoint handler"},

    // 处理单步完成异常 (ESR EC=0x32/0x33)。先调 reinstall_suspended_bps 恢复之前为 step-over 临时禁用的断点，再遍历step_hook 链表。未处理则发 SIGTRAP 给用户进程
    {"single_step_handler",      "table slot[1]: single-step handler"}, 

    //处理数据 watchpoint (ESR EC=0x34/0x35)。从 FAR 寄存器获取触发地址，用距离启发式匹配 DBGWVR，判断读/写类型（ESR bit 6），触发 perf 事件
    {"watchpoint_handler",       "table slot[2]: watchpoint handler"},

    //处理 BRK 指令异常 (ESR EC=0x3C)。提取 BRK 的 #imm16，遍历 break_hook链表匹配。kprobes(#4)、uprobes(#5)、BUG(#0x800) 都走这里。未匹配则发 SIGTRAP。
    // 我们的 BRK #0xDB0也会走这里——但因为我们在更上层 hook 了 do_debug_exception，根本不会到这一步
    {"brk_handler",              "table slot[6]: BRK instruction handler"},

/* 单步控制 */
    {"user_enable_single_step",  "user-mode single-step enable (reference)"},//内核给用户态进程启用单步的函数
    {"user_disable_single_step", "user-mode single-step disable"},
    {"kernel_enable_single_step","kernel-mode single-step enable"},//内核给自身启用单步

    /* memory access */
    //安全读内核内存。带异常保护
    {"copy_from_kernel_nofault", "safe kernel memory read"},
    {"copy_to_kernel_nofault",   "safe kernel memory write"},

    //读用户态内存。R3 通过prctl传来的参数是用户态指针，KPM 要用这个函数把数据从用户空间拷到内核空间。所有 prctl命令的结构体参数（kdbg_bp_args、kdbg_mem_args 等）都靠它      
    {"_copy_from_user",          "read user memory (prctl data)"},
    {"_copy_to_user",            "write user memory"},

    /* process / signal */
    {"__task_pid_nr_ns",         "get PID (no hardcoded offset)"},//获取进程 PID/TGID
    {"send_sig",                 "send signal (SIGSTOP/SIGCONT)"},

    //Android 上 app 启动时 zygote fork 后会调prctl(PR_SET_NAME)设置进程名（包名），最终走到这个函数。kdbg hook它来检测目标包名是否出现
    //这就是 spawn 调试的核心：app一启动就被我们检测到并冻结                                         
    {"__set_task_comm",          "task comm change hook (spawn detect)"},

    /* file path */

    //在 mmap hook 中，我们从 fd 拿到 struct file，再用 d_path取出文件路径，检查是不是目标 SO（如 libnative.so）                                       
    {"d_path",                   "dentry -> path (SO name resolve)"},

    //fd → struct file。mmap syscall 参数里只有 fd 数字，要用 fget 转换成 struct file* 才能拿到路径信息。会增加引用计数                                                 
    {"fget",                     "fd -> struct file"},
    
    //释放 file 引用。fget 拿到后用完必须 fput，否则文件引用泄漏
    {"fput",                     "release file ref"},

/* 断点注册链表 */
    //向内核的 user_step_hook 链表注册回调。单步异常时 single_step_handler会遍历这个链表。
    {"register_user_step_hook",  "register user step hook chain"},
    {"unregister_user_step_hook","unregister user step hook chain"},

    //向内核的 user_break_hook 链表注册回调，匹配特定 BRK #imm。           
    {"register_user_break_hook", "register user BRK hook chain"},

    /* 调试监控引用计数 — MDSCR 管理 Monitor Debug System Control Register，调试系统的总开关*/
    {"enable_debug_monitors",    "enable MDSCR debug monitors (refcnt)"},
    {"disable_debug_monitors",   "disable MDSCR debug monitors"},

    {0, 0} /* sentinel */
};

void probe_symbols(void)
{
    pr_info("kdbg-probe: ========== SYMBOL PROBE ==========\n");

    int found = 0, total = 0;

    for (struct sym_entry *e = g_symbols; e->name; e++) {
        total++;
        unsigned long addr = (unsigned long)kallsyms_lookup_name(e->name);
        if (addr) {
            pr_info("kdbg-probe: [OK]  %-35s = 0x%lx  (%s)\n",
                    e->name, addr, e->purpose);
            found++;
        } else {
            pr_warn("kdbg-probe: [--]  %-35s = NOT FOUND        (%s)\n",
                    e->name, e->purpose);
        }
    }

    pr_info("kdbg-probe: symbols done: %d/%d found\n", found, total);
}

/* ================================================================
 * 2. debug_fault_info[] dispatch table verification
 * ================================================================ */

void probe_fault_info_table(void)
{
    pr_info("kdbg-probe: ========== DISPATCH TABLE ==========\n");

    struct fault_info *table = (struct fault_info *)
        kallsyms_lookup_name("debug_fault_info");

    if (!table) {
        pr_warn("kdbg-probe: debug_fault_info symbol NOT FOUND, skip\n");
        return;
    }

    pr_info("kdbg-probe: debug_fault_info addr = 0x%lx\n", (unsigned long)table);

    /* expected handler addresses */
    unsigned long expected[] = {
        (unsigned long)kallsyms_lookup_name("breakpoint_handler"),   /* [0] */
        (unsigned long)kallsyms_lookup_name("single_step_handler"),  /* [1] */
        (unsigned long)kallsyms_lookup_name("watchpoint_handler"),   /* [2] */
        0, 0, 0,                                                      /* [3-5] */
        (unsigned long)kallsyms_lookup_name("brk_handler"),          /* [6] */
    };
    const char *slot_names[] = {
        "breakpoint_handler",
        "single_step_handler",
        "watchpoint_handler",
        "(reserved)",
        "(reserved)",
        "(reserved)",
        "brk_handler",
    };

    for (int i = 0; i < 7; i++) {
        struct fault_info *entry = &table[i];
        unsigned long fn_addr = (unsigned long)entry->fn;
        const char *name = entry->name ? entry->name : "(null)";

        if (expected[i]) {
            int match = (fn_addr == expected[i]);
            pr_info("kdbg-probe:   [%d] fn=0x%lx  sig=%d  name=\"%s\"  expect=%s  %s\n",
                    i, fn_addr, entry->sig, name,
                    slot_names[i],
                    match ? "[OK match]" : "[!! MISMATCH - likely CFI trampoline]");
            if (!match) {
                pr_info("kdbg-probe:        kallsyms=0x%lx  table_fn=0x%lx  delta=0x%lx\n",
                        expected[i], fn_addr,
                        fn_addr > expected[i] ? fn_addr - expected[i] : expected[i] - fn_addr);
            }
        } else {
            pr_info("kdbg-probe:   [%d] fn=0x%lx  sig=%d  name=\"%s\"  (%s)\n",
                    i, fn_addr, entry->sig, name, slot_names[i]);
        }
    }

    pr_info("kdbg-probe: NOTE: mismatch is normal on GKI with CFI enabled\n");
    pr_info("kdbg-probe: Our hook on do_debug_exception bypasses the table entirely\n");
}
