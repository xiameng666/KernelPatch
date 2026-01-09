/* 内核符号表解析工具 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#define _GNU_SOURCE
#define __USE_GNU

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "kallsym.h"
#include "order.h"
#include "insn.h"
#include "common.h"

// 内核配置段标识符
#define IKCFG_ST "IKCFG_ST"
#define IKCFG_ED "IKCFG_ED"
#include "zlib.h"

#ifdef _WIN32
#include <string.h>
/**
 * @brief Windows平台下的memmem函数实现
 * @details 在大缓冲区中搜索小缓冲区的第一次出现
 * @param haystack 大缓冲区指针
 * @param haystack_len 大缓冲区长度
 * @param needle 待搜索的小缓冲区指针
 * @param needle_len 小缓冲区长度
 * @return 找到的位置指针，未找到返回NULL
 */
static void *memmem(const void *haystack, size_t haystack_len, const void *const needle, const size_t needle_len)
{
    if (haystack == NULL) return NULL;
    if (haystack_len == 0) return NULL;
    if (needle == NULL) return NULL;
    if (needle_len == 0) return NULL;

    // 在大缓冲区中逐字节搜索
    for (const char *h = haystack; haystack_len >= needle_len; ++h, --haystack_len) {
        if (!memcmp(h, needle, needle_len)) {
            return (void *)h;
        }
    }
    return NULL;
}
#endif

/**
 * @brief 查找Linux banner字符串
 * @details 在内核镜像中搜索Linux版本信息字符串
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 找到的banner数量
 */
static int find_linux_banner(kallsym_t *info, char *img, int32_t imglen)
{
    /*
    Linux banner示例：
    "Linux version 4.9.270-g862f51bac900-ab7613625 (android-build@abfarm-east4-101)
    (Android (7284624, based on r416183b) clang version 12.0.5
    (https://android.googlesource.com/toolchain/llvm-project
    c935d99d7cf2016289302412d708641d52d2f7ee)) #0 SMP PREEMPT Thu Aug 5 07:04:42
    UTC 2021"
    */
    char linux_banner_prefix[] = "Linux version ";
    size_t prefix_len = strlen(linux_banner_prefix);

    char *imgend = img + imglen;
    char *banner = (char *)img;
    info->banner_num = 0;
    while ((banner = (char *)memmem(banner + 1, imgend - banner - 1, linux_banner_prefix, prefix_len)) != NULL) {
        if (isdigit(*(banner + prefix_len)) && *(banner + prefix_len + 1) == '.') {
            info->linux_banner_offset[info->banner_num++] = (int32_t)(banner - img);
            tools_logi("linux_banner %d: %s", info->banner_num, banner);
            tools_logi("linux_banner offset: 0x%lx\n", banner - img);
        }
    }
    banner = img + info->linux_banner_offset[info->banner_num - 1];

    char *uts_release_start = banner + prefix_len;
    char *space = strchr(banner + prefix_len, ' ');

    char *dot = NULL;

    // VERSION
    info->version.major = (uint8_t)strtoul(uts_release_start, &dot, 10);
    // PATCHLEVEL
    info->version.minor = (uint8_t)strtoul(dot + 1, &dot, 10);
    // SUBLEVEL
    int32_t patch = (int32_t)strtoul(dot + 1, &dot, 10);
    info->version.patch = patch <= 256 ? patch : 255;

    tools_logi("kernel version major: %d, minor: %d, patch: %d\n", info->version.major, info->version.minor,
               info->version.patch);
    return 0;
}

/**
 * @brief 转储内核配置信息
 * @details 从内核镜像中提取CONFIG_IKCONFIG配置信息
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功
 */
static int dump_kernel_config(kallsym_t *info, char *img, int32_t imglen)
{
    // TODO: 实现内核配置转储
    /*
    内核配置信息
    当CONFIG_IKCONFIG启用时
    以GZip格式归档在内置内核中的魔法字符串'IKCFG_ST'和'IKCFG_ED'之间
    */
    tools_logw("not implemented\n");
    return 0;
}

/**
 * @brief 查找token表
 * @details 在内核镜像中定位kallsyms_token_table的位置
 *          通过搜索数字和字母序列来确定token表的起始位置
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，-1表示失败
 */
static int find_token_table(kallsym_t *info, char *img, int32_t imglen)
{
    // 构造数字符号模式：'0\x00','1\x00',...'9\x00'
    char nums_syms[20] = { '\0' };
    for (int32_t i = 0; i < 10; i++)
        nums_syms[i * 2] = '0' + i;

    // 只检查前10个字母，不是所有字母都保证出现
    // 实际上，前面的数字也可能不总是出现
    char letters_syms[20] = { '\0' };
    for (int32_t i = 0; i < 10; i++)
        letters_syms[i * 2] = 'a' + i;

    char *pos = img;
    char *num_start = NULL;
    char *imgend = img + imglen;
    
    // 搜索数字序列的起始位置
    for (; pos < imgend; pos = num_start + 1) {
        num_start = (char *)memmem(pos, imgend - pos, nums_syms, sizeof(nums_syms));
        if (!num_start) {
            tools_loge("find token_table error\n");
            return -1;
        }
        char *num_end = num_start + sizeof(nums_syms);
        if (!*num_end || !*(num_end + 1)) continue;  // 检查结束标记

        // 向前搜索到字母序列的开始
        char *letter = num_end;
        for (int32_t i = 0; letter < imgend && i < 'a' - '9' - 1; letter++) {
            if (!*letter) i++;  // 计算空字符数量
        }
        // 验证字母序列的正确性
        if (letter != (char *)memmem(letter, sizeof(letters_syms), letters_syms, sizeof(letters_syms))) continue;
        break;
    }

    // 向后回退到token表的真正起始位置
    pos = num_start;
    for (int32_t i = 0; pos > img && i < '0' + 1; pos--) {
        if (!*pos) i++;  // 找到足够的空字符分隔符
    }
    int32_t offset = pos + 2 - img;

    // 按4字节对齐
    offset = align_ceil(offset, 4);

    info->kallsyms_token_table_offset = offset;

    tools_logi("kallsyms_token_table offset: 0x%08x\n", offset);

    // 重建token表指针数组
    pos = img + info->kallsyms_token_table_offset;
    for (int32_t i = 0; i < KSYM_TOKEN_NUMS; i++) {
        info->kallsyms_token_table[i] = pos;  // 存储每个token的起始地址
        while (*(pos++)) {  // 跳过当前token字符串
        };
    }
    
    return 0;
}

/**
 * @brief 查找token索引表
 * @details 根据token表内容构建索引表，并在内核镜像中定位它
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，-1表示失败
 */
