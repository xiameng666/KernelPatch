/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <compiler.h>
#include <kpmodule.h>
#include <common.h>
#include <hook.h>
#include <kputils.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>

KPM_NAME("kpm-wxshadow-lite-demo");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("OpenAI x 24151");
KPM_DESCRIPTION("Teaching KPM: skeleton, control path, and hook basics.");

static char g_last_event[64] = "unknown";
static char g_last_args[128] = "";
static int g_add_bias = 100;
static int g_hook_installed = 0;
static u64 g_hook_hits = 0;
static u64 g_ctl0_calls = 0;
static u64 g_last_result = 0;

static __noinline int demo_add(int left, int right)
{
    return left + right;
}

static void demo_add_before(hook_fargs2_t *args, void *udata)
{
    args->local.data0 = args->arg0;
    args->local.data1 = args->arg1;
    pr_info("wxshadow-lite before demo_add left=%lld right=%lld\n", args->arg0, args->arg1);
}

static void demo_add_after(hook_fargs2_t *args, void *udata)
{
    int bias = *(int *)udata;
    g_hook_hits++;
    args->ret = (u64)((long)args->ret + bias);
    pr_info("wxshadow-lite after demo_add left=%lld right=%lld ret=%lld bias=%d\n",
            args->local.data0, args->local.data1, args->ret, bias);
}

static int install_demo_hook(void)
{
    hook_err_t err;

    if (g_hook_installed)
        return 0;

    err = hook_wrap2((void *)demo_add, demo_add_before, demo_add_after, &g_add_bias);
    if (err) {
        pr_err("wxshadow-lite hook_wrap2 failed: %d\n", err);
        return -err;
    }

    g_hook_installed = 1;
    pr_info("wxshadow-lite hook installed\n");
    return 0;
}

static void remove_demo_hook(void)
{
    if (!g_hook_installed)
        return;

    hook_unwrap((void *)demo_add, demo_add_before, demo_add_after);
    g_hook_installed = 0;
    pr_info("wxshadow-lite hook removed\n");
}

static long write_ctl0_reply(char __user *out_msg, int outlen, const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    int len;

    if (!out_msg || outlen <= 0)
        return 0;

    va_start(ap, fmt);
    len = vscnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len >= outlen)
        len = outlen - 1;
    if (len < 0)
        len = 0;

    buf[len] = '\0';
    compat_copy_to_user(out_msg, buf, len + 1);
    return 0;
}

static long run_demo_add(int left, int right, char __user *out_msg, int outlen)
{
    int result = demo_add(left, right);

    g_last_result = (u64)result;
    pr_info("wxshadow-lite test demo_add(%d, %d) => %d\n", left, right, result);
    return write_ctl0_reply(out_msg, outlen, "demo_add(%d,%d)=%d", left, right, result);
}

static long wxshadow_lite_init(const char *args, const char *event, void *__user reserved)
{
    memset(g_last_event, 0, sizeof(g_last_event));
    memset(g_last_args, 0, sizeof(g_last_args));

    strscpy(g_last_event, event ? event : "(null)", sizeof(g_last_event));
    strscpy(g_last_args, args ? args : "", sizeof(g_last_args));

    pr_info("wxshadow-lite init event=%s args=%s kpver=%x kver=%x\n",
            g_last_event, g_last_args, kpver, kver);

    if (args && strcmp(args, "autohook") == 0)
        return install_demo_hook();

    return 0;
}

static long wxshadow_lite_ctl0(const char *args, char __user *out_msg, int outlen)
{
    int left;
    int right;
    int bias;

    g_ctl0_calls++;
    pr_info("wxshadow-lite ctl0 args=%s\n", args ? args : "(null)");

    if (!args || !args[0])
        return write_ctl0_reply(out_msg, outlen, "commands: ping state hook unhook test <a> <b> bias <n>");

    if (!strcmp(args, "ping"))
        return write_ctl0_reply(out_msg, outlen, "pong");

    if (!strcmp(args, "state"))
        return write_ctl0_reply(out_msg, outlen,
                                "hooked=%d bias=%d hits=%llu ctl0=%llu last=%llu event=%s",
                                g_hook_installed, g_add_bias, g_hook_hits, g_ctl0_calls,
                                g_last_result, g_last_event);

    if (!strcmp(args, "hook")) {
        long rc = install_demo_hook();
        return write_ctl0_reply(out_msg, outlen, "hook rc=%ld", rc);
    }

    if (!strcmp(args, "unhook")) {
        remove_demo_hook();
        return write_ctl0_reply(out_msg, outlen, "hook rc=0");
    }

    if (sscanf(args, "test %d %d", &left, &right) == 2)
        return run_demo_add(left, right, out_msg, outlen);

    if (sscanf(args, "bias %d", &bias) == 1) {
        g_add_bias = bias;
        return write_ctl0_reply(out_msg, outlen, "bias=%d", g_add_bias);
    }

    return write_ctl0_reply(out_msg, outlen, "unknown command: %s", args);
}

static long wxshadow_lite_ctl1(void *a1, void *a2, void *a3)
{
    g_add_bias = (int)(long)a1;
    pr_info("wxshadow-lite ctl1 a1=%llx a2=%llx a3=%llx new_bias=%d\n",
            (u64)(uintptr_t)a1, (u64)(uintptr_t)a2, (u64)(uintptr_t)a3, g_add_bias);
    return g_add_bias;
}

static long wxshadow_lite_exit(void *__user reserved)
{
    remove_demo_hook();
    pr_info("wxshadow-lite exit hits=%llu ctl0=%llu last=%llu\n",
            g_hook_hits, g_ctl0_calls, g_last_result);
    return 0;
}

KPM_INIT(wxshadow_lite_init);
KPM_CTL0(wxshadow_lite_ctl0);
KPM_CTL1(wxshadow_lite_ctl1);
KPM_EXIT(wxshadow_lite_exit);
