/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * kdbg-probe.h - shared declarations for kdbg-probe module
 */
#ifndef KDBG_PROBE_H
#define KDBG_PROBE_H

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <ksyms.h>
#include <stdint.h>

/* ---- fault_info struct (mirrors fault.c debug_fault_info[]) ---- */
struct fault_info {
    void *fn;           /* handler function pointer */
    int   sig;          /* signal number */
    int   code;         /* si_code */
    const char *name;   /* name string */
};

/* ---- register_utils.c ---- */
void print_mdscr(void);
void probe_hw_capabilities(void);
void probe_debug_registers(void);
void probe_esr_examples(void);
void probe_cpu_info(void);

/* ---- function_utils.c ---- */
void probe_symbols(void);
void probe_fault_info_table(void);

#endif /* KDBG_PROBE_H */
