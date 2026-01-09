/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 * 
 * ARM64 内核镜像解析与处理工具
 * 
 * 本文件实现了 ARM64 Linux 内核镜像的解析功能，包括：
 * - 内核头部结构解析
 * - 魔数验证和类型检测
 * - UEFI/非UEFI 镜像格式支持  
 * - 内核入口点地址计算
 * - 页面大小和字节序配置提取
 * - 内核大小调整功能
 * 
 * 支持标准的 ARM64 内核镜像格式，兼容 UEFI 可引导镜像。
 */

#include "image.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "order.h"
#include "common.h"

#define EFI_MAGIC_SIG "MZ"        // UEFI 镜像魔数签名
#define KERNEL_MAGIC "ARM\x64"    // ARM64 内核魔数

/**
 * ARM64 内核镜像头部结构
 * 定义了 ARM64 Linux 内核镜像的标准头部格式
 */
typedef struct
{
    union _entry
    {
        // UEFI 可引导镜像头部
        struct _efi
        {
            uint8_t mz[4];     // "MZ" UEFI 要求的签名
            uint32_t b_insn;   // 跳转到内核入口的指令
        } efi;
        // 非 UEFI 镜像头部
        struct _nefi
        {
            uint32_t b_insn;     // 跳转到内核入口的指令
            uint32_t reserved0;  // 保留字段
        } nefi;
    } hdr;

    uint64_t kernel_offset;   // 从 RAM 起始位置的镜像加载偏移量（小端序）
    uint64_t kernel_size_le;  // 内核镜像的有效大小（小端序）
    uint64_t kernel_flag_le;  // 信息标志位（小端序）

    uint64_t reserved0;       // 保留字段
    uint64_t reserved1;       // 保留字段
    uint64_t reserved2;       // 保留字段

    char magic[4];            // 魔数 "ARM\x64"

    union _pe
    {
        uint64_t pe_offset;     // UEFI：PE 头部偏移量
        uint64_t npe_reserved;  // 非UEFI：保留字段
    } pe;
} arm64_hdr_t;

/**
 * 解析内核镜像信息
 * @param kinfo 输出的内核信息结构体
 * @param img 内核镜像数据
 * @param imglen 镜像数据长度
 * @return 成功返回0，失败退出程序
 */
int32_t get_kernel_info(kernel_info_t *kinfo, const char *img, int32_t imglen)
{
    kinfo->is_be = 0;  // 初始化为小端序

    // 将镜像数据转换为 ARM64 内核头部结构体指针
    arm64_hdr_t *khdr = (arm64_hdr_t *)img;
    // 验证内核魔数，确保这是有效的 ARM64 内核镜像
    if (strncmp(khdr->magic, KERNEL_MAGIC, strlen(KERNEL_MAGIC))) {
        tools_loge_exit("kernel image magic error: %s\n", khdr->magic);
    }

    // 检测是否为 UEFI 可引导镜像，通过检查 EFI 魔数签名
    kinfo->uefi = !strncmp((const char *)khdr->hdr.efi.mz, EFI_MAGIC_SIG, strlen(EFI_MAGIC_SIG));

    uint32_t b_primary_entry_insn;    // 主入口跳转指令
    uint32_t b_stext_insn_offset;     // stext 指令偏移量
    // 根据 UEFI 或非 UEFI 镜像类型，获取不同的跳转指令和偏移量
    if (kinfo->uefi) {
        // UEFI 镜像从 EFI 头部获取跳转指令
        b_primary_entry_insn = khdr->hdr.efi.b_insn;
        b_stext_insn_offset = 4;    // UEFI 镜像的 stext 偏移为 4 字节
    } else {
        // 非 UEFI 镜像从非 EFI 头部获取跳转指令
        b_primary_entry_insn = khdr->hdr.nefi.b_insn;
        b_stext_insn_offset = 0;    // 非 UEFI 镜像的 stext 偏移为 0
    }
    // 保存 stext 指令偏移量到内核信息结构体
    kinfo->b_stext_insn_offset = b_stext_insn_offset;

    // 转换跳转指令的字节序（小端序）
    b_primary_entry_insn = u32le(b_primary_entry_insn);
    // 验证是否为有效的 ARM64 无条件跳转指令（B 指令编码：0x14000000）
    if ((b_primary_entry_insn & 0xFC000000) != 0x14000000) {
        tools_loge_exit("kernel primary entry: %x\n", b_primary_entry_insn);
    } else {
        // 提取跳转指令的 26 位立即数，左移 2 位得到字节偏移
        uint32_t imm = (b_primary_entry_insn & 0x03ffffff) << 2;
        // 计算主入口点的绝对偏移量（相对于镜像起始位置）
        kinfo->primary_entry_offset = imm + b_stext_insn_offset;
    }

    // 从内核头部提取加载偏移量和内核大小（转换字节序）
    kinfo->load_offset = u64le(khdr->kernel_offset);
    kinfo->kernel_size = u64le(khdr->kernel_size_le);

    // 提取内核标志的低 4 位，用于解析架构相关信息
    uint8_t flag = u64le(khdr->kernel_flag_le) & 0x0f;
    kinfo->is_be = flag & 0x01;    // 检查是否为大端序（bit 0）

    // KernelPatch 不支持大端序 ARM64 内核
    if (kinfo->is_be) tools_loge_exit("kernel unexpected arm64 big endian img\n");

    // 根据内核标志的 bit 1-2 解析页面大小配置
    switch ((flag & 0b0110) >> 1) {
    case 2: // 16KB 页面大小
        kinfo->page_shift = 14;
        break;
    case 3: // 64KB 页面大小
        kinfo->page_shift = 16;
        break;
    case 1: // 4KB 页面大小
    default:
        kinfo->page_shift = 12;    // 默认 4KB 页面
    }

    // 打印内核镜像的详细信息用于调试
    tools_logi("kernel image_size: 0x%08x\n", imglen);
    tools_logi("kernel uefi header: %s\n", kinfo->uefi ? "true" : "false");
    tools_logi("kernel load_offset: 0x%08x\n", kinfo->load_offset);
    tools_logi("kernel kernel_size: 0x%08x\n", kinfo->kernel_size);
    tools_logi("kernel page_shift: %d\n", kinfo->page_shift);

    return 0;
}

/**
 * 调整内核镜像大小信息
 * 
 * @param kinfo 内核信息结构体指针，包含字节序等配置信息
 * @param img 内核镜像数据指针
 * @param size 新的内核大小值
 * @return 成功返回 0
 * 
 * 功能说明：
 * - 更新内核头部中的 kernel_size_le 字段
 * - 根据主机和内核的字节序差异进行必要的字节序转换
 * - 确保内核大小信息与实际镜像大小保持一致
 */
int32_t kernel_resize(kernel_info_t *kinfo, char *img, int32_t size)
{
    // 获取 ARM64 内核头部结构体指针
    arm64_hdr_t *khdr = (arm64_hdr_t *)img;
    uint64_t ksize = size;
    
    // 如果主机字节序与内核字节序不同，需要进行字节序转换
    if (is_be() ^ kinfo->is_be) ksize = u64swp(size);
    
    // 更新内核头部中的内核大小字段
    khdr->kernel_size_le = ksize;
    return 0;
}