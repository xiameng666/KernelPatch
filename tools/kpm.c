/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2024 bmax121. All Rights Reserved.
 */

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "kpm.h"

// 检查 ELF 文件是否为 ARM64 架构
#define elf_check_arch(x) ((x)->e_machine == EM_AARCH64)

/**
 * 在字符串区段中获取下一个字符串
 * 
 * @param string 当前字符串指针
 * @param secsize 剩余区段大小的指针
 * @return 下一个字符串的指针，如果没有则返回 NULL
 * 
 * 功能说明：
 * - 跳过当前字符串的结尾（空字符）
 * - 跳过连续的空字符（填充字符）
 * - 返回下一个有效字符串的起始位置
 * - 同时更新剩余区段大小
 */
static char *next_string(char *string, uint64_t *secsize)
{
    // 跳过当前字符串内容直到遇到结尾空字符
    while (string[0]) {
        string++;
        if ((*secsize)-- <= 1) return 0;
    }
    // 跳过连续的空字符（字符串间的分隔符）
    while (!string[0]) {
        string++;
        if ((*secsize)-- <= 1) return 0;
    }
    return string;
}

/**
 * 在模块信息中获取下一个指定标签的值
 * 
 * @param info 加载信息结构体，包含 ELF 文件数据
 * @param tag 要搜索的标签名称（如 "name", "version" 等）
 * @param prev 上一个找到的标签位置，NULL 表示从头开始搜索
 * @return 找到的标签值字符串，未找到返回 NULL
 * 
 * 功能说明：
 * - 在 .kpm.info 区段中搜索指定标签
 * - 支持连续搜索同名标签的多个实例
 * - 标签格式为 "tag=value"，返回 "value" 部分
 */
static char *get_next_modinfo(const struct load_info *info, const char *tag, char *prev)
{
    char *p;
    uint32_t taglen = strlen(tag);
    // 获取 .kpm.info 区段头部信息
    Elf_Shdr *infosec = &info->sechdrs[info->index.info];
    uint64_t size = infosec->sh_size;
    char *modinfo = (char *)info->hdr + infosec->sh_offset;
    
    // 如果指定了前一个位置，从该位置继续搜索
    if (prev) {
        size -= prev - modinfo;
        modinfo = next_string(prev, &size);
    }
    
    // 遍历模块信息字符串，查找匹配的标签
    for (p = modinfo; p; p = next_string(p, &size)) {
        // 检查标签名称和等号分隔符
        if (strncmp(p, tag, taglen) == 0 && p[taglen] == '=') return p + taglen + 1;
    }
    return 0;
}

/**
 * 获取模块信息中指定标签的值
 * 
 * @param info 加载信息结构体
 * @param tag 标签名称
 * @return 标签值字符串，未找到返回 NULL
 */
static char *get_modinfo(const struct load_info *info, const char *tag)
{
    return get_next_modinfo(info, tag, 0);
}

/**
 * 在 ELF 文件中查找指定名称的区段
 * 
 * @param info 加载信息结构体
 * @param name 区段名称
 * @return 区段索引，未找到返回 0
 * 
 * 功能说明：
 * - 遍历所有区段头部表项
 * - 检查区段是否具有 SHF_ALLOC 标志（可分配内存）
 * - 比较区段名称字符串
 */
static int find_sec(const struct load_info *info, const char *name)
{
    // 从索引 1 开始遍历区段（索引 0 为无效区段）
    for (int i = 1; i < info->hdr->e_shnum; i++) {
        Elf_Shdr *shdr = &info->sechdrs[i];
        // 检查区段标志和名称匹配
        if ((shdr->sh_flags & SHF_ALLOC) && strcmp(info->secstrings + shdr->sh_name, name) == 0) return i;
    }
    return 0;
}

/**
 * 获取指定区段的基地址
 * 
 * @param info 加载信息结构体
 * @param secname 区段名称
 * @return 区段在内存中的基地址，未找到返回 NULL
 */
static void *get_sh_base(struct load_info *info, const char *secname)
{
    int idx = find_sec(info, secname);
    if (!idx) return 0;
    Elf_Shdr *infosec = &info->sechdrs[idx];
    void *addr = (void *)info->hdr + infosec->sh_offset;
    return addr;
}

/**
 * 获取指定区段的大小
 * 
 * @param info 加载信息结构体
 * @param secname 区段名称
 * @return 区段大小，未找到返回 0
 */
static uint64_t get_sh_size(struct load_info *info, const char *secname)
{
    int idx = find_sec(info, secname);
    if (!idx) return 0;
    Elf_Shdr *infosec = &info->sechdrs[idx];
    return infosec->sh_entsize;
}

