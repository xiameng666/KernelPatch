/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 访问控制模块 - 管理进程权限提升和SELinux上下文绕过
// 提供root权限授予、能力位设置和安全上下文管理功能

#include "accctl.h"

#include <pgtable.h>
#include <ksyms.h>
#include <taskext.h>
#include <uapi/scdefs.h>
#include <linux/spinlock.h>
#include <linux/capability.h>
#include <linux/security.h>
#include <asm/current.h>
#include <linux/pid.h>
#include <linux/sched/task.h>
#include <linux/sched.h>
#include <linux/seccomp.h>
#include <asm/thread_info.h>
#include <uapi/asm-generic/errno.h>
#include <hook.h>
#include <linux/string.h>
#include <security/selinux/include/avc.h>
#include <security/selinux/include/security.h>
#include <predata.h>
#include <linux/slab.h>

char all_allow_sctx[SUPERCALL_SCONTEXT_LEN] = { '\0' };  // 允许所有操作的SELinux上下文
uint32_t all_allow_sid = SECSID_NULL;                    // 对应的安全标识符

// 设置超级用户凭据 - 为指定UID设置完整的root权限
static void su_cred(struct cred *cred, uid_t uid)
{
    // 设置所有能力位为完整权限
    *(kernel_cap_t *)((uintptr_t)cred + cred_offset.cap_inheritable_offset) = full_cap;  // 可继承能力
    *(kernel_cap_t *)((uintptr_t)cred + cred_offset.cap_permitted_offset) = full_cap;    // 允许能力
    *(kernel_cap_t *)((uintptr_t)cred + cred_offset.cap_effective_offset) = full_cap;    // 有效能力
    *(kernel_cap_t *)((uintptr_t)cred + cred_offset.cap_bset_offset) = full_cap;         // 边界集能力
    *(kernel_cap_t *)((uintptr_t)cred + cred_offset.cap_ambient_offset) = full_cap;      // 环境能力

    // 设置所有UID相关字段为指定值
    *(uid_t *)((uintptr_t)cred + cred_offset.uid_offset) = uid;     // 实际用户ID
    *(uid_t *)((uintptr_t)cred + cred_offset.euid_offset) = uid;    // 有效用户ID
    *(uid_t *)((uintptr_t)cred + cred_offset.fsuid_offset) = uid;   // 文件系统用户ID
    *(uid_t *)((uintptr_t)cred + cred_offset.suid_offset) = uid;    // 保存的用户ID

    // 设置所有GID相关字段为指定值
    *(uid_t *)((uintptr_t)cred + cred_offset.gid_offset) = uid;     // 实际组ID
    *(uid_t *)((uintptr_t)cred + cred_offset.egid_offset) = uid;    // 有效组ID
    *(uid_t *)((uintptr_t)cred + cred_offset.fsgid_offset) = uid;   // 文件系统组ID
    *(uid_t *)((uintptr_t)cred + cred_offset.sgid_offset) = uid;    // 保存的组ID
}

/**
 * 设置全局允许的SELinux安全上下文
 * @param sctx SELinux安全上下文字符串
 * @return 成功返回0，失败返回错误码
 */
int set_all_allow_sctx(const char *sctx)
{
    if (!sctx || !sctx[0]) {  // 如果参数为空或空字符串
        all_allow_sctx[0] = 0;           // 清空全局上下文字符串
        all_allow_sid = SECSID_NULL;     // 重置安全标识符
        dsb(ish);                        // 内存屏障确保写入完成
        logkfd("clear all allow sconetxt\n");
        return 0;
    }

    // 将安全上下文字符串转换为安全标识符
    int rc = security_secctx_to_secid(sctx, strlen(sctx), &all_allow_sid);
    if (!rc && all_allow_sid != SECSID_NULL) {  // 转换成功且SID有效
        strncpy(all_allow_sctx, sctx, sizeof(all_allow_sctx) - 1);  // 复制上下文字符串
        all_allow_sctx[sizeof(all_allow_sctx) - 1] = '\0';          // 确保字符串结尾
        dsb(ish);  // 内存屏障确保写入完成
        logkfd("set all allow sconetxt: %s, sid: %d\n", all_allow_sctx, all_allow_sid);
    }
    return rc;  // 返回转换结果
}

/**
 * 提交内核超级用户权限
 * 使用内核凭据并设置全局允许的安全上下文
 * @return 成功返回0，失败返回错误码
 */
int commit_kernel_su()
{
    struct cred *new = prepare_kernel_cred(0);    // 准备内核凭据
    set_security_override(new, all_allow_sid);    // 设置安全覆盖
    return commit_creds(new);                     // 提交新凭据
}

/**
 * 提交普通用户的超级用户权限
 * @param to_uid 目标用户ID
 * @param sctx SELinux安全上下文
 * @return 成功返回0，失败返回错误码
 */