static int find_token_index(kallsym_t *info, char *img, int32_t imglen)
{
    uint16_t le_index[KSYM_TOKEN_NUMS] = { 0 };  // 小端序索引数组
    uint16_t be_index[KSYM_TOKEN_NUMS] = { 0 };  // 大端序索引数组

    int32_t start = info->kallsyms_token_table_offset;
    int32_t offset = start;

    // 根据kallsyms_token_table构建kallsyms_token_index
    for (int32_t i = 0; i < KSYM_TOKEN_NUMS; i++) {
        uint16_t token_index = offset - start;
        le_index[i] = u16le(token_index);  // 转换为小端序
        be_index[i] = u16be(token_index);  // 转换为大端序
        // 跳过当前token字符串
        while (img[offset++]) {
        };
    }
    
    // 在内核镜像中查找kallsyms_token_index
    char *lepos = (char *)memmem(img, imglen, le_index, sizeof(le_index));
    char *bepos = (char *)memmem(img, imglen, be_index, sizeof(be_index));

    if (!lepos && !bepos) {
        tools_loge("kallsyms_token_index error\n");
        return -1;
    }
    tools_logi("endian: %s\n", lepos ? "little" : "big");

    char *pos = lepos ? lepos : bepos;
    info->is_be = lepos ? 0 : 1;  // 确定字节序

    info->kallsyms_token_index_offset = pos - img;

    tools_logi("kallsyms_token_index offset: 0x%08x\n", info->kallsyms_token_index_offset);
    return 0;
}

/**
 * @brief 获取markers表元素大小
 * @details 根据内核版本确定markers表中每个元素的字节大小
 * @param info kallsym信息结构体
 * @return 元素大小（4或8字节）
 */
static int get_markers_elem_size(kallsym_t *info)
{
    if (info->kallsyms_markers_elem_size) return info->kallsyms_markers_elem_size;

    int32_t elem_size = info->asm_long_size;
    // 4.20版本之前使用指针大小
    if (info->version.major < 4 || (info->version.major == 4 && info->version.minor < 20))
        elem_size = info->asm_PTR_size;

    return elem_size;
}

/**
 * @brief 获取符号数量字段的元素大小
 * @details 与kallsyms_markers相同的大小规则
 * @param info kallsym信息结构体
 * @return 元素大小
 */
static int get_num_syms_elem_size(kallsym_t *info)
{
    // 与kallsyms_markers相同
    int32_t elem_size = info->asm_long_size;
    if (info->version.major < 4 || (info->version.major == 4 && info->version.minor < 20))
        elem_size = info->asm_PTR_size;
    return elem_size;
}

/**
 * @brief 获取地址表元素大小
 * @details 地址表元素大小等于指针大小
 * @param info kallsym信息结构体
 * @return 指针大小
 */
static inline int get_addresses_elem_size(kallsym_t *info)
{
    return info->asm_PTR_size;
}

/**
 * @brief 获取偏移表元素大小
 * @details 偏移表元素大小等于long类型大小
 * @param info kallsym信息结构体
 * @return long类型大小
 */
static inline int get_offsets_elem_size(kallsym_t *info)
{
    return info->asm_long_size;
}

/**
 * @brief 尝试查找ARM64重定位表
 * @details 在内核镜像中查找并处理ARM64架构的重定位信息
 *          用于确定内核基地址并应用重定位修复
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，-1表示失败
 */
static int try_find_arm64_relo_table(kallsym_t *info, char *img, int32_t imglen)
{
    if (!info->try_relo) return 0;  // 如果不尝试重定位则直接返回

    uint64_t min_va = ELF64_KERNEL_MIN_VA;  // 内核最小虚拟地址
    uint64_t max_va = ELF64_KERNEL_MAX_VA;  // 内核最大虚拟地址
    uint64_t kernel_va = max_va;            // 候选内核虚拟地址
    int32_t cand = 0;                       // 当前搜索位置
    int rela_num = 0;                       // 重定位条目数量
    
    // 搜索重定位表条目
    while (cand < imglen - 24) {
        // 解析重定位条目的三个字段
        uint64_t r_offset = uint_unpack(img + cand, 8, info->is_be);      // 重定位偏移
        uint64_t r_info = uint_unpack(img + cand + 8, 8, info->is_be);    // 重定位信息
        uint64_t r_addend = uint_unpack(img + cand + 16, 8, info->is_be); // 重定位加数
        
        // 检查是否为有效的ARM64重定位条目
        if ((r_offset & 0xffff000000000000) == 0xffff000000000000 && r_info == 0x403) {
            // 更新内核基地址候选值
            if (!(r_addend & 0xfff) && r_addend >= min_va && r_addend < kernel_va) kernel_va = r_addend;
            cand += 24;
            rela_num++;
        } else if (rela_num && !r_offset && !r_info && !r_addend) {
            // 空条目，继续计数
            cand += 24;
            rela_num++;
        } else {
            // 如果已找到足够的重定位条目则退出
            if (rela_num >= ARM64_RELO_MIN_NUM) break;
            cand += 8;
            rela_num = 0;
            kernel_va = max_va;
        }
    }

    // 确定内核基地址
    if (info->kernel_base) {
        tools_logi("arm64 relocation kernel_va: 0x%" PRIx64 ", try: %" PRIx64 "\n", kernel_va, info->kernel_base);
        kernel_va = info->kernel_base;  // 使用预设的内核基地址
    } else {
        info->kernel_base = kernel_va;  // 保存检测到的内核基地址
        tools_logi("arm64 relocation kernel_va: 0x%" PRIx64 "\n", kernel_va);
    }

    // 确定重定位表的精确范围
    int32_t cand_start = cand - 24 * rela_num;  // 重定位表起始位置
    int32_t cand_end = cand - 24;               // 重定位表结束位置
    
    // 向前搜索到最后一个非空的重定位条目
    while (1) {
        if (*(uint64_t *)(img + cand_end) && *(uint64_t *)(img + cand_end + 8) && *(uint64_t *)(img + cand_end + 16))
            break;  // 找到非空条目
        cand_end -= 24;
    }
    cand_end += 24;

    rela_num = (cand_end - cand_start) / 24;  // 重新计算重定位条目数量
    if (rela_num < ARM64_RELO_MIN_NUM) {
        tools_logw("can't find arm64 relocation table\n");
        return 0;
    }

    tools_logi("arm64 relocation table range: [0x%08x, 0x%08x), count: 0x%08x\n", cand_start, cand_end, rela_num);

    // 应用重定位
    int32_t max_offset = imglen - 8;    // 最大偏移限制
    int32_t apply_num = 0;              // 应用的重定位数量
    
    for (cand = cand_start; cand < cand_end; cand += 24) {
        // 解析重定位条目
        uint64_t r_offset = uint_unpack(img + cand, 8, info->is_be);
        uint64_t r_info = uint_unpack(img + cand + 8, 8, info->is_be);
        uint64_t r_addend = uint_unpack(img + cand + 16, 8, info->is_be);
        
        if (!r_offset && !r_info && !r_addend) continue;  // 跳过空条目
        
        // 检查重定位偏移的有效性
        if (r_offset <= kernel_va || r_offset >= max_va - imglen) {
            // tools_logw("warn ignore arm64 relocation r_offset: 0x%08lx at 0x%08x\n", r_offset, cand);
            continue;
        }

        int32_t offset = r_offset - kernel_va;  // 计算在镜像中的偏移
        if (offset < 0 || offset >= max_offset) {
            tools_logw("bad rela offset: 0x%" PRIx64 "\n", r_offset);
            info->try_relo = 0;
            return -1;
        }

        // 应用重定位修复
        uint64_t value = uint_unpack(img + offset, 8, info->is_be);
        if (value == r_addend) continue;  // 已经修复过的跳过
        *(uint64_t *)(img + offset) = value + r_addend;  // 应用重定位
        apply_num++;
    }
    if (apply_num) apply_num--;  // 调整计数
    tools_logi("apply 0x%08x relocation entries\n", apply_num);

    if (apply_num) info->relo_applied = 1;  // 标记重定位已应用

#if 0
#include <stdio.h>
    FILE *frelo = fopen("./kernel.relo", "wb+");
    int w_len = fwrite(img, 1, imglen, frelo);
    tools_logi("===== write relo kernel image: %d ====\n", w_len);
    fclose(frelo);
#endif

    return 0;
}

