/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

/**
 * @file misc.c
 * @brief 内核杂项符号导入模块 - 导入各种内核子系统的函数符号
 * @details 本模块负责导入内核中各个子系统的函数符号，包括凭证管理、锁机制、
 * 进程管理、安全框架、内存管理等，为KernelPatch提供完整的内核API访问能力
 */

#include <ksyms.h>
#include <ktypes.h>
#include <symbol.h>
#include <common.h>
#include <stdarg.h>

#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/sched/task.h>

#ifndef INIT_USE_KALLSYMS_LOOKUP_NAME
/**
 * @brief 本地字符串比较函数
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @return 0相等，<0 s1<s2，>0 s1>s2
 * @details 当不使用kallsyms_lookup_name时的备用字符串比较实现
 */
int _ksym_local_strcmp(const char *s1, const char *s2)
{
    const unsigned char *c1 = (const unsigned char *)s1;
    const unsigned char *c2 = (const unsigned char *)s2;
    unsigned char ch;
    int d = 0;
    while (1) {
        d = (int)(ch = *c1++) - (int)*c2++;
        if (d || !ch) break;
    }
    return d;
}
#endif

//=============================================================================
// 凭证管理相关函数（来自kernel/cred.c）
//=============================================================================

/**
 * @brief 分配用户组信息结构
 * @param gidsetsize 组ID集合大小
 * @return 分配的group_info结构指针
 * @details 为进程的补充组ID信息分配内存
 */
struct group_info *kfunc_def(groups_alloc)(int gidsetsize) = 0;

/**
 * @brief 设置凭证的用户组信息
 * @param cred 目标凭证结构
 * @param group_info 要设置的组信息
 * @details 将组信息关联到指定的凭证结构中
 */
void kfunc_def(set_groups)(struct cred *, struct group_info *group_info) = 0;

/**
 * @brief 释放凭证结构（内部函数）
 * @param cred 要释放的凭证结构
 * @details 当凭证引用计数降为0时调用的清理函数
 */
void kfunc_def(__put_cred)(struct cred *) = 0;

/**
 * @brief 退出进程时清理凭证
 * @param task 目标进程结构
 * @details 进程退出时清理其关联的凭证信息
 */
void kfunc_def(exit_creds)(struct task_struct *) = 0;

/**
 * @brief 复制凭证信息到新进程
 * @param task 目标进程结构
 * @param clone_flags 克隆标志
 * @return 0成功，负值表示错误
 * @details fork时复制父进程的凭证信息到子进程
 */
int kfunc_def(copy_creds)(struct task_struct *, unsigned long) = 0;

/**
 * @brief 获取进程的凭证信息
 * @param task 目标进程结构
 * @return 进程的凭证结构指针
 * @details 安全地获取进程凭证，会增加引用计数
 */
const struct cred *kfunc_def(get_task_cred)(struct task_struct *) = 0;

/**
 * @brief 分配空白凭证结构
 * @return 新分配的凭证结构指针
 * @details 分配一个未初始化的凭证结构
 */
struct cred *kfunc_def(cred_alloc_blank)(void) = 0;

/**
 * @brief 准备修改当前进程凭证
 * @return 可修改的凭证结构副本
 * @details 为修改当前进程凭证创建一个可写副本
 */
struct cred *kfunc_def(prepare_creds)(void) = 0;

/**
 * @brief 准备exec时的凭证
 * @return 用于exec的凭证结构
 * @details 为execve系统调用准备新的凭证结构
 */
struct cred *kfunc_def(prepare_exec_creds)(void) = 0;

/**
 * @brief 提交凭证修改
 * @param cred 要提交的新凭证
 * @return 0成功，负值表示错误
 * @details 将修改后的凭证应用到当前进程
 */
int kfunc_def(commit_creds)(struct cred *) = 0;

/**
 * @brief 中止凭证修改
 * @param cred 要中止的凭证
 * @details 取消凭证修改，释放相关资源
 */
void kfunc_def(abort_creds)(struct cred *) = 0;

/**
 * @brief 临时覆盖当前凭证
 * @param new_cred 要覆盖的新凭证
 * @return 原凭证指针，用于后续恢复
 * @details 临时改变当前线程的凭证，用于权限检查
 */
const struct cred *kfunc_def(override_creds)(const struct cred *) = 0;

/**
 * @brief 恢复之前的凭证
 * @param old_cred 要恢复的原凭证
 * @details 恢复override_creds之前的凭证
 */
void kfunc_def(revert_creds)(const struct cred *) = 0;

/**
 * @brief 准备内核线程凭证
 * @param daemon 参考进程（可为NULL）
 * @return 内核线程凭证结构
 * @details 为内核线程准备适当的凭证结构
 */
struct cred *kfunc_def(prepare_kernel_cred)(struct task_struct *) = 0;

/**
 * @brief 改变文件创建时的用户身份
 * @param new_cred 目标凭证
 * @param inode 参考inode
 * @return 0成功，负值表示错误
 */
int kfunc_def(change_create_files_as)(struct cred *, struct inode *) = 0;

/**
 * @brief 设置安全覆盖标志
 * @param new_cred 目标凭证
 * @param secid 安全ID
 * @return 0成功，负值表示错误
 */
int kfunc_def(set_security_override)(struct cred *, u32) = 0;

/**
 * @brief 从上下文字符串设置安全覆盖
 * @param new_cred 目标凭证
 * @param secctx 安全上下文字符串
 * @return 0成功，负值表示错误
 */
int kfunc_def(set_security_override_from_ctx)(struct cred *, const char *) = 0;

/**
 * @brief 设置文件创建身份
 * @param new_cred 目标凭证
 * @param inode 参考inode
 * @return 0成功，负值表示错误
 */
int kfunc_def(set_create_files_as)(struct cred *, struct inode *) = 0;

/**
 * @brief 比较两个凭证的文件系统访问权限
 * @param a 第一个凭证
 * @param b 第二个凭证
 * @return 比较结果
 */
int kfunc_def(cred_fscmp)(const struct cred *, const struct cred *) = 0;

/**
 * @brief 初始化凭证子系统
 * @details 系统启动时初始化凭证管理机制
 */
void kfunc_def(cred_init)(void) = 0;

/**
 * @brief 检查凭证是否有效
 * @param cred 要检查的凭证
 * @return true表示凭证无效，false表示有效
 * @details 验证凭证结构的完整性和有效性
 */
bool kfunc_def(creds_are_invalid)(const struct cred *cred) = 0;

/**
 * @brief Linux内核凭证符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 通过符号名称匹配凭证相关函数的地址，为运行时动态绑定提供支持
 */
void _linux_kernel_cred_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(groups_alloc, name, addr);
    kfunc_match(set_groups, name, addr);

    kfunc_match(__put_cred, name, addr);
    // kfunc_match(exit_creds, name, addr);
    kfunc_match(copy_creds, name, addr);
    kfunc_match(get_task_cred, name, addr);
    kfunc_match(cred_alloc_blank, name, addr);
    kfunc_match(prepare_creds, name, addr);
    kfunc_match(prepare_exec_creds, name, addr);
    kfunc_match(commit_creds, name, addr);
    // kfunc_match(abort_creds, name, addr);
    kfunc_match(override_creds, name, addr);
    // kfunc_match(revert_creds, name, addr);
    kfunc_match(prepare_kernel_cred, name, addr);
    // kfunc_match(change_create_files_as, name, addr);
    kfunc_match(set_security_override, name, addr);
    kfunc_match(set_security_override_from_ctx, name, addr);
    // kfunc_match(set_create_files_as, name, addr);
    // kfunc_match(cred_fscmp, name, addr);
    // kfunc_match(cred_init, name, addr);
    // kfunc_match(creds_are_invalid, name, addr);
}

//=============================================================================
// 自旋锁相关函数（来自kernel/locking/spinlock.c）
//=============================================================================

#include <linux/spinlock.h>

/**
 * @brief 尝试获取原始自旋锁（非阻塞）
 * @param lock 目标自旋锁
 * @return 1成功获取，0锁已被占用
 * @details 非阻塞方式尝试获取自旋锁，不会导致进程休眠
 */
int kfunc_def(_raw_spin_trylock)(raw_spinlock_t *lock) = 0;

/**
 * @brief 尝试获取自旋锁（禁用软中断）
 * @param lock 目标自旋锁
 * @return 1成功获取，0锁已被占用
 * @details 在禁用软中断的情况下尝试获取自旋锁
 */
int kfunc_def(_raw_spin_trylock_bh)(raw_spinlock_t *lock) = 0;

/**
 * @brief 获取原始自旋锁（阻塞）
 * @param lock 目标自旋锁
 * @details 阻塞等待直到获得自旋锁，会自旋等待
 */
void kfunc_def(_raw_spin_lock)(raw_spinlock_t *lock) = 0;

/**
 * @brief 获取自旋锁并保存中断状态
 * @param lock 目标自旋锁
 * @return 之前的中断状态标志
 * @details 禁用中断并获取自旋锁，返回值用于后续恢复中断状态
 */
unsigned long kfunc_def(_raw_spin_lock_irqsave)(raw_spinlock_t *lock) = 0;

/**
 * @brief 获取自旋锁并禁用中断
 * @param lock 目标自旋锁
 * @details 禁用中断并获取自旋锁，用于中断上下文保护
 */
void kfunc_def(_raw_spin_lock_irq)(raw_spinlock_t *lock) = 0;

/**
 * @brief 获取自旋锁并禁用软中断
 * @param lock 目标自旋锁
 * @details 禁用软中断并获取自旋锁，用于软中断上下文保护
 */
void kfunc_def(_raw_spin_lock_bh)(raw_spinlock_t *lock) = 0;

/**
 * @brief 释放原始自旋锁
 * @param lock 目标自旋锁
 * @details 释放之前获取的自旋锁
 */
void kfunc_def(_raw_spin_unlock)(raw_spinlock_t *lock) = 0;

/**
 * @brief 释放自旋锁并恢复中断状态
 * @param lock 目标自旋锁
 * @param flags 要恢复的中断状态标志
 * @details 释放自旋锁并恢复到之前的中断状态
 */
void kfunc_def(_raw_spin_unlock_irqrestore)(raw_spinlock_t *lock, unsigned long flags) = 0;

/**
 * @brief 释放自旋锁并启用中断
 * @param lock 目标自旋锁
 * @details 释放自旋锁并重新启用中断
 */
void kfunc_def(_raw_spin_unlock_irq)(raw_spinlock_t *lock) = 0;

/**
 * @brief 释放自旋锁并启用软中断
 * @param lock 目标自旋锁
 * @details 释放自旋锁并重新启用软中断
 */
void kfunc_def(_raw_spin_unlock_bh)(raw_spinlock_t *lock) = 0;

//=============================================================================
// 读写锁相关函数（读锁操作）
//=============================================================================

/**
 * @brief 尝试获取读锁（非阻塞）
 * @param lock 目标读写锁
 * @return 1成功获取，0锁已被占用
 * @details 非阻塞方式尝试获取读锁，允许多个读者并发访问
 */
int kfunc_def(_raw_read_trylock)(rwlock_t *lock) = 0;

/**
 * @brief 获取读锁（阻塞）
 * @param lock 目标读写锁
 * @details 阻塞等待获取读锁，允许多个读者并发访问
 */
void kfunc_def(_raw_read_lock)(rwlock_t *lock) = 0;

/**
 * @brief 获取读锁并保存中断状态
 * @param lock 目标读写锁
 * @return 之前的中断状态标志
 * @details 禁用中断并获取读锁
 */
unsigned long kfunc_def(_raw_read_lock_irqsave)(rwlock_t *lock) = 0;

