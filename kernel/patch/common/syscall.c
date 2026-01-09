/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 系统调用hook管理模块 - 提供系统调用表发现、hook拦截和管理功能

#include "syscall.h"

#include <cache.h>
#include <ktypes.h>
#include <hook.h>
#include <common.h>
#include <linux/string.h>
#include <symbol.h>
#include <uapi/asm-generic/errno.h>
#include <asm-generic/compat.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <uapi/asm-generic/errno.h>
#include <predata.h>
#include <kputils.h>
#include <linux/kernel.h>
#include <linux/string.h>

uintptr_t *sys_call_table = 0;        // 64位系统调用表指针，指向内核系统调用表数组
KP_EXPORT_SYMBOL(sys_call_table);

uintptr_t *compat_sys_call_table = 0; // 32位兼容系统调用表指针，用于支持32位应用程序
KP_EXPORT_SYMBOL(compat_sys_call_table);

int has_syscall_wrapper = 0;          // 标识内核是否使用系统调用包装器（pt_regs传参）
KP_EXPORT_SYMBOL(has_syscall_wrapper);

int has_config_compat = 0;            // 标识内核是否编译了32位兼容模式支持
KP_EXPORT_SYMBOL(has_config_compat);

// 用户参数指针结构 - 封装用户空间传入的字符串数组指针
struct user_arg_ptr
{
    union
    {
        const char __user *const __user *native;  // 64位环境下的用户空间指针
    } ptr;
};

// 兼容模式用户参数指针结构 - 支持32位和64位参数传递的统一接口
struct user_arg_ptr_compat
{
    bool is_compat;  // 标识当前是否为32位兼容模式
    union
    {
        const char __user *const __user *native;  // 64位模式下的用户空间指针
        const compat_uptr_t __user *compat;       // 32位兼容模式下的用户空间指针
    } ptr;
};

/**
 * 从用户空间参数数组中获取指定索引的字符串指针
 * @param a0 兼容模式标识参数，非空表示32位兼容模式
 * @param a1 用户空间字符串指针数组
 * @param nr 要获取的参数索引
 * @return 用户空间字符串指针，失败时返回错误指针
 */
const char __user *get_user_arg_ptr(void *a0, void *a1, int nr)
{
    char __user *const __user *native = (char __user *const __user *)a0;
    int size = 8;  // 默认64位指针大小
    if (has_config_compat) {
        native = (char __user *const __user *)a1;  // 兼容模式下使用a1参数
        if (a0) size = 4; // a0非空表示32位兼容模式，指针大小为4字节
    }
    // 计算目标参数在数组中的位置
    native = (char __user *const __user *)((unsigned long)native + nr * size);
    // 从用户空间复制指针数据到内核空间
    char __user **upptr = memdup_user(native, size);
    if (IS_ERR(upptr)) return ERR_PTR((long)upptr);

    char __user *uptr;
    if (size == 8) {
        uptr = *upptr;  // 64位直接取值
    } else {
        uptr = (char __user *)(unsigned long)*(int32_t *)upptr;  // 32位需要类型转换
    }
    kfree(upptr);  // 释放临时分配的内核内存
    return uptr;
}

/**
 * 设置用户空间参数数组中指定索引的值
 * @param a0 兼容模式标识参数
 * @param a1 用户空间字符串指针数组
 * @param nr 要设置的参数索引
 * @param val 要设置的新值
 * @return 成功返回0，失败返回错误码
 */
int set_user_arg_ptr(void *a0, void *a1, int nr, uintptr_t val)
{
    uintptr_t valp = (uintptr_t)&val;
    char __user *const __user *native = (char __user *const __user *)a0;
    int size = 8;  // 默认64位指针大小
    if (has_config_compat) {
        native = (char __user *const __user *)a1;
        if (a0) {
            size = 4; // 32位兼容模式
            valp += 4;  // 跳过高32位，使用低32位
        }
    }
    // 计算目标参数在数组中的位置
    native = (char __user *const __user *)((unsigned long)native + nr * size);
    // 将数据复制到用户空间
    int cplen = compat_copy_to_user((void *)native, (void *)valp, size);
    return cplen == size ? 0 : cplen;  // 返回0表示成功
}