/**
 * @brief 查找大致的地址表位置
 * @details 在内核镜像中定位kallsyms_addresses表的大致位置
 *          通过寻找连续递增的内核地址序列来确定
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，-1表示失败
 */
static int find_approx_addresses(kallsym_t *info, char *img, int32_t imglen)
{
    int32_t sym_num = 0;                    // 符号计数
    int32_t elem_size = info->asm_PTR_size; // 元素大小等于指针大小
    uint64_t prev_offset = 0;               // 前一个地址
    int32_t cand = 0;                       // 候选位置

    // 搜索连续递增的地址序列
    for (; cand < imglen - KSYM_MIN_NEQ_SYMS * elem_size; cand += elem_size) {
        uint64_t address = uint_unpack(img + cand, elem_size, info->is_be);
        
        if (!sym_num) { // 第一个地址
            if (address & 0xff) continue;  // 地址应该对齐
            
            // 检查地址范围的有效性
            if (elem_size == 4 && (address & 0xff800000) != 0xff800000) continue;  // 32位内核地址检查
            if (elem_size == 8 && (address & 0xffff000000000000) != 0xffff000000000000) continue;  // 64位内核地址检查
            
            prev_offset = address;
            sym_num++;
            continue;
        }
        
        // 检查地址是否递增
        if (address >= prev_offset) {
            prev_offset = address;
            if (sym_num++ >= KSYM_MIN_NEQ_SYMS) break;  // 找到足够的符号
        } else {
            // 重新开始搜索
            prev_offset = 0;
            sym_num = 0;
        }
    }
    
    if (sym_num < KSYM_MIN_NEQ_SYMS) {
        tools_loge("find approximate kallsyms_addresses error\n");
        return -1;
    }

    cand -= KSYM_MIN_NEQ_SYMS * elem_size;  // 回退到起始位置
    int32_t approx_offset = cand;
    info->_approx_addresses_or_offsets_offset = approx_offset;

    // 找到大致的地址表结束位置
    prev_offset = 0;
    for (; cand < imglen; cand += elem_size) {
        uint64_t offset = uint_unpack(img + cand, elem_size, info->is_be);
        if (offset < prev_offset) break;  // 不再递增则结束
        prev_offset = offset;
    }
    
    // 结束位置不包含在内
    info->_approx_addresses_or_offsets_end = cand;
    info->has_relative_base = 0;  // 地址表不使用相对基址
    int32_t approx_num_syms = (cand - approx_offset) / elem_size;
    info->_approx_addresses_or_offsets_num = approx_num_syms;
    
    tools_logi("approximate kallsyms_addresses range: [0x%08x, 0x%08x) "
               "count: 0x%08x\n",
               approx_offset, cand, approx_num_syms);

    // 如果应用了重定位，可能会有不匹配情况
    if (info->relo_applied) {
        tools_logw("mismatch relo applied, subsequent operations may be undefined\n");
    }

    return 0;
}

/**
 * @brief 查找大致的偏移表位置
 * @details 在内核镜像中定位kallsyms_offsets表的大致位置
 *          通过寻找连续递增的相对偏移序列来确定
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，-1表示失败
 */
static int find_approx_offsets(kallsym_t *info, char *img, int32_t imglen)
{
    int32_t sym_num = 0;                    // 符号计数
    int32_t elem_size = info->asm_long_size; // 元素大小等于long大小
    int64_t prev_offset = 0;                // 前一个偏移
    int32_t cand = 0;                       // 候选位置
    int32_t MAX_ZERO_OFFSET_NUM = 10;       // 最大零偏移数量
    int32_t zero_offset_num = 0;            // 零偏移计数
    
    // 搜索连续递增的偏移序列
    for (; cand < imglen - KSYM_MIN_NEQ_SYMS * elem_size; cand += elem_size) {
        int64_t offset = int_unpack(img + cand, elem_size, info->is_be);
        
        if (offset == prev_offset) { 
            // 零偏移，继续搜索
            continue;
        } else if (offset > prev_offset) {
            prev_offset = offset;
            if (sym_num++ >= KSYM_MIN_NEQ_SYMS) break;  // 找到足够的符号
        } else {
            // 重新开始搜索
            prev_offset = 0;
            sym_num = 0;
        }
    }
    
    if (sym_num < KSYM_MIN_NEQ_SYMS) {
        tools_logw("find approximate kallsyms_offsets error\n");
        return -1;
    }
    
    cand -= KSYM_MIN_NEQ_SYMS * elem_size;  // 回退到起始位置
    
    // 向前查找零偏移的起始点
    for (;; cand -= elem_size)
        if (!int_unpack(img + cand, elem_size, info->is_be)) break;  // 找到零偏移
    
    for (;; cand -= elem_size) {
        if (int_unpack(img + cand, elem_size, info->is_be)) break;   // 找到非零偏移
        if (zero_offset_num++ >= MAX_ZERO_OFFSET_NUM) break;         // 限制搜索范围
    }
    cand += elem_size;
    
    int32_t approx_offset = cand;
    info->_approx_addresses_or_offsets_offset = approx_offset;

    // 找到大致的偏移表结束位置
    prev_offset = 0;
    for (; cand < imglen; cand += elem_size) {
        int64_t offset = int_unpack(img + cand, elem_size, info->is_be);
        if (offset < prev_offset) break;  // 不再递增则结束
        prev_offset = offset;
    }
    
    // 最后一个符号可能不是4K对齐的
    // 结束位置不包含在内
    int32_t end = cand;
    info->_approx_addresses_or_offsets_end = end;
    info->has_relative_base = 1;  // 偏移表使用相对基址
    int32_t approx_num_syms = (end - approx_offset) / elem_size;
    info->_approx_addresses_or_offsets_num = approx_num_syms;
    
    // 真实的区间包含在这个近似区间内
    tools_logi("approximate kallsyms_offsets range: [0x%08x, 0x%08x) "
               "count: 0x%08x\n",
               approx_offset, end, approx_num_syms);
    return 0;
}

