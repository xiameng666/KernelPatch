/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// Hook内存管理器 - 管理hook数据结构的内存分配和释放

#include "hook.h"

#include <stdint.h>

static uint64_t mem_region_start = 0;  // 内存区域起始地址
static uint64_t mem_region_end = 0;    // 内存区域结束地址

// Hook内存包装结构 - 管理不同类型的hook数据
typedef struct
{
    int using;              // 使用标志（0=未使用，1=已使用）
    enum hook_type type;    // Hook类型
    uintptr_t addr;         // 原始地址
    // 必须8字节对齐
    union
    {
        hook_t inl;                 // 内联hook
        hook_chain_t inl_chain;     // 内联hook链
        fp_hook_chain_t fp_chain;   // 函数指针hook链
    } chain __attribute__((aligned(8)));   // hook链数据，8字节对齐
} hook_mem_warp_t __attribute__((aligned(16)));  // 整个结构16字节对齐

// 添加hook内存区域 - 初始化可用的hook内存池
int hook_mem_add(uint64_t start, int32_t size)
{
    // 清零整个内存区域
    for (uint64_t i = start; i < start + size; i += 8) {
        *(uint64_t *)i = 0;
    }
    mem_region_start = start;      // 设置区域起始地址
    mem_region_end = start + size; // 设置区域结束地址
    return 0;
}

// 分配并清零hook内存 - 为指定地址和类型分配hook数据结构
void *hook_mem_zalloc(uintptr_t origin_addr, enum hook_type type)
{
    uint64_t start = mem_region_start;
    // 遍历内存区域寻找空闲slot
    for (uint64_t addr = start; addr < mem_region_end; addr += sizeof(hook_mem_warp_t)) {
        hook_mem_warp_t *wrap = (hook_mem_warp_t *)addr;
        if (wrap->using) continue;  // 跳过已使用的slot

        wrap->using = 1;            // 标记为已使用
        wrap->addr = origin_addr;   // 设置原始地址
        wrap->type = type;          // 设置hook类型

        // 清零chain数据区域
        for (uintptr_t i = (uintptr_t)&wrap->chain; i < (uintptr_t)&wrap->chain + sizeof(wrap->chain); i += 8) {
            *(uint64_t *)i = 0;
        }

        // TODO: 添加断言检查8字节对齐
        if (((uintptr_t)&wrap->chain) & 0b111) {
            return 0;  // 对齐检查失败
        }
        return &wrap->chain;  // 返回chain数据的地址
    }
    return 0;  // 未找到空闲slot
}

// 释放hook内存 - 标记指定内存为可用状态
void hook_mem_free(void *hook_mem)
{
    // 通过container_of宏获取包装结构体地址
    hook_mem_warp_t *warp = local_container_of(hook_mem, hook_mem_warp_t, chain);
    warp->using = 0;  // 标记为未使用
}

// 根据原始地址获取对应的hook内存 - 查找已分配的hook数据
void *hook_get_mem_from_origin(uint64_t origin_addr)
{
    uint64_t start = mem_region_start;

    // 遍历内存区域寻找匹配的原始地址
    for (uint64_t addr = start; addr < mem_region_end; addr += sizeof(hook_mem_warp_t)) {
        hook_mem_warp_t *wrap = (hook_mem_warp_t *)addr;
        if (wrap->using && wrap->addr == origin_addr) {
            return &wrap->chain;  // 返回找到的chain数据地址
        }
    }
    return 0;  // 未找到匹配项
}
