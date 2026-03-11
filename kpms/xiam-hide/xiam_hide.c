/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * xiam_hide.c - KernelPatch module: hide XiaM injection artifacts from /proc/<pid>/maps
 *
 * Hooks proc_pid_maps_op->show and proc_pid_smaps_op->show to filter out:
 *   1. memfd:xiam entries (SO loading memfd)
 *   2. Anonymous executable segments with no backing file (hook pool, etc.)
 *   3. /dev/zero (deleted) mappings
 *
 * Only active for processes with uid > 10000 (app processes).
 */

#include <compiler.h>
#include <kpmodule.h>
#include <kputils.h>
#include <ksyms.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <asm/current.h>
#include <linux/cred.h>
#include <hook.h>

KPM_NAME("xiam-hide");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("XiaM");
KPM_DESCRIPTION("Hide XiaM injection artifacts from /proc/pid/maps");

/* ========== seq_file minimal definition ========== */

typedef struct seq_file {
    char *buf;
    size_t size;
    size_t from;
    size_t count;
    size_t pad_until;
    long long index;
    long long read_pos;
} seq_file;

struct seq_operations {
    void *(*start)(struct seq_file *m, long long *pos);
    void (*stop)(struct seq_file *m, void *v);
    void *(*next)(struct seq_file *m, void *v, long long *pos);
    int (*show)(struct seq_file *m, void *v);
};

/* ========== kallsyms resolved pointers ========== */

static struct seq_operations *proc_pid_maps_op;
static struct seq_operations *proc_pid_smaps_op;
static int (*orig_show_map)(struct seq_file *m, void *v);
static int (*orig_show_smap)(struct seq_file *m, void *v);

/* vmalloc/vfree for temp buffers */
static void *(*kfn_vmalloc)(unsigned long size);
static void (*kfn_vfree)(void *ptr);

/* ========== process filter ========== */

static int is_app_process(void) {
    uid_t uid = current_uid();
    return uid > 10000;
}

/* ========== maps line filter ========== */

/*
 * Check if a maps line should be hidden.
 * Returns 1 if the line should be removed from output.
 */
static int should_hide_line(const char *line) {
    /* 1. memfd:xiam — our SO loading memfd */
    if (strstr(line, "/memfd:xiam"))
        return 1;

    /* 2. Hook pool: named via prctl(PR_SET_VMA_ANON_NAME, "xiam") */
    if (strstr(line, "[anon:xiam]"))
        return 1;

    return 0;
}

/* ========== hooked show functions ========== */

static int xiam_show_map(struct seq_file *m, void *v) {
    if (!is_app_process())
        return orig_show_map(m, v);

    size_t start = m->count;
    int ret = orig_show_map(m, v);
    size_t end = m->count;

    if (end <= start || !kfn_vmalloc)
        return ret;

    /* Copy the line to a temporary buffer for inspection */
    size_t len = end - start;
    char *line = kfn_vmalloc(len + 1);
    if (!line)
        return ret;

    /* memcpy from seq_file buf */
    for (size_t i = 0; i < len; i++)
        line[i] = m->buf[start + i];
    line[len] = '\0';

    if (should_hide_line(line)) {
        /* Rollback: discard this line from output */
        m->count = start;
    }

    kfn_vfree(line);
    return ret;
}

static int xiam_show_smap(struct seq_file *m, void *v) {
    if (!is_app_process())
        return orig_show_smap(m, v);

    size_t start = m->count;
    int ret = orig_show_smap(m, v);
    size_t end = m->count;

    if (end <= start || !kfn_vmalloc)
        return ret;

    size_t len = end - start;
    char *line = kfn_vmalloc(len + 1);
    if (!line)
        return ret;

    for (size_t i = 0; i < len; i++)
        line[i] = m->buf[start + i];
    line[len] = '\0';

    if (should_hide_line(line)) {
        m->count = start;
    }

    kfn_vfree(line);
    return ret;
}

/* ========== KPM lifecycle ========== */

static long xiam_hide_init(const char *args, const char *event, void *__user reserved) {
    pr_info("xiam-hide: init (kpver=%x)\n", kpver);

    /* Resolve vmalloc/vfree */
    kfn_vmalloc = (typeof(kfn_vmalloc))kallsyms_lookup_name("vmalloc");
    kfn_vfree = (typeof(kfn_vfree))kallsyms_lookup_name("vfree");
    if (!kfn_vmalloc || !kfn_vfree) {
        pr_info("xiam-hide: vmalloc/vfree resolve failed\n");
        return -1;
    }

    /* Resolve proc_pid_maps_op */
    proc_pid_maps_op = (struct seq_operations *)kallsyms_lookup_name("proc_pid_maps_op");
    if (!proc_pid_maps_op) {
        pr_info("xiam-hide: proc_pid_maps_op not found\n");
        return -1;
    }
    orig_show_map = proc_pid_maps_op->show;

    /* Resolve proc_pid_smaps_op */
    proc_pid_smaps_op = (struct seq_operations *)kallsyms_lookup_name("proc_pid_smaps_op");
    if (proc_pid_smaps_op) {
        orig_show_smap = proc_pid_smaps_op->show;
    }

    /* Hook maps show function via fp_hook (function pointer hook) */
    fp_hook((uintptr_t)&proc_pid_maps_op->show, (void *)xiam_show_map,
            (void **)&orig_show_map);
    pr_info("xiam-hide: maps hook installed (orig=%p)\n", orig_show_map);

    if (proc_pid_smaps_op && orig_show_smap) {
        fp_hook((uintptr_t)&proc_pid_smaps_op->show, (void *)xiam_show_smap,
                (void **)&orig_show_smap);
        pr_info("xiam-hide: smaps hook installed (orig=%p)\n", orig_show_smap);
    }

    pr_info("xiam-hide: ready\n");
    return 0;
}

static long xiam_hide_exit(void *__user reserved) {
    if (proc_pid_maps_op && orig_show_map) {
        fp_unhook((uintptr_t)&proc_pid_maps_op->show, orig_show_map);
        pr_info("xiam-hide: maps hook removed\n");
    }
    if (proc_pid_smaps_op && orig_show_smap) {
        fp_unhook((uintptr_t)&proc_pid_smaps_op->show, orig_show_smap);
        pr_info("xiam-hide: smaps hook removed\n");
    }
    pr_info("xiam-hide: exit\n");
    return 0;
}

static long xiam_hide_ctl0(const char *args, char *__user out_msg, int outlen) {
    pr_info("xiam-hide: ctl0 args=%s\n", args);
    /* 可通过 kpatch ctl0 动态添加过滤关键字，后续扩展 */
    return 0;
}

KPM_INIT(xiam_hide_init);
KPM_CTL0(xiam_hide_ctl0);
KPM_EXIT(xiam_hide_exit);