int commit_common_su(uid_t to_uid, const char *sctx)
{
    int rc = 0;
    struct task_struct *task = current;           // 获取当前任务
    struct task_ext *ext = get_task_ext(task);    // 获取任务扩展信息
    if (unlikely(!task_ext_valid(ext))) {         // 检查扩展信息有效性
        logkfe("dirty task_ext, pid(maybe dirty): %d\n", ext->pid);
        rc = -ENOMEM;
        goto out;
    }

    struct thread_info *thi = current_thread_info();  // 获取线程信息
    thi->flags &= ~(_TIF_SECCOMP);                     // 禁用seccomp标志

    // 如果已知seccomp偏移量，直接禁用seccomp模式
    if (task_struct_offset.comm_offset > 0) {
        struct seccomp *seccomp = (struct seccomp *)((uintptr_t)task + task_struct_offset.seccomp_offset);
        seccomp->mode = SECCOMP_MODE_DISABLED;  // 禁用seccomp安全模式
        // 只在任务退出时调用，所以不需要内存屏障
        // todo: WARN_ON(tsk->sighand != NULL);
        // seccomp_filter_release(task);
    }

    ext->sel_allow = 1;                    // 标记允许SELinux绕过
    struct cred *new = prepare_creds();    // 准备新的凭据结构
    su_cred(new, to_uid);                  // 设置超级用户凭据

    struct group_info *group_info = groups_alloc(0);  // 分配空的组信息
    set_groups(new, group_info);                      // 设置组信息

    // 如果指定了安全上下文，尝试设置安全覆盖
    if (sctx && sctx[0]) {
        ext->sel_allow = !!set_security_override_from_ctx(new, sctx);
    }
    commit_creds(new);  // 提交新凭据

out:
    logkfi("pid: %d, tgid: %d, to_uid: %d, sctx: %s, via_hook: %d\n", ext->pid, ext->tgid, to_uid, sctx,
           ext->sel_allow);
    return rc;  // 返回结果
}

/**
 * 统一的超级用户权限提交接口
 * @param to_uid 目标用户ID
 * @param sctx SELinux安全上下文
 * @return 成功返回0，失败返回错误码
 */
int commit_su(uid_t to_uid, const char *sctx)
{
    // 如果设置了全局允许SID且目标是root用户，使用内核凭据
    if (all_allow_sid != SECSID_NULL && !to_uid) {
        return commit_kernel_su();
    } else {
        return commit_common_su(to_uid, sctx);  // 否则使用普通用户凭据
    }
}

/**
 * 为指定PID的任务设置超级用户权限
 * 直接修改目标任务的凭据，不使用标准的凭据提交流程
 * @param pid 目标进程ID
 * @param to_uid 目标用户ID
 * @param sctx SELinux安全上下文
 * @return 成功返回0，失败返回错误码
 */
// todo: 需要考虑RCU保护
int task_su(pid_t pid, uid_t to_uid, const char *sctx)
{
    int rc = 0;
    int scontext_changed = 0;                         // 安全上下文是否改变
    struct task_struct *task = find_get_task_by_vpid(pid);  // 通过PID查找任务
    if (!task) {  // 任务不存在
        logkfe("no such pid: %d\n", pid);
        return -ESRCH;
    }
    struct task_ext *ext = get_task_ext(task);  // 获取任务扩展信息

    if (unlikely(!task_ext_valid(ext))) {  // 检查扩展信息有效性
        logkfe("dirty task_ext, pid(maybe dirty): %d\n", ext->pid);
        rc = -ENOMEM;
        goto out;
    }

    struct thread_info *thi = get_task_thread_info(task);  // 获取任务的线程信息
    thi->flags &= ~(_TIF_SECCOMP);                         // 禁用seccomp标志

    // 如果已知seccomp偏移量，直接禁用seccomp模式
    if (task_struct_offset.comm_offset > 0) {
        struct seccomp *seccomp = (struct seccomp *)((uintptr_t)task + task_struct_offset.seccomp_offset);
        seccomp->mode = SECCOMP_MODE_DISABLED;  // 禁用seccomp安全模式
        // 只在任务退出时调用，所以不需要内存屏障
        // todo: WARN_ON(tsk->sighand != NULL);
        // seccomp_filter_release(task);
    }

    // 直接修改任务的有效凭据
    struct cred *cred = *(struct cred **)((uintptr_t)task + task_struct_offset.cred_offset);
    su_cred(cred, to_uid);  // 设置超级用户凭据
    if (sctx && sctx[0]) scontext_changed = !set_security_override_from_ctx(cred, sctx);

    // 如果真实凭据与有效凭据不同，也需要修改真实凭据
    struct cred *real_cred = *(struct cred **)((uintptr_t)task + task_struct_offset.real_cred_offset);
    if (cred != real_cred) {
        su_cred(real_cred, to_uid);  // 设置真实凭据的超级用户权限
        if (sctx && sctx[0]) scontext_changed = scontext_changed && !set_security_override_from_ctx(real_cred, sctx);
    }
    ext->priv_sel_allow = !scontext_changed;  // 设置特权SELinux允许标志

    logkfi("pid: %d, tgid: %d, to_uid: %d, sctx: %s, via_hook: %d\n", ext->pid, ext->tgid, to_uid, sctx,
           ext->priv_sel_allow);
out:
    return rc;  // 返回操作结果
}