// 系统调用函数指针类型定义 - 支持不同参数数量的系统调用
typedef long (*warp_raw_syscall_f)(const struct pt_regs *regs);  // 包装器模式（传递pt_regs）
typedef long (*raw_syscall0_f)();  // 无参数系统调用
typedef long (*raw_syscall1_f)(long arg0);  // 1个参数
typedef long (*raw_syscall2_f)(long arg0, long arg1);  // 2个参数
typedef long (*raw_syscall3_f)(long arg0, long arg1, long arg2);  // 3个参数
typedef long (*raw_syscall4_f)(long arg0, long arg1, long arg2, long arg3);  // 4个参数
typedef long (*raw_syscall5_f)(long arg0, long arg1, long arg2, long arg3, long arg4);  // 5个参数
typedef long (*raw_syscall6_f)(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);  // 6个参数

/**
 * 通过符号名称查找系统调用函数地址
 * @param nr 系统调用编号
 * @param is_compat 是否为兼容模式系统调用
 * @return 系统调用函数地址，失败返回0
 */
uintptr_t syscalln_name_addr(int nr, int is_compat)
{
    const char *name = 0;
    // 根据是否兼容模式选择相应的系统调用名称表
    if (!is_compat) {
        if (syscall_name_table[nr].addr) {
            return syscall_name_table[nr].addr;  // 返回已缓存的地址
        }
        name = syscall_name_table[nr].name;  // 获取系统调用名称
    } else {
        if (compat_syscall_name_table[nr].addr) {
            return compat_syscall_name_table[nr].addr;  // 返回已缓存的兼容模式地址
        }
        name = compat_syscall_name_table[nr].name;  // 获取兼容模式系统调用名称
    }

    if (!name) return 0;  // 名称不存在则返回0

    // 定义可能的符号名称前缀
    const char *prefix[2];
    prefix[0] = "__arm64_";  // ARM64架构的系统调用前缀
    prefix[1] = "";          // 无前缀
    // 定义可能的符号名称后缀
    const char *suffix[3];
    suffix[0] = ".cfi_jt";   // CFI跳转表后缀
    suffix[1] = ".cfi";      // CFI后缀
    suffix[2] = "";          // 无后缀

    uintptr_t addr = 0;

    // 构造完整的符号名称并查找地址
    char buffer[256];
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            snprintf(buffer, sizeof(buffer), "%s%s%s", prefix[i], name, suffix[j]);
            addr = kallsyms_lookup_name(buffer);  // 通过kallsyms查找符号地址
            if (addr) break;  // 找到地址则跳出内层循环
        }
        if (addr) break;  // 找到地址则跳出外层循环
    }
    // 缓存查找到的地址以避免重复查找
    if (!is_compat) {
        syscall_name_table[nr].addr = addr;
    } else {
        compat_syscall_name_table[nr].addr = addr;
    }
    return addr;
}
KP_EXPORT_SYMBOL(syscalln_name_addr);

/**
 * 获取系统调用函数地址
 * @param nr 系统调用编号
 * @param is_compat 是否为兼容模式系统调用
 * @return 系统调用函数地址
 */
uintptr_t syscalln_addr(int nr, int is_compat)
{
    // 优先从系统调用表中获取地址
    if (!is_compat && sys_call_table) return sys_call_table[nr];
    if (is_compat && compat_sys_call_table) return compat_sys_call_table[nr];
    // 系统调用表不可用时通过符号名称查找
    return syscalln_name_addr(nr, is_compat);
}
KP_EXPORT_SYMBOL(syscalln_addr);

/**
 * 调用无参数的系统调用
 * @param nr 系统调用编号
 * @return 系统调用返回值
 */
long raw_syscall0(long nr)
{
    uintptr_t addr = syscalln_addr(nr, 0);
    if (has_syscall_wrapper) {
        // 使用系统调用包装器模式，通过pt_regs传递参数
        struct pt_regs regs;
        regs.syscallno = nr;  // 设置系统调用编号
        regs.regs[8] = nr;    // ARM64中x8寄存器存放系统调用编号
        return ((warp_raw_syscall_f)addr)(&regs);
    }
    // 直接调用系统调用函数
    return ((raw_syscall0_f)addr)();
}
KP_EXPORT_SYMBOL(raw_syscall0);

/**
 * 调用单参数的系统调用
 * @param nr 系统调用编号
 * @param arg0 第一个参数
 * @return 系统调用返回值
 */
