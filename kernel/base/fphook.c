/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 函数指针Hook实现 - 提供对函数指针的透明拦截和调用链功能

#include <hook.h>
#include <symbol.h>
#include <pgtable.h>
#include <cache.h>
#include "hmem.h"

// 0参数转换函数类型定义
typedef uint64_t (*transit0_func_t)();

// 0参数函数指针转换函数 - 存储在专用代码段中
uint64_t __attribute__((section(".fp.transit0.text"))) __attribute__((__noinline__)) _fp_transit0()
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));  // 获取当前地址
    uint32_t *vptr = (uint32_t *)this_va;
    // 向前查找NOP指令边界
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    // 通过container_of获取hook链结构
    fp_hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, fp_hook_chain_t, transit);
    hook_fargs0_t fargs;
    fargs.skip_origin = 0;  // 是否跳过原始函数
    fargs.chain = hook_chain;
    
    // 执行前置回调函数链
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain0_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    
    // 调用原始函数（如果未被跳过）
    if (!fargs.skip_origin) {
        transit0_func_t origin_func = (transit0_func_t)hook_chain->hook.origin_fp;
        fargs.ret = origin_func();
    }
    
    // 执行后置回调函数链（逆序）
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain0_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}
extern void _fp_transit0_end();  // 转换函数结束标记

// 4参数转换函数类型定义
typedef uint64_t (*transit4_func_t)(uint64_t, uint64_t, uint64_t, uint64_t);

// 4参数函数指针转换函数 - 处理1-4个参数的函数调用
uint64_t __attribute__((section(".fp.transit4.text"))) __attribute__((__noinline__))
_fp_transit4(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));
    uint32_t *vptr = (uint32_t *)this_va;
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    fp_hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, fp_hook_chain_t, transit);
    hook_fargs4_t fargs;
    fargs.skip_origin = 0;
    fargs.arg0 = arg0;  // 保存所有参数
    fargs.arg1 = arg1;
    fargs.arg2 = arg2;
    fargs.arg3 = arg3;
    fargs.chain = hook_chain;
    
    // 执行前置回调函数链
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain4_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    
    // 调用原始函数
    if (!fargs.skip_origin) {
        transit4_func_t origin_func = (transit4_func_t)hook_chain->hook.origin_fp;
        fargs.ret = origin_func(fargs.arg0, fargs.arg1, fargs.arg2, fargs.arg3);
    }
    
    // 执行后置回调函数链
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain4_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}

extern void _fp_transit4_end();  // 4参数转换函数结束标记

// 8参数转换函数类型定义 - 处理5-8个参数的函数调用
typedef uint64_t (*transit8_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t __attribute__((section(".fp.transit8.text"))) __attribute__((__noinline__))
_fp_transit8(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6,
             uint64_t arg7)
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));
    uint32_t *vptr = (uint32_t *)this_va;
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    fp_hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, fp_hook_chain_t, transit);
    hook_fargs8_t fargs;
    fargs.skip_origin = 0;
    fargs.arg0 = arg0;
    fargs.arg1 = arg1;
    fargs.arg2 = arg2;
    fargs.arg3 = arg3;
    fargs.arg4 = arg4;
    fargs.arg5 = arg5;
    fargs.arg6 = arg6;
    fargs.arg7 = arg7;
    fargs.chain = hook_chain;
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain8_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    if (!fargs.skip_origin) {
        transit8_func_t origin_func = (transit8_func_t)hook_chain->hook.origin_fp;
        fargs.ret =
            origin_func(fargs.arg0, fargs.arg1, fargs.arg2, fargs.arg3, fargs.arg4, fargs.arg5, fargs.arg6, fargs.arg7);
    }
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain8_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}

extern void _fp_transit8_end();  // 8参数转换函数结束标记

