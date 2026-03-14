/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _XIAM_UTILS_H
#define _XIAM_UTILS_H

#include <ktypes.h>
#include <ksyms.h>
#include <kputils.h>
#include <linux/cred.h>
#include <linux/string.h>
#include <stdbool.h>

/* ========== vmalloc / vfree (resolved at runtime) ========== */

extern void *(*xiam_vmalloc)(unsigned long size);
extern void (*xiam_vfree)(void *ptr);
extern int (*xiam_arch_copy_from_user)(void *to, const void __user *from, unsigned long n);

/* ========== shared process filter ========== */

static inline int is_app_process(void) {
    uid_t uid = current_uid();
    return uid > 10000;
}

/* ========== string helpers ========== */

static inline bool ends_with(const char *buf, size_t buf_size, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    size_t buf_len = strnlen(buf, buf_size);
    if (buf_len < suffix_len)
        return false;
    return (memcmp(buf + (buf_len - suffix_len), suffix, suffix_len) == 0);
}

/* ========== shared init (call once from xiam_tools_init) ========== */

static inline int xiam_utils_init(void) {
    xiam_vmalloc = (typeof(xiam_vmalloc))kallsyms_lookup_name("vmalloc");
    xiam_vfree = (typeof(xiam_vfree))kallsyms_lookup_name("vfree");
    xiam_arch_copy_from_user = (typeof(xiam_arch_copy_from_user))kallsyms_lookup_name("__arch_copy_from_user");
    if (!xiam_vmalloc || !xiam_vfree) {
        return -1;
    }
    return 0;
}

#endif /* _XIAM_UTILS_H */