/**
 * @brief 获取读锁并禁用中断
 * @param lock 目标读写锁
 * @details 禁用中断并获取读锁
 */
void kfunc_def(_raw_read_lock_irq)(rwlock_t *lock) = 0;

/**
 * @brief 获取读锁并禁用软中断
 * @param lock 目标读写锁
 * @details 禁用软中断并获取读锁
 */
void kfunc_def(_raw_read_lock_bh)(rwlock_t *lock) = 0;

/**
 * @brief 释放读锁
 * @param lock 目标读写锁
 * @details 释放之前获取的读锁
 */
void kfunc_def(_raw_read_unlock)(rwlock_t *lock) = 0;

/**
 * @brief 释放读锁并恢复中断状态
 * @param lock 目标读写锁
 * @param flags 要恢复的中断状态标志
 * @details 释放读锁并恢复到之前的中断状态
 */
void kfunc_def(_raw_read_unlock_irqrestore)(rwlock_t *lock, unsigned long flags) = 0;

/**
 * @brief 释放读锁并启用中断
 * @param lock 目标读写锁
 * @details 释放读锁并重新启用中断
 */
void kfunc_def(_raw_read_unlock_irq)(rwlock_t *lock) = 0;

/**
 * @brief 释放读锁并启用软中断
 * @param lock 目标读写锁
 * @details 释放读锁并重新启用软中断
 */
void kfunc_def(_raw_read_unlock_bh)(rwlock_t *lock) = 0;

//=============================================================================
// 读写锁相关函数（写锁操作）
//=============================================================================

/**
 * @brief 尝试获取写锁（非阻塞）
 * @param lock 目标读写锁
 * @return 1成功获取，0锁已被占用
 * @details 非阻塞方式尝试获取写锁，独占访问资源
 */
int kfunc_def(_raw_write_trylock)(rwlock_t *lock) = 0;

/**
 * @brief 获取写锁（阻塞）
 * @param lock 目标读写锁
 * @details 阻塞等待获取写锁，独占访问资源
 */
void kfunc_def(_raw_write_lock)(rwlock_t *lock) = 0;

/**
 * @brief 获取写锁并保存中断状态
 * @param lock 目标读写锁
 * @return 之前的中断状态标志
 * @details 禁用中断并获取写锁
 */
unsigned long kfunc_def(_raw_write_lock_irqsave)(rwlock_t *lock) = 0;

/**
 * @brief 获取写锁并禁用中断
 * @param lock 目标读写锁
 * @details 禁用中断并获取写锁
 */
void kfunc_def(_raw_write_lock_irq)(rwlock_t *lock) = 0;

/**
 * @brief 获取写锁并禁用软中断
 * @param lock 目标读写锁
 * @details 禁用软中断并获取写锁
 */
void kfunc_def(_raw_write_lock_bh)(rwlock_t *lock) = 0;

/**
 * @brief 释放写锁
 * @param lock 目标读写锁
 * @details 释放之前获取的写锁
 */
void kfunc_def(_raw_write_unlock)(rwlock_t *lock) = 0;

/**
 * @brief 释放写锁并恢复中断状态
 * @param lock 目标读写锁
 * @param flags 要恢复的中断状态标志
 * @details 释放写锁并恢复到之前的中断状态
 */
void kfunc_def(_raw_write_unlock_irqrestore)(rwlock_t *lock, unsigned long flags) = 0;

/**
 * @brief 释放写锁并启用中断
 * @param lock 目标读写锁
 * @details 释放写锁并重新启用中断
 */
void kfunc_def(_raw_write_unlock_irq)(rwlock_t *lock) = 0;

/**
 * @brief 释放写锁并启用软中断
 * @param lock 目标读写锁
 * @details 释放写锁并重新启用软中断
 */
void kfunc_def(_raw_write_unlock_bh)(rwlock_t *lock) = 0;

/**
 * @brief Linux锁机制符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 通过符号名称匹配锁相关函数的地址，包括自旋锁和读写锁操作
 */
void _linux_locking_spinlock_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(_raw_spin_trylock, name, addr);
    // kfunc_match(_raw_spin_trylock_bh, name, addr);
    kfunc_match(_raw_spin_lock, name, addr);
    kfunc_match(_raw_spin_lock_irqsave, name, addr);
    kfunc_match(_raw_spin_lock_irq, name, addr);
    // kfunc_match(_raw_spin_lock_bh, name, addr);
    kfunc_match(_raw_spin_unlock, name, addr);
    kfunc_match(_raw_spin_unlock_irqrestore, name, addr);
    kfunc_match(_raw_spin_unlock_irq, name, addr);
    // kfunc_match(_raw_spin_unlock_bh, name, addr);
    // kfunc_match(_raw_read_trylock, name, addr);
    // kfunc_match(_raw_read_lock, name, addr);
    // kfunc_match(_raw_read_lock_irqsave, name, addr);
    // kfunc_match(_raw_read_lock_irq, name, addr);
    // kfunc_match(_raw_read_lock_bh, name, addr);
    // kfunc_match(_raw_read_unlock, name, addr);
    // kfunc_match(_raw_read_unlock_irqrestore, name, addr);
    // kfunc_match(_raw_read_unlock_irq, name, addr);
    // kfunc_match(_raw_read_unlock_bh, name, addr);
    // kfunc_match(_raw_write_trylock, name, addr);
    // kfunc_match(_raw_write_lock, name, addr);
    // kfunc_match(_raw_write_lock_irqsave, name, addr);
    // kfunc_match(_raw_write_lock_irq, name, addr);
    // kfunc_match(_raw_write_lock_bh, name, addr);
    // kfunc_match(_raw_write_unlock, name, addr);
    // kfunc_match(_raw_write_unlock_irqrestore, name, addr);
    // kfunc_match(_raw_write_unlock_irq, name, addr);
    // kfunc_match(_raw_write_unlock_bh, name, addr);
}

//=============================================================================
// 进程管理相关函数（来自kernel/fork.c）
//=============================================================================

#include <ksyms.h>

struct file;
struct mm_struct;
struct task_struct;
struct kernel_clone_args;
struct files_struct;

/**
 * @brief 从文件描述符获取PID结构
 * @param file 文件结构指针
 * @return 关联的PID结构指针
 * @details 从pidfd文件描述符中提取对应的PID结构
 */
struct pid *kfunc_def(pidfd_pid)(const struct file *file) = 0;

/**
 * @brief 释放进程结构
 * @param tsk 要释放的进程结构
 * @details 释放进程结构占用的内存和资源
 */
void kfunc_def(free_task)(struct task_struct *tsk) = 0;

/**
 * @brief 减少进程结构引用计数
 * @param tsk 目标进程结构
 * @details 当引用计数降为0时释放进程结构
 */
void kfunc_def(__put_task_struct)(struct task_struct *tsk) = 0;

/**
 * @brief 初始化fork子系统
 * @details 系统启动时初始化进程创建相关的数据结构
 */
void kfunc_def(fork_init)(void) = 0;

/**
 * @brief 设置内存描述符的可执行文件
 * @param mm 内存描述符
 * @param new_exe_file 新的可执行文件
 * @details 将可执行文件关联到内存映射中
 */
void kfunc_def(set_mm_exe_file)(struct mm_struct *mm, struct file *new_exe_file) = 0;

/**
 * @brief 获取内存描述符的可执行文件
 * @param mm 内存描述符
 * @return 可执行文件指针
 * @details 获取与内存映射关联的可执行文件
 */
struct file *kfunc_def(get_mm_exe_file)(struct mm_struct *mm) = 0;

/**
 * @brief 获取进程的可执行文件
 * @param task 目标进程
 * @return 可执行文件指针
 * @details 获取进程对应的可执行文件
 */
struct file *kfunc_def(get_task_exe_file)(struct task_struct *task) = 0;

/**
 * @brief 访问进程内存映射
 * @param task 目标进程
 * @param mode 访问模式
 * @return 内存描述符指针
 * @details 安全地访问其他进程的内存映射
 */
struct mm_struct *kfunc_def(mm_access)(struct task_struct *task, unsigned int mode) = 0;

/**
 * @brief 进程退出时释放内存映射
 * @param tsk 退出的进程
 * @param mm 内存描述符
 * @details 进程退出时清理其内存映射资源
 */
void kfunc_def(exit_mm_release)(struct task_struct *tsk, struct mm_struct *mm) = 0;

/**
 * @brief exec时释放内存映射
 * @param tsk 执行exec的进程
 * @param mm 旧的内存描述符
 * @details exec系统调用时清理旧的内存映射
 */
void kfunc_def(exec_mm_release)(struct task_struct *tsk, struct mm_struct *mm) = 0;

/**
 * @brief 创建空闲进程
 * @param cpu 目标CPU编号
 * @return 新创建的进程结构
 * @details 为指定CPU创建空闲进程（idle进程）
 */
struct task_struct *kfunc_def(fork_idle)(int cpu) = 0;

/**
 * @brief 复制初始内存映射
 * @return 复制的内存描述符
 * @details 创建init进程内存映射的副本
 */
struct mm_struct *kfunc_def(copy_init_mm)(void) = 0;

/**
 * @brief 创建IO线程
 * @param fn 线程函数
 * @param arg 线程参数
 * @param node NUMA节点
 * @return 新创建的线程结构
 * @details 创建专门用于IO操作的内核线程
 */
struct task_struct *kfunc_def(create_io_thread)(int (*fn)(void *), void *arg, int node) = 0;

/**
 * @brief 内核进程克隆
 * @param args 克隆参数结构
 * @return 新进程的PID
 * @details 根据参数创建新进程，是fork/clone的底层实现
 */
pid_t kfunc_def(kernel_clone)(struct kernel_clone_args *args) = 0;

/**
 * @brief 创建内核线程
 * @param fn 线程函数
 * @param arg 线程参数
 * @param flags 创建标志
 * @return 新线程的PID
 * @details 创建在内核空间运行的线程
 */
pid_t kfunc_def(kernel_thread)(int (*fn)(void *), void *arg, unsigned long flags) = 0;

/**
 * @brief 取消共享文件描述符表
 * @param unshare_flags 取消共享标志
 * @param max_fds 最大文件描述符数
 * @param new_fdp 新文件描述符表指针
 * @return 0成功，负值表示错误
 * @details 为进程创建独立的文件描述符表
 */
int kfunc_def(unshare_fd)(unsigned long unshare_flags, unsigned int max_fds, struct files_struct **new_fdp) = 0;

/**
 * @brief 系统调用unshare的内核实现
 * @param unshare_flags 取消共享标志
 * @return 0成功，负值表示错误
 * @details 取消进程与父进程共享的各种资源
 */
int kfunc_def(ksys_unshare)(unsigned long unshare_flags) = 0;

/**
 * @brief 取消共享文件结构
 * @param displaced 被替换的文件结构
 * @return 0成功，负值表示错误
 * @details 为进程创建独立的文件结构
 */
int kfunc_def(unshare_files)(struct files_struct **displaced) = 0;

/**
 * @brief Linux内核fork符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 通过符号名称匹配进程管理相关函数的地址
 */