// 12参数转换函数类型定义 - 处理9-12个参数的函数调用
typedef uint64_t (*transit12_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                     uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t __attribute__((section(".fp.transit12.text"))) __attribute__((__noinline__))
_fp_transit12(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6,
              uint64_t arg7, uint64_t arg8, uint64_t arg9, uint64_t arg10, uint64_t arg11)
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));
    uint32_t *vptr = (uint32_t *)this_va;
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    fp_hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, fp_hook_chain_t, transit);
    hook_fargs12_t fargs;
    fargs.skip_origin = 0;
    fargs.arg0 = arg0;
    fargs.arg1 = arg1;
    fargs.arg2 = arg2;
    fargs.arg3 = arg3;
    fargs.arg4 = arg4;
    fargs.arg5 = arg5;
    fargs.arg6 = arg6;
    fargs.arg7 = arg7;
    fargs.arg8 = arg8;
    fargs.arg9 = arg9;
    fargs.arg10 = arg10;
    fargs.arg11 = arg11;
    fargs.chain = hook_chain;
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain12_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    if (!fargs.skip_origin) {
        transit12_func_t origin_func = (transit12_func_t)hook_chain->hook.origin_fp;
        fargs.ret = origin_func(fargs.arg0, fargs.arg1, fargs.arg2, fargs.arg3, fargs.arg4, fargs.arg5, fargs.arg6,
                                fargs.arg7, fargs.arg8, fargs.arg9, fargs.arg10, fargs.arg11);
    }
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain12_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}

extern void _fp_transit12_end();  // 12参数转换函数结束标记

// Hook链准备函数 - 根据参数数量选择合适的转换函数并复制代码
static hook_err_t hook_chain_prepare(uint32_t *transit, int32_t argno)
{
    uint64_t transit_start, transit_end;
    // 根据参数数量选择对应的转换函数
    switch (argno) {
    case 0:
        transit_start = (uint64_t)_fp_transit0;
        transit_end = (uint64_t)_fp_transit0_end;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        transit_start = (uint64_t)_fp_transit4;
        transit_end = (uint64_t)_fp_transit4_end;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
        transit_start = (uint64_t)_fp_transit8;
        transit_end = (uint64_t)_fp_transit8_end;
        break;
    default:
        // 默认使用12参数转换函数处理更多参数的情况
        transit_start = (uint64_t)_fp_transit12;
        transit_end = (uint64_t)_fp_transit12_end;
        break;
    }

    int32_t transit_num = (transit_end - transit_start) / 4;  // 计算指令数量

    // TODO: 添加断言检查空间足够
    if (transit_num >= TRANSIT_INST_NUM) return -HOOK_TRANSIT_NO_MEM;

    // 设置转换代码：BTI指令 + NOP + 转换函数代码
    transit[0] = ARM64_BTI_JC;  // 分支目标识别指令
    transit[1] = ARM64_NOP;     // 空操作指令作为标记
    for (int i = 0; i < transit_num; i++) {
        transit[i + 2] = ((uint32_t *)transit_start)[i];  // 复制转换函数代码
    }
    return HOOK_NO_ERR;
}

// 函数指针Hook - 替换函数指针指向的地址
void fp_hook(uintptr_t fp_addr, void *replace, void **backup)
{
    uint64_t *entry = pgtable_entry_kernel(fp_addr);  // 获取页表条目
    uint64_t ori_prot = *entry;                        // 保存原始保护属性
    // 临时设置为可写（添加DBM位，清除只读位）
    modify_entry_kernel(fp_addr, entry, (ori_prot | PTE_DBM) & ~PTE_RDONLY);
    flush_tlb_kernel_page(fp_addr);                    // 刷新TLB
    *(uintptr_t *)backup = *(uintptr_t *)fp_addr;      // 备份原始函数指针
    *(uintptr_t *)fp_addr = (uintptr_t)replace;        // 设置新的函数指针
    dsb(ish);                                          // 数据同步屏障
    modify_entry_kernel(fp_addr, entry, ori_prot);     // 恢复原始保护属性
}
KP_EXPORT_SYMBOL(fp_hook);  // 导出符号供外部使用

// 函数指针UnHook - 恢复原始函数指针
void fp_unhook(uintptr_t fp_addr, void *backup)
{
    uint64_t *entry = pgtable_entry_kernel(fp_addr);  // 获取页表条目
    uint64_t ori_prot = *entry;                        // 保存原始保护属性
    // 临时设置为可写
    modify_entry_kernel(fp_addr, entry, (ori_prot | PTE_DBM) & ~PTE_RDONLY);
    *(uintptr_t *)fp_addr = (uintptr_t)backup;         // 恢复原始函数指针
    dsb(ish);                                          // 数据同步屏障
    isb();                                             // 指令同步屏障
    flush_icache_all();                                // 刷新指令缓存
    modify_entry_kernel(fp_addr, entry, ori_prot);     // 恢复保护属性
}
KP_EXPORT_SYMBOL(fp_unhook);

