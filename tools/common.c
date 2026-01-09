/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2024 bmax121. All Rights Reserved.
 */

// 工具通用函数库 - 提供ARM64指令处理、文件操作等辅助功能

#include "common.h"
#include "order.h"

bool log_enable = false;  // 日志输出开关

// 检查是否可以使用B指令进行跳转（ARM64分支指令范围检查）
int can_b_imm(uint64_t from, uint64_t to)
{
    // B指令：最大跳转范围为128M（±128MB）
    uint32_t imm26 = 1 << 25 << 2;  // 26位立即数，左移2位（4字节对齐）
    return (to >= from && to - from <= imm26) || (from >= to && from - to <= imm26);
}

// 生成ARM64的B（分支）指令
int b(uint32_t *buf, uint64_t from, uint64_t to)
{
    if (can_b_imm(from, to)) {
        // 构造B指令：31-26位为操作码0b000101，25-0位为相对偏移
        buf[0] = 0x14000000u | (((to - from) & 0x0FFFFFFFu) >> 2u);
        return 4;  // 返回指令长度（4字节）
    }
    return 0;  // 跳转距离超出范围
}

// 重定位分支函数 - 处理函数中的分支指令重定位
int32_t relo_branch_func(const char *img, int32_t func_offset)
{
    uint32_t inst = *(uint32_t *)(img + func_offset);  // 读取指令
    int32_t relo_offset = func_offset;
    
    if (INSN_IS_B(inst)) {  // 如果是B指令
        uint64_t imm26 = bits32(inst, 25, 0);         // 提取26位立即数
        uint64_t imm64 = sign64_extend(imm26 << 2u, 28u);  // 符号扩展到64位
        relo_offset = func_offset + (int32_t)imm64;    // 计算目标地址
        tools_logi("relocate branch function 0x%x to 0x%x\n", func_offset, relo_offset);
    }
    return relo_offset;
}

// 按对齐方式读取文件内容
void read_file_align(const char *path, char **con, int *out_len, int align)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) tools_log_errno_exit("open file %s\n", path);
    
    fseek(fp, 0, SEEK_END);
    int len = (int)ftell(fp);    // 获取文件大小
    fseek(fp, 0, SEEK_SET);
    
    int align_len = (int)align_ceil(len, align);  // 计算对齐后的大小
    char *buf = (char *)malloc(align_len);
    memset(buf + len, 0, align_len - len);        // 填充对齐空间为0
    int readlen = fread(buf, 1, len, fp);         // 读取文件内容
    if (readlen != len) tools_log_errno_exit("read file %s\n", path);
    fclose(fp);
    *con = buf;
    *out_len = align_len;
}

void write_file(const char *path, const char *con, int len, bool append)
{
    FILE *fout = fopen(path, append ? "ab" : "wb");
    if (!fout) tools_log_errno_exit("open file %s\n", path);
    int writelen = fwrite(con, 1, len, fout);
    if (writelen != len) tools_log_errno_exit("write file %s\n", path);
    fclose(fout);
}

int64_t int_unpack(void *ptr, int32_t size, bool is_be)
{
    bool swp = is_be ^ is_be();
    int64_t res64;
    int32_t res32;
    int16_t res16;
    switch (size) {
    case 8:
        res64 = *(int64_t *)ptr;
        return swp ? i64swp(res64) : res64;
    case 4:
        res32 = *(int32_t *)ptr;
        return swp ? i32swp(res32) : res32;
    case 2:
        res16 = *(int16_t *)ptr;
        return swp ? i16swp(res16) : res16;
    default:
        return *(int8_t *)ptr;
    }
}

uint64_t uint_unpack(void *ptr, int32_t size, bool is_be)
{
    bool swp = is_be ^ is_be();
    uint64_t res64;
    uint32_t res32;
    uint16_t res16;
    switch (size) {
    case 8:
        res64 = *(uint64_t *)ptr;
        return swp ? u64swp(res64) : res64;
    case 4:
        res32 = *(uint32_t *)ptr;
        return swp ? u32swp(res32) : res32;
    case 2:
        res16 = *(uint16_t *)ptr;
        return swp ? u16swp(res16) : res16;
    default:
        return *(uint8_t *)ptr;
    }
}