static void _linux_kernel_fork_sym_match(const char *name, unsigned long addr)
{
    // kfunc_match(pidfd_pid, name, addr);
    // kfunc_match(get_mm_exe_file, name, addr);
    // kfunc_match(free_task, name, addr);
    // kfunc_match(__put_task_struct, name, addr);
    // kfunc_match(fork_init, name, addr);
    // kfunc_match(set_mm_exe_file, name, addr);
    // kfunc_match(get_mm_exe_file, name, addr);
    // kfunc_match(get_task_exe_file, name, addr);
    // kfunc_match(mm_access, name, addr);
    // kfunc_match(exit_mm_release, name, addr);
    // kfunc_match(exec_mm_release, name, addr);
    // kfunc_match(fork_idle, name, addr);
    // kfunc_match(copy_init_mm, name, addr);
    // kfunc_match(create_io_thread, name, addr);
    // kfunc_match(kernel_clone, name, addr);
    // kfunc_match(kernel_thread, name, addr);
    // kfunc_match(unshare_fd, name, addr);
    // kfunc_match(ksys_unshare, name, addr);
    // kfunc_match(unshare_files, name, addr);
}

//=============================================================================
// PID管理相关函数（来自kernel/pid.c）
//=============================================================================
#include <linux/pid.h>
#include <linux/sched/task.h>
#include <linux/sched.h>

/**
 * @brief 从文件描述符获取PID结构
 * @param fd 文件描述符
 * @param flags 标志位指针
 * @return PID结构指针
 * @details 从pidfd文件描述符中获取对应的PID结构
 */
struct pid *kfunc_def(pidfd_get_pid)(unsigned int fd, unsigned int *flags) = 0;

/**
 * @brief 减少PID结构引用计数
 * @param pid 目标PID结构
 * @details 当引用计数降为0时释放PID结构
 */
void kfunc_def(put_pid)(struct pid *pid) = 0;

/**
 * @brief 从PID结构获取进程结构
 * @param pid PID结构
 * @param type PID类型（PIDTYPE_PID, PIDTYPE_TGID等）
 * @return 进程结构指针
 * @details 根据PID类型从PID结构中获取对应的进程
 */
struct task_struct *kfunc_def(pid_task)(struct pid *pid, enum pid_type) = 0;

/**
 * @brief 获取PID对应的进程并增加引用计数
 * @param pid PID结构
 * @param type PID类型
 * @return 进程结构指针
 * @details 获取进程的同时增加其引用计数
 */
struct task_struct *kfunc_def(get_pid_task)(struct pid *pid, enum pid_type) = 0;

/**
 * @brief 从进程获取指定类型的PID
 * @param task 目标进程
 * @param type PID类型
 * @return PID结构指针
 * @details 获取进程的特定类型PID（进程ID、线程组ID等）
 */
struct pid *kfunc_def(get_task_pid)(struct task_struct *task, enum pid_type type) = 0;

/**
 * @brief 将PID关联到进程
 * @param task 目标进程
 * @param type PID类型
 * @details 建立进程与PID的关联关系
 */
void kfunc_def(attach_pid)(struct task_struct *task, enum pid_type) = 0;

/**
 * @brief 解除进程与PID的关联
 * @param task 目标进程
 * @param type PID类型
 * @details 断开进程与特定类型PID的关联
 */
void kfunc_def(detach_pid)(struct task_struct *task, enum pid_type) = 0;

/**
 * @brief 改变进程的PID
 * @param task 目标进程
 * @param type PID类型
 * @param pid 新的PID结构
 * @details 更新进程的特定类型PID
 */
void kfunc_def(change_pid)(struct task_struct *task, enum pid_type, struct pid *pid) = 0;

/**
 * @brief 交换两个进程的线程ID
 * @param task 第一个进程
 * @param old 第二个进程
 * @details 交换两个进程的TID，用于某些特殊情况
 */
void kfunc_def(exchange_tids)(struct task_struct *task, struct task_struct *old) = 0;

/**
 * @brief 转移PID从旧进程到新进程
 * @param old 原进程
 * @param new 新进程
 * @param type PID类型
 * @details 将PID的归属从一个进程转移到另一个进程
 */
void kfunc_def(transfer_pid)(struct task_struct *old, struct task_struct *new, enum pid_type) = 0;

/**
 * @brief 获取进程在指定命名空间中的PID号
 * @param task 目标进程
 * @param type PID类型
 * @param ns PID命名空间
 * @return PID号
 * @details 获取进程在特定命名空间中的数字ID
 */
pid_t kfunc_def(__task_pid_nr_ns)(struct task_struct *task, enum pid_type type, struct pid_namespace *ns) = 0;

/**
 * @brief 获取进程的活动PID命名空间
 * @param tsk 目标进程
 * @return PID命名空间指针
 * @details 获取进程当前所在的PID命名空间
 */
struct pid_namespace *kfunc_def(task_active_pid_ns)(struct task_struct *tsk) = 0;

/**
 * @brief 在指定命名空间中查找PID
 * @param nr PID号
 * @param ns PID命名空间
 * @return PID结构指针
 * @details 在特定命名空间中根据数字ID查找PID结构
 */
struct pid *kfunc_def(find_pid_ns)(int nr, struct pid_namespace *ns) = 0;

/**
 * @brief 查找虚拟PID
 * @param nr PID号
 * @return PID结构指针
 * @details 在当前命名空间中查找虚拟PID
 */
struct pid *kfunc_def(find_vpid)(int nr) = 0;

/**
 * @brief 查找PID并增加引用计数
 * @param nr PID号
 * @return PID结构指针
 * @details 查找PID的同时增加其引用计数
 */
struct pid *kfunc_def(find_get_pid)(int nr) = 0;

/**
 * @brief 查找大于等于指定值的PID
 * @param nr 起始PID号
 * @param ns PID命名空间
 * @return PID结构指针
 * @details 查找命名空间中第一个大于等于指定值的PID
 */
struct pid *kfunc_def(find_ge_pid)(int nr, struct pid_namespace *ns) = 0;

/**
 * @brief 分配新的PID
 * @param ns PID命名空间
 * @param set_tid 指定的TID集合
 * @param set_tid_size TID集合大小
 * @return 新分配的PID结构
 * @details 在指定命名空间中分配新的PID
 */
struct pid *kfunc_def(alloc_pid)(struct pid_namespace *ns, pid_t *set_tid, size_t set_tid_size) = 0;

/**
 * @brief 释放PID结构
 * @param pid 要释放的PID
 * @details 释放PID占用的资源
 */
void kfunc_def(free_pid)(struct pid *pid) = 0;

/**
 * @brief 禁用PID命名空间的分配
 * @param ns 目标命名空间
 * @details 禁止在指定命名空间中分配新的PID
 */
void kfunc_def(disable_pid_allocation)(struct pid_namespace *ns) = 0;

/**
 * @brief 获取PID在指定命名空间中的号码
 * @param pid PID结构
 * @param ns PID命名空间
 * @return PID号
 * @details 获取PID在特定命名空间中的数字表示
 */
pid_t kfunc_def(pid_nr_ns)(struct pid *pid, struct pid_namespace *ns) = 0;

/**
 * @brief 获取PID的虚拟号码
 * @param pid PID结构
 * @return 虚拟PID号
 * @details 获取PID在当前命名空间中的数字表示
 */
pid_t kfunc_def(pid_vnr)(struct pid *pid) = 0;

/**
 * @brief 通过虚拟PID查找进程
 * @param nr 虚拟PID号
 * @return 进程结构指针
 * @details 在当前命名空间中根据PID查找进程
 */
struct task_struct *kfunc_def(find_task_by_vpid)(pid_t nr) = 0;

/**
 * @brief 在指定命名空间中通过PID查找进程
 * @param nr PID号
 * @param ns PID命名空间
 * @return 进程结构指针
 * @details 在特定命名空间中根据PID查找进程
 */
struct task_struct *kfunc_def(find_task_by_pid_ns)(pid_t nr, struct pid_namespace *ns) = 0;

/**
 * @brief 通过虚拟PID查找进程并增加引用计数
 * @param nr 虚拟PID号
 * @return 进程结构指针
 * @details 查找进程的同时增加其引用计数
 */
struct task_struct *kfunc_def(find_get_task_by_vpid)(pid_t nr) = 0;

/**
 * @brief Linux内核PID符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 通过符号名称匹配PID管理相关函数的地址
 */
void _linux_kernel_pid_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(pidfd_get_pid, name, addr);
    kfunc_match(put_pid, name, addr);
    kfunc_match(pid_task, name, addr);
    kfunc_match(get_pid_task, name, addr);
    kfunc_match(get_task_pid, name, addr);
    // kfunc_match(attach_pid, name, addr);
    // kfunc_match(detach_pid, name, addr);
    // kfunc_match(change_pid, name, addr);
    // kfunc_match(exchange_tids, name, addr);
    // kfunc_match(transfer_pid, name, addr);

    kfunc_match(__task_pid_nr_ns, name, addr);
    kfunc_match(task_active_pid_ns, name, addr);
    kfunc_match(find_pid_ns, name, addr);
    kfunc_match(find_vpid, name, addr);
    // kfunc_match(find_get_pid, name, addr);
    // kfunc_match(find_ge_pid, name, addr);
    // kfunc_match(alloc_pid, name, addr);
    // kfunc_match(free_pid, name, addr);
    // kfunc_match(disable_pid_allocation, name, addr);
    kfunc_match(pid_nr_ns, name, addr);
    kfunc_match(pid_vnr, name, addr);

    kfunc_match(find_task_by_vpid, name, addr);
    kfunc_match(find_task_by_pid_ns, name, addr);
    kfunc_match(find_get_task_by_vpid, name, addr);
}

//=============================================================================
// 其他内核函数（来自各种子系统）
//=============================================================================

//=============================================================================
// CPU停机管理（来自kernel/stop_machine.c）
//=============================================================================
#include <linux/stop_machine.h>

/**
 * @brief 停机机制初始化状态标志
 * @details 指示stop_machine子系统是否已完成初始化
 */
bool kvar_def(stop_machine_initialized) = 0;

/**
 * @brief 在线CPU掩码
 * @details 指示当前系统中处于在线状态的CPU集合
 */
const struct cpumask *kvar_def(cpu_online_mask) = 0;

/**
 * @brief 在指定CPU上停机执行函数
 * @param fn 要执行的函数
 * @param data 传递给函数的数据
 * @param cpus 目标CPU掩码
 * @return 执行结果
 * @details 停止指定CPU的正常调度，执行特定函数，常用于需要原子性的系统操作
 */
int kfunc_def(stop_machine)(int (*fn)(void *), void *data, const struct cpumask *cpus) = 0;

/**
 * @brief 停机管理符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配CPU停机管理相关的函数和变量地址
 */
static void _linux_kernel_stop_machine_sym_match(const char *name, unsigned long addr)
{
    // kvar_match(stop_machine_initialized, name, addr);
    // kvar_match(cpu_online_mask, name, addr);
    kfunc_match(stop_machine, name, addr);
}

//=============================================================================
// 内存管理工具函数（来自mm/util.c）
//=============================================================================

struct file;
struct page;
struct address_space;
struct task_struct;

/**
 * @brief 从用户空间复制字符串
 * @param user_ptr 用户空间字符串指针
 * @param max_len 最大长度
 * @return 内核中复制的字符串指针
 * @details 安全地从用户空间复制有限长度的字符串到内核空间
 */
char *kfunc_def(strndup_user)(const char __user *, long) = 0;

/**
 * @brief 从用户空间复制内存块
 * @param user_ptr 用户空间内存指针
 * @param size 复制大小
 * @return 内核中复制的内存指针
 * @details 安全地从用户空间复制指定大小的内存到内核空间
 */
void *kfunc_def(memdup_user)(const void __user *, size_t) = 0;

/**
 * @brief 使用vmalloc从用户空间复制内存
 * @param user_ptr 用户空间内存指针
 * @param size 复制大小
 * @return 内核中复制的内存指针
 * @details 使用vmalloc分配器从用户空间复制大块内存
 */
