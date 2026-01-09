/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// Hook系统头文件 - 定义内核函数hook的数据结构和接口

#ifndef _KP_HOOK_H_
#define _KP_HOOK_H_

#include <stdint.h>
#include <log.h>

#define HOOK_INTO_BRANCH_FUNC

// Hook错误码枚举 - 定义各种hook操作的错误类型
typedef enum
{
    HOOK_NO_ERR = 0,          // 无错误
    HOOK_BAD_ADDRESS = 4095,  // 地址无效
    HOOK_DUPLICATED = 4094,   // 重复hook
    HOOK_NO_MEM = 4093,       // 内存不足
    HOOK_BAD_RELO = 4092,     // 重定位错误
    HOOK_TRANSIT_NO_MEM = 4091, // 转换代码内存不足
    HOOK_CHAIN_FULL = 4090,   // hook链已满
} hook_err_t;

// Hook类型枚举 - 定义不同的hook实现方式
enum hook_type
{
    NONE = 0,                    // 无hook
    INLINE,                      // 内联hook
    INLINE_CHAIN,               // 内联hook链
    FUNCTION_POINTER_CHAIN,     // 函数指针hook链
};

typedef int8_t chain_item_state;  // 链表项状态类型

// 链表项状态常量 - 管理hook链中每个项的状态
#define CHAIN_ITEM_STATE_EMPTY 0  // 空闲状态
#define CHAIN_ITEM_STATE_READY 1  // 就绪状态
#define CHAIN_ITEM_STATE_BUSY 2   // 忙状态

// 容器宏定义 - 用于从成员指针获取包含结构体指针
#define local_offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#define local_container_of(ptr, type, member) ({ (type *)((char *)(ptr) - local_offsetof(type, member)); })

// Hook系统配置常量
#define HOOK_MEM_REGION_NUM 4               // hook内存区域数量
#define TRAMPOLINE_NUM 4                    // 跳板指令数量
#define RELOCATE_INST_NUM (TRAMPOLINE_NUM * 8 + 8)  // 重定位指令数量

#define HOOK_CHAIN_NUM 0x10                 // hook链最大长度
#define TRANSIT_INST_NUM 0x60               // 转换指令数量

#define FP_HOOK_CHAIN_NUM 0x20              // 函数指针hook链最大长度

// ARM64指令常量 - 用于代码生成和识别
#define ARM64_NOP 0xd503201f     // 空操作指令
#define ARM64_BTI_C 0xd503245f   // 分支目标识别指令（调用）
#define ARM64_BTI_J 0xd503249f   // 分支目标识别指令（跳转）
#define ARM64_BTI_JC 0xd50324df  // 分支目标识别指令（调用+跳转）

// Hook核心数据结构 - 包含hook的所有必要信息
typedef struct
{
    // 输入参数
    uint64_t func_addr;       // 目标函数地址
    uint64_t origin_addr;     // 原始地址
    uint64_t replace_addr;    // 替换函数地址
    uint64_t relo_addr;       // 重定位地址
    
    // 输出结果
    int32_t tramp_insts_num;  // 跳板指令数量
    int32_t relo_insts_num;   // 重定位指令数量
    uint32_t origin_insts[TRAMPOLINE_NUM] __attribute__((aligned(8)));  // 原始指令
    uint32_t tramp_insts[TRAMPOLINE_NUM] __attribute__((aligned(8)));   // 跳板指令
    uint32_t relo_insts[RELOCATE_INST_NUM] __attribute__((aligned(8))); // 重定位指令
} hook_t __attribute__((aligned(8)));

struct _hook_chain;  // 前向声明

#define HOOK_LOCAL_DATA_NUM 8  // 本地数据项数量

// Hook本地数据结构 - 在hook链中传递的局部数据
typedef struct
{
    union
    {
        struct
        {
            uint64_t data0;  // 数据项0
            uint64_t data1;  // 数据项1
            uint64_t data2;  // 数据项2
            uint64_t data3;  // 数据项3
            uint64_t data4;  // 数据项4
            uint64_t data5;  // 数据项5
            uint64_t data6;  // 数据项6
            uint64_t data7;  // 数据项7
        };
        uint64_t data[HOOK_LOCAL_DATA_NUM];  // 数组形式访问
    };
} hook_local_t;

typedef struct
{
    void *chain;
    int skip_origin;
    hook_local_t local;
    uint64_t ret;
    union
    {
        struct
        {
        };
        uint64_t args[0];
    };
} hook_fargs0_t __attribute__((aligned(8)));

