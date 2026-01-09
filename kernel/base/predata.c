/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 预设数据管理模块 - 处理补丁配置和超级密钥验证

#include <predata.h>
#include <common.h>
#include <log.h>
#include <sha256.h>
#include <symbol.h>

#include "start.h"
#include "pgtable.h"
#include "baselib.h"

extern start_preset_t start_preset;

static char *superkey = 0;        // 超级密钥
static char *root_superkey = 0;   // 根超级密钥哈希值

struct patch_config *patch_config = 0;  // 补丁配置结构
KP_EXPORT_SYMBOL(patch_config);

// Base62字符集用于随机密钥生成
static const char bstr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static uint64_t _rand_next = 1000000007;  // 随机数种子
static bool enable_root_key = false;      // 是否启用根密钥验证

// 验证超级密钥 - 支持普通密钥和根密钥验证
int auth_superkey(const char *key)
{
    int rc = 0;
    // 首先验证普通超级密钥（常量时间比较）
    for (int i = 0; superkey[i]; i++) {
        rc |= (superkey[i] ^ key[i]);
    }
    if (!rc) goto out;  // 普通密钥验证成功

    if (!enable_root_key) goto out;  // 根密钥验证未启用

    // 使用SHA256验证根密钥
    BYTE hash[SHA256_BLOCK_SIZE];
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const BYTE *)key, lib_strnlen(key, SUPER_KEY_LEN));
    sha256_final(&ctx, hash);
    int len = SHA256_BLOCK_SIZE > ROOT_SUPER_KEY_HASH_LEN ? ROOT_SUPER_KEY_HASH_LEN : SHA256_BLOCK_SIZE;
    rc = lib_memcmp(root_superkey, hash, len);

    // 首次使用根密钥时，重置为新的普通密钥
    static bool first_time = true;
    if (!rc && first_time) {
        first_time = false;
        reset_superkey(key);  // 设置新的普通密钥
        enable_root_key = false;  // 禁用根密钥验证
        enable_root_key = false;  // 禁用根密钥验证
    }

out:
    return !!rc;  // 返回验证结果：0=成功，非0=失败
}

// 重置超级密钥
void reset_superkey(const char *key)
{
    lib_strlcpy(superkey, key, SUPER_KEY_LEN);  // 复制新密钥
    dsb(ish);  // 内存屏障确保写入完成
}

// 启用或禁用根密钥验证
void enable_auth_root_key(bool enable)
{
    enable_root_key = enable;
}

// 线性同余随机数生成器
uint64_t rand_next()
{
    _rand_next = 1103515245 * _rand_next + 12345;  // LCG算法
    return _rand_next;
}

// 获取当前超级密钥
const char *get_superkey()
{
    return superkey;
}

// 获取编译时间
const char *get_build_time()
{
    return setup_header->compile_time;
}

// 遍历所有额外补丁项 - 回调函数模式
int on_each_extra_item(int (*callback)(const patch_extra_item_t *extra, const char *arg, const void *con, void *udata),
                       void *udata)
{
    int rc = 0;
    uint64_t item_addr = _kp_extra_start;  // 额外项起始地址
    while (item_addr < _kp_extra_end) {    // 遍历所有额外项
        patch_extra_item_t *item = (patch_extra_item_t *)item_addr;
        if (item->type == EXTRA_TYPE_NONE) break;  // 遇到结束标记
        
        // 验证魔数
        for (int i = 0; i < sizeof(item->magic); i++) {
            if (item->magic[i] != EXTRA_HDR_MAGIC[i]) break;
        }
        
        // 计算参数和内容地址
        const char *args = item->args_size > 0 ? (const char *)(item_addr + sizeof(patch_extra_item_t)) : 0;
        const void *con = (void *)(item_addr + sizeof(patch_extra_item_t) + item->args_size);
        
        // 调用回调函数处理当前项
        rc = callback(item, args, con, udata);
        if (rc) break;  // 回调函数返回非零值时停止遍历
        
        // 移动到下一项
        item_addr += sizeof(patch_extra_item_t);
        item_addr += item->args_size;
        item_addr += item->con_size;
    }
    return rc;
}

// 预设数据初始化 - 设置密钥、随机数和补丁配置
void predata_init()
{
    superkey = (char *)start_preset.superkey;          // 设置超级密钥指针
    root_superkey = (char *)start_preset.root_superkey; // 设置根密钥哈希指针
    char *compile_time = start_preset.header.compile_time;

    // 初始化随机数种子（使用多个系统参数）
    _rand_next *= kernel_va;        // 内核虚拟地址
    _rand_next *= kver;            // 内核版本
    _rand_next *= kpver;           // KernelPatch版本
    _rand_next *= _kp_region_start; // KP区域起始地址
    _rand_next *= _kp_region_end;   // KP区域结束地址
    if (*(uint64_t *)compile_time) _rand_next *= *(uint64_t *)compile_time;  // 编译时间
    if (*(uint64_t *)(superkey)) _rand_next *= *(uint64_t *)(superkey);      // 超级密钥
    if (*(uint64_t *)(root_superkey)) _rand_next *= *(uint64_t *)(root_superkey); // 根密钥

    enable_root_key = false;  // 默认禁用根密钥验证

    // 如果没有预设超级密钥，生成随机密钥
    if (lib_strnlen(superkey, SUPER_KEY_LEN) <= 0) {
        enable_root_key = true;  // 启用根密钥验证作为后备
        int len = SUPER_KEY_LEN > 16 ? 16 : SUPER_KEY_LEN;
        len--;  // 保留空间给字符串结束符
        for (int i = 0; i < len; ++i) {
            uint64_t rand = rand_next() % (sizeof(bstr) - 1);
            superkey[i] = bstr[rand];  // 从Base62字符集中随机选择
        }
    }
    log_boot("gen rand key: %s\n", superkey);  // 记录生成的密钥

    patch_config = &start_preset.patch_config;  // 设置补丁配置指针

    // 重定位补丁配置中的所有指针（加上内核虚拟地址偏移）
    for (uintptr_t addr = (uint64_t)patch_config; addr < (uintptr_t)patch_config + PATCH_CONFIG_LEN;
         addr += sizeof(uintptr_t)) {
        uintptr_t *p = (uintptr_t *)addr;
        if (*p) *p += kernel_va;  // 只重定位非空指针
    }

    dsb(ish);  // 内存屏障确保所有写入完成
}