void *kfunc_def(vmemdup_user)(const void __user *, size_t) = 0;

/**
 * @brief 从用户空间复制内存并添加空终止符
 * @param user_ptr 用户空间内存指针
 * @param size 复制大小
 * @return 内核中复制的内存指针
 * @details 复制用户空间内存并在末尾添加空字符
 */
void *kfunc_def(memdup_user_nul)(const void __user *, size_t) = 0;

/**
 * @brief 释放常量内存
 * @param x 要释放的内存指针
 * @details 释放可能是常量的内存，会检查是否需要实际释放
 */
void kfunc_def(kfree_const)(const void *x) = 0;

/**
 * @brief 复制字符串
 * @param s 源字符串
 * @param gfp 内存分配标志
 * @return 复制的字符串指针
 * @details 分配内存并复制字符串
 */
char *kfunc_def(kstrdup)(const char *s, gfp_t gfp) = 0;

/**
 * @brief 复制常量字符串
 * @param s 源字符串
 * @param gfp 内存分配标志
 * @return 复制的字符串指针
 * @details 如果字符串是常量则直接返回，否则复制
 */
const char *kfunc_def(kstrdup_const)(const char *s, gfp_t gfp) = 0;

/**
 * @brief 复制有限长度字符串
 * @param s 源字符串
 * @param max 最大长度
 * @param gfp 内存分配标志
 * @return 复制的字符串指针
 * @details 复制最多指定长度的字符串
 */
char *kfunc_def(kstrndup)(const char *s, size_t max, gfp_t gfp) = 0;

/**
 * @brief 复制内存块
 * @param src 源内存地址
 * @param len 复制长度
 * @param gfp 内存分配标志
 * @return 复制的内存指针
 * @details 分配内存并复制指定长度的数据
 */
void *kfunc_def(kmemdup)(const void *src, size_t len, gfp_t gfp) = 0;

/**
 * @brief 复制内存并添加空终止符
 * @param s 源内存地址
 * @param len 复制长度
 * @param gfp 内存分配标志
 * @return 复制的内存指针
 * @details 复制内存并在末尾添加空字符
 */
char *kfunc_def(kmemdup_nul)(const char *s, size_t len, gfp_t gfp) = 0;

/**
 * @brief 映射虚拟内存
 * @param file 要映射的文件
 * @param addr 起始地址
 * @param len 映射长度
 * @param prot 保护属性
 * @param flag 映射标志
 * @param offset 文件偏移
 * @return 映射的虚拟地址
 * @details 在虚拟地址空间中创建内存映射
 */
unsigned long kfunc_def(vm_mmap)(struct file *file, unsigned long addr, unsigned long len, unsigned long prot,
                                 unsigned long flag, unsigned long offset) = 0;

/**
 * @brief 在指定NUMA节点分配内存
 * @param size 分配大小
 * @param flags 分配标志
 * @param node NUMA节点号
 * @return 分配的内存指针
 * @details 优先在指定NUMA节点上分配内存
 */
void *kfunc_def(kvmalloc_node)(size_t size, gfp_t flags, int node) = 0;

/**
 * @brief 释放kvmalloc分配的内存
 * @param addr 要释放的内存地址
 * @details 释放通过kvmalloc分配的内存
 */
void kfunc_def(kvfree)(const void *addr) = 0;

/**
 * @brief 安全释放敏感内存
 * @param addr 要释放的内存地址
 * @param len 内存长度
 * @details 在释放前清零内存内容，防止敏感数据泄露
 */
void kfunc_def(kvfree_sensitive)(const void *addr, size_t len) = 0;

/**
 * @brief 重新分配内存
 * @param p 原内存指针
 * @param oldsize 原大小
 * @param newsize 新大小
 * @param flags 分配标志
 * @return 新内存指针
 * @details 调整内存块大小，保留原有数据
 */
void *kfunc_def(kvrealloc)(const void *p, size_t oldsize, size_t newsize, gfp_t flags) = 0;

/**
 * @brief 检查页面是否被映射
 * @param page 目标页面
 * @return true表示已映射，false表示未映射
 * @details 检查物理页面是否在某个虚拟地址空间中被映射
 */
bool kfunc_def(page_mapped)(struct page *page) = 0;

/**
 * @brief 获取页面的地址空间
 * @param page 目标页面
 * @return 地址空间结构指针
 * @details 获取页面所属的地址空间对象
 */
struct address_space *kfunc_def(page_mapping)(struct page *page) = 0;

/**
 * @brief 获取页面映射计数
 * @param page 目标页面
 * @return 映射计数
 * @details 获取页面被映射的次数
 */
int kfunc_def(__page_mapcount)(struct page *page) = 0;

/**
 * @brief 获取已提交的虚拟内存量
 * @return 已提交的内存大小
 * @details 返回系统中已提交的虚拟内存总量
 */
unsigned long kfunc_def(vm_memory_committed)(void) = 0;

/**
 * @brief 获取进程命令行
 * @param task 目标进程
 * @param buffer 输出缓冲区
 * @param buflen 缓冲区长度
 * @return 实际复制的字节数
 * @details 获取进程的命令行参数（未导出符号）
 */
int kfunc_def(get_cmdline)(struct task_struct *task, char *buffer, int buflen) = 0; // not exported

/**
 * @brief 分配内存（内部函数）
 * @param size 分配大小
 * @param flags 分配标志
 * @return 分配的内存指针
 * @details kmalloc的底层实现函数
 */
void *kfunc_def(__kmalloc)(size_t size, gfp_t flags) = 0;

/**
 * @brief 分配内存
 * @param size 分配大小
 * @param flags 分配标志
 * @return 分配的内存指针
 * @details 内核中最常用的内存分配函数
 */
void *kfunc_def(kmalloc)(size_t size, gfp_t flags) = 0;

/**
 * @brief 释放内存
 * @param ptr 要释放的内存指针
 * @details 释放通过kmalloc分配的内存
 */
void kfunc_def(kfree)(const void *) = 0;

/**
 * @brief 内存管理工具函数符号匹配
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配内存管理相关工具函数的地址
 */
static void _linux_mm_utils_sym_match(const char *name, unsigned long addr)
{
    // kfunc_match(kfree_const, name, addr);
    // kfunc_match(kstrdup, name, addr);
    // kfunc_match(kstrdup_const, name, addr);
    // kfunc_match(kstrndup, name, addr);
    // kfunc_match(kmemdup, name, addr);
    // kfunc_match(kmemdup_nul, name, addr);
    kfunc_match(memdup_user, name, addr);
    // kfunc_match(vmemdup_user, name, addr);
    kfunc_match(strndup_user, name, addr);
    // kfunc_match(memdup_user_nul, name, addr);
    // kfunc_match(vm_mmap, name, addr);
    // kfunc_match(kvmalloc_node, name, addr);
    kfunc_match(kvfree, name, addr);
    // kfunc_match(kvfree_sensitive, name, addr);
    // kfunc_match(kvrealloc, name, addr);
    // kfunc_match(page_mapped, name, addr);
    // kfunc_match(page_mapping, name, addr);
    // kfunc_match(__page_mapcount, name, addr);
    // kfunc_match(vm_memory_committed, name, addr);
    // kfunc_match(get_cmdline, name, addr);
    // kfunc_match(__kmalloc, name, addr);
    // kfunc_match(kmalloc, name, addr);
    kfunc_match(kfree, name, addr);
}

//=============================================================================
// 虚拟内存分配（来自mm/vmalloc.c）
//=============================================================================
#include <linux/vmalloc.h>

/**
 * @brief 取消RAM页面的线性映射
 * @param mem 内存地址
 * @param count 页面数量
 * @details 取消之前通过vm_map_ram创建的线性映射
 */
void kfunc_def(vm_unmap_ram)(const void *mem, unsigned int count) = 0;

/**
 * @brief 将页面线性映射到RAM
 * @param pages 页面数组
 * @param count 页面数量
 * @param node NUMA节点
 * @return 映射的虚拟地址
 * @details 将页面数组连续映射到虚拟地址空间
 */
void *kfunc_def(vm_map_ram)(struct page **pages, unsigned int count, int node) = 0;

/**
 * @brief 取消映射别名
 * @details 刷新TLB以确保所有映射别名被取消
 */
void kfunc_def(vm_unmap_aliases)(void) = 0;

/**
 * @brief 分配虚拟内存
 * @param size 分配大小
 * @return 分配的虚拟地址
 * @details 分配连续的虚拟内存区域
 */
void *kfunc_def(vmalloc)(unsigned long size) = 0;

/**
 * @brief 分配并清零虚拟内存
 * @param size 分配大小
 * @return 分配的虚拟地址
 * @details 分配虚拟内存并初始化为零
 */
void *kfunc_def(vzalloc)(unsigned long size) = 0;

/**
 * @brief 分配用户可访问的虚拟内存
 * @param size 分配大小
 * @return 分配的虚拟地址
 * @details 分配可以映射到用户空间的虚拟内存
 */
void *kfunc_def(vmalloc_user)(unsigned long size) = 0;

/**
 * @brief 在指定NUMA节点分配虚拟内存
 * @param size 分配大小
 * @param node NUMA节点
 * @return 分配的虚拟地址
 * @details 优先在指定NUMA节点分配虚拟内存
 */
void *kfunc_def(vmalloc_node)(unsigned long size, int node) = 0;

/**
 * @brief 在指定NUMA节点分配并清零虚拟内存
 * @param size 分配大小
 * @param node NUMA节点
 * @return 分配的虚拟地址
 * @details 在指定节点分配虚拟内存并初始化为零
 */
void *kfunc_def(vzalloc_node)(unsigned long size, int node) = 0;

/**
 * @brief 分配32位可寻址的虚拟内存
 * @param size 分配大小
 * @return 分配的虚拟地址
 * @details 分配在32位地址空间内的虚拟内存
 */
void *kfunc_def(vmalloc_32)(unsigned long size) = 0;

/**
 * @brief 分配32位用户可访问的虚拟内存
 * @param size 分配大小
 * @return 分配的虚拟地址
 * @details 分配在32位地址空间内且用户可访问的虚拟内存
 */
void *kfunc_def(vmalloc_32_user)(unsigned long size) = 0;

/**
 * @brief 低级虚拟内存分配
 * @param size 分配大小
 * @param gfp_mask 内存分配标志
 * @return 分配的虚拟地址
 * @details vmalloc的底层实现函数
 */
void *kfunc_def(__vmalloc)(unsigned long size, gfp_t gfp_mask) = 0;

/**
 * @brief 在指定范围内分配虚拟内存
 * @param size 分配大小
 * @param align 对齐要求
 * @param start 起始地址
 * @param end 结束地址
 * @param gfp_mask 内存分配标志
 * @param prot 页面保护属性
 * @param vm_flags 虚拟内存标志
 * @param node NUMA节点
 * @param caller 调用者地址
 * @return 分配的虚拟地址
 * @details 在指定地址范围内分配虚拟内存
 */
void *kfunc_def(__vmalloc_node_range)(unsigned long size, unsigned long align, unsigned long start, unsigned long end,
                                      gfp_t gfp_mask, pgprot_t prot, unsigned long vm_flags, int node,
                                      const void *caller) = 0;

/**
 * @brief 在指定节点分配虚拟内存
 * @param size 分配大小
 * @param align 对齐要求
 * @param gfp_mask 内存分配标志
 * @param node NUMA节点
 * @param caller 调用者地址
 * @return 分配的虚拟地址
 * @details 在指定NUMA节点分配对齐的虚拟内存
 */
void *kfunc_def(__vmalloc_node)(unsigned long size, unsigned long align, gfp_t gfp_mask, int node,
                                const void *caller) = 0;

