/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 通用工具函数模块 - 提供内核与用户空间数据传输的兼容性接口

#include <kputils.h>
#include <linux/seq_buf.h>
#include <linux/trace_seq.h>
#include <pgtable.h>
#include <linux/string.h>
#include <symbol.h>
#include <asm/processor.h>
#include <predata.h>
#include <linux/ptrace.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/cred.h>

extern int kfunc_def(xt_data_to_user)(void __user *dst, const void *src, int usersize, int size, int aligned_size);

/**
 * xt_data_to_user包装函数 - 通过xtables数据传输接口复制数据到用户空间
 * @param dst 用户空间目标地址
 * @param src 内核空间源地址
 * @param size 数据大小
 * @return 实际复制的字节数
 */
static inline int compat_xt_data_copy_to_user(void __user *dst, const void *src, int size)
{
    kfunc_direct_call(xt_data_to_user, dst, src, size, size, size);
}

extern int kfunc_def(bits_to_user)(unsigned long *bits, unsigned int maxbit, unsigned int maxlen, void __user *p,
                                   int compat);

/**
 * bits_to_user包装函数 - 通过位图传输接口复制数据到用户空间
 * @param dst 用户空间目标地址
 * @param src 内核空间源地址
 * @param size 数据大小
 * @return 实际复制的字节数
 */
static inline int compat_bits_copy_to_user(void __user *dst, const void *src, int size)
{
    kfunc_direct_call(bits_to_user, (unsigned long *)src, size * sizeof(unsigned long), size, dst, 0);
}

/**
 * trace_seq数据传输函数 - 通过trace序列接口复制数据到用户空间
 * @param to 用户空间目标地址
 * @param from 内核空间源地址
 * @param n 数据大小
 * @return 实际复制的字节数
 */
__noinline int trace_seq_copy_to_user(void __user *to, const void *from, int n)
{
    // 限制：数据大小不能超过页面大小
    if (n > page_size) return 0;

    unsigned char trace_seq_data[page_size + 0x20];
    struct trace_seq *trace_seq = (struct trace_seq *)trace_seq_data;
    int *fp = (int *)(((uintptr_t)trace_seq) + page_size);
    int *plen = fp;          // 数据长度指针
    int *preadpos = fp + 1;  // 读取位置指针
    int *pfull = fp + 2;     // 缓冲区满标志指针
    *plen = n;
    *preadpos = 0;
    *pfull = 0;

    memcpy((void *)trace_seq, from, n);  // 复制数据到trace_seq缓冲区
    int sz = kfunc(trace_seq_to_user)(trace_seq, to, n);
    return sz;
}

/**
 * seq_buf数据传输函数 - 通过序列缓冲区接口复制数据到用户空间
 * @param to 用户空间目标地址
 * @param from 内核空间源地址
 * @param n 数据大小
 * @return 实际复制的字节数
 */
int seq_buf_copy_to_user(void __user *to, const void *from, int n)
{
    struct seq_buf seq_buf;
    seq_buf.size = n;        // 缓冲区大小
    seq_buf.len = n;         // 数据长度
    seq_buf.readpos = 0;     // 读取位置
    seq_buf.buffer = (void *)from;  // 数据缓冲区
    return kfunc(seq_buf_to_user)(&seq_buf, to, n);
}

/**
 * 兼容性用户空间数据复制函数 - 自动选择可用的传输方法
 * @param to 用户空间目标地址
 * @param from 内核空间源地址
 * @param n 数据大小
 * @return 实际复制的字节数
 */
int __must_check compat_copy_to_user(void __user *to, const void *from, int n)
{
    int cplen = 0;

    // 按优先级尝试不同的传输方法
    if (kfunc(seq_buf_to_user)) {
        cplen = seq_buf_copy_to_user(to, from, n);
    } else if (kfunc(xt_data_to_user)) {
        // xt_data_to_user, xt_obj_to_user
        cplen = compat_xt_data_copy_to_user(to, from, n);
        if (!cplen) cplen = n;
    } else if (kfunc(bits_to_user)) {
        // bits_to_user, str_to_user
        cplen = compat_bits_copy_to_user(to, from, n);
    } else if (kfunc(trace_seq_to_user)) {
        cplen = trace_seq_copy_to_user(to, from, n);
    } else {
        logke("no compat_copy_to_user\n");
        // copy_arg_to_user,
    }
    return cplen;
}
KP_EXPORT_SYMBOL(compat_copy_to_user);

