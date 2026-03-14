/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * xiam_utils.c - Shared utility globals for xiam-tools KPM.
 */

#include "xiam_utils.h"

/* Global function pointers, resolved in xiam_utils_init() */
void *(*xiam_vmalloc)(unsigned long size) = 0;
void (*xiam_vfree)(void *ptr) = 0;
int (*xiam_arch_copy_from_user)(void *to, const void __user *from, unsigned long n) = 0;