/**
 * @brief 释放虚拟内存
 * @param addr 要释放的地址
 * @details 释放通过vmalloc分配的虚拟内存
 */
void kfunc_def(vfree)(const void *addr) = 0;

/**
 * @brief 原子地释放虚拟内存
 * @param addr 要释放的地址
 * @details 在原子上下文中释放虚拟内存
 */
void kfunc_def(vfree_atomic)(const void *addr) = 0;

/**
 * @brief 映射页面到虚拟地址
 * @param pages 页面数组
 * @param count 页面数量
 * @param flags 映射标志
 * @param prot 页面保护属性
 * @return 映射的虚拟地址
 * @details 将页面数组映射到连续的虚拟地址空间
 */
void *kfunc_def(vmap)(struct page **pages, unsigned int count, unsigned long flags, pgprot_t prot) = 0;

/**
 * @brief 映射页帧号到虚拟地址
 * @param pfns 页帧号数组
 * @param count 页面数量
 * @param prot 页面保护属性
 * @return 映射的虚拟地址
 * @details 将页帧号数组映射到虚拟地址空间
 */
void *kfunc_def(vmap_pfn)(unsigned long *pfns, unsigned int count, pgprot_t prot) = 0;

/**
 * @brief 取消虚拟地址映射
 * @param addr 要取消映射的地址
 * @details 取消之前通过vmap创建的映射
 */
void kfunc_def(vunmap)(const void *addr) = 0;

/**
 * @brief 部分重映射vmalloc区域到VMA
 * @param vma 虚拟内存区域
 * @param uaddr 用户地址
 * @param kaddr 内核地址
 * @param pgoff 页面偏移
 * @param size 映射大小
 * @return 0成功，负值表示错误
 * @details 将vmalloc区域的一部分重映射到用户VMA
 */
int kfunc_def(remap_vmalloc_range_partial)(struct vm_area_struct *vma, unsigned long uaddr, void *kaddr,
                                           unsigned long pgoff, unsigned long size) = 0;

/**
 * @brief 重映射vmalloc区域到VMA
 * @param vma 虚拟内存区域
 * @param addr 内核地址
 * @param pgoff 页面偏移
 * @return 0成功，负值表示错误
 * @details 将vmalloc区域重映射到用户VMA
 */
int kfunc_def(remap_vmalloc_range)(struct vm_area_struct *vma, void *addr, unsigned long pgoff) = 0;

/**
 * @brief 获取虚拟内存区域
 * @param size 区域大小
 * @param flags 标志
 * @return 虚拟内存区域结构
 * @details 分配指定大小的虚拟内存区域
 */
struct vm_struct *kfunc_def(get_vm_area)(unsigned long size, unsigned long flags) = 0;

/**
 * @brief 获取虚拟内存区域（带调用者信息）
 * @param size 区域大小
 * @param flags 标志
 * @param caller 调用者地址
 * @return 虚拟内存区域结构
 * @details 分配虚拟内存区域并记录调用者信息
 */
struct vm_struct *kfunc_def(get_vm_area_caller)(unsigned long size, unsigned long flags, const void *caller) = 0;

/**
 * @brief 在指定范围内获取虚拟内存区域
 * @param size 区域大小
 * @param flags 标志
 * @param start 起始地址
 * @param end 结束地址
 * @param caller 调用者地址
 * @return 虚拟内存区域结构
 * @details 在指定地址范围内分配虚拟内存区域
 */
struct vm_struct *kfunc_def(__get_vm_area_caller)(unsigned long size, unsigned long flags, unsigned long start,
                                                  unsigned long end, const void *caller) = 0;

/**
 * @brief 释放虚拟内存区域
 * @param area 要释放的区域
 * @details 释放虚拟内存区域结构和相关资源
 */
void kfunc_def(free_vm_area)(struct vm_struct *area) = 0;

/**
 * @brief 移除虚拟内存区域
 * @param addr 区域地址
 * @return 移除的区域结构
 * @details 从虚拟内存管理中移除指定区域
 */
struct vm_struct *kfunc_def(remove_vm_area)(const void *addr) = 0;

/**
 * @brief 查找虚拟内存区域
 * @param addr 查找地址
 * @return 找到的区域结构
 * @details 根据地址查找对应的虚拟内存区域
 */
struct vm_struct *kfunc_def(find_vm_area)(const void *addr) = 0;

/**
 * @brief 映射内核地址范围（不刷新TLB）
 * @param start 起始地址
 * @param size 映射大小
 * @param prot 页面保护属性
 * @param pages 页面数组
 * @return 0成功，负值表示错误
 * @details 映射页面到内核地址范围，不刷新TLB
 */
int kfunc_def(map_kernel_range_noflush)(unsigned long start, unsigned long size, pgprot_t prot,
                                        struct page **pages) = 0;

/**
 * @brief 映射内核地址范围
 * @param start 起始地址
 * @param size 映射大小
 * @param prot 页面保护属性
 * @param pages 页面数组
 * @return 0成功，负值表示错误
 * @details 映射页面到内核地址范围并刷新TLB
 */
int kfunc_def(map_kernel_range)(unsigned long start, unsigned long size, pgprot_t prot, struct page **pages) = 0;

/**
 * @brief 取消映射内核地址范围（不刷新TLB）
 * @param addr 起始地址
 * @param size 取消映射大小
 * @details 取消内核地址范围映射，不刷新TLB
 */
void kfunc_def(unmap_kernel_range_noflush)(unsigned long addr, unsigned long size) = 0;

/**
 * @brief 取消映射内核地址范围
 * @param addr 起始地址
 * @param size 取消映射大小
 * @details 取消内核地址范围映射并刷新TLB
 */
void kfunc_def(unmap_kernel_range)(unsigned long addr, unsigned long size) = 0;

/**
 * @brief 从虚拟地址读取数据
 * @param buf 输出缓冲区
 * @param addr 源虚拟地址
 * @param count 读取字节数
 * @return 实际读取的字节数
 * @details 安全地从虚拟地址读取数据
 */
long kfunc_def(vread)(char *buf, char *addr, unsigned long count) = 0;

/**
 * @brief 向虚拟地址写入数据
 * @param buf 源缓冲区
 * @param addr 目标虚拟地址
 * @param count 写入字节数
 * @return 实际写入的字节数
 * @details 安全地向虚拟地址写入数据
 */
long kfunc_def(vwrite)(char *buf, char *addr, unsigned long count) = 0;

/**
 * @brief 虚拟内存分配符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配vmalloc相关函数的地址
 */
static void _linux_mm_vmalloc_sym_match(const char *name, unsigned long addr)
{
    // kfunc_match(vm_unmap_ram, name, addr);
    // kfunc_match(vm_map_ram, name, addr);
    // kfunc_match(vm_unmap_aliases, name, addr);

    kfunc_match(vmalloc, name, addr);
    kfunc_match(vzalloc, name, addr);
    // kfunc_match(vmalloc_user, name, addr);
    // kfunc_match(vmalloc_node, name, addr);
    // kfunc_match(vzalloc_node, name, addr);
    // kfunc_match(vmalloc_32, name, addr);
    // kfunc_match(vmalloc_32_user, name, addr);
    kfunc_match(__vmalloc, name, addr);
    // kfunc_match(__vmalloc_node_range, name, addr);
    // kfunc_match(__vmalloc_node, name, addr);

    kfunc_match(vfree, name, addr);
    // kfunc_match(vfree_atomic, name, addr);

    // kfunc_match(vmap, name, addr);
    // kfunc_match(vmap_pfn, name, addr);
    // kfunc_match(vunmap, name, addr);
    // kfunc_match(remap_vmalloc_range_partial, name, addr);
    // kfunc_match(remap_vmalloc_range, name, addr);

    // kfunc_match(get_vm_area, name, addr);
    // kfunc_match(get_vm_area_caller, name, addr);
    // kfunc_match(__get_vm_area_caller, name, addr);
    // kfunc_match(free_vm_area, name, addr);
    // kfunc_match(remove_vm_area, name, addr);
    // kfunc_match(find_vm_area, name, addr);

    // kfunc_match(map_kernel_range_noflush, name, addr);
    // kfunc_match(map_kernel_range, name, addr);
    // kfunc_match(unmap_kernel_range_noflush, name, addr);
    // kfunc_match(unmap_kernel_range, name, addr);

    // kfunc_match(vread, name, addr);
    // kfunc_match(vwrite, name, addr);
}

//=============================================================================
// 文件系统相关函数（来自fs/）
//=============================================================================
#include <linux/fs.h>

/**
 * @brief 增加inode的链接计数
 * @param inode 目标inode
 * @details 增加文件的硬链接数量
 */
void kfunc_def(inc_nlink)(struct inode *inode) = 0;

/**
 * @brief 减少inode的链接计数
 * @param inode 目标inode
 * @details 减少文件的硬链接数量
 */
void kfunc_def(drop_nlink)(struct inode *inode) = 0;

/**
 * @brief 清零inode的链接计数
 * @param inode 目标inode
 * @details 将文件的硬链接数量设为0
 */
void kfunc_def(clear_nlink)(struct inode *inode) = 0;

/**
 * @brief 设置inode的链接计数
 * @param inode 目标inode
 * @param nlink 新的链接数量
 * @details 设置文件的硬链接数量为指定值
 */
void kfunc_def(set_nlink)(struct inode *inode, unsigned int nlink) = 0;

/**
 * @brief 内核文件读取
 * @param file 文件结构
 * @param buf 读取缓冲区
 * @param count 读取字节数
 * @param pos 文件位置指针
 * @return 实际读取的字节数
 * @details 在内核空间中读取文件内容
 */
ssize_t kfunc_def(kernel_read)(struct file *file, void *buf, size_t count, loff_t *pos) = 0;

/**
 * @brief 内核文件写入
 * @param file 文件结构
 * @param buf 写入缓冲区
 * @param count 写入字节数
 * @param pos 文件位置指针
 * @return 实际写入的字节数
 * @details 在内核空间中写入文件内容
 */
ssize_t kfunc_def(kernel_write)(struct file *file, const void *buf, size_t count, loff_t *pos) = 0;

/**
 * @brief 打开可执行文件
 * @param path 文件路径
 * @return 文件结构指针
 * @details 打开可执行文件，用于exec系统调用
 */
struct file *kfunc_def(open_exec)(const char *) = 0;

/**
 * @brief 根据文件名结构打开文件
 * @param name 文件名结构
 * @param flags 打开标志
 * @param mode 文件模式
 * @return 文件结构指针
 * @details 使用filename结构打开文件
 */
struct file *kfunc_def(file_open_name)(struct filename *, int, umode_t) = 0;

/**
 * @brief 根据路径字符串打开文件
 * @param filename 文件路径字符串
 * @param flags 打开标志
 * @param mode 文件模式
 * @return 文件结构指针
 * @details 根据路径字符串打开文件，是最常用的文件打开函数
 */
struct file *kfunc_def(filp_open)(const char *, int, umode_t) = 0;

/**
 * @brief 相对于根目录打开文件
 * @param dentry 目录项
 * @param mnt 挂载点
 * @param name 文件名
 * @param flags 打开标志
 * @param mode 文件模式
 * @return 文件结构指针
 * @details 相对于指定的根目录和挂载点打开文件
 */
struct file *kfunc_def(file_open_root)(struct dentry *, struct vfsmount *, const char *, int, umode_t) = 0;

/**
 * @brief 根据路径和凭证打开文件
 * @param path 文件路径
 * @param flags 打开标志
 * @param cred 访问凭证
 * @return 文件结构指针
 * @details 使用指定凭证打开文件
 */