/**
 * @brief 查找大致的地址或偏移表
 * @details 根据内核版本选择查找地址表还是偏移表
 *          4.6版本以后可能包含kallsyms_relative_base
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，非0表示失败
 */
static int32_t find_approx_addresses_or_offset(kallsym_t *info, char *img, int32_t imglen)
{
    int32_t ret = 0;
    
    // 4.6版本以后可能有kallsyms_relative_base
    if (info->version.major > 4 || (info->version.major == 4 && info->version.minor >= 6)) {
        ret = find_approx_offsets(info, img, imglen);  // 尝试查找偏移表
        if (!ret) return 0;  // 成功找到偏移表
    }
    
    ret = find_approx_addresses(info, img, imglen);  // 查找地址表
    return ret;
}

/**
 * @brief 查找符号数量字段
 * @details 在内核镜像中定位kallsyms_num_syms字段
 *          通过与大致符号数量进行比较来确定
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功
 */
static int find_num_syms(kallsym_t *info, char *img, int32_t imglen)
{
#define NSYMS_MAX_GAP 10  // 符号数量的最大差值

    int32_t approx_end = info->kallsyms_names_offset;  // 从names表开始向前搜索
    // int32_t num_syms_elem_size = get_num_syms_elem_size(info);
    int32_t num_syms_elem_size = 4;  // 符号数量字段大小为4字节

    int32_t approx_num_syms = info->_approx_addresses_or_offsets_num;  // 大致的符号数量

    // 在names表前4KB范围内搜索符号数量字段
    for (int32_t cand = approx_end; cand > approx_end - 4096; cand -= num_syms_elem_size) {
        int nsyms = (int)int_unpack(img + cand, num_syms_elem_size, info->is_be);
        if (!nsyms) continue;  // 跳过零值
        
        // 检查符号数量是否在合理范围内
        if (approx_num_syms > nsyms && approx_num_syms - nsyms > NSYMS_MAX_GAP) continue;
        if (nsyms > approx_num_syms && nsyms - approx_num_syms > NSYMS_MAX_GAP) continue;
        
        // 找到匹配的符号数量
        info->kallsyms_num_syms = nsyms;
        info->kallsyms_num_syms_offset = cand;
        break;
    }

    if (!info->kallsyms_num_syms_offset || !info->kallsyms_num_syms) {
        // 如果没有找到，使用估算值
        info->kallsyms_num_syms = approx_num_syms - NSYMS_MAX_GAP;
        tools_logw("can't find kallsyms_num_syms, try: 0x%08x\n", info->kallsyms_num_syms);
    } else {
        tools_logi("kallsyms_num_syms offset: 0x%08x, value: 0x%08x\n", info->kallsyms_num_syms_offset,
                   info->kallsyms_num_syms);
    }
    return 0;
}

/**
 * @brief 内部函数：查找markers表
 * @details 在token表之前查找kallsyms_markers表
 *          markers表记录每256个符号的偏移量，用于快速索引
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @param elem_size 每个marker元素的大小
 * @return 0表示成功，-1表示失败
 */
static int find_markers_internal(kallsym_t *info, char *img, int32_t imglen, int32_t elem_size)
{
    int32_t cand = info->kallsyms_token_table_offset;  // 从token表开始向前搜索

    int64_t marker, last_marker = imglen;  // 当前和上一个marker值
    int count = 0;                         // marker计数
    
    // 从token表向前搜索markers表
    while (cand > 0x10000) {
        marker = int_unpack(img + cand, elem_size, info->is_be);
        
        // markers应该是递减的序列，最后以0结尾
        if (last_marker > marker) {
            count++;
            if (!marker && count > KSYM_MIN_MARKER) break;  // 找到足够的markers且以0结尾
        } else {
            // 重新开始计数
            count = 0;
            last_marker = imglen;
        }

        last_marker = marker;
        cand -= elem_size;  // 向前移动一个元素
    }

    if (count < KSYM_MIN_MARKER) {
        tools_logw("find kallsyms_markers error\n");
        return -1;
    }

    int32_t marker_end = cand + count * elem_size + elem_size;  // markers表结束位置
    info->kallsyms_markers_offset = cand;                       // markers表起始位置
    info->_marker_num = count;                                  // marker数量
    info->kallsyms_markers_elem_size = elem_size;               // 元素大小

    tools_logi("kallsyms_markers range: [0x%08x, 0x%08x), count: 0x%08x\n", cand, marker_end, count);
    return 0;
}

/**
 * @brief 查找markers表的包装函数
 * @details 尝试使用不同的元素大小查找markers表
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，非0表示失败
 */
static int find_markers(kallsym_t *info, char *img, int32_t imglen)
{
    int32_t elem_size = get_markers_elem_size(info);  // 获取推荐的元素大小
    int rc = find_markers_internal(info, img, imglen, elem_size);
    
    // 如果8字节失败，尝试4字节
    if (rc && elem_size == 8) {
        return find_markers_internal(info, img, imglen, 4);
    }
    return rc;
}

static int decompress_symbol_name(kallsym_t *info, char *img, int32_t *pos_to_next, char *out_type, char *out_symbol)
{
    int32_t pos = *pos_to_next;
    int32_t len = *(uint8_t *)(img + pos++);
    if (len > 0x7F) len = (len & 0x7F) + (*(uint8_t *)(img + pos++) << 7);
    if (!len || len >= KSYM_SYMBOL_LEN) return -1;

    *pos_to_next = pos + len;
    for (int32_t i = 0; i < len; i++) {
        int32_t tokidx = *(uint8_t *)(img + pos + i);
        char *token = info->kallsyms_token_table[tokidx];
        if (!i) { // first character, symbol type
            if (out_type) *out_type = *token;
            token++;
        }
        if (out_symbol) strcat(out_symbol, token);
    }
    return 0;
}