typedef struct
{
    void *chain;
    int skip_origin;
    hook_local_t local;
    uint64_t ret;
    union
    {
        struct
        {
            uint64_t arg0;
            uint64_t arg1;
            uint64_t arg2;
            uint64_t arg3;
        };
        uint64_t args[4];
    };
} hook_fargs4_t __attribute__((aligned(8)));

typedef hook_fargs4_t hook_fargs1_t;
typedef hook_fargs4_t hook_fargs2_t;
typedef hook_fargs4_t hook_fargs3_t;

typedef struct
{
    void *chain;
    int skip_origin;
    hook_local_t local;
    uint64_t ret;
    union
    {
        struct
        {
            uint64_t arg0;
            uint64_t arg1;
            uint64_t arg2;
            uint64_t arg3;
            uint64_t arg4;
            uint64_t arg5;
            uint64_t arg6;
            uint64_t arg7;
        };
        uint64_t args[8];
    };
} hook_fargs8_t __attribute__((aligned(8)));

typedef hook_fargs8_t hook_fargs5_t;
typedef hook_fargs8_t hook_fargs6_t;
typedef hook_fargs8_t hook_fargs7_t;

typedef struct
{
    void *chain;
    int skip_origin;
    hook_local_t local;
    uint64_t ret;
    union
    {
        struct
        {
            uint64_t arg0;
            uint64_t arg1;
            uint64_t arg2;
            uint64_t arg3;
            uint64_t arg4;
            uint64_t arg5;
            uint64_t arg6;
            uint64_t arg7;
            uint64_t arg8;
            uint64_t arg9;
            uint64_t arg10;
            uint64_t arg11;
        };
        uint64_t args[12];
    };
} hook_fargs12_t __attribute__((aligned(8)));

typedef hook_fargs12_t hook_fargs9_t;
typedef hook_fargs12_t hook_fargs10_t;
typedef hook_fargs12_t hook_fargs11_t;

typedef void (*hook_chain0_callback)(hook_fargs0_t *fargs, void *udata);
typedef void (*hook_chain1_callback)(hook_fargs1_t *fargs, void *udata);
typedef void (*hook_chain2_callback)(hook_fargs2_t *fargs, void *udata);
typedef void (*hook_chain3_callback)(hook_fargs3_t *fargs, void *udata);
typedef void (*hook_chain4_callback)(hook_fargs4_t *fargs, void *udata);
typedef void (*hook_chain5_callback)(hook_fargs5_t *fargs, void *udata);
typedef void (*hook_chain6_callback)(hook_fargs6_t *fargs, void *udata);
typedef void (*hook_chain7_callback)(hook_fargs7_t *fargs, void *udata);
typedef void (*hook_chain8_callback)(hook_fargs8_t *fargs, void *udata);
typedef void (*hook_chain9_callback)(hook_fargs9_t *fargs, void *udata);
typedef void (*hook_chain10_callback)(hook_fargs10_t *fargs, void *udata);
typedef void (*hook_chain11_callback)(hook_fargs11_t *fargs, void *udata);
typedef void (*hook_chain12_callback)(hook_fargs12_t *fargs, void *udata);

typedef struct _hook_chain
{
    // must be the first element
    hook_t hook;
    int32_t chain_items_max;
    chain_item_state states[HOOK_CHAIN_NUM];
    void *udata[HOOK_CHAIN_NUM];
    void *befores[HOOK_CHAIN_NUM];
    void *afters[HOOK_CHAIN_NUM];
    uint32_t transit[TRANSIT_INST_NUM];
} hook_chain_t __attribute__((aligned(8)));

typedef struct
{
    uintptr_t fp_addr;
    uint64_t replace_addr;
    uint64_t origin_fp;
} fp_hook_t __attribute__((aligned(8)));

typedef struct _fphook_chain
{
    fp_hook_t hook;
    int32_t chain_items_max;
    chain_item_state states[FP_HOOK_CHAIN_NUM];
    void *udata[FP_HOOK_CHAIN_NUM];
    void *befores[FP_HOOK_CHAIN_NUM];
    void *afters[FP_HOOK_CHAIN_NUM];
    uint32_t transit[TRANSIT_INST_NUM];
} fp_hook_chain_t __attribute__((aligned(8)));

static inline int is_bad_address(void *addr)
{
    return ((uint64_t)addr & 0x8000000000000000) != 0x8000000000000000;
}

