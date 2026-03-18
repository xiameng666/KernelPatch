/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mmap_log.c — 最小 mmap hook: 只打印所有 PROT_EXEC mmap 的 SO 路径
 *
 * 用途: 验证 hook vm_mmap_pgoff 是否安全, 不做任何冻结/断点
 *
 * 用法:
 *   kpmctl <key> load /sdcard/Download/demo-mmap-log.kpm
 *   (观察 dmesg)
 *   kpmctl <key> unload demo-mmap-log
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <hook.h>
#include <ksyms.h>

#define TAG "mmap-log"
#define PROT_EXEC 0x04
#define FILE_F_PATH_OFFSET 16

KPM_NAME("demo-mmap-log");
KPM_VERSION("0.1.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("kdbg");
KPM_DESCRIPTION("Minimal: log all PROT_EXEC mmap (no freeze, no breakpoint)");

/* 内核函数指针 */
static unsigned long (*kfn_vm_mmap_pgoff)(void *file, unsigned long addr,
                                          unsigned long len, unsigned long prot,
                                          unsigned long flag, unsigned long pgoff);
static char *(*kfn_d_path)(void *path, char *buf, int buflen);

/* 计数器: 避免日志风暴 */
static volatile int g_log_count = 0;
#define MAX_LOG_COUNT 200

/* ========== before hook: 标记 PROT_EXEC ========== */

static void mmap_before(hook_fargs6_t *args, void *udata)
{
    void *file = (void *)args->arg0;
    unsigned long prot = args->arg3;

    /* 只关注有文件的 PROT_EXEC 映射 */
    if (!file || !(prot & PROT_EXEC))
        return;

    /* 限制日志数量 */
    if (g_log_count >= MAX_LOG_COUNT)
        return;

    /* 从 file->f_path 提取文件名 */
    void *path_ptr = (void *)((char *)file + FILE_F_PATH_OFFSET);
    char buf[256];
    char *name = kfn_d_path(path_ptr, buf, sizeof(buf));
    if (!name || (unsigned long)name < (unsigned long)buf ||
        (unsigned long)name >= (unsigned long)(buf + sizeof(buf)))
        return;

    /* 只打印 .so 文件 */
    if (!strstr(name, ".so"))
        return;

    g_log_count++;
    pr_info(TAG ": [%d] EXEC mmap: %s (len=0x%lx)\n",
            g_log_count, name, args->arg2);
}

/* ========== init ========== */

static long mmap_log_init(const char *args, const char *event, void *__user reserved)
{
    pr_info(TAG ": ==============================\n");
    pr_info(TAG ":   Minimal mmap EXEC logger\n");
    pr_info(TAG ": ==============================\n");

    /* 解析符号 */
    kfn_vm_mmap_pgoff = (typeof(kfn_vm_mmap_pgoff))kallsyms_lookup_name("vm_mmap_pgoff");
    kfn_d_path = (typeof(kfn_d_path))kallsyms_lookup_name("d_path");

    if (!kfn_vm_mmap_pgoff) {
        pr_err(TAG ": vm_mmap_pgoff not found!\n");
        return -1;
    }
    if (!kfn_d_path) {
        pr_err(TAG ": d_path not found!\n");
        return -1;
    }

    /* 只 hook before, 不 hook after (不需要拦截返回值) */
    hook_err_t err = hook_wrap6((void *)kfn_vm_mmap_pgoff,
                                (hook_chain6_callback)mmap_before,
                                0, 0);
    if (err) {
        pr_err(TAG ": hook vm_mmap_pgoff failed: %d\n", err);
        return -1;
    }

    pr_info(TAG ": hooked vm_mmap_pgoff, logging .so PROT_EXEC mmaps (max %d)\n", MAX_LOG_COUNT);
    return 0;
}

/* ========== control: 重置计数器 ========== */

static long mmap_log_ctl(const char *args, char *__user out_msg, int outlen)
{
    if (args && !strcmp(args, "reset")) {
        g_log_count = 0;
        pr_info(TAG ": log counter reset\n");
    } else {
        pr_info(TAG ": logged %d mmap events\n", g_log_count);
    }
    return 0;
}

/* ========== exit ========== */

static long mmap_log_exit(void *__user reserved)
{
    hook_unwrap((void *)kfn_vm_mmap_pgoff, (void *)mmap_before, 0);
    pr_info(TAG ": unhooked, total logged: %d\n", g_log_count);
    return 0;
}

KPM_INIT(mmap_log_init);
KPM_CTL0(mmap_log_ctl);
KPM_EXIT(mmap_log_exit);