/**
 * @brief 检查指定位置是否为目标符号名称
 * @details 解压指定位置的符号名称并与目标符号比较
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param pos 检查位置
 * @param symbol 目标符号名称
 * @return 1表示匹配，0表示不匹配
 */
static int is_symbol_name_pos(kallsym_t *info, char *img, int32_t pos, char *symbol)
{
    int32_t len = *(uint8_t *)(img + pos++);                     // 读取长度
    if (len > 0x7F) len = (len & 0x7F) + (*(uint8_t *)(img + pos++) << 7);  // 处理长长度编码
    if (!len || len >= KSYM_SYMBOL_LEN) return 0;                // 长度检查
    
    int32_t symidx = 0;  // 符号字符索引
    
    // 逐token解压并比较
    for (int32_t i = 0; i < len; i++) {
        int32_t tokidx = *(uint8_t *)(img + pos + i);             // 获取token索引
        char *token = info->kallsyms_token_table[tokidx];         // 查找token字符串
        if (!i) token++;                                          // 跳过第一个token的符号类型字符
        
        int32_t toklen = strlen(token);
        if (strncmp(symbol + symidx, token, toklen)) break;       // 比较字符串
        symidx += toklen;
    }
    
    return (int32_t)strlen(symbol) == symidx;  // 检查是否完全匹配
}

/**
 * @brief 查找符号名称表
 * @details 通过测试已知符号名称来定位kallsyms_names表的起始位置
 *          使用标记表来验证找到的符号名称表的正确性
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，-1表示失败
 */
static int find_names(kallsym_t *info, char *img, int32_t imglen)
{
    int32_t marker_elem_size = get_markers_elem_size(info);  // 标记表元素大小
    // int32_t cand = info->_approx_addresses_or_offsets_offset;
    int32_t cand = 0x4000;                                   // 从16KB位置开始搜索
    int32_t test_marker_num = -1;                            // 测试的标记索引
    
    // 在标记表之前搜索符号名称表
    for (; cand < info->kallsyms_markers_offset; cand++) {
        int32_t pos = cand;
        test_marker_num = KSYM_FIND_NAMES_USED_MARKER;  // 检查n*256个符号
        
        // 逐个解析符号名称，验证与标记表的一致性
        for (int32_t i = 0;; i++) {
            int32_t len = *(uint8_t *)(img + pos++);
            if (len > 0x7F) len = (len & 0x7F) + (*(uint8_t *)(img + pos++) << 7);
            if (!len || len >= KSYM_SYMBOL_LEN) break;  // 无效长度
            pos += len;
            
            if (pos >= info->kallsyms_markers_offset) break;  // 超出标记表范围

            // 每256个符号检查一次标记表的一致性
            if (i && (i & 0xFF) == 0xFF) {
                int32_t mark_len = int_unpack(img + info->kallsyms_markers_offset + ((i >> 8) + 1) * marker_elem_size,
                                              marker_elem_size, info->is_be);
                if (pos - cand != mark_len) break;  // 标记不匹配
                if (!--test_marker_num) break;      // 测试完成
            }
        }
        
        if (!test_marker_num) break;  // 找到有效的符号名称表
    }
    
    if (test_marker_num) {
        tools_loge("find kallsyms_names error\n");
        return -1;
    }
    
    info->kallsyms_names_offset = cand;
    tools_logi("kallsyms_names offset: 0x%08x\n", cand);

#if 0
    // 打印所有符号用于测试
    // 如果CONFIG_KALLSYMS=y且CONFIG_KALLSYMS_ALL=n
    // 内核镜像中的kallsyms_names表会被截断，只导出函数
    int32_t pos = info->kallsyms_names_offset;
    int32_t index = 0;
    char symbol[KSYM_SYMBOL_LEN] = { '\0' };
    while (pos < info->kallsyms_markers_offset) {
        memset(symbol, 0, sizeof(symbol));
        int32_t ret = decompress_symbol_name(info, img, &pos, NULL, symbol);
        if (ret) break;
        tools_logi("index: %d, %08x, symbol: %s\n", index, pos, symbol);
        index++;
    }
#endif
    return 0;
}

/**
 * @brief ARM64架构下验证pid_vnr函数的实现类型
 * @details 通过分析汇编指令确定获取当前进程PID的实现方式
 *          检查是否使用SP_EL0寄存器或栈指针SP
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param offset 函数偏移位置
 * @return 0表示验证成功，-1表示失败
 */
static int arm64_verify_pid_vnr(kallsym_t *info, char *img, int32_t offset)
{
    // 检查函数开头的6条指令
    for (int i = 0; i < 6; i++) {
        int32_t insn_offset = offset + i * 4;
        uint32_t insn = uint_unpack(img + insn_offset, 4, 0);  // 获取指令
        enum aarch64_insn_encoding_class enc = aarch64_get_insn_class(insn);
        
        if (enc == AARCH64_INSN_CLS_BR_SYS) {
            // 系统寄存器访问指令
            if (aarch64_insn_extract_system_reg(insn) == AARCH64_INSN_SPCLREG_SP_EL0) {
                tools_logi("pid_vnr verfied sp_el0, insn: 0x%x\n", insn);
                info->current_type = SP_EL0;  // 使用SP_EL0寄存器
                return 0;
            }
        } else if (enc == AARCH64_INSN_CLS_DP_IMM) {
            // 立即数数据处理指令
            u32 rn = aarch64_insn_decode_register(AARCH64_INSN_REGTYPE_RN, insn);
            if (rn == AARCH64_INSN_REG_SP) {
                tools_logi("pid_vnr verfied sp, insn: 0x%x\n", insn);
                info->current_type = SP;      // 使用栈指针SP
                return 0;
            }
        }
    }
    return -1;  // 无法确定实现类型
}

/**
 * @brief 通过向量表修正地址或偏移表
 * @details 使用已知的向量表和pid_vnr符号来验证和修正符号地址/偏移的准确性
 *          向量表按11位对齐，可用作地址验证的基准
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，非0表示失败
 */