int32_t branch_from_to(uint32_t *tramp_buf, uint64_t src_addr, uint64_t dst_addr);
int32_t branch_relative(uint32_t *buf, uint64_t src_addr, uint64_t dst_addr);
int32_t branch_absolute(uint32_t *buf, uint64_t addr);
int32_t ret_absolute(uint32_t *buf, uint64_t addr);

hook_err_t hook_prepare(hook_t *hook);
void hook_install(hook_t *hook);
void hook_uninstall(hook_t *hook);

/**
 * @brief Inline-hook function which address is @param func with function @param replace, 
 * after hook, original @param func is backuped in @param backup.
 * 
 * @note If multiple modules hook this function simultaneously, 
 * it will cause abnormality when unload the modules. Please use hook_wrap instead
 * 
 * @see hook_wrap
 * 
 * @param func 
 * @param replace 
 * @param backup 
 * @return hook_err_t 
 */
hook_err_t hook(void *func, void *replace, void **backup);

/**
 * @brief unhook of hooked function
 * 
 * @param func 
 */
void unhook(void *func);

/**
 * @brief 
 * 
 * @param chain 
 * @param before 
 * @param after 
 * @param udata 
 * @return hook_err_t 
 */
hook_err_t hook_chain_add(hook_chain_t *chain, void *before, void *after, void *udata);
/**
 * @brief 
 * 
 * @param chain 
 * @param before 
 * @param after 
 */
void hook_chain_remove(hook_chain_t *chain, void *before, void *after);

/**
 * @brief Wrap a function with before and after function. 
 * The same function can do hook and unhook multiple times 
 * 
 * @see hook_chain0_callback
 * @see hook_fargs0_t
 * 
 * @param func The address of function 
 * @param argno The number of method arguments
 * @param before This function will be called before hooked function, 
 * the type of before is hook_chain{n}_callback which n is equal to argno.
 * @param after The same as before but will be call after hooked function
 * @param udata 
 * @return hook_err_t 
 */
hook_err_t hook_wrap(void *func, int32_t argno, void *before, void *after, void *udata);

/**
 * @brief 
 * 
 * @param func 
 * @param before 
 * @param after 
 * @param remove 
 */
void hook_unwrap_remove(void *func, void *before, void *after, int remove);

static inline void hook_unwrap(void *func, void *before, void *after)
{
    return hook_unwrap_remove(func, before, after, 1);
}

/**
 * @param hook_args
 */
static inline void *wrap_get_origin_func(void *hook_args)
{
    hook_fargs0_t *args = (hook_fargs0_t *)hook_args;
    hook_chain_t *chain = (hook_chain_t *)args->chain;
    return (void *)chain->hook.relo_addr;
}

/**
 * @brief 
 * 
 * @param fp_addr 
 * @param replace 
 * @param backup 
 */
void fp_hook(uintptr_t fp_addr, void *replace, void **backup);

/**
 * @brief 
 * 
 * @param fp_addr 
 * @param backup 
 */
void fp_unhook(uintptr_t fp_addr, void *backup);

/**
 * @brief 
 * 
 * @param fp_addr 
 * @param argno 
 * @param before 
 * @param after 
 * @param udata 
 * @return hook_err_t 
 */
hook_err_t fp_hook_wrap(uintptr_t fp_addr, int32_t argno, void *before, void *after, void *udata);

/**
 * @brief 
 * 
 * @param fp_addr 
 * @param before 
 * @param after 
 */
void fp_hook_unwrap(uintptr_t fp_addr, void *before, void *after);

/**
 * 
 */
static inline void *fp_get_origin_func(void *hook_args)
{
    hook_fargs0_t *args = (hook_fargs0_t *)hook_args;
    fp_hook_chain_t *chain = (fp_hook_chain_t *)args->chain;
    return (void *)chain->hook.origin_fp;
}

static inline void hook_chain_install(hook_chain_t *chain)
{
    hook_install(&chain->hook);
}

static inline void hook_chain_uninstall(hook_chain_t *chain)
{
    hook_uninstall(&chain->hook);
}

static inline hook_err_t hook_wrap0(void *func, hook_chain0_callback before, hook_chain0_callback after, void *udata)
{
    return hook_wrap(func, 0, before, after, udata);
}

static inline hook_err_t hook_wrap1(void *func, hook_chain1_callback before, hook_chain1_callback after, void *udata)
{
    return hook_wrap(func, 1, before, after, udata);
}