long raw_syscall1(long nr, long arg0)
{
    uintptr_t addr = syscalln_addr(nr, 0);
    if (has_syscall_wrapper) {
        // 使用系统调用包装器模式
        struct pt_regs regs;
        regs.syscallno = nr;
        regs.regs[8] = nr;     // 系统调用编号
        regs.regs[0] = arg0;   // 第一个参数存放在x0寄存器
        return ((warp_raw_syscall_f)addr)(&regs);
    }
    return ((raw_syscall1_f)addr)(arg0);
}
KP_EXPORT_SYMBOL(raw_syscall1);

/**
 * 调用双参数的系统调用
 * @param nr 系统调用编号
 * @param arg0 第一个参数
 * @param arg1 第二个参数
 * @return 系统调用返回值
 */
long raw_syscall2(long nr, long arg0, long arg1)
{
    uintptr_t addr = syscalln_addr(nr, 0);
    if (has_syscall_wrapper) {
        struct pt_regs regs;
        regs.syscallno = nr;
        regs.regs[8] = nr;
        regs.regs[0] = arg0;   // x0寄存器
        regs.regs[1] = arg1;   // x1寄存器
        return ((warp_raw_syscall_f)addr)(&regs);
    }
    return ((raw_syscall2_f)addr)(arg0, arg1);
}
KP_EXPORT_SYMBOL(raw_syscall2);

/**
 * 调用三参数的系统调用
 * @param nr 系统调用编号
 * @param arg0 第一个参数
 * @param arg1 第二个参数
 * @param arg2 第三个参数
 * @return 系统调用返回值
 */
long raw_syscall3(long nr, long arg0, long arg1, long arg2)
{
    uintptr_t addr = syscalln_addr(nr, 0);
    if (has_syscall_wrapper) {
        struct pt_regs regs;
        regs.syscallno = nr;
        regs.regs[8] = nr;
        regs.regs[0] = arg0;
        regs.regs[1] = arg1;
        regs.regs[2] = arg2;   // x2寄存器
        return ((warp_raw_syscall_f)addr)(&regs);
    }
    return ((raw_syscall3_f)addr)(arg0, arg1, arg2);
}
KP_EXPORT_SYMBOL(raw_syscall3);

/**
 * 调用四参数的系统调用
 * @param nr 系统调用编号
 * @param arg0-arg3 四个参数
 * @return 系统调用返回值
 */
long raw_syscall4(long nr, long arg0, long arg1, long arg2, long arg3)
{
    uintptr_t addr = syscalln_addr(nr, 0);
    if (has_syscall_wrapper) {
        struct pt_regs regs;
        regs.syscallno = nr;
        regs.regs[8] = nr;
        regs.regs[0] = arg0;
        regs.regs[1] = arg1;
        regs.regs[2] = arg2;
        regs.regs[3] = arg3;   // x3寄存器
        return ((warp_raw_syscall_f)addr)(&regs);
    }
    return ((raw_syscall4_f)addr)(arg0, arg1, arg2, arg3);
}
KP_EXPORT_SYMBOL(raw_syscall4);

/**
 * 调用五参数的系统调用
 * @param nr 系统调用编号
 * @param arg0-arg4 五个参数
 * @return 系统调用返回值
 */
long raw_syscall5(long nr, long arg0, long arg1, long arg2, long arg3, long arg4)
{
    uintptr_t addr = syscalln_addr(nr, 0);
    if (has_syscall_wrapper) {
        struct pt_regs regs;
        regs.syscallno = nr;
        regs.regs[8] = nr;
        regs.regs[0] = arg0;
        regs.regs[1] = arg1;
        regs.regs[2] = arg2;
        regs.regs[3] = arg3;
        regs.regs[4] = arg4;   // x4寄存器
        return ((warp_raw_syscall_f)addr)(&regs);
    }
    return ((raw_syscall5_f)addr)(arg0, arg1, arg2, arg3, arg4);
}
KP_EXPORT_SYMBOL(raw_syscall5);

/**
 * 调用六参数的系统调用
 * @param nr 系统调用编号
 * @param arg0-arg5 六个参数
 * @return 系统调用返回值
 */
