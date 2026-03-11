/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * xiam_tools.c - KernelPatch module main entry.
 *
 * Orchestrates sub-modules:
 *   - hide_maps: hide injection artifacts from /proc/pid/maps & smaps
 *   - (future: hide_ports, hide_socket, hide_logcat, ...)
 */

#include <compiler.h>
#include <kpmodule.h>
#include <kputils.h>
#include <ksyms.h>
#include <linux/printk.h>

#include "hide_maps.h"

KPM_NAME("xiam-tools");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("XiaM");
KPM_DESCRIPTION("XiaM anti-detection toolkit for KernelPatch");

#define TAG "xiam-tools"

static long xiam_tools_init(const char *args, const char *event, void *__user reserved) {
    int rc;
    pr_info(TAG ": init\n");

    rc = hide_maps_init();
    if (rc < 0) {
        pr_info(TAG ": hide_maps_init failed (%d)\n", rc);
        return rc;
    }
    pr_info(TAG ": hide_maps OK\n");

    /* future sub-modules init here */

    pr_info(TAG ": ready\n");
    return 0;
}

static long xiam_tools_exit(void *__user reserved) {
    pr_info(TAG ": exit\n");

    hide_maps_exit();

    /* future sub-modules exit here */

    pr_info(TAG ": all modules unloaded\n");
    return 0;
}

static long xiam_tools_ctl0(const char *args, char *__user out_msg, int outlen) {
    pr_info(TAG ": ctl0 args=%s\n", args);
    /* future: dispatch sub-commands to modules */
    return 0;
}

KPM_INIT(xiam_tools_init);
KPM_CTL0(xiam_tools_ctl0);
KPM_EXIT(xiam_tools_exit);