struct file *kfunc_def(dentry_open)(const struct path *, int, const struct cred *) = 0;

/**
 * @brief 关闭文件
 * @param file 文件结构
 * @param id 文件所有者ID
 * @return 0成功，负值表示错误
 * @details 关闭已打开的文件并释放相关资源
 */
int kfunc_def(filp_close)(struct file *, fl_owner_t id) = 0;

/**
 * @brief 从用户空间获取文件名
 * @param filename 用户空间文件名指针
 * @return 文件名结构指针
 * @details 安全地从用户空间复制文件名到内核空间
 */
struct filename *kfunc_def(getname)(const char __user *) = 0;

/**
 * @brief 从内核空间获取文件名
 * @param filename 内核空间文件名指针
 * @return 文件名结构指针
 * @details 从内核空间字符串创建文件名结构
 */
struct filename *kfunc_def(getname_kernel)(const char *) = 0;

/**
 * @brief 释放文件名结构
 * @param name 文件名结构
 * @details 减少文件名结构的引用计数
 */
void kfunc_def(putname)(struct filename *name) = 0;

/**
 * @brief 最终释放文件名结构
 * @param name 文件名结构
 * @details 当引用计数为0时最终释放文件名结构
 */
void kfunc_def(final_putname)(struct filename *name) = 0;

/**
 * @brief VFS层文件定位
 * @param file 文件结构
 * @param offset 偏移量
 * @param whence 起始位置
 * @return 新的文件位置
 * @details 改变文件的当前读写位置
 */
loff_t kfunc_def(vfs_llseek)(struct file *file, loff_t offset, int whence) = 0;

/**
 * @brief 文件系统符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配文件系统相关函数的地址
 */
static void _linux_fs_sym_match(const char *name, unsigned long addr)
{
    // kfunc_match(inc_nlink, name, addr);
    // kfunc_match(drop_nlink, name, addr);
    // kfunc_match(clear_nlink, name, addr);
    // kfunc_match(set_nlink, name, addr);
    kfunc_match(kernel_read, name, addr);
    kfunc_match(kernel_write, name, addr);
    // kfunc_match(open_exec, name, addr);
    kfunc_match(file_open_name, name, addr);
    kfunc_match(filp_open, name, addr);
    // kfunc_match(file_open_root, name, addr);
    // kfunc_match(dentry_open, name, addr);
    kfunc_match(filp_close, name, addr);
    // kfunc_match(getname, name, addr);
    // kfunc_match(getname_kernel, name, addr);
    // kfunc_match(putname, name, addr);
    // kfunc_match(final_putname, name, addr);
    kfunc_match(vfs_llseek, name, addr);
}

//=============================================================================
// 堆栈跟踪相关函数（来自kernel/stacktrace.c）
//=============================================================================
#include <linux/stacktrace.h>

/**
 * @brief 保存当前堆栈跟踪
 * @param trace 堆栈跟踪结构
 * @details 获取当前执行点的堆栈跟踪信息
 */
void kfunc_def(save_stack_trace)(struct stack_trace *trace) = 0;

/**
 * @brief 从寄存器保存堆栈跟踪
 * @param regs 寄存器状态
 * @param trace 堆栈跟踪结构
 * @details 从指定的寄存器状态获取堆栈跟踪
 */
void kfunc_def(save_stack_trace_regs)(struct pt_regs *regs, struct stack_trace *trace) = 0;

/**
 * @brief 保存指定进程的堆栈跟踪
 * @param tsk 目标进程
 * @param trace 堆栈跟踪结构
 * @details 获取指定进程的堆栈跟踪信息
 */
void kfunc_def(save_stack_trace_tsk)(struct task_struct *tsk, struct stack_trace *trace) = 0;

/**
 * @brief 打印堆栈跟踪
 * @param trace 堆栈跟踪结构
 * @param spaces 缩进空格数
 * @details 将堆栈跟踪信息打印到内核日志
 */
void kfunc_def(print_stack_trace)(struct stack_trace *trace, int spaces) = 0;

/**
 * @brief 保存用户空间堆栈跟踪
 * @param trace 堆栈跟踪结构
 * @details 获取用户空间的堆栈跟踪信息
 */
void kfunc_def(save_stack_trace_user)(struct stack_trace *trace) = 0;

/**
 * @brief 堆栈跟踪符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配堆栈跟踪相关函数的地址
 */
static void _linux_stacktrace_sym_match(const char *name, unsigned long addr)
{
    // kfunc_match(save_stack_trace, name, addr);
    // kfunc_match(save_stack_trace_regs, name, addr);
    kfunc_match(save_stack_trace_tsk, name, addr);
    // kfunc_match(print_stack_trace, name, addr);
    // kfunc_match(save_stack_trace_user, name, addr);
}

//=============================================================================
// SELinux访问控制相关函数（来自security/selinux/avc.c）
//=============================================================================
#include <security/selinux/include/avc.h>

/**
 * @brief 检查访问是否被拒绝
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param requested 请求的权限
 * @param driver 驱动类型
 * @param xperm 扩展权限
 * @param flags 标志
 * @param avd 访问决策
 * @return 1表示拒绝，0表示允许
 * @details 检查指定的访问请求是否被SELinux策略拒绝
 */
int kfunc_def(avc_denied)(u32 ssid, u32 tsid, u16 tclass, u32 requested, u8 driver, u8 xperm, unsigned int flags,
                          struct av_decision *avd) = 0;

/**
 * @brief 慢速AVC审计
 * @param state SELinux状态
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param requested 请求的权限
 * @param audited 审计的权限
 * @param denied 拒绝的权限
 * @param result 访问结果
 * @param a 审计数据
 * @return 审计结果
 * @details 执行详细的SELinux访问审计
 */
int kfunc_def(slow_avc_audit)(struct selinux_state *state, u32 ssid, u32 tsid, u16 tclass, u32 requested, u32 audited,
                              u32 denied, int result, struct common_audit_data *a) = 0;

/**
 * @brief 检查权限（无审计）
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param requested 请求的权限
 * @param flags 标志
 * @param avd 访问决策
 * @return 0成功，负值表示拒绝
 * @details 检查SELinux权限但不生成审计记录
 */
int kfunc_def(avc_has_perm_noaudit)(u32 ssid, u32 tsid, u16 tclass, u32 requested, unsigned flags,
                                    struct av_decision *avd) = 0;

/**
 * @brief 检查SELinux权限
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param requested 请求的权限
 * @param auditdata 审计数据
 * @return 0成功，负值表示拒绝
 * @details 检查SELinux权限并生成适当的审计记录
 */
int kfunc_def(avc_has_perm)(u32 ssid, u32 tsid, u16 tclass, u32 requested, struct common_audit_data *auditdata) = 0;

/**
 * @brief 带标志的权限检查
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param requested 请求的权限
 * @param auditdata 审计数据
 * @param flags 标志
 * @return 0成功，负值表示拒绝
 * @details 带有额外标志的SELinux权限检查
 */
int kfunc_def(avc_has_perm_flags)(u32 ssid, u32 tsid, u16 tclass, u32 requested, struct common_audit_data *auditdata,
                                  int flags) = 0;

/**
 * @brief 检查扩展权限
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param requested 请求的权限
 * @param driver 驱动类型
 * @param perm 具体权限
 * @param ad 审计数据
 * @return 0成功，负值表示拒绝
 * @details 检查SELinux扩展权限
 */
int kfunc_def(avc_has_extended_perms)(u32 ssid, u32 tsid, u16 tclass, u32 requested, u8 driver, u8 perm,
                                      struct common_audit_data *ad) = 0;

/**
 * @brief 查找AVC节点
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @return AVC节点指针
 * @details 在访问向量缓存中查找对应的节点
 */
struct avc_node *kfunc_def(avc_lookup)(u32 ssid, u32 tsid, u16 tclass) = 0;

/**
 * @brief 计算访问向量
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param avd 访问决策
 * @param xp_node 扩展权限节点
 * @return AVC节点指针
 * @details 计算并缓存访问向量决策
 */
struct avc_node *kfunc_def(avc_compute_av)(u32 ssid, u32 tsid, u16 tclass, struct av_decision *avd,
                                           struct avc_xperms_node *xp_node) = 0;

/**
 * @brief SELinux AVC符号匹配函数
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配SELinux访问控制相关函数的地址
 */
static void _linux_security_selinux_avc_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(avc_denied, name, addr);
    kfunc_match(slow_avc_audit, name, addr);

    // kfunc_match(avc_has_perm_noaudit, name, addr);
    // kfunc_match(avc_has_perm, name, addr);
    // kfunc_match(avc_has_perm_flags, name, addr);
    // kfunc_match(avc_has_extended_perms, name, addr);
    // kfunc_match(avc_lookup, name, addr);
    // kfunc_match(avc_compute_av, name, addr);
}

//=============================================================================
// SELinux安全策略相关函数（来自security/selinux/ss/）
//=============================================================================
#include <security/selinux/include/security.h>
#include <security/selinux/include/classmap.h>

/**
 * @brief SELinux启动时启用状态
 * @details 指示SELinux在系统启动时是否被启用
 */
int kvar_def(selinux_enabled_boot) = 0;

/**
 * @brief SELinux当前启用状态
 * @details 指示SELinux当前是否处于启用状态
 */
int kvar_def(selinux_enabled) = 0;

/**
 * @brief SELinux全局状态结构
 * @details 包含SELinux的全局状态信息
 */
struct selinux_state kvar_def(selinux_state) = 0;

/**
 * @brief 安全类别映射表
 * @details SELinux安全类别到权限的映射表
 */
struct security_class_mapping kvar_def(secclass_map)[] = 0;

/**
 * @brief 检查MLS是否启用
 * @return 1表示启用，0表示未启用
 * @details 检查多级安全(MLS)是否在当前策略中启用
 */
int kfunc_def(security_mls_enabled)(void) = 0;

/**
 * @brief 加载安全策略
 * @param data 策略数据
 * @param len 数据长度
 * @param load_state 加载状态
 * @return 0成功，负值表示错误
 * @details 加载新的SELinux安全策略
 */
int kfunc_def(security_load_policy)(void *data, size_t len, struct selinux_load_state *load_state) = 0;

/**
 * @brief 提交策略加载
 * @param load_state 加载状态
 * @details 提交之前加载的安全策略使其生效
 */
void kfunc_def(selinux_policy_commit)(struct selinux_load_state *load_state) = 0;

/**
 * @brief 取消策略加载
 * @param load_state 加载状态
 * @details 取消之前的策略加载操作
 */
void kfunc_def(selinux_policy_cancel)(struct selinux_load_state *load_state) = 0;

/**
 * @brief 读取安全策略
 * @param data 输出数据指针
 * @param len 输出长度指针
 * @return 0成功，负值表示错误
 * @details 读取当前的SELinux安全策略
 */
int kfunc_def(security_read_policy)(void **data, size_t *len) = 0;

/**
 * @brief 读取内核安全状态
 * @param data 输出数据指针
 * @param len 输出长度指针
 * @return 0成功，负值表示错误
 * @details 读取内核的安全状态信息
 */
int kfunc_def(security_read_state_kernel)(void **data, size_t *len) = 0;

/**
 * @brief 检查策略能力是否支持
 * @param req_cap 请求的能力
 * @return 1表示支持，0表示不支持
 * @details 检查当前策略是否支持指定的能力
 */
int kfunc_def(security_policycap_supported)(unsigned int req_cap) = 0;

/**
 * @brief 计算访问向量
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param avd 访问决策
 * @param xperms 扩展权限
 * @details 根据安全策略计算访问权限
 */