long raw_syscall6(long nr, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5)
{
    uintptr_t addr = syscalln_addr(nr, 0);
    if (has_syscall_wrapper) {
        struct pt_regs regs;
        regs.syscallno = nr;
        regs.regs[8] = nr;
        regs.regs[0] = arg0;
        regs.regs[1] = arg1;
        regs.regs[2] = arg2;
        regs.regs[3] = arg3;
        regs.regs[4] = arg4;
        regs.regs[5] = arg5;   // x5寄存器
        return ((warp_raw_syscall_f)addr)(&regs);
    }
    return ((raw_syscall6_f)addr)(arg0, arg1, arg2, arg3, arg4, arg5);
}
KP_EXPORT_SYMBOL(raw_syscall6);

/**
 * 使用函数指针hook的方式包装系统调用
 * @param nr 系统调用编号
 * @param narg 参数个数
 * @param is_compat 是否为兼容模式
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 * @param udata 用户数据
 * @return hook结果，成功返回HOOK_SUCCESS
 */
hook_err_t fp_wrap_syscalln(int nr, int narg, int is_compat, void *before, void *after, void *udata)
{
    if (!is_compat) {
        if (!sys_call_table) return HOOK_BAD_ADDRESS;  // 系统调用表不可用
        uintptr_t fp_addr = (uintptr_t)(sys_call_table + nr);  // 获取系统调用表项地址
        if (has_syscall_wrapper) narg = 1;  // 包装器模式下参数统一为1个（pt_regs）
        return fp_hook_wrap(fp_addr, narg, before, after, udata);  // 执行函数指针hook
    } else {
        if (!compat_sys_call_table) return HOOK_BAD_ADDRESS;  // 兼容系统调用表不可用
        uintptr_t fp_addr = (uintptr_t)(compat_sys_call_table + nr);  // 获取兼容系统调用表项地址
        if (has_syscall_wrapper) narg = 1;
        return fp_hook_wrap(fp_addr, narg, before, after, udata);
    }
}
KP_EXPORT_SYMBOL(fp_wrap_syscalln);

/**
 * 解除函数指针hook的系统调用包装
 * @param nr 系统调用编号
 * @param is_compat 是否为兼容模式
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 */
void fp_unwrap_syscalln(int nr, int is_compat, void *before, void *after)
{
    if (!is_compat) {
        if (!sys_call_table) return;  // 系统调用表不可用
        uintptr_t fp_addr = (uintptr_t)(sys_call_table + nr);
        fp_hook_unwrap(fp_addr, before, after);  // 移除函数指针hook
    } else {
        if (!compat_sys_call_table) return;
        uintptr_t fp_addr = (uintptr_t)(compat_sys_call_table + nr);
        fp_hook_unwrap(fp_addr, before, after);
    }
}
KP_EXPORT_SYMBOL(fp_unwrap_syscalln);

/*
 * 系统调用CFI跳转表示例：
 * sys_xxx.cfi_jt
 * 
 * hint #0x22  # bti c    - 分支目标标识指令
 * b #0xfffffffffeb452f4  - 无条件跳转到实际系统调用函数
 */

/**
 * 使用内联hook的方式包装系统调用
 * @param nr 系统调用编号
 * @param narg 参数个数
 * @param is_compat 是否为兼容模式
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 * @param udata 用户数据
 * @return hook结果
 */
hook_err_t inline_wrap_syscalln(int nr, int narg, int is_compat, void *before, void *after, void *udata)
{
    uintptr_t addr = syscalln_name_addr(nr, is_compat);  // 通过符号名称获取系统调用函数地址
    if (!addr) return -HOOK_BAD_ADDRESS;  // 地址无效
    if (has_syscall_wrapper) narg = 1;    // 包装器模式参数调整
    return hook_wrap((void *)addr, narg, before, after, udata);  // 执行内联hook
}
KP_EXPORT_SYMBOL(inline_wrap_syscalln);

/**
 * 解除内联hook的系统调用包装
 * @param nr 系统调用编号
 * @param is_compat 是否为兼容模式
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 */
void inline_unwrap_syscalln(int nr, int is_compat, void *before, void *after)
{
    uintptr_t addr = syscalln_name_addr(nr, is_compat);
    hook_unwrap((void *)addr, before, after);  // 移除内联hook
}
KP_EXPORT_SYMBOL(inline_unwrap_syscalln);