static int correct_addresses_or_offsets_by_vectors(kallsym_t *info, char *img, int32_t imglen)
{
    // vectors .align 11  (按2048字节对齐)
    // todo: tramp_vectors .align 11
    int32_t pos = info->kallsyms_names_offset;
    int32_t index = 0, vector_index = 0, pid_vnr_index = 0;
    char symbol[KSYM_SYMBOL_LEN] = { '\0' };
    
    // 在符号名称表中查找vectors和pid_vnr符号
    while (pos < info->kallsyms_markers_offset) {
        memset(symbol, 0, sizeof(symbol));
        int32_t ret = decompress_symbol_name(info, img, &pos, NULL, symbol);
        if (ret) return ret;

        if (!vector_index && !strcmp(symbol, "vectors")) {
            vector_index = index;  // 记录vectors符号的索引
        } else if (!pid_vnr_index && !strcmp(symbol, "pid_vnr")) {
            pid_vnr_index = index;  // 记录pid_vnr符号的索引
        }
        
        if (vector_index && pid_vnr_index) {
            tools_logi("names table vector index: 0x%08x, pid_vnr index: 0x%08x\n", vector_index, pid_vnr_index);
            break;
        }
        index++;
    }

    if (pos >= info->kallsyms_markers_offset) {
        tools_loge("no verify symbol in names table\n");
        return -1;
    }

    int32_t elem_size = info->has_relative_base ? get_offsets_elem_size(info) : get_addresses_elem_size(info);

    uint64_t base_cand[3] = { 0 };  // 候选基地址数组
    int base_cand_num = 1;          // 候选基地址数量

    if (!info->has_relative_base) {
        // 使用绝对地址模式
        uint64_t base = uint_unpack(img + info->_approx_addresses_or_offsets_offset, elem_size, info->is_be);
        base_cand[0] = base;
        
        // 添加其他候选基地址
        if (info->kernel_base) {
            base_cand[base_cand_num++] = info->kernel_base;
        }
        if (info->kernel_base != ELF64_KERNEL_MIN_VA) {
            base_cand[base_cand_num++] = ELF64_KERNEL_MIN_VA;
        }
    }

    // 确定搜索范围
    int32_t search_start = info->_approx_addresses_or_offsets_offset;
    int32_t search_end = info->_approx_addresses_or_offsets_end - pid_vnr_index * elem_size;

    int break_flag = 0;
    
    // 尝试不同的基地址
    for (int i = 0; i < base_cand_num; i++) {
        uint64_t base = base_cand[i];

        // 在搜索范围内查找正确的地址表位置
        for (pos = search_start; pos < search_end; pos += elem_size) {
            // 获取vectors符号的地址
            int32_t vector_offset = uint_unpack(img + pos + vector_index * elem_size, elem_size, info->is_be) - base;
            int32_t vector_next_offset =
                uint_unpack(img + pos + vector_index * elem_size + elem_size, elem_size, info->is_be) - base;
            
            // 验证vectors的对齐和大小
            if (vector_next_offset - vector_offset >= 0x600 && (vector_offset & ((1 << 11) - 1)) == 0) {
                // 获取pid_vnr符号的地址并验证
                int32_t pid_vnr_offset =
                    uint_unpack(img + pos + pid_vnr_index * elem_size, elem_size, info->is_be) - base;
                
                if (!arm64_verify_pid_vnr(info, img, pid_vnr_offset)) {
                    tools_logi("vectors index: %d, offset: 0x%08x\n", vector_index, vector_offset);
                    tools_logi("pid_vnr offset: 0x%08x\n", pid_vnr_offset);
                    info->kernel_base = base;
                    break_flag = 1;
                    break;
                }
            }
        }

        if (break_flag) break;
    }

    if (pos >= search_end) {
        tools_loge("can't locate vectors\n");
        return -1;
    }

    // 根据地址类型设置正确的偏移
    if (info->has_relative_base) {
        info->kallsyms_offsets_offset = pos;
        tools_logi("kallsyms_offsets offset: 0x%08x\n", pos);
    } else {
        info->kallsyms_addresses_offset = pos;
        tools_logi("kallsyms_addresses offset: 0x%08x\n", pos);
        tools_logi("kernel base address: 0x%08llx\n", info->kernel_base);
    }

    return 0;
}

/**
 * @brief 通过Linux横幅修正地址或偏移表
 * @details 使用已知的linux_banner符号来验证和修正符号地址/偏移的准确性
 *          linux_banner包含内核版本信息，是可靠的验证标志
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，非0表示失败
 */
static int correct_addresses_or_offsets_by_banner(kallsym_t *info, char *img, int32_t imglen)
{
    int32_t pos = info->kallsyms_names_offset;
    int32_t index = 0;
    char symbol[KSYM_SYMBOL_LEN] = { '\0' };

    // 在符号名称表中查找linux_banner符号
    while (pos < info->kallsyms_markers_offset) {
        memset(symbol, 0, sizeof(symbol));
        int32_t ret = decompress_symbol_name(info, img, &pos, NULL, symbol);
        if (ret) return ret;

        if (!strcmp(symbol, "linux_banner")) {
            tools_logi("names table linux_banner index: 0x%08x\n", index);
            break;
        }
        if (!strcmp(symbol, "pid_vnr")) {
            // 备用验证符号
        }
        index++;
    }

    if (pos >= info->kallsyms_markers_offset) {
        tools_loge("no linux_banner in names table\n");
        return -1;
    }
    info->symbol_banner_idx = -1;

    // 查找正确的地址或偏移
    for (int i = 0; i < info->banner_num; i++) {
        int32_t target_offset = info->linux_banner_offset[i];  // 目标横幅偏移

        int32_t elem_size = info->has_relative_base ? get_offsets_elem_size(info) : get_addresses_elem_size(info);
        pos = info->_approx_addresses_or_offsets_offset;

        int32_t end = pos + 4096 + elem_size;
        
        // 在范围内搜索匹配的横幅偏移
        for (; pos < end; pos += elem_size) {
            uint64_t base = uint_unpack(img + pos, elem_size, info->is_be);
            int32_t offset = uint_unpack(img + pos + index * elem_size, elem_size, info->is_be) - base;
            if (offset == target_offset) break;  // 找到匹配的偏移
        }
        
        if (pos < end) {
            info->symbol_banner_idx = i;
            tools_logi("linux_banner index: %d\n", i);
            break;
        }
    }
    
    if (info->symbol_banner_idx < 0) {
        tools_loge("correct address or offsets error\n");
        return -1;
    }

    int32_t elem_size = info->has_relative_base ? get_offsets_elem_size(info) : get_addresses_elem_size(info);

    // 根据地址类型设置正确的偏移
    if (info->has_relative_base) {
        info->kallsyms_offsets_offset = pos;
        tools_logi("kallsyms_offsets offset: 0x%08x\n", pos);
    } else {
        info->kallsyms_addresses_offset = pos;
        tools_logi("kallsyms_addresses offset: 0x%08x\n", pos);
        info->kernel_base = uint_unpack(img + info->kallsyms_addresses_offset, elem_size, info->is_be);
        tools_logi("kernel base address: 0x%llx\n", info->kernel_base);
    }

    // 验证pid_vnr符号
    int32_t pid_vnr_offset = get_symbol_offset(info, img, "pid_vnr");
    if (arm64_verify_pid_vnr(info, img, pid_vnr_offset)) {
        tools_logw("pid_vnr verification failed\n");
    }

    return 0;
}

/**
 * @brief 修正地址或偏移表
 * @details 尝试不同的方法来修正和验证符号地址/偏移表的准确性
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，非0表示失败
 */
