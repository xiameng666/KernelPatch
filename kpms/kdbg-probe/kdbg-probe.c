/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * kdbg-probe.c - ARM64 debug infrastructure probe module (main)
 *
 * Probes on load, all results via printk to dmesg.
 * PC side: dmesg viewer, filter "kdbg-probe"
 *
 * Files:
 *   kdbg-probe.c      - KPM lifecycle (init/exit/control)
 *   register_utils.c   - HW capability, debug registers, ESR parser, CPU info
 *   function_utils.c   - symbol probe, dispatch table verification
 */

#include "kdbg-probe.h"

KPM_NAME("kdbg-probe");
KPM_VERSION("1.2.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("kdbg");
KPM_DESCRIPTION("ARM64 debug infrastructure probe - dump everything to dmesg");

/* ================================================================
 * KPM lifecycle
 * ================================================================ */

static long probe_init(const char *kpm_args, const char *event, void *__user reserved)
{
    pr_info("kdbg-probe: ==========================================\n");
    pr_info("kdbg-probe:   ARM64 Debug Infrastructure Probe v1.2\n");
    pr_info("kdbg-probe:   PC: dmesg viewer, filter 'kdbg-probe'\n");
    pr_info("kdbg-probe: ==========================================\n");

    probe_cpu_info();
    probe_hw_capabilities();
    probe_debug_registers();
    probe_symbols();
    probe_fault_info_table();
    probe_esr_examples();

    pr_info("kdbg-probe: ========== PROBE COMPLETE ==========\n");
    pr_info("kdbg-probe: module stays loaded, unload anytime\n");

    return 0;
}

static long probe_control(const char *ctl_args, char *__user out_msg, int outlen)
{
    if (ctl_args && !strcmp(ctl_args, "rescan")) {
        pr_info("kdbg-probe: === RESCAN ===\n");
        probe_cpu_info();
        probe_hw_capabilities();
        probe_debug_registers();
        probe_symbols();
        probe_fault_info_table();
        probe_esr_examples();
        pr_info("kdbg-probe: === RESCAN DONE ===\n");
    }
    return 0;
}

static long probe_exit(void *__user reserved)
{
    pr_info("kdbg-probe: unloaded\n");
    return 0;
}

KPM_INIT(probe_init);
KPM_CTL0(probe_control);
KPM_EXIT(probe_exit);