static inline hook_err_t hook_wrap2(void *func, hook_chain2_callback before, hook_chain2_callback after, void *udata)
{
    return hook_wrap(func, 2, before, after, udata);
}

static inline hook_err_t hook_wrap3(void *func, hook_chain3_callback before, hook_chain3_callback after, void *udata)
{
    return hook_wrap(func, 3, before, after, udata);
}

static inline hook_err_t hook_wrap4(void *func, hook_chain4_callback before, hook_chain4_callback after, void *udata)
{
    return hook_wrap(func, 4, before, after, udata);
}

static inline hook_err_t hook_wrap5(void *func, hook_chain5_callback before, hook_chain5_callback after, void *udata)
{
    return hook_wrap(func, 5, before, after, udata);
}

static inline hook_err_t hook_wrap6(void *func, hook_chain6_callback before, hook_chain6_callback after, void *udata)
{
    return hook_wrap(func, 6, before, after, udata);
}

static inline hook_err_t hook_wrap7(void *func, hook_chain7_callback before, hook_chain7_callback after, void *udata)
{
    return hook_wrap(func, 7, before, after, udata);
}

static inline hook_err_t hook_wrap8(void *func, hook_chain8_callback before, hook_chain8_callback after, void *udata)
{
    return hook_wrap(func, 8, before, after, udata);
}

static inline hook_err_t hook_wrap9(void *func, hook_chain9_callback before, hook_chain9_callback after, void *udata)
{
    return hook_wrap(func, 9, before, after, udata);
}

static inline hook_err_t hook_wrap10(void *func, hook_chain10_callback before, hook_chain10_callback after, void *udata)
{
    return hook_wrap(func, 10, before, after, udata);
}

static inline hook_err_t hook_wrap11(void *func, hook_chain11_callback before, hook_chain11_callback after, void *udata)
{
    return hook_wrap(func, 11, before, after, udata);
}

static inline hook_err_t hook_wrap12(void *func, hook_chain12_callback before, hook_chain12_callback after, void *udata)
{
    return hook_wrap(func, 12, before, after, udata);
}

static inline hook_err_t fp_hook_wrap0(uintptr_t fp_addr, hook_chain0_callback before, hook_chain0_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 0, before, after, udata);
}

static inline hook_err_t fp_hook_wrap1(uintptr_t fp_addr, hook_chain1_callback before, hook_chain1_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 1, before, after, udata);
}

static inline hook_err_t fp_hook_wrap2(uintptr_t fp_addr, hook_chain2_callback before, hook_chain2_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 2, before, after, udata);
}

static inline hook_err_t fp_hook_wrap3(uintptr_t fp_addr, hook_chain3_callback before, hook_chain3_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 3, before, after, udata);
}

static inline hook_err_t fp_hook_wrap4(uintptr_t fp_addr, hook_chain4_callback before, hook_chain4_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 4, before, after, udata);
}

static inline hook_err_t fp_hook_wrap5(uintptr_t fp_addr, hook_chain5_callback before, hook_chain5_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 5, before, after, udata);
}

static inline hook_err_t fp_hook_wrap6(uintptr_t fp_addr, hook_chain6_callback before, hook_chain6_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 6, before, after, udata);
}

static inline hook_err_t fp_hook_wrap7(uintptr_t fp_addr, hook_chain7_callback before, hook_chain7_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 7, before, after, udata);
}

static inline hook_err_t fp_hook_wrap8(uintptr_t fp_addr, hook_chain8_callback before, hook_chain8_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 8, before, after, udata);
}

static inline hook_err_t fp_hook_wrap9(uintptr_t fp_addr, hook_chain9_callback before, hook_chain9_callback after,
                                       void *udata)
{
    return fp_hook_wrap(fp_addr, 9, before, after, udata);
}

static inline hook_err_t fp_hook_wrap10(uintptr_t fp_addr, hook_chain10_callback before, hook_chain10_callback after,
                                        void *udata)
{
    return fp_hook_wrap(fp_addr, 10, before, after, udata);
}

static inline hook_err_t fp_hook_wrap11(uintptr_t fp_addr, hook_chain11_callback before, hook_chain11_callback after,
                                        void *udata)
{
    return fp_hook_wrap(fp_addr, 11, before, after, udata);
}

static inline hook_err_t fp_hook_wrap12(uintptr_t fp_addr, hook_chain12_callback before, hook_chain12_callback after,
                                        void *udata)
{
    return fp_hook_wrap(fp_addr, 12, before, after, udata);
}

#endif