#include <linux/uaccess.h>

/**
 * 兼容性用户空间字符串复制函数 - 从用户空间复制字符串到内核空间
 * @param dest 内核空间目标缓冲区
 * @param src 用户空间源字符串
 * @param count 最大复制长度
 * @return 实际复制的字符数，失败返回负数
 */
long compat_strncpy_from_user(char *dest, const char __user *src, long count)
{
    // 尝试使用标准的strncpy_from_user函数
    if (kfunc(strncpy_from_user)) {
        long rc = kfunc(strncpy_from_user)(dest, src, count);
        if (rc >= count) {
            rc = count;
            dest[rc - 1] = '\0';  // 确保字符串以null结尾
        } else if (rc > 0) {
            rc++;
        }
        return rc;
    }
    // 备选方案：使用无故障复制函数
    kfunc_call(strncpy_from_user_nofault, dest, src, count);
    kfunc_call(strncpy_from_unsafe_user, dest, src, count);
    return 0;
}
KP_EXPORT_SYMBOL(compat_strncpy_from_user);

int16_t pt_regs_offset = -1;  // pt_regs结构体在栈中的偏移量

/**
 * 获取任务的寄存器状态结构体
 * @param task 目标任务结构体
 * @return 指向pt_regs结构体的指针
 */
struct pt_regs *_task_pt_reg(struct task_struct *task)
{
    unsigned long stack = (unsigned long)task_stack_page(task);  // 获取任务栈页面
    uintptr_t addr = (uintptr_t)(thread_size + stack);           // 计算栈顶地址
    
    // 根据已知偏移量或内核版本计算pt_regs位置
    if (pt_regs_offset > 0) {
        addr -= pt_regs_offset;  // 使用预计算的偏移量
    } else {
#ifndef ANDROID
        // 根据内核版本选择不同的pt_regs结构体大小
        if (kver < VERSION(4, 4, 19)) {
            addr -= sizeof(struct pt_regs_lt4419); // 0x120
        } else if (kver < VERSION(4, 14, 0)) {
            addr -= sizeof(struct pt_regs_lt4140); // 0x130
        } else
#endif
        if (kver < VERSION(5, 10, 0)) {
            addr -= sizeof(struct pt_regs_lt5100); // 0x140
        } else {
            addr -= sizeof(struct pt_regs); // 0x150
        }
    }

    return (struct pt_regs *)(addr);
}
KP_EXPORT_SYMBOL(_task_pt_reg);

/**
 * 复制数据到用户栈空间
 * @param data 要复制的数据
 * @param len 数据长度
 * @return 用户空间地址，失败返回错误码
 */
void *__user __must_check copy_to_user_stack(const void *data, int len)
{
    uintptr_t addr = current_user_stack_pointer();  // 获取当前用户栈指针
    addr -= len;                                    // 在栈上分配空间
    addr &= 0xFFFFFFFFFFFFFFF8;                     // 8字节对齐
    int cplen = compat_copy_to_user((void *)addr, data, len);  // 复制数据到用户空间
    return cplen > 0 ? (void *__user)addr : (void *)(long)cplen;
}
KP_EXPORT_SYMBOL(copy_to_user_stack);

/**
 * 获取64位随机数
 * @return 64位随机数值
 */
uint64_t get_random_u64(void)
{
    // 尝试使用内核提供的随机数函数
    kfunc_call(get_random_u64);
    kfunc_call(get_random_long);
    return rand_next();  // 备用随机数生成器
}
KP_EXPORT_SYMBOL(get_random_u64);

// todo: rcu_dereference_protected
uid_t current_uid()
{
    struct cred *cred = *(struct cred **)((uintptr_t)current + task_struct_offset.cred_offset);
    uid_t uid = *(uid_t *)((uintptr_t)cred + cred_offset.uid_offset);
    return uid;
}
KP_EXPORT_SYMBOL(current_uid);