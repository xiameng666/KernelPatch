/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 * 
 * 安全绕过模块 - 主要用于绕过CFI(Control Flow Integrity)检查
 * 允许KernelPatch的代码绕过内核的控制流完整性检查
 */

#include <ktypes.h>
#include <hook.h>
#include <kallsyms.h>
#include <common.h>
#include <uapi/asm-generic/errno.h>

#include <predata.h>

struct pt_regs;

// 判断目标地址是否应该绕过CFI检查
// target: 目标函数地址
// 返回值: true=允许绕过CFI检查，false=需要进行CFI检查
static inline bool should_cfi_pass(unsigned long target)
{
    // 允许KernelPatch自身代码区域、hook区域、KPM模块区域绕过CFI检查
    return is_kp_text_area(target) || is_kp_hook_area(target) || is_kpm_rox_area(target);
}

// Bug trap类型枚举，用于CFI失败处理
enum bug_trap_type
{
    BUG_TRAP_TYPE_NONE = 0,  // 无trap
    BUG_TRAP_TYPE_WARN = 1,  // 警告级别
    BUG_TRAP_TYPE_BUG = 2,   // 错误级别
};

// 备份原始的CFI失败报告函数指针
static enum bug_trap_type (*backup_report_cfi_failure)(struct pt_regs *regs, unsigned long addr, unsigned long *target,
                                                       u32 type) = 0;

// 替换的CFI失败报告函数
// 对于KernelPatch相关的代码区域，将错误降级为警告
static enum bug_trap_type replace_report_cfi_failure(struct pt_regs *regs, unsigned long addr, unsigned long *target,
                                                     u32 type)
{
    // 如果目标地址是KernelPatch相关区域，降级为警告
    if (should_cfi_pass(*target)) {
        return BUG_TRAP_TYPE_WARN;
    }
    // 其他情况调用原始函数
    enum bug_trap_type rc = backup_report_cfi_failure(regs, addr, target, type);
    return rc;
}

// CFI检查函数类型定义
typedef void (*cfi_check_fn)(uint64_t id, void *ptr, void *diag);

// 备份原始的CFI慢路径函数指针
static void (*backup__cfi_slowpath)(uint64_t id, void *ptr, void *diag) = 0;

// 替换的CFI慢路径函数
// 对于KernelPatch相关的代码直接跳过CFI检查
static void replace__cfi_slowpath(uint64_t id, void *ptr, void *diag)
{
    // 如果是KernelPatch相关区域，直接返回跳过检查
    if (should_cfi_pass((unsigned long)ptr)) return;
    // 其他情况调用原始函数
    backup__cfi_slowpath(id, ptr, diag);
}

// 绕过内核CFI(Control Flow Integrity)检查
// 返回值: 0=成功，<0=失败
int bypass_kcfi()
{
    int rc = 0;

    // 针对内核6.1.0版本的CFI绕过
    // todo: Is there more elegant way?
    // 获取CFI失败报告函数地址
    unsigned long report_cfi_failure_addr = patch_config->report_cfi_failure;
    if (report_cfi_failure_addr) {
        // hook CFI失败报告函数
        hook_err_t err = hook((void *)report_cfi_failure_addr, (void *)replace_report_cfi_failure,
                              (void **)&backup_report_cfi_failure);
        if (err) {
            log_boot("hook report_cfi_failure: %llx, error: %d\n", report_cfi_failure_addr, err);
            rc = err;
            goto out;
        }
    }

    // todo: direct modify cfi_shadow, __cfi_check?
    // 获取CFI慢路径函数地址
    unsigned long __cfi_slowpath_addr = patch_config->__cfi_slowpath_diag;
    if (!__cfi_slowpath_addr) {
        __cfi_slowpath_addr = patch_config->__cfi_slowpath;
    }
    if (__cfi_slowpath_addr) {
        // hook CFI慢路径函数
        hook_err_t err =
            hook((void *)__cfi_slowpath_addr, (void *)replace__cfi_slowpath, (void **)&backup__cfi_slowpath);
        if (err) {
            log_boot("hook __cfi_slowpath_diag: %llx, error: %d\n", __cfi_slowpath_addr, err);
            rc = err;
            goto out;
        }
    }

    // 如果没有找到CFI相关符号，记录日志但不报错
    if (!report_cfi_failure_addr && !__cfi_slowpath_addr) {
        // not error
        log_boot("no symbol for pass kcfi\n");
    }

out:
    return rc;
}