/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 内核符号查找模块 - 提供符号地址解析功能

#include <symbol.h>
#include <log.h>
#include <stdint.h>

#include "start.h"
#include "setup.h"

// 外部链接器定义的符号表边界
extern void _kp_symbol_start();  // 符号表起始位置
extern void _kp_symbol_end();    // 符号表结束位置
static uint64_t symbol_start = 0;  // 运行时符号表起始地址
static uint64_t symbol_end = 0;    // 运行时符号表结束地址

// DJB2 哈希算法 - 用于快速符号名匹配
static unsigned long sym_hash(const char *str)
{
    unsigned long hash = 5381;  // DJB2算法初始值
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}

// 本地字符串比较函数 - 避免依赖标准库
int local_strcmp(const char *s1, const char *s2)
{
    const unsigned char *c1 = (const unsigned char *)s1;
    const unsigned char *c2 = (const unsigned char *)s2;
    unsigned char ch;
    int d = 0;
    while (1) {
        d = (int)(ch = *c1++) - (int)*c2++;  // 比较当前字符
        if (d || !ch) break;  // 不相等或到达字符串末尾则退出
    }
    return d;  // 返回差值，0表示相等
}

// 根据符号名查找符号地址 - 支持快速哈希匹配
unsigned long symbol_lookup_name(const char *name)
{
    unsigned long hash = sym_hash(name);  // 计算目标符号名的哈希值
    // 遍历整个符号表
    for (uint64_t addr = symbol_start; addr < symbol_end; addr += sizeof(kp_symbol_t)) {
        kp_symbol_t *symbol = (kp_symbol_t *)addr;
        // 先比较哈希值（快速过滤），再比较字符串（精确匹配）
        if (hash == symbol->hash && !local_strcmp(name, symbol->name)) {
            return symbol->addr;  // 找到匹配符号，返回其地址
        }
    }
    return 0;  // 未找到符号，返回0
}

// 符号表初始化 - 重定位符号地址并计算哈希值
void symbol_init()
{
    symbol_start = (uint64_t)_kp_symbol_start;  // 获取符号表起始地址
    symbol_end = (uint64_t)_kp_symbol_end;      // 获取符号表结束地址
    log_boot("Symbol: %llx, %llx\n", symbol_start, symbol_end);
    
    // 遍历符号表，重定位地址并预计算哈希值
    for (uint64_t addr = symbol_start; addr < symbol_end; addr += sizeof(kp_symbol_t)) {
        kp_symbol_t *symbol = (kp_symbol_t *)addr;
        // 将链接地址转换为运行时地址
        symbol->addr = symbol->addr - link_base_addr + runtime_base_addr;
        // 预计算符号名哈希值以提高查找效率
        symbol->hash = sym_hash(symbol->name);
    }
}