/**
 * 从 KPM (KernelPatch Module) 文件中提取模块信息
 * 
 * @param kpm KPM 文件数据指针
 * @param len KPM 文件大小
 * @param out_info 输出的模块信息结构体
 * @return 成功返回 0，失败返回负数错误码
 * 
 * 功能说明：
 * - 验证 ELF 文件格式和架构兼容性
 * - 解析区段头部表和字符串表
 * - 定位 .kpm.info 区段并提取模块元数据
 * - 填充模块名称、版本、许可证、作者、描述等信息
 */
int get_kpm_info(const char *kpm, int len, kpm_info_t *out_info)
{
    struct load_info load_info = { .len = len, .hdr = (Elf_Ehdr *)kpm };
    struct load_info *info = &load_info;

    // 验证 ELF 头部基本格式
    if (info->len <= sizeof(*(info->hdr))) return -ENOEXEC;
    // 检查 ELF 魔数、文件类型、架构和区段头部大小
    if (memcmp(info->hdr->e_ident, ELFMAG, SELFMAG) || info->hdr->e_type != ET_REL || !elf_check_arch(info->hdr) ||
        info->hdr->e_shentsize != sizeof(Elf_Shdr))
        return -ENOEXEC;
    // 验证区段头部表的位置和大小
    if (info->hdr->e_shoff >= info->len || (info->hdr->e_shnum * sizeof(Elf_Shdr) > info->len - info->hdr->e_shoff))
        return -ENOEXEC;

    // 初始化区段头部表和字符串表指针
    info->sechdrs = (void *)info->hdr + info->hdr->e_shoff;
    info->secstrings = (void *)info->hdr + info->sechdrs[info->hdr->e_shstrndx].sh_offset;
    info->sechdrs[0].sh_addr = 0;
    
    // 遍历所有区段，计算虚拟地址并验证数据完整性
    for (int i = 1; i < info->hdr->e_shnum; i++) {
        Elf_Shdr *shdr = &info->sechdrs[i];
        // 检查非 NOBITS 区段的数据是否在文件范围内
        if (shdr->sh_type != SHT_NOBITS && info->len < shdr->sh_offset + shdr->sh_size) {
            return -ENOEXEC;
        }
        // 计算区段在内存中的地址
        shdr->sh_addr = (size_t)info->hdr + shdr->sh_offset;
    }
    
    // 查找并验证 .kpm.info 区段的存在
    info->index.info = find_sec(info, ".kpm.info");
    if (!info->index.info) {
        tools_loge("no .kpm.info section\n");
        return -ENOEXEC;
    }
    info->info.base = get_sh_base(info, ".kpm.info");
    info->info.size = get_sh_size(info, ".kpm.info");

    // 从模块信息区段中提取各项元数据
    out_info->name = get_modinfo(info, "name");
    out_info->version = get_modinfo(info, "version");
    out_info->license = get_modinfo(info, "license");
    out_info->author = get_modinfo(info, "author");
    out_info->description = get_modinfo(info, "description");

    return 0;
}

/**
 * 打印 KPM 模块信息到标准输出
 * 
 * @param info 模块信息结构体指针
 * 
 * 功能说明：
 * - 格式化输出模块的基本信息
 * - 包括名称、版本、许可证、作者、描述等字段
 * - 用于调试和信息展示
 */
void print_kpm_info(kpm_info_t *info)
{
    fprintf(stdout, "name=%s\n", info->name);
    fprintf(stdout, "version=%s\n", info->version);
    fprintf(stdout, "license=%s\n", info->license);
    fprintf(stdout, "author=%s\n", info->author);
    fprintf(stdout, "description=%s\n", info->description);
}

/**
 * 从文件路径读取并打印 KPM 模块信息
 * 
 * @param kpm_path KPM 文件的文件系统路径
 * @return 成功返回 0，失败返回错误码
 * 
 * 功能说明：
 * - 读取指定路径的 KPM 文件
 * - 解析模块信息并格式化输出
 * - 自动管理文件内存资源
 * - 输出会话标识符用于日志分析
 */
int print_kpm_info_path(const char *kpm_path)
{
    char *img;
    int len = 0;
    // 读取 KPM 文件到内存
    read_file(kpm_path, &img, &len);
    // 输出会话标识符
    fprintf(stdout, INFO_EXTRA_KPM_SESSION "\n");
    kpm_info_t kpm_info = { 0 };
    // 解析模块信息
    int rc = get_kpm_info(img, len, &kpm_info);
    if (!rc) {
        // 打印模块详细信息
        print_kpm_info(&kpm_info);
    }
    // 释放文件内存
    free(img);
    return rc;
}
