/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 热补丁功能实现 - 支持运行时安全地修改内核代码

#include <hotpatch.h>

#include <linux/stop_machine.h>
#include <linux/init_task.h>
#include <linux/sched.h>
#include <linux/include/vdso/limits.h>
#include <linux/stacktrace.h>
#include <uapi/asm-generic/errno.h>

#define MAX_STACK_TRACE_DEPTH 64  // 最大栈跟踪深度
// static unsigned long stack_entries[MAX_STACK_TRACE_DEPTH];  // 栈地址条目数组
// static struct stack_trace trace = { 0 };  // 栈跟踪结构

// 验证栈回溯地址的有效性（当前已注释）
// static int backtrace_address_verify(unsigned long address, bool replace)
// {
//     return 0;
// }

/*
 * 参考：https://github.com/dynup/kpatch/blob/922cd458091915b0dad8c1892d7a609addd4afd7/kmod/core/core.c#L274C20-L274C20
 * 验证活跃性安全，即确保没有待打补丁的函数在任何任务的栈上
 */
int patch_verify_safety()
{
    // if (task_struct_offset.tasks_offset < 0) {  // 检查任务结构偏移
    //     return -1;
    // }
    int ret = 0;

    // trace.max_entries = sizeof(stack_entries) / sizeof(stack_entries[0]);  // 设置最大条目数
    // trace.entries = &stack_entries[0];  // 设置条目数组

    // struct task_struct *g, *t;  // 任务结构指针

    // 遍历所有任务进程（当前代码已注释）
    // do_each_thread(p)
    // {
    //         trace.nr_entries = 0;  // 重置条目计数
    //         save_stack_trace_tsk(t, &trace);  // 保存任务栈跟踪

    //         if (trace.nr_entries >= trace.max_entries) {  // 检查条目数溢出
    //             logke("more than %u trace entries!\n", trace.max_entries);
    //             ret = -EBUSY;
    //             goto out;
    //         }

    //         for (int i = 0; i < trace.nr_entries; i++) {  // 遍历栈地址
    //             if (trace.entries[i] == ULONG_MAX)
    //                 break;
    //             ret = backtrace_address_verify(trace.entries[i], 0);  // 验证地址
    //             if (ret)
    //                 goto out;
    //         }
    // }

    // out:
    //     if (ret) {  // 如果发现问题，输出诊断信息
    //         // pid_t pid =
    //         logke("Comm: %.20s\n", get_task_comm(t));  // 输出任务名
    //         for (int i = 0; i < trace.nr_entries; i++) {
    //             if (trace.entries[i] == ULONG_MAX)
    //                 break;
    //             logke("  [<%pK>] %pB\n", (void *)trace.entries[i], (void *)trace.entries[i]);  // 输出栈地址
    //         }
    //     }

    return ret;
}

// 检查所有任务的函数（当前已注释）
// static int check_all_task(void *data)
// {
//     hook_t *hook = (hook_t *)data;
//     logkd("check_all_task data: %llx\n", hook);
//     return 0;
// }

// 热补丁文本修改函数
int hot_patch_text()
{
    // int rc = stop_machine(check_all_task, &data, 0);  // 停止机器执行检查
    // logkd("stop_machine rc: %d\n", rc);
    return 0;  // 当前返回成功
}

// 内核指令补丁文本修改函数
int kp_insn_patch_text(void *addrs[], uint32_t insn[], int cnt)
{
    // 当前为空实现，用于修改指定地址的指令
}