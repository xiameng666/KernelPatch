/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * hide_net.c - 隐藏 XiaM 网络特征
 *
 * 1. /proc/net/tcp  — 隐藏 127.0.0.1:12708 (0100007F:31A4) 回环连接
 * 2. /proc/net/unix — 隐藏 @xiam_patcher 抽象 socket
 *
 * 使用和 hide_maps.c 相同的 seq_file->show hook + count 回滚技术。
 */

#include "hide_maps.h"   /* seq_file / seq_operations 定义 */
#include "xiam_utils.h"

#include <ksyms.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <kputils.h>
#include <hook.h>

#define TAG "xiam-tools/net"

/* ========== kallsyms resolved pointers ========== */

static struct seq_operations *tcp4_seq_ops;
static struct seq_operations *tcp6_seq_ops;
static struct seq_operations *unix_seq_ops;

static int (*orig_tcp4_show)(struct seq_file *m, void *v);
static int (*orig_tcp6_show)(struct seq_file *m, void *v);
static int (*orig_unix_show)(struct seq_file *m, void *v);

/* ========== TCP 行过滤 ========== */

/*
 * /proc/net/tcp 每行格式:
 *   sl  local_address rem_address   st tx_queue ...
 *   0: 0100007F:31A4 0100007F:XXXX 01 ...
 *
 * 12708 = 0x31A4
 * 127.0.0.1 = 0100007F (小端十六进制)
 *
 * 过滤条件: 本地地址包含 ":31A4" (端口 12708)
 */
static int should_hide_tcp_line(const char *line) {
    /* 跳过 header 行 (以 "  sl" 开头) */
    if (strstr(line, "local_address"))
        return 0;

    /* 过滤包含端口 31A4 的行 (XiaM TCP 通信) */
    if (strstr(line, ":31A4"))
        return 1;

    return 0;
}

/* ========== Unix socket 行过滤 ========== */

/*
 * /proc/net/unix 每行格式:
 *   Num RefCount Protocol Flags Type St Inode Path
 *   ...                                      @xiam_patcher
 *
 * 过滤条件: 路径包含 "xiam"
 */
static int should_hide_unix_line(const char *line) {
    /* 跳过 header 行 */
    if (strstr(line, "RefCount"))
        return 0;

    if (strstr(line, "xiam"))
        return 1;

    return 0;
}

/* ========== 通用 seq_file show 过滤器 ========== */

/*
 * 通用的 seq_file show hook: 调用原始 show 后检查输出，
 * 如果匹配过滤条件则回滚 m->count 隐藏该行。
 */
static int filtered_show(struct seq_file *m, void *v,
                         int (*orig_show)(struct seq_file *, void *),
                         int (*should_hide)(const char *),
                         const char *label) {
    if (!is_app_process())
        return orig_show(m, v);

    size_t start = m->count;
    int ret = orig_show(m, v);
    size_t end = m->count;

    if (end <= start || !xiam_vmalloc)
        return ret;

    size_t len = end - start;
    char *line = xiam_vmalloc(len + 1);
    if (!line)
        return ret;

    for (size_t i = 0; i < len; i++)
        line[i] = m->buf[start + i];
    line[len] = '\0';

    if (should_hide(line)) {
        m->count = start;
        pr_info(TAG ": %s hidden (uid=%d, %zu bytes)\n", label, current_uid(), len);
    }

    xiam_vfree(line);
    return ret;
}

/* ========== hooked show 函数 ========== */

static int xiam_tcp4_show(struct seq_file *m, void *v) {
    return filtered_show(m, v, orig_tcp4_show, should_hide_tcp_line, "tcp");
}

static int xiam_tcp6_show(struct seq_file *m, void *v) {
    return filtered_show(m, v, orig_tcp6_show, should_hide_tcp_line, "tcp6");
}

static int xiam_unix_show(struct seq_file *m, void *v) {
    return filtered_show(m, v, orig_unix_show, should_hide_unix_line, "unix");
}

/* ========== init / exit ========== */

int hide_net_init(void) {
    /* === /proc/net/tcp === */
    tcp4_seq_ops = (struct seq_operations *)kallsyms_lookup_name("tcp4_seq_ops");
    if (tcp4_seq_ops) {
        orig_tcp4_show = tcp4_seq_ops->show;
        fp_hook((uintptr_t)&tcp4_seq_ops->show, (void *)xiam_tcp4_show,
                (void **)&orig_tcp4_show);
        pr_info(TAG ": tcp4 hook installed (orig=%p)\n", orig_tcp4_show);
    } else {
        pr_info(TAG ": tcp4_seq_ops not found, trying tcp_seq_ops\n");
        /* 部分内核版本符号名不同 */
        tcp4_seq_ops = (struct seq_operations *)kallsyms_lookup_name("tcp_seq_ops");
        if (tcp4_seq_ops) {
            orig_tcp4_show = tcp4_seq_ops->show;
            fp_hook((uintptr_t)&tcp4_seq_ops->show, (void *)xiam_tcp4_show,
                    (void **)&orig_tcp4_show);
            pr_info(TAG ": tcp_seq_ops hook installed (orig=%p)\n", orig_tcp4_show);
        } else {
            pr_info(TAG ": tcp seq_ops not found\n");
        }
    }

    /* === /proc/net/tcp6 === */
    tcp6_seq_ops = (struct seq_operations *)kallsyms_lookup_name("tcp6_seq_ops");
    if (tcp6_seq_ops) {
        orig_tcp6_show = tcp6_seq_ops->show;
        fp_hook((uintptr_t)&tcp6_seq_ops->show, (void *)xiam_tcp6_show,
                (void **)&orig_tcp6_show);
        pr_info(TAG ": tcp6 hook installed (orig=%p)\n", orig_tcp6_show);
    } else {
        pr_info(TAG ": tcp6_seq_ops not found\n");
    }

    /* === /proc/net/unix === */
    unix_seq_ops = (struct seq_operations *)kallsyms_lookup_name("unix_seq_ops");
    if (unix_seq_ops) {
        orig_unix_show = unix_seq_ops->show;
        fp_hook((uintptr_t)&unix_seq_ops->show, (void *)xiam_unix_show,
                (void **)&orig_unix_show);
        pr_info(TAG ": unix hook installed (orig=%p)\n", orig_unix_show);
    } else {
        pr_info(TAG ": unix_seq_ops not found\n");
    }

    return 0;
}

void hide_net_exit(void) {
    if (tcp4_seq_ops && orig_tcp4_show) {
        fp_unhook((uintptr_t)&tcp4_seq_ops->show, orig_tcp4_show);
        pr_info(TAG ": tcp4 hook removed\n");
    }
    if (tcp6_seq_ops && orig_tcp6_show) {
        fp_unhook((uintptr_t)&tcp6_seq_ops->show, orig_tcp6_show);
        pr_info(TAG ": tcp6 hook removed\n");
    }
    if (unix_seq_ops && orig_unix_show) {
        fp_unhook((uintptr_t)&unix_seq_ops->show, orig_unix_show);
        pr_info(TAG ": unix hook removed\n");
    }
}
