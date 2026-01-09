/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 任务观察器模块 - 负责跟踪进程创建和管理任务扩展数据

#include <taskob.h>
#include <taskext.h>
#include <kallsyms.h>
#include <hook.h>
#include <asm/current.h>
#include <linux/sched/task.h>
#include <linux/pid.h>
#include <linux/security.h>
#include <log.h>
#include <linux/cred.h>
#include <linux/err.h>
#include <pgtable.h>
#include <linux/fs.h>
#include <linux/seccomp.h>
#include <uapi/asm-generic/errno.h>
#include <predata.h>
#include <symbol.h>

/**
 * 初始化任务扩展数据结构
 * @param task 目标任务结构体
 */
static inline void prepare_init_ext(struct task_struct *task)
{
    struct task_ext *ext = get_task_ext(task);
    // 清零任务扩展区域
    for (uintptr_t i = (uintptr_t)ext; i < (uintptr_t)ext + sizeof(struct task_ext); i += 8) {
        *(uintptr_t *)i = 0;
    }
    ext->size = task_ext_size;        // 设置扩展数据大小
    ext->_magic = TASK_EXT_MAGIC;     // 设置魔数标识
    dsb(ish);                         // 数据同步屏障，确保写入完成
}

/**
 * 为新创建的任务准备扩展数据
 * @param new 新创建的任务
 * @param old 父任务（当前任务）
 */
static void prepare_task_ext(struct task_struct *new, struct task_struct *old)
{
    struct task_ext *old_ext = get_task_ext(old);
    // 验证父任务扩展数据的有效性
    if (unlikely(!task_ext_valid(old_ext))) {
        logkfe("dirty task_ext, pid(maybe dirty): %d\n", old_ext->pid);
        return;
    }
    struct task_ext *new_ext = get_task_ext(new);
    // 清零新任务的扩展区域
    for (uintptr_t i = (uintptr_t)new_ext; i < (uintptr_t)new_ext + sizeof(struct task_ext); i += 8) {
        *(uintptr_t *)i = 0;
    }
    new_ext->size = task_ext_size;
    new_ext->_magic = TASK_EXT_MAGIC;

    // 设置新任务的进程信息
    new_ext->pid = __task_pid_nr_ns(new, PIDTYPE_PID, 0);   // 进程ID
    new_ext->tgid = __task_pid_nr_ns(new, PIDTYPE_TGID, 0); // 线程组ID
    new_ext->sel_allow = old_ext->sel_allow;                 // 继承SELinux权限设置

    dsb(ish);  // 确保所有写入完成
}

int task_ext_size = offsetof(struct task_ext, _magic);  // 任务扩展大小
KP_EXPORT_SYMBOL(task_ext_size);

/**
 * copy_process函数的后置hook - 在进程复制完成后设置扩展数据
 * @param args hook参数，返回值包含新创建的任务结构体
 * @param udata 用户数据
 */
static void after_copy_process(hook_fargs8_t *args, void *udata)
{
    struct task_struct *new = (struct task_struct *)args->ret;
    if (unlikely(!new || IS_ERR(new))) return;  // 检查任务创建是否成功
    prepare_task_ext(new, current);              // 为新任务准备扩展数据
}

/**
 * cgroup_post_fork函数的后置hook - 在cgroup处理后设置扩展数据
 * @param args hook参数，第一个参数是新创建的任务
 * @param udata 用户数据
 */
static void after_cgroup_post_fork(hook_fargs4_t *args, void *udata)
{
    struct task_struct *new = (struct task_struct *)args->arg0;
    prepare_task_ext(new, current);  // 为新任务准备扩展数据
}

/**
 * 初始化任务观察器系统
 * @return 成功返回0，失败返回错误码
 */
int task_observer()
{
    int rc = 0;

    prepare_init_ext(init_task);  // 为init任务初始化扩展数据

    // 优先尝试hook copy_process函数
    unsigned long copy_process_addr = patch_config->copy_process;
    if (copy_process_addr) {
        rc |= hook_wrap8((void *)copy_process_addr, 0, after_copy_process, 0);
        log_boot("hook copy_process: %llx, rc: %d\n", copy_process_addr, rc);
    } else {
        // 备选方案：hook cgroup_post_fork函数
        unsigned long cgroup_post_fork_addr = patch_config->cgroup_post_fork;
        if (cgroup_post_fork_addr) {
            rc |= hook_wrap4((void *)cgroup_post_fork_addr, 0, after_cgroup_post_fork, 0);
            log_boot("hook cgroup_post_fork: %llx, rc: %d\n", cgroup_post_fork_addr, rc);
        } else {
            rc = HOOK_BAD_ADDRESS;  // 没有可用的hook地址
        }
    }

    return rc;
}