void kfunc_def(security_compute_av)(u32 ssid, u32 tsid, u16 tclass, struct av_decision *avd,
                                    struct extended_perms *xperms) = 0;

/**
 * @brief 计算扩展权限决策
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param driver 驱动类型
 * @param xpermd 扩展权限决策
 * @details 计算扩展权限的具体决策
 */
void kfunc_def(security_compute_xperms_decision)(u32 ssid, u32 tsid, u16 tclass, u8 driver,
                                                 struct extended_perms_decision *xpermd) = 0;

/**
 * @brief 为用户计算访问向量
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param avd 访问决策
 * @details 为用户空间请求计算访问权限
 */
void kfunc_def(security_compute_av_user)(u32 ssid, u32 tsid, u16 tclass, struct av_decision *avd) = 0;

/**
 * @brief 安全ID转换
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param qstr 对象名称
 * @param out_sid 输出安全ID
 * @return 0成功，负值表示错误
 * @details 计算对象创建时的安全ID
 */
int kfunc_def(security_transition_sid)(u32 ssid, u32 tsid, u16 tclass, const struct qstr *qstr, u32 *out_sid) = 0;

/**
 * @brief 用户安全ID转换
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param objname 对象名称
 * @param out_sid 输出安全ID
 * @return 0成功，负值表示错误
 * @details 为用户空间请求计算安全ID转换
 */
int kfunc_def(security_transition_sid_user)(u32 ssid, u32 tsid, u16 tclass, const char *objname, u32 *out_sid) = 0;

/**
 * @brief 计算成员安全ID
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param out_sid 输出安全ID
 * @return 0成功，负值表示错误
 * @details 计算组成员的安全ID
 */
int kfunc_def(security_member_sid)(u32 ssid, u32 tsid, u16 tclass, u32 *out_sid) = 0;

/**
 * @brief 计算变更后的安全ID
 * @param ssid 源安全ID
 * @param tsid 目标安全ID
 * @param tclass 目标类别
 * @param out_sid 输出安全ID
 * @return 0成功，负值表示错误
 * @details 计算安全上下文变更后的安全ID
 */
int kfunc_def(security_change_sid)(u32 ssid, u32 tsid, u16 tclass, u32 *out_sid) = 0;

/**
 * @brief 安全ID转安全上下文
 * @param sid 安全ID
 * @param scontext 输出安全上下文
 * @param scontext_len 上下文长度
 * @return 0成功，负值表示错误
 * @details 将安全ID转换为安全上下文字符串
 */
int kfunc_def(security_sid_to_context)(u32 sid, char **scontext, u32 *scontext_len) = 0;

/**
 * @brief 强制安全ID转上下文
 * @param sid 安全ID
 * @param scontext 输出安全上下文
 * @param scontext_len 上下文长度
 * @return 0成功，负值表示错误
 * @details 强制转换安全ID到上下文，即使策略中不存在
 */
int kfunc_def(security_sid_to_context_force)(u32 sid, char **scontext, u32 *scontext_len) = 0;

/**
 * @brief 无效安全ID转上下文
 * @param sid 安全ID
 * @param scontext 输出安全上下文
 * @param scontext_len 上下文长度
 * @return 0成功，负值表示错误
 * @details 转换可能无效的安全ID到上下文
 */
int kfunc_def(security_sid_to_context_inval)(u32 sid, char **scontext, u32 *scontext_len) = 0;

/**
 * @brief 安全上下文转安全ID
 * @param scontext 安全上下文字符串
 * @param scontext_len 上下文长度
 * @param out_sid 输出安全ID
 * @param gfp 内存分配标志
 * @return 0成功，负值表示错误
 * @details 将安全上下文字符串转换为安全ID
 */
int kfunc_def(security_context_to_sid)(const char *scontext, u32 scontext_len, u32 *out_sid, gfp_t gfp) = 0;

/**
 * @brief 安全上下文字符串转安全ID
 * @param scontext 安全上下文字符串
 * @param out_sid 输出安全ID
 * @param gfp 内存分配标志
 * @return 0成功，负值表示错误
 * @details 将以空字符结尾的安全上下文转换为安全ID
 */
int kfunc_def(security_context_str_to_sid)(const char *scontext, u32 *out_sid, gfp_t gfp) = 0;
int kfunc_def(security_context_to_sid_default)(const char *scontext, u32 scontext_len, u32 *out_sid, u32 def_sid,
                                               gfp_t gfp_flags) = 0;
int kfunc_def(security_context_to_sid_force)(const char *scontext, u32 scontext_len, u32 *sid) = 0;
int kfunc_def(security_get_user_sids)(u32 callsid, char *username, u32 **sids, u32 *nel) = 0;
int kfunc_def(security_port_sid)(u8 protocol, u16 port, u32 *out_sid) = 0;
int kfunc_def(security_ib_pkey_sid)(u64 subnet_prefix, u16 pkey_num, u32 *out_sid) = 0;
int kfunc_def(security_ib_endport_sid)(const char *dev_name, u8 port_num, u32 *out_sid) = 0;
int kfunc_def(security_netif_sid)(char *name, u32 *if_sid) = 0;
int kfunc_def(security_node_sid)(u16 domain, void *addr, u32 addrlen, u32 *out_sid) = 0;
int kfunc_def(security_validate_transition)(u32 oldsid, u32 newsid, u32 tasksid, u16 tclass) = 0;
int kfunc_def(security_validate_transition_user)(u32 oldsid, u32 newsid, u32 tasksid, u16 tclass) = 0;
int kfunc_def(security_bounded_transition)(u32 oldsid, u32 newsid) = 0;
int kfunc_def(security_sid_mls_copy)(u32 sid, u32 mls_sid, u32 *new_sid) = 0;
int kfunc_def(security_net_peersid_resolve)(u32 nlbl_sid, u32 nlbl_type, u32 xfrm_sid, u32 *peer_sid) = 0;
int kfunc_def(security_get_classes)(struct selinux_policy *policy, char ***classes, int *nclasses) = 0;
int kfunc_def(security_get_permissions)(struct selinux_policy *policy, char *class, char ***perms, int *nperms) = 0;
int kfunc_def(security_get_reject_unknown)(void) = 0;
int kfunc_def(security_get_allow_unknown)(void) = 0;

int kfunc_def(security_fs_use)(struct super_block *sb) = 0;
int kfunc_def(security_genfs_sid)(const char *fstype, const char *path, u16 sclass, u32 *sid) = 0;
int kfunc_def(selinux_policy_genfs_sid)(struct selinux_policy *policy, const char *fstype, const char *path, u16 sclass,
                                        u32 *sid) = 0;
int kfunc_def(security_netlbl_secattr_to_sid)(struct netlbl_lsm_secattr *secattr, u32 *sid) = 0;
int kfunc_def(security_netlbl_sid_to_secattr)(u32 sid, struct netlbl_lsm_secattr *secattr) = 0;
const char *kfunc_def(security_get_initial_sid_context)(u32 sid) = 0;

void kfunc_def(selinux_status_update_setenforce)(int enforcing) = 0;
void kfunc_def(selinux_status_update_policyload)(int seqno) = 0;
void kfunc_def(selinux_complete_init)(void) = 0;
void kfunc_def(exit_sel_fs)(void) = 0;
void kfunc_def(selnl_notify_setenforce)(int val) = 0;
void kfunc_def(selnl_notify_policyload)(u32 seqno) = 0;
int kfunc_def(selinux_nlmsg_lookup)(u16 sclass, u16 nlmsg_type, u32 *perm) = 0;

void kfunc_def(avtab_cache_init)(void) = 0;
void kfunc_def(ebitmap_cache_init)(void) = 0;
void kfunc_def(hashtab_cache_init)(void) = 0;
int kfunc_def(security_sidtab_hash_stats)(char *page) = 0;

static void _linux_security_selinux_sym_match(const char *name, unsigned long addr)
{
    // kvar_match(selinux_enabled_boot, name, addr);
    // kvar_match(selinux_enabled, name, addr);
    // kvar_match(selinux_state, name, addr);
    // kvar_match(secclass_map, name, addr);
    // kfunc_match(security_mls_enabled, name, addr);
    // kfunc_match(security_load_policy, name, addr);
    // kfunc_match(selinux_policy_commit, name, addr);
    // kfunc_match(selinux_policy_cancel, name, addr);
    // kfunc_match(security_read_policy, name, addr);
    // kfunc_match(security_read_state_kernel, name, addr);
    // kfunc_match(security_policycap_supported, name, addr);
    // kfunc_match(security_compute_av, name, addr);
    // kfunc_match(security_compute_xperms_decision, name, addr);
    // kfunc_match(security_compute_av_user, name, addr);
    // kfunc_match(security_transition_sid, name, addr);
    // kfunc_match(security_transition_sid_user, name, addr);
    // kfunc_match(security_member_sid, name, addr);
    // kfunc_match(security_change_sid, name, addr);
    // kfunc_match(security_sid_to_context, name, addr);
    // kfunc_match(security_sid_to_context_force, name, addr);
    // kfunc_match(security_sid_to_context_inval, name, addr);
    // kfunc_match(security_context_to_sid, name, addr);
    // kfunc_match(security_context_str_to_sid, name, addr);
    // kfunc_match(security_context_to_sid_default, name, addr);
    // kfunc_match(security_context_to_sid_force, name, addr);
    // kfunc_match(security_get_user_sids, name, addr);
    // kfunc_match(security_port_sid, name, addr);
    // kfunc_match(security_ib_pkey_sid, name, addr);
    // kfunc_match(security_ib_endport_sid, name, addr);
    // kfunc_match(security_netif_sid, name, addr);
    // kfunc_match(security_node_sid, name, addr);
    // kfunc_match(security_validate_transition, name, addr);
    // kfunc_match(security_validate_transition_user, name, addr);
    // kfunc_match(security_bounded_transition, name, addr);
    // kfunc_match(security_sid_mls_copy, name, addr);
    // kfunc_match(security_net_peersid_resolve, name, addr);
    // kfunc_match(security_get_classes, name, addr);
    // kfunc_match(security_get_permissions, name, addr);
    // kfunc_match(security_get_reject_unknown, name, addr);
    // kfunc_match(security_get_allow_unknown, name, addr);

    // kfunc_match(security_fs_use, name, addr);
    // kfunc_match(security_genfs_sid, name, addr);
    // kfunc_match(selinux_policy_genfs_sid, name, addr);
    // kfunc_match(security_netlbl_secattr_to_sid, name, addr);
    // kfunc_match(security_netlbl_sid_to_secattr, name, addr);
    // kfunc_match(security_get_initial_sid_context, name, addr);

    // kfunc_match(selinux_status_update_setenforce, name, addr);
    // kfunc_match(selinux_status_update_policyload, name, addr);
    // kfunc_match(selinux_complete_init, name, addr);
    // kfunc_match(exit_sel_fs, name, addr);
    // kfunc_match(selnl_notify_setenforce, name, addr);
    // kfunc_match(selnl_notify_policyload, name, addr);
    // kfunc_match(selinux_nlmsg_lookup, name, addr);

    // kfunc_match(avtab_cache_init, name, addr);
    // kfunc_match(ebitmap_cache_init, name, addr);
    // kfunc_match(hashtab_cache_init, name, addr);
    // kfunc_match(security_sidtab_hash_stats, name, addr);
}

#include <linux/security.h>