static int correct_addresses_or_offsets(kallsym_t *info, char *img, int32_t imglen)
{
    int rc = 0;
#if 1
    // 首先尝试使用横幅方法
    rc = correct_addresses_or_offsets_by_banner(info, img, imglen);
    info->is_kallsysms_all_yes = 1;  // 假设CONFIG_KALLSYMS_ALL=y
#endif
    if (rc) {
        info->is_kallsysms_all_yes = 0;  // CONFIG_KALLSYMS_ALL=n
        tools_logw("no linux_banner, CONFIG_KALLSYMS_ALL=n\n");
    }
    
    // 横幅方法失败时使用向量表方法
    if (rc) rc = correct_addresses_or_offsets_by_vectors(info, img, imglen);
    return rc;
}

/**
 * @brief 初始化ARM64架构的kallsym结构体
 * @details 设置ARM64架构特定的参数，包括数据类型大小和重定位支持
 * @param info kallsym信息结构体指针
 */
void init_arm64_kallsym_t(kallsym_t *info)
{
    memset(info, 0, sizeof(kallsym_t));
    info->is_64 = 1;            // 64位架构
    info->asm_long_size = 4;    // long类型大小为4字节
    info->asm_PTR_size = 8;     // 指针大小为8字节
    info->try_relo = 1;         // 支持重定位处理
}

/**
 * @brief 初始化未测试架构的kallsym结构体
 * @details 为其他架构设置通用的参数配置
 * @param info kallsym信息结构体指针
 * @param is_64 是否为64位架构
 */
void init_not_tested_arch_kallsym_t(kallsym_t *info, int32_t is_64)
{
    memset(info, 0, sizeof(kallsym_t));
    info->is_64 = is_64;
    info->asm_long_size = 4;    // long类型大小为4字节
    info->asm_PTR_size = 4;     // 默认指针大小为4字节
    info->try_relo = 0;         // 不支持重定位处理
    if (is_64) info->asm_PTR_size = 8;  // 64位架构指针大小为8字节
}

/**
 * @brief 重试重定位处理
 * @details 依次执行各个分析步骤来完成kallsym信息的解析
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，非0表示失败
 */
static int retry_relo(kallsym_t *info, char *img, int32_t imglen)
{
    int rc = -1;
    
    // 按顺序执行的分析函数
    static int32_t (*funcs[])(kallsym_t *, char *, int32_t) = {
        try_find_arm64_relo_table,   // ARM64重定位表处理
        find_markers,                // 查找标记表
        find_approx_addresses_or_offset, // 查找地址或偏移表
        find_names,                  // 查找符号名称表
        find_num_syms,              // 查找符号数量
        correct_addresses_or_offsets // 修正地址或偏移
    };

    for (int i = 0; i < (int)(sizeof(funcs) / sizeof(funcs[0])); i++) {
        if ((rc = funcs[i](info, img, imglen))) break;
    }

    return rc;
}

/**
 * @brief 分析内核符号表信息
 * @details 完整解析内核镜像中的kallsyms符号表结构
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @param arch 目标架构类型
 * @param is_64 是否为64位架构
 * @return 0表示成功，非0表示失败
 * 
 * 解析的符号表结构包括:
 * R kallsyms_offsets        - 符号偏移表
 * R kallsyms_relative_base  - 相对基地址
 * R kallsyms_num_syms       - 符号数量
 * R kallsyms_names          - 符号名称表
 * R kallsyms_markers        - 标记表
 * R kallsyms_token_table    - 令牌表
 * R kallsyms_token_index    - 令牌索引表
 */
int analyze_kallsym_info(kallsym_t *info, char *img, int32_t imglen, enum arch_type arch, int32_t is_64)
{
    memset(info, 0, sizeof(kallsym_t));
    info->is_64 = is_64;
    info->asm_long_size = 4;    // long类型大小
    info->asm_PTR_size = 4;     // 指针大小
    if (arch == ARM64) info->try_relo = 1;  // ARM64支持重定位
    if (is_64) info->asm_PTR_size = 8;      // 64位指针大小

    int rc = -1;
    
    // 基础分析函数，按顺序执行
    static int32_t (*base_funcs[])(kallsym_t *, char *, int32_t) = {
        find_linux_banner,   // 查找Linux横幅
        find_token_table,    // 查找令牌表
        find_token_index,    // 查找令牌索引
    };
    
    for (int i = 0; i < (int)(sizeof(base_funcs) / sizeof(base_funcs[0])); i++) {
        if ((rc = base_funcs[i](info, img, imglen))) return rc;
    }

    // 创建镜像副本用于重定位处理
    char *copied_img = (char *)malloc(imglen);
    memcpy(copied_img, img, imglen);

    // 第一次尝试
    rc = retry_relo(info, copied_img, imglen);
    if (!rc) goto out;

    // 第二次尝试（禁用重定位）
    if (!info->try_relo) {
        memcpy(copied_img, img, imglen);
        rc = retry_relo(info, copied_img, imglen);
        if (!rc) goto out;
    }

    // 第三次尝试（使用默认内核基地址）
    if (info->kernel_base != ELF64_KERNEL_MIN_VA) {
        info->kernel_base = ELF64_KERNEL_MIN_VA;
        memcpy(copied_img, img, imglen);
        rc = retry_relo(info, copied_img, imglen);
    }

out:
    memcpy(img, copied_img, imglen);  // 复制处理后的镜像
    free(copied_img);
    return rc;
}

/**
 * @brief 获取指定索引符号的偏移量
 * @details 根据符号索引从地址表或偏移表中获取对应的偏移值
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param index 符号索引
 * @return 符号偏移量
 */
int32_t get_symbol_index_offset(kallsym_t *info, char *img, int32_t index)
{
    int32_t elem_size;
    int32_t pos;
    
    if (info->has_relative_base) {
        // 使用偏移表
        elem_size = get_offsets_elem_size(info);
        pos = info->kallsyms_offsets_offset;
    } else {
        // 使用地址表
        elem_size = get_addresses_elem_size(info);
        pos = info->kallsyms_addresses_offset;
    }
    
    uint64_t target = uint_unpack(img + pos + index * elem_size, elem_size, info->is_be);
    
    if (info->has_relative_base) return target;  // 直接返回偏移
    return (int32_t)(target - info->kernel_base);  // 计算相对偏移
}

/**
 * @brief 获取符号的偏移量和大小
 * @details 通过符号名称查找对应的偏移和大小信息
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param symbol 符号名称
 * @param size 输出符号大小的指针
 * @return 符号偏移量，-1表示未找到
 */