// AVC（访问向量缓存）拒绝检查函数的备份指针
static int (*avc_denied_backup)(struct selinux_state *state, void *ssid, void *tsid, void *tclass, void *requested,
                                void *driver, void *xperm, void *flags, struct av_decision *avd) = 0;

/**
 * AVC拒绝检查的替换函数 - 绕过SELinux权限检查
 * 如果当前任务有允许标志或全局允许SID匹配，则允许所有操作
 */
static int avc_denied_replace(struct selinux_state *_state, void *_ssid, void *_tsid, void *_tclass, void *_requested,
                              void *_driver, void *_xperm, void *_flags, struct av_decision *_avd)
{
    // 检查全局允许的安全标识符
    if (all_allow_sid != SECSID_NULL) {
        u32 ssid = (u32)(u64)_ssid;
        if ((uint64_t)_state <= 0xffffffffL) {  // 兼容不同内核版本的参数布局
            ssid = (u32)(u64)_state;
        }
        if (ssid == all_allow_sid) {  // 如果匹配全局允许SID，直接允许
            goto allow;
        }
    }

    // 检查当前任务的扩展标志
    struct task_ext *ext = get_current_task_ext();
    if (unlikely(task_ext_valid(ext) && (ext->sel_allow || ext->priv_sel_allow))) {
        goto allow;  // 如果任务有SELinux绕过标志，直接允许
    }

    // 否则调用原始的检查函数
    int rc = avc_denied_backup(_state, _ssid, _tsid, _tclass, _requested, _driver, _xperm, _flags, _avd);
    return rc;

allow:
    // 设置访问决策为允许所有操作
    struct av_decision *avd = (struct av_decision *)_avd;
    if ((uint64_t)_state <= 0xffffffffL) {  // 兼容不同内核版本的参数布局
        avd = (struct av_decision *)_flags;
    }
    avd->allowed = 0xffffffff;  // 允许所有权限
    avd->auditallow = 0;        // 不记录允许的审计
    avd->auditdeny = 0;         // 不记录拒绝的审计
    return 0;
}

// 慢速AVC审计函数的备份指针
static int (*slow_avc_audit_backup)(struct selinux_state *_state, void *_ssid, void *_tsid, void *_tclass,
                                    void *_requested, void *_audited, void *_denied, void *_result,
                                    struct common_audit_data *_a) = 0;

/**
 * 慢速AVC审计的替换函数 - 绕过SELinux审计记录
 * 如果当前任务有允许标志或全局允许SID匹配，则跳过审计
 */
static int slow_avc_audit_replace(struct selinux_state *_state, void *_ssid, void *_tsid, void *_tclass,
                                  void *_requested, void *_audited, void *_denied, void *_result,
                                  struct common_audit_data *_a)
{
    // 检查全局允许的安全标识符
    if (all_allow_sid != SECSID_NULL) {
        u32 ssid = (u64)_ssid;
        if ((uint64_t)_state <= 0xffffffffL) {  // 兼容不同内核版本的参数布局
            ssid = (u64)_state;
        }
        if (ssid == all_allow_sid) {  // 如果匹配全局允许SID，跳过审计
            return 0;
        }
    }

    // 检查当前任务的扩展标志
    struct task_ext *ext = get_current_task_ext();
    if (unlikely(task_ext_valid(ext) && (ext->sel_allow || ext->priv_sel_allow))) {
        return 0;  // 如果任务有SELinux绕过标志，跳过审计
    }

    // 否则调用原始的审计函数
    int rc = slow_avc_audit_backup(_state, _ssid, _tsid, _tclass, _requested, _audited, _denied, _result, _a);
    return rc;
}

/**
 * 安装SELinux绕过钩子
 * Hook关键的SELinux检查函数以实现权限绕过
 * @return 成功返回0
 */
int bypass_selinux()
{
    // Hook AVC拒绝检查函数
    unsigned long avc_denied_addr = patch_config->avc_denied;
    if (avc_denied_addr) {
        hook_err_t err = hook((void *)avc_denied_addr, (void *)avc_denied_replace, (void **)&avc_denied_backup);
        if (err != HOOK_NO_ERR) {
            log_boot("hook avc_denied_addr: %llx, error: %d\n", avc_denied_addr, err);
        }
    }

    // Hook 慢速AVC审计函数
    unsigned long slow_avc_audit_addr = patch_config->slow_avc_audit;
    if (slow_avc_audit_addr) {
        hook_err_t err =
            hook((void *)slow_avc_audit_addr, (void *)slow_avc_audit_replace, (void **)&slow_avc_audit_backup);
        if (err != HOOK_NO_ERR) {
            log_boot("hook slow_avc_audit: %llx, error: %d\n", slow_avc_audit_addr, err);
        }
    }

    return 0;  // 成功返回
}