int kfunc_def(cap_capable)(const struct cred *cred, struct user_namespace *ns, int cap, unsigned int opts) = 0;
int kfunc_def(cap_settime)(const struct timespec64 *ts, const struct timezone *tz) = 0;
int kfunc_def(cap_ptrace_access_check)(struct task_struct *child, unsigned int mode) = 0;
int kfunc_def(cap_ptrace_traceme)(struct task_struct *parent) = 0;
int kfunc_def(cap_capget)(struct task_struct *target, kernel_cap_t *effective, kernel_cap_t *inheritable,
                          kernel_cap_t *permitted) = 0;
int kfunc_def(cap_capset)(struct cred *new, const struct cred *old, const kernel_cap_t *effective,
                          const kernel_cap_t *inheritable, const kernel_cap_t *permitted) = 0;
int kfunc_def(cap_bprm_creds_from_file)(struct linux_binprm *bprm, struct file *file) = 0;
int kfunc_def(cap_inode_setxattr)(struct dentry *dentry, const char *name, const void *value, size_t size,
                                  int flags) = 0;
int kfunc_def(cap_inode_removexattr)(struct dentry *dentry, const char *name) = 0;
int kfunc_def(cap_inode_need_killpriv)(struct dentry *dentry) = 0;
int kfunc_def(cap_inode_killpriv)(struct dentry *dentry) = 0;
int kfunc_def(cap_inode_getsecurity)(struct inode *inode, const char *name, void **buffer, bool alloc) = 0;
int kfunc_def(cap_mmap_addr)(unsigned long addr) = 0;
int kfunc_def(cap_mmap_file)(struct file *file, unsigned long reqprot, unsigned long prot, unsigned long flags) = 0;
int kfunc_def(cap_task_fix_setuid)(struct cred *new, const struct cred *old, int flags) = 0;
int kfunc_def(cap_task_prctl)(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4,
                              unsigned long arg5) = 0;
int kfunc_def(cap_task_setscheduler)(struct task_struct *p) = 0;
int kfunc_def(cap_task_setioprio)(struct task_struct *p, int ioprio) = 0;
int kfunc_def(cap_task_setnice)(struct task_struct *p, int nice) = 0;
int kfunc_def(cap_vm_enough_memory)(struct mm_struct *mm, long pages) = 0;
// int kfunc_def(security_secid_to_secctx)(u32 secid, char **secdata, u32 *seclen) = 0;
int kfunc_def(security_secctx_to_secid)(const char *secdata, u32 seclen, u32 *secid) = 0;

kernel_cap_t full_cap = { 0 };

static void _linux_security_commoncap_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(cap_capable, name, addr);
    // kfunc_match(cap_settime, name, addr);
    // kfunc_match(cap_ptrace_access_check, name, addr);
    // kfunc_match(cap_ptrace_traceme, name, addr);
    kfunc_match(cap_capget, name, addr);
    kfunc_match(cap_capset, name, addr);
    // kfunc_match(cap_bprm_creds_from_file, name, addr);
    // kfunc_match(cap_inode_setxattr, name, addr);
    // kfunc_match(cap_inode_removexattr, name, addr);
    // kfunc_match(cap_inode_need_killpriv, name, addr);
    // kfunc_match(cap_inode_killpriv, name, addr);
    // kfunc_match(cap_inode_getsecurity, name, addr);
    // kfunc_match(cap_mmap_addr, name, addr);
    // kfunc_match(cap_mmap_file, name, addr);
    // kfunc_match(cap_task_fix_setuid, name, addr);
    kfunc_match(cap_task_prctl, name, addr);
    // kfunc_match(cap_task_setscheduler, name, addr);
    // kfunc_match(cap_task_setioprio, name, addr);
    // kfunc_match(cap_task_setnice, name, addr);
    // kfunc_match(security_secid_to_secctx, name, addr);
    kfunc_match(security_secctx_to_secid, name, addr);
}

#include <linux/seccomp.h>

long kfunc_def(prctl_get_seccomp)(void) = 0;
long kfunc_def(prctl_set_seccomp)(unsigned long seccomp_mode, char __user *filter) = 0;

void kfunc_def(put_seccomp_filter)(struct task_struct *tsk) = 0;
void kfunc_def(get_seccomp_filter)(struct task_struct *tsk) = 0;

void kfunc_def(seccomp_filter_release)(struct task_struct *tsk) = 0;

static void _linux_seccomp_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(prctl_get_seccomp, name, addr);
    // kfunc_match(prctl_set_seccomp, name, addr);
    // kfunc_match(put_seccomp_filter, name, addr);
    // kfunc_match(get_seccomp_filter, name, addr);
    // kfunc_match(seccomp_filter_release, name, addr);
}

#include <linux/panic.h>
#include <linux/umh.h>

void kfunc_def(panic)(const char *fmt, ...) __noreturn __cold = 0;
int kfunc_def(call_usermodehelper)(const char *path, char **argv, char **envp, int wait) = 0;

// /drivers/char/random.c
void kfunc_def(get_random_bytes)(void *buf, int nbytes) = 0;
uint64_t kfunc_def(get_random_u64)(void) = 0;
uint64_t kfunc_def(get_random_long)(void) = 0;

static void _linux_misc_misc(const char *name, unsigned long addr)
{
    kfunc_match(panic, name, addr);
    // kfunc_match(call_usermodehelper, name, addr);
    // kfunc_match(get_random_bytes, name, addr);
    // kfunc_match(get_random_u64, name, addr);
    // kfunc_match(get_random_long, name, addr);
}

// linux/bottom_half.h
struct rcu_gp_oldstate;
void kfunc_def(__local_bh_disable_ip)(unsigned long ip, unsigned int cnt) = 0;
void kfunc_def(__local_bh_enable_ip)(unsigned long ip, unsigned int cnt) = 0;
void kfunc_def(_local_bh_enable)(void) = 0;
bool kfunc_def(local_bh_blocked)(void) = 0;

void kfunc_def(call_rcu)(struct rcu_head *head, rcu_callback_t func);
void kfunc_def(rcu_barrier_tasks)(void);
void kfunc_def(rcu_barrier_tasks_rude)(void);
void kfunc_def(synchronize_rcu)(void);
unsigned long kfunc_def(get_completed_synchronize_rcu)(void);
void kfunc_def(get_completed_synchronize_rcu_full)(struct rcu_gp_oldstate *rgosp);

void kfunc_def(__rcu_read_lock)(void);
void kfunc_def(__rcu_read_unlock)(void);
void kfunc_def(rcu_read_unlock_strict)(void);

// linux/rcupdate
void kfunc_def(rcu_init)(void) = 0;
void kfunc_def(rcu_sched_clock_irq)(int user) = 0;
void kfunc_def(rcu_report_dead)(unsigned int cpu) = 0;
void kfunc_def(rcutree_migrate_callbacks)(int cpu) = 0;

void kfunc_def(rcu_init_tasks_generic)(void) = 0;

void kfunc_def(rcu_sysrq_start)(void) = 0;
void kfunc_def(rcu_sysrq_end)(void) = 0;
void kfunc_def(rcu_irq_work_resched)(void) = 0;

int kfunc_def(rcu_read_lock_held)(void) = 0;
int kfunc_def(rcu_read_lock_bh_held)(void) = 0;
int kfunc_def(rcu_read_lock_sched_held)(void) = 0;
int kfunc_def(rcu_read_lock_any_held)(void) = 0;

void kfunc_def(rcu_init_nohz)(void) = 0;
int kfunc_def(rcu_nocb_cpu_offload)(int cpu) = 0;
int kfunc_def(rcu_nocb_cpu_deoffload)(int cpu) = 0;
void kfunc_def(rcu_nocb_flush_deferred_wakeup)(void) = 0;

void kfunc_def(exit_tasks_rcu_start)(void) = 0;
void kfunc_def(exit_tasks_rcu_stop)(void) = 0;
void kfunc_def(exit_tasks_rcu_finish)(void) = 0;

static void _linux_rcu_symbol_init(const char *name, unsigned long addr)
{
    // kfunc_match(__local_bh_disable_ip, name, addr);
    // kfunc_match(__local_bh_enable_ip, name, addr);
    kfunc_match(_local_bh_enable, name, addr);
    kfunc_match(local_bh_blocked, name, addr);

    kfunc_match(call_rcu, name, addr);
    // kfunc_match(rcu_barrier_tasks, name, addr);
    // kfunc_match(rcu_barrier_tasks_rude, name, addr);
    kfunc_match(synchronize_rcu, name, addr);
    // kfunc_match(get_completed_synchronize_rcu, name, addr);
    // kfunc_match(get_completed_synchronize_rcu_full, name, addr);

    kfunc_match(__rcu_read_lock, name, addr);
    kfunc_match(__rcu_read_unlock, name, addr);
    // kfunc_match(rcu_read_unlock_strict, name, addr);

    // kfunc_match(rcu_init, name, addr);
    // kfunc_match(rcu_sched_clock_irq, name, addr);
    // kfunc_match(rcu_report_dead, name, addr);
    // kfunc_match(rcutree_migrate_callbacks, name, addr);

    // kfunc_match(rcu_init_tasks_generic, name, addr);

    // kfunc_match(rcu_sysrq_start, name, addr);
    // kfunc_match(rcu_sysrq_end, name, addr);
    // kfunc_match(rcu_irq_work_resched, name, addr);

    // kfunc_match(rcu_read_lock_held, name, addr);
    // kfunc_match(rcu_read_lock_bh_held, name, addr);
    // kfunc_match(rcu_read_lock_sched_held, name, addr);
    // kfunc_match(rcu_read_lock_any_held, name, addr);

    // kfunc_match(rcu_init_nohz, name, addr);
    // kfunc_match(rcu_nocb_cpu_offload, name, addr);
    // kfunc_match(rcu_nocb_cpu_deoffload, name, addr);
    // kfunc_match(rcu_nocb_flush_deferred_wakeup, name, addr);

    // kfunc_match(exit_tasks_rcu_start, name, addr);
    // kfunc_match(exit_tasks_rcu_stop, name, addr);
    // kfunc_match(exit_tasks_rcu_finish, name, addr);
}

void kfunc_def(mmput)(struct mm_struct *);
void kfunc_def(mmput_async)(struct mm_struct *);
struct mm_struct *kfunc_def(get_task_mm)(struct task_struct *task);

static void _linux_sched_mm_init(const char *name, unsigned long addr)
{
    kfunc_match(mmput, name, addr);
    kfunc_match(mmput_async, name, addr);
    kfunc_match(get_task_mm, name, addr);
}

static int _linux_misc_symbol_init(void *data, const char *name, struct module *m, unsigned long addr)
{
    _linux_kernel_cred_sym_match(name, addr);
    _linux_kernel_pid_sym_match(name, addr);
    _linux_kernel_stop_machine_sym_match(name, addr);
    _linux_mm_utils_sym_match(name, addr);
    _linux_mm_vmalloc_sym_match(name, addr);
    _linux_fs_sym_match(name, addr);
    _linux_locking_spinlock_sym_match(name, addr);
    _linux_stacktrace_sym_match(name, addr);
    _linux_security_selinux_sym_match(name, addr);
    _linux_security_commoncap_sym_match(name, addr);
    _linux_misc_misc(name, addr);
    _linux_security_selinux_avc_sym_match(name, addr);
    _linux_kernel_fork_sym_match(name, addr);
    _linux_rcu_symbol_init(name, addr);
    _linux_seccomp_sym_match(name, addr);
    _linux_sched_mm_init(name, addr);
    return 0;
}

void linux_misc_symbol_init()
{
#ifdef INIT_USE_KALLSYMS_LOOKUP_NAME
    _linux_misc_symbol_init(0, 0, 0, 0);
#else
    kallsyms_on_each_symbol(_linux_misc_symbol_init, 0);
#endif
}