/**
 * hook 64位系统调用的通用接口
 * @param nr 系统调用编号
 * @param narg 参数个数
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 * @param udata 用户数据
 * @return hook结果
 */
hook_err_t hook_syscalln(int nr, int narg, void *before, void *after, void *udata)
{
    // 优先使用函数指针hook，系统调用表不可用时使用内联hook
    if (sys_call_table) return fp_wrap_syscalln(nr, narg, 0, before, after, udata);
    return inline_wrap_syscalln(nr, narg, 0, before, after, udata);
}
KP_EXPORT_SYMBOL(hook_syscalln);

/**
 * 解除64位系统调用hook的通用接口
 * @param nr 系统调用编号
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 */
void unhook_syscalln(int nr, void *before, void *after)
{
    if (sys_call_table) return fp_unwrap_syscalln(nr, 0, before, after);
    return inline_unwrap_syscalln(nr, 0, before, after);
}
KP_EXPORT_SYMBOL(unhook_syscalln);

/**
 * hook 32位兼容系统调用的通用接口
 * @param nr 系统调用编号
 * @param narg 参数个数
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 * @param udata 用户数据
 * @return hook结果
 */
hook_err_t hook_compat_syscalln(int nr, int narg, void *before, void *after, void *udata)
{
    // 优先使用函数指针hook，兼容系统调用表不可用时使用内联hook
    if (compat_sys_call_table) return fp_wrap_syscalln(nr, narg, 1, before, after, udata);
    return inline_wrap_syscalln(nr, narg, 1, before, after, udata);
}
KP_EXPORT_SYMBOL(hook_compat_syscalln);

/**
 * 解除32位兼容系统调用hook的通用接口
 * @param nr 系统调用编号
 * @param before hook前置回调函数
 * @param after hook后置回调函数
 */
void unhook_compat_syscalln(int nr, void *before, void *after)
{
    if (compat_sys_call_table) return fp_unwrap_syscalln(nr, 1, before, after);
    return inline_unwrap_syscalln(nr, 1, before, after);
}
KP_EXPORT_SYMBOL(unhook_compat_syscalln);

/**
 * 系统调用模块初始化函数
 * 负责转换符号名称地址、查找系统调用表、检测内核配置特性
 */
void syscall_init()
{
    // 转换64位系统调用名称表中的符号地址
    for (int i = 0; i < sizeof(syscall_name_table) / sizeof(syscall_name_table[0]); i++) {
        uintptr_t *addr = (uintptr_t *)&syscall_name_table[i].name;
        *addr = link2runtime(*addr);  // 将链接时地址转换为运行时地址
    }

    // 转换32位兼容系统调用名称表中的符号地址
    for (int i = 0; i < sizeof(compat_syscall_name_table) / sizeof(compat_syscall_name_table[0]); i++) {
        uintptr_t *addr = (uintptr_t *)&compat_syscall_name_table[i].name;
        *addr = link2runtime(*addr);  // 将链接时地址转换为运行时地址
    }

    // 通过kallsyms查找64位系统调用表
    sys_call_table = (typeof(sys_call_table))kallsyms_lookup_name("sys_call_table");
    log_boot("sys_call_table addr: %llx\n", sys_call_table);

    // 通过kallsyms查找32位兼容系统调用表
    compat_sys_call_table = (typeof(compat_sys_call_table))kallsyms_lookup_name("compat_sys_call_table");
    log_boot("compat_sys_call_table addr: %llx\n", compat_sys_call_table);

    // 初始化配置标志
    has_config_compat = 0;
    has_syscall_wrapper = 0;

    // 检测内核是否同时启用了兼容模式和系统调用包装器
    if (kallsyms_lookup_name("__arm64_compat_sys_openat")) {
        has_config_compat = 1;    // 支持32位兼容模式
        has_syscall_wrapper = 1;  // 使用系统调用包装器
    } else {
        // 单独检测兼容模式支持
        if (kallsyms_lookup_name("compat_sys_call_table") || kallsyms_lookup_name("compat_sys_openat")) {
            has_config_compat = 1;
        }
        // 单独检测系统调用包装器
        if (kallsyms_lookup_name("__arm64_sys_openat")) {
            has_syscall_wrapper = 1;
        }
    }

    log_boot("syscall config_compat: %d\n", has_config_compat);
    log_boot("syscall has_wrapper: %d\n", has_syscall_wrapper);
}