// 函数指针Hook包装 - 添加前置和后置回调到函数指针调用链
hook_err_t fp_hook_wrap(uintptr_t fp_addr, int32_t argno, void *before, void *after, void *udata)
{
    hook_err_t err = HOOK_NO_ERR;
    if (is_bad_address((void *)fp_addr)) return -HOOK_BAD_ADDRESS;  // 地址有效性检查
    
    // 尝试获取已存在的hook链
    fp_hook_chain_t *chain = hook_get_mem_from_origin(fp_addr);
    if (!chain) {
        // 创建新的hook链
        chain = (fp_hook_chain_t *)hook_mem_zalloc(fp_addr, FUNCTION_POINTER_CHAIN);
        if (!chain) return -HOOK_NO_MEM;
        
        chain->hook.fp_addr = fp_addr;                       // 设置原始函数指针地址
        chain->hook.replace_addr = (uint64_t)chain->transit; // 设置替换地址为转换函数
        err = hook_chain_prepare(chain->transit, argno);     // 准备转换代码
        if (err) return err;
        
        flush_icache_all();  // 刷新指令缓存
        // 执行函数指针hook
        fp_hook(chain->hook.fp_addr, (void *)chain->hook.replace_addr, (void **)&chain->hook.origin_fp);
    }

    // 在链中查找空闲位置添加回调
    for (int i = 0; i < FP_HOOK_CHAIN_NUM; i++) {
        // 检查是否重复添加相同的回调
        if ((before && chain->befores[i] == before) || (after && chain->afters[i] == after)) return -HOOK_DUPLICATED;

        // TODO: 需要原子操作或锁保护
        if (chain->states[i] == CHAIN_ITEM_STATE_EMPTY) {
            chain->states[i] = CHAIN_ITEM_STATE_BUSY;   // 标记为忙状态
            dsb(ish);
            chain->udata[i] = udata;      // 设置用户数据
            chain->befores[i] = before;   // 设置前置回调
            chain->afters[i] = after;     // 设置后置回调
            if (i + 1 > chain->chain_items_max) {
                chain->chain_items_max = i + 1;  // 更新最大项数
            }
            dsb(ish);
            chain->states[i] = CHAIN_ITEM_STATE_READY;  // 标记为就绪状态
            logkv("Wrap func pointer add: %llx, %llx, %llx successed\n", chain->hook.fp_addr, before, after);
            return HOOK_NO_ERR;
        }
    }
    logkv("Wrap func pointer add: %llx, %llx, %llx failed\n", chain->hook.fp_addr, before, after);
    return -HOOK_CHAIN_FULL;  // 链已满
}
KP_EXPORT_SYMBOL(fp_hook_wrap);

// 函数指针Hook解包装 - 从函数指针调用链中移除回调
void fp_hook_unwrap(uintptr_t fp_addr, void *before, void *after)
{
    if (is_bad_address((void *)fp_addr)) return;  // 地址有效性检查
    
    fp_hook_chain_t *chain = (fp_hook_chain_t *)hook_get_mem_from_origin(fp_addr);
    if (!chain) return;  // 未找到对应的hook链
    
    // 在链中查找并移除指定的回调
    for (int i = 0; i < FP_HOOK_CHAIN_NUM; i++) {
        if (chain->states[i] == CHAIN_ITEM_STATE_READY)
            if ((before && chain->befores[i] == before) || (after && chain->afters[i] == after)) {
                chain->states[i] = CHAIN_ITEM_STATE_BUSY;  // 标记为忙状态
                dsb(ish);
                chain->udata[i] = 0;      // 清除用户数据
                chain->befores[i] = 0;    // 清除前置回调
                chain->afters[i] = 0;     // 清除后置回调
                dsb(ish);
                chain->states[i] = CHAIN_ITEM_STATE_EMPTY;  // 标记为空状态
                break;
            }
    }
    logkv("Wrap func pointer remove: %llx, %llx, %llx\n", chain->hook.fp_addr, before, after);

    // 检查是否所有回调都已移除
    for (int i = 0; i < FP_HOOK_CHAIN_NUM; i++) {
        if (chain->states[i] != CHAIN_ITEM_STATE_EMPTY) return;  // 还有其他回调，不需要完全移除hook
    }
    
    // 所有回调都已移除，完全移除hook
    fp_unhook(chain->hook.fp_addr, (void *)chain->hook.origin_fp);
    // TODO: 这里可能不安全，需要确保没有其他线程在使用
    hook_mem_free(chain);  // 释放hook链内存
    logkv("Unwrap func pointer: %llx, %llx, %llx\n", fp_addr, before, after);
}
KP_EXPORT_SYMBOL(fp_hook_unwrap);