int get_symbol_offset_and_size(kallsym_t *info, char *img, char *symbol, int32_t *size)
{
    char decomp[KSYM_SYMBOL_LEN] = { '\0' };
    char type = 0;
    *size = 0;
    char **tokens = info->kallsyms_token_table;
    int32_t pos = info->kallsyms_names_offset;
    
    // 遍历所有符号查找匹配的名称
    for (int32_t i = 0; i < info->kallsyms_num_syms; i++) {
        memset(decomp, 0, sizeof(decomp));
        decompress_symbol_name(info, img, &pos, &type, decomp);
        
        if (!strcmp(decomp, symbol)) {
            int32_t offset = get_symbol_index_offset(info, img, i);
            int32_t next_offset = offset;
            
            // 查找下一个不同偏移的符号来计算大小
            for (int32_t j = i + 1; j < info->kallsyms_num_syms; j++) {
                next_offset = get_symbol_index_offset(info, img, j);
                if (next_offset != offset) {
                    *size = next_offset - offset;
                    break;
                }
            }
            tools_logi("%s: type: %c, offset: 0x%08x, size: 0x%x\n", symbol, type, offset, *size);
            return offset;
        }
    }
    tools_logw("no symbol: %s\n", symbol);
    return -1;
}

/**
 * @brief 获取符号的偏移量
 * @details 通过符号名称查找对应的偏移量
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param symbol 符号名称
 * @return 符号偏移量，-1表示未找到
 */
int get_symbol_offset(kallsym_t *info, char *img, char *symbol)
{
    char decomp[KSYM_SYMBOL_LEN] = { '\0' };
    char type = 0;
    char **tokens = info->kallsyms_token_table;
    int32_t pos = info->kallsyms_names_offset;
    
    // 遍历所有符号查找匹配的名称
    for (int32_t i = 0; i < info->kallsyms_num_syms; i++) {
        memset(decomp, 0, sizeof(decomp));
        decompress_symbol_name(info, img, &pos, &type, decomp);
        
        if (!strcmp(decomp, symbol)) {
            int32_t offset = get_symbol_index_offset(info, img, i);
            tools_logi("%s: type: %c, offset: 0x%08x\n", symbol, type, offset);
            return offset;
        }
    }
    tools_logw("no symbol: %s\n", symbol);
    return -1;
}

/**
 * @brief 导出所有符号信息
 * @details 将所有符号的偏移、类型和名称输出到标准输出
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @return 0表示成功
 */
int dump_all_symbols(kallsym_t *info, char *img)
{
    char symbol[KSYM_SYMBOL_LEN] = { '\0' };
    char type = 0;
    char **tokens = info->kallsyms_token_table;
    int32_t pos = info->kallsyms_names_offset;
    
    // 遍历所有符号并输出
    for (int32_t i = 0; i < info->kallsyms_num_syms; i++) {
        memset(symbol, 0, sizeof(symbol));
        decompress_symbol_name(info, img, &pos, &type, symbol);
        int32_t offset = get_symbol_index_offset(info, img, i);
        fprintf(stdout, "0x%08x %c %s\n", offset, type, symbol);
    }
    return 0;
}
/**
 * @brief 解压缩数据
 * @details 将gzip压缩的数据解压并输出到标准输出
 * @param compressed_data 压缩数据指针
 * @param compressed_size 压缩数据大小
 * @return 0表示成功，-1表示失败
 */
int decompress_data(const unsigned char *compressed_data, size_t compressed_size)
{
    FILE *temp = fopen("temp.gz", "wb");
    if (!temp) {
        fprintf(stderr, "Failed to create temp file\n");
        return -1;
    }

    fwrite(compressed_data, 1, compressed_size, temp);
    fclose(temp);

    gzFile gz = gzopen("temp.gz", "rb");
    if (!gz) {
        fprintf(stderr, "Failed to open temp file for decompression\n");
        return -1;
    }

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = gzread(gz, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, bytes_read, stdout);
    }

    gzclose(gz);
    return 0;
}

/**
 * @brief 导出内核配置信息
 * @details 从内核镜像中提取并解压内核配置信息(ikconfig)
 * @param img 内核镜像数据指针
 * @param imglen 内核镜像长度
 * @return 0表示成功，1表示失败
 */
int dump_all_ikconfig(char *img, int32_t imglen)
{
    // 查找内核配置起始标记"IKCFG_ST"
    char *pos_start = memmem(img, imglen, IKCFG_ST, strlen(IKCFG_ST));
    if (pos_start == NULL) {
        fprintf(stderr, "Cannot find kernel config start (IKCFG_ST).\n");
        return 1;
    }
    size_t kcfg_start = pos_start - img + 8;

    // 查找内核配置结束标记"IKCFG_ED"
    char *pos_end = memmem(img, imglen, IKCFG_ED, strlen(IKCFG_ED));
    if (pos_end == NULL) {
        fprintf(stderr, "Cannot find kernel config end (IKCFG_ED).\n");
        return 1;
    }
    size_t kcfg_end = pos_end - img - 1;
    size_t kcfg_bytes = kcfg_end - kcfg_start + 1;

    printf("Kernel config start: %zu, end: %zu, bytes: %zu\n", kcfg_start, kcfg_end, kcfg_bytes);

    unsigned char *extracted_data = (unsigned char *)malloc(kcfg_bytes);
    if (!extracted_data) {
        fprintf(stderr, "Memory allocation for extracted data failed.\n");
        return 1;
    }

    memcpy(extracted_data, img + kcfg_start, kcfg_bytes);

    int ret = decompress_data(extracted_data, kcfg_bytes);

    free(extracted_data);

    return 0;
}

/**
 * @brief 遍历每个符号并执行回调函数
 * @details 对每个符号执行用户提供的回调函数，可用于自定义符号处理
 * @param info kallsym信息结构体
 * @param img 内核镜像数据指针
 * @param userdata 用户数据指针
 * @param fn 回调函数指针
 * @return 0表示成功，非0表示回调函数返回的错误码
 */
int on_each_symbol(kallsym_t *info, char *img, void *userdata,
                   int32_t (*fn)(int32_t index, char type, const char *symbol, int32_t offset, void *userdata))
{
    char symbol[KSYM_SYMBOL_LEN] = { '\0' };
    char type = 0;
    char **tokens = info->kallsyms_token_table;
    int32_t pos = info->kallsyms_names_offset;
    
    // 遍历所有符号并调用回调函数
    for (int32_t i = 0; i < info->kallsyms_num_syms; i++) {
        memset(symbol, 0, sizeof(symbol));
        decompress_symbol_name(info, img, &pos, &type, symbol);
        int32_t offset = get_symbol_index_offset(info, img, i);
        int rc = fn(i, type, symbol, offset, userdata);
        if (rc) return rc;  // 回调函数返回错误时退出
    }
    return 0;
}
