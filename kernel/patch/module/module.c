/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// KernelPatch模块管理系统 - 处理内核模块的加载、卸载和管理
// 支持ELF格式的KernelPatch模块，提供动态符号重定位和内存管理

#include <uapi/asm-generic/errno.h>
#include <pgtable.h>
#include <kpmalloc.h>
#include <linux/err.h>
#include <linux/string.h>
#include <symbol.h>
#include <kallsyms.h>
#include <cache.h>
#include <common.h>
#include <linux/fs.h>
#include <uapi/linux/fs.h>
#include <hotpatch.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/rcupdate.h>
#include <linux/rculist.h>

#include "module.h"
#include "relo.h"

#define SZ_128M 0x08000000  // 128MB大小定义

// 对齐相关宏定义
#define ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) ALIGN_MASK(x, (typeof(x))(a)-1)
#define align(X) ALIGN(X, page_size)  // 页面对齐

// 检查ELF架构是否为ARM64
#define elf_check_arch(x) ((x)->e_machine == EM_AARCH64)

#define ARCH_SHF_SMALL 0  // 架构特定的段标志

/**
 * @brief 检查字符串是否以指定前缀开始
 * @param str 要检查的目标字符串
 * @param prefix 要匹配的前缀字符串
 * @return 如果str以prefix开始返回true，否则返回false
 */
static inline bool strstarts(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;  // 比较前缀长度的字符是否相等
}

/**
 * @brief 在字符串表中获取下一个有效字符串
 * @param string 当前字符串位置指针
 * @param secsize 剩余段大小的指针（会被更新）
 * @return 下一个有效字符串的指针，到达末尾或越界时返回NULL
 */
static char *next_string(char *string, unsigned long *secsize)
{
    // 跳过当前字符串的剩余部分
    while (string[0]) {
        string++;
        if ((*secsize)-- <= 1) return 0;  // 防止越界
    }
    // 跳过连续的空字节，找到下一个字符串的开始
    while (!string[0]) {
        string++;
        if ((*secsize)-- <= 1) return 0;
    }
    return string;
}

/**
 * @brief 计算模块段的内存偏移并更新模块大小
 * @param mod 模块结构体指针
 * @param size 当前模块大小指针（会被更新）
 * @param sechdr ELF段头指针
 * @param section 段索引号
 * @return 计算出的段偏移地址
 */
static long get_offset(struct module *mod, unsigned int *size, Elf_Shdr *sechdr, unsigned int section)
{
    long ret = ALIGN(*size, sechdr->sh_addralign ?: 1);  // 按段要求的对齐方式对齐
    *size = ret + sechdr->sh_size;  // 更新模块总大小
    return ret;
}

/**
 * @brief 在模块信息段中查找下一个指定标签的值
 * @param info 加载信息结构体指针
 * @param tag 要查找的标签名称
 * @param prev 上一次查找的位置（NULL表示从头开始）
 * @return 找到的标签值字符串指针，未找到返回NULL
 */
static char *get_next_modinfo(const struct load_info *info, const char *tag, char *prev)
{
    char *p;
    unsigned int taglen = strlen(tag);  // 标签名称长度
    Elf_Shdr *infosec = &info->sechdrs[info->index.info];  // 获取.kpm.info段头
    unsigned long size = infosec->sh_size;  // 信息段大小
    char *modinfo = (char *)info->hdr + infosec->sh_offset;  // 信息段数据起始地址
    
    if (prev) {
        // 从上次位置继续搜索
        size -= prev - modinfo;
        modinfo = next_string(prev, &size);
    }
    
    // 遍历信息段中的所有字符串
    for (p = modinfo; p; p = next_string(p, &size)) {
        // 检查是否匹配指定标签格式："tag=value"
        if (strncmp(p, tag, taglen) == 0 && p[taglen] == '=') return p + taglen + 1;
    }
    return 0;
}

/**
 * @brief 获取模块信息段中指定标签的值
 * @param info 加载信息结构体指针
 * @param tag 要查找的标签名称
 * @return 找到的标签值字符串指针，未找到返回NULL
 */
static char *get_modinfo(const struct load_info *info, const char *tag)
{
    return get_next_modinfo(info, tag, 0);  // 从头开始查找
}

/**
 * @brief 查找指定名称的ELF段索引
 * @param info 加载信息结构体指针
 * @param name 要查找的段名称
 * @return 找到的段索引号，未找到返回0
 */
static int find_sec(const struct load_info *info, const char *name)
{
    // 遍历所有段头（跳过第0个空段）
    for (int i = 1; i < info->hdr->e_shnum; i++) {
        Elf_Shdr *shdr = &info->sechdrs[i];
        // 检查段是否需要分配内存且名称匹配
        if ((shdr->sh_flags & SHF_ALLOC) && strcmp(info->secstrings + shdr->sh_name, name) == 0) return i;
    }
    return 0;
}

/**
 * @brief 获取指定段的基地址
 * @param info 加载信息结构体指针
 * @param secname 段名称
 * @return 段的基地址指针，未找到返回NULL
 */
static void *get_sh_base(struct load_info *info, const char *secname)
{
    int idx = find_sec(info, secname);  // 查找段索引
    if (!idx) return 0;
    Elf_Shdr *infosec = &info->sechdrs[idx];  // 获取段头
    void *addr = (void *)info->hdr + infosec->sh_offset;  // 计算段在文件中的地址
    return addr;
}

/**
 * @brief 获取指定段的条目大小
 * @param info 加载信息结构体指针
 * @param secname 段名称
 * @return 段的条目大小，未找到返回0
 */
static unsigned long get_sh_size(struct load_info *info, const char *secname)
{
    int idx = find_sec(info, secname);  // 查找段索引
    if (!idx) return 0;
    Elf_Shdr *infosec = &info->sechdrs[idx];  // 获取段头
    return infosec->sh_entsize;  // 返回条目大小
}

/**
 * @brief 布局模块的各个段，计算每个段的偏移和模块总大小
 * @param mod 模块结构体指针
 * @param info 加载信息结构体指针
 */
static void layout_sections(struct module *mod, struct load_info *info)
{
    // 定义段布局的掩码数组，按优先级排序
    // 注意：所有可执行代码必须是数组中的第一个段，否则需要修改下面两个循环中的text_size查找器
    static unsigned long const masks[][2] = {
        { SHF_EXECINSTR | SHF_ALLOC, ARCH_SHF_SMALL },  // 可执行段（代码段）
        { SHF_ALLOC, SHF_WRITE | ARCH_SHF_SMALL },      // 只读数据段
        { SHF_WRITE | SHF_ALLOC, ARCH_SHF_SMALL },      // 可写数据段  
        { ARCH_SHF_SMALL | SHF_ALLOC, 0 }               // 其他分配段
    };

    // 初始化所有段的entsize为最大值，用作未处理标记
    for (int i = 0; i < info->hdr->e_shnum; i++)
        info->sechdrs[i].sh_entsize = ~0UL;

    // TODO: 使用tlsf分配所有rwx段且不按页对齐
    // 按掩码优先级处理各个段
    for (int m = 0; m < sizeof(masks) / sizeof(masks[0]); ++m) {
        for (int i = 0; i < info->hdr->e_shnum; ++i) {
            Elf_Shdr *s = &info->sechdrs[i];
            // 检查段标志是否匹配当前掩码，且尚未处理
            if ((s->sh_flags & masks[m][0]) != masks[m][0] || (s->sh_flags & masks[m][1]) || s->sh_entsize != ~0UL)
                continue;
            // 计算段的偏移地址
            s->sh_entsize = get_offset(mod, &mod->size, s, i);
            // const char *sname = info->secstrings + s->sh_name;
        }
        // 根据段类型设置模块的大小边界
        switch (m) {
        case 0: /* 可执行段 */
            mod->size = align(mod->size);  // 页对齐
            mod->text_size = mod->size;    // 记录代码段大小
            break;
        case 1: /* 只读段：代码和只读数据 */
            mod->size = align(mod->size);  // 页对齐
            mod->ro_size = mod->size;      // 记录只读段大小
            break;
        case 2:
            break;
        case 3: /* 整体 */
            mod->size = align(mod->size);  // 页对齐
            break;
        }
    }
}

/**
 * @brief 判断符号是否为核心符号（需要保留的符号）
 * @param src ELF符号表条目指针
 * @param sechdrs 段头数组指针
 * @param shnum 段数量
 * @return 如果是核心符号返回true，否则返回false
 */
static bool is_core_symbol(const Elf_Sym *src, const Elf_Shdr *sechdrs, unsigned int shnum)
{
    const Elf_Shdr *sec;
    
    // 检查符号的基本有效性
    if (src->st_shndx == SHN_UNDEF || src->st_shndx >= shnum || !src->st_name) return false;
    
    sec = sechdrs + src->st_shndx;  // 获取符号所在段的段头
    
    // 检查段是否为可分配的可执行段
    if (!(sec->sh_flags & SHF_ALLOC) || !(sec->sh_flags & SHF_EXECINSTR)) return false;
    
    return true;
}

/**
 * @brief 简化符号表，将所有符号的st_value字段设置为直接指针地址
 * @param mod 模块结构体指针
 * @param info 加载信息结构体指针
 * @return 成功返回0，失败返回错误码
 */
static int simplify_symbols(struct module *mod, const struct load_info *info)
{
    Elf_Shdr *symsec = &info->sechdrs[info->index.sym];  // 获取符号表段头
    Elf_Sym *sym = (void *)symsec->sh_addr;              // 符号表起始地址
    unsigned long secbase;
    unsigned int i;
    int ret = 0;

    // 遍历符号表中的所有符号（跳过第0个空符号）
    for (i = 1; i < symsec->sh_size / sizeof(Elf_Sym); i++) {
        const char *name = info->strtab + sym[i].st_name;  // 获取符号名称
        
        switch (sym[i].st_shndx) {
        case SHN_COMMON:  // 通用符号
            if (!strncmp(name, "__gnu_lto", 9)) {
                logkd("Please compile with -fno-common\n");  // 提示需要使用-fno-common编译选项
                ret = -ENOEXEC;
            }
            break;
            
        case SHN_ABS:  // 绝对符号
            break;
            
        case SHN_UNDEF:  // 未定义符号，需要外部解析
            unsigned long addr = symbol_lookup_name(name);  // 在符号表中查找地址
            // 内核符号在重定位时可能导致溢出
            // if (!addr) addr = kallsyms_lookup_name(name);
            if (!addr) {
                logke("unknown symbol: %s\n", name);  // 符号未找到
                ret = -ENOENT;
                break;
            }
            sym[i].st_value = addr;  // 设置符号的绝对地址
            break;
            
        default:  // 定义在某个段中的符号
            secbase = info->sechdrs[sym[i].st_shndx].sh_addr;  // 获取符号所在段的基地址
            sym[i].st_value += secbase;  // 符号值加上段基地址得到最终地址
            break;
        }
    }
    return ret;
}

/**
 * @brief 应用ELF重定位信息到模块
 * @param mod 模块结构体指针
 * @param info 加载信息结构体指针
 * @return 成功返回0，失败返回错误码
 */
static int apply_relocations(struct module *mod, const struct load_info *info)
{
    int rc = 0;
    unsigned int i;
    
    // 遍历所有段，查找重定位段
    for (i = 1; i < info->hdr->e_shnum; i++) {
        unsigned int infosec = info->sechdrs[i].sh_info;  // 重定位段作用的目标段索引
        
        // 检查目标段索引有效性
        if (infosec >= info->hdr->e_shnum) continue;
        
        // 只处理需要分配内存的段
        if (!(info->sechdrs[infosec].sh_flags & SHF_ALLOC)) continue;
        
        // 根据重定位段类型调用相应的处理函数
        if (info->sechdrs[i].sh_type == SHT_REL) {
            // 处理REL类型重定位（不含addend）
            rc = apply_relocate(info->sechdrs, info->strtab, info->index.sym, i, mod);
        } else if (info->sechdrs[i].sh_type == SHT_RELA) {
            // 处理RELA类型重定位（含addend）
            rc = apply_relocate_add(info->sechdrs, info->strtab, info->index.sym, i, mod);
        }
        if (rc < 0) break;  // 重定位失败则退出
    }
    return rc;
}

/**
 * @brief 布局符号表和字符串表在模块内存中的位置
 * @param mod 模块结构体指针
 * @param info 加载信息结构体指针
 * 
 * TODO: 重定位完成后释放.strtab和.symtab
 */
static void layout_symtab(struct module *mod, struct load_info *info)
{
    Elf_Shdr *symsect = info->sechdrs + info->index.sym;  // 符号表段头
    Elf_Shdr *strsect = info->sechdrs + info->index.str;  // 字符串表段头
    const Elf_Sym *src;
    unsigned int i, nsrc, ndst, strtab_size = 0;

    // 将符号表段标记为需要分配内存
    symsect->sh_flags |= SHF_ALLOC;
    symsect->sh_entsize = get_offset(mod, &mod->size, symsect, info->index.sym);

    src = (void *)info->hdr + symsect->sh_offset;  // 源符号表地址
    nsrc = symsect->sh_size / sizeof(*src);        // 源符号数量

    // 字符串表总是以null开始，所以偏移0是空字符串
    strtab_size = 1;
    
    // 计算核心符号字符串表所需的总空间
    for (ndst = i = 0; i < nsrc; i++) {
        if (i == 0 || is_core_symbol(src + i, info->sechdrs, info->hdr->e_shnum)) {
            strtab_size += strlen(&info->strtab[src[i].st_name]) + 1;  // 符号名长度+1
            ndst++;  // 目标符号计数
        }
    }

    // 在模块末尾为核心符号预留空间
    info->symoffs = ALIGN(mod->size, symsect->sh_addralign ?: 1);    // 符号表偏移
    info->stroffs = mod->size = info->symoffs + ndst * sizeof(Elf_Sym);  // 字符串表偏移
    mod->size += strtab_size;  // 更新模块总大小

    // 将字符串表段标记为需要分配内存
    strsect->sh_flags |= SHF_ALLOC;
    strsect->sh_entsize = get_offset(mod, &mod->size, strsect, info->index.str);
}

/**
 * @brief 重写段头，设置段的临时映射地址
 * @param info 加载信息结构体指针
 * @return 成功返回0，失败返回-ENOEXEC
 */
static int rewrite_section_headers(struct load_info *info)
{
    info->sechdrs[0].sh_addr = 0;  // 第0个段地址设为0
    
    // 遍历所有段，设置临时地址
    for (int i = 1; i < info->hdr->e_shnum; i++) {
        Elf_Shdr *shdr = &info->sechdrs[i];
        
        // 检查段偏移和大小是否在文件范围内
        if (shdr->sh_type != SHT_NOBITS && info->len < shdr->sh_offset + shdr->sh_size) {
            return -ENOEXEC;
        }
        
        // 将所有段的sh_addr标记为它们在临时镜像中的地址
        shdr->sh_addr = (size_t)info->hdr + shdr->sh_offset;
    }
    return 0;
}

/**
 * @brief 将模块数据移动到最终的执行内存位置
 * @param mod 模块结构体指针
 * @param info 加载信息结构体指针
 * @return 成功返回0，失败返回错误码
 */
static int move_module(struct module *mod, struct load_info *info)
{
    // TODO: 改进内存分配策略
    logki("alloc module size: %llx\n", mod->size);
    mod->start = kp_malloc_exec(mod->size);  // 分配可执行内存
    if (!mod->start) {
        return -ENOMEM;
    }
    memset(mod->start, 0, mod->size);  // 清零内存

    // 传输所有指定SHF_ALLOC的段
    logkd("final section addresses:\n");

    for (int i = 1; i < info->hdr->e_shnum; i++) {
        void *dest;
        Elf_Shdr *shdr = &info->sechdrs[i];
        if (!(shdr->sh_flags & SHF_ALLOC)) continue;  // 跳过不需要分配的段

        dest = mod->start + shdr->sh_entsize;  // 计算段在模块中的目标地址
        const char *sname = info->secstrings + shdr->sh_name;  // 获取段名称

        logkd("    %s %llx %llx\n", sname, dest, shdr->sh_size);

        // 复制段数据（SHT_NOBITS段不需要复制）
        if (shdr->sh_type != SHT_NOBITS) memcpy(dest, (void *)shdr->sh_addr, shdr->sh_size);

        shdr->sh_addr = (unsigned long)dest;  // 更新段头的地址为最终地址

        // 根据段名称设置模块的特殊函数指针
        if (!mod->init && !strcmp(".kpm.init", sname)) mod->init = (mod_initcall_t *)dest;

        if (!strcmp(".kpm.ctl0", sname)) mod->ctl0 = (mod_ctl0call_t *)dest;
        if (!strcmp(".kpm.ctl1", sname)) mod->ctl1 = (mod_ctl1call_t *)dest;

        if (!mod->exit && !strcmp(".kpm.exit", sname)) mod->exit = (mod_exitcall_t *)dest;

        if (!mod->info.base && !strcmp(".kpm.info", sname)) mod->info.base = (const char *)dest;
    }
    
    // 更新模块信息字段的指针，从加载时的临时地址转换为最终地址
    mod->info.name = info->info.name - info->info.base + mod->info.base;
    mod->info.version = info->info.version - info->info.base + mod->info.base;

    if (info->info.license) mod->info.license = info->info.license - info->info.base + mod->info.base;
    if (info->info.author) mod->info.author = info->info.author - info->info.base + mod->info.base;
    if (info->info.description) mod->info.description = info->info.description - info->info.base + mod->info.base;

    return 0;
}

/**
 * @brief 设置加载信息结构体，解析ELF文件结构
 * @param info 加载信息结构体指针
 * @return 成功返回0，失败返回错误码
 */
static int setup_load_info(struct load_info *info)
{
    int rc = 0;
    info->sechdrs = (void *)info->hdr + info->hdr->e_shoff;      // 段头表起始地址
    info->secstrings = (void *)info->hdr + info->sechdrs[info->hdr->e_shstrndx].sh_offset;  // 段名字符串表

    // 重写段头信息
    if ((rc = rewrite_section_headers(info))) {
        logke("rewrite section error\n");
        return rc;
    }

    // 检查必需的段是否存在
    if (!find_sec(info, ".kpm.init") || !find_sec(info, ".kpm.exit")) {
        logke("no .kpm.init or .kpm.exit section\n");
        return -ENOEXEC;
    }

    // 查找并设置模块信息段
    info->index.info = find_sec(info, ".kpm.info");
    if (!info->index.info) {
        logke("no .kpm.info section\n");
        return -ENOEXEC;
    }
    info->info.base = get_sh_base(info, ".kpm.info");
    info->info.size = get_sh_size(info, ".kpm.info");

    // 解析模块名称
    const char *name = get_modinfo(info, "name");
    if (!name) {
        logke("module name not found\n");
        return -ENOEXEC;
    }
    info->info.name = name;
    logkd("loading module: \n");
    logkd("    name: %s\n", name);

    // 解析模块版本
    const char *version = get_modinfo(info, "version");
    if (!version) {
        logkd("module version not found\n");
        return -ENOEXEC;
    }
    info->info.version = version;
    logkd("    version: %s\n", version);

    // 解析可选的模块信息
    const char *license = get_modinfo(info, "license");
    info->info.license = license;
    logkd("    license: %s\n", license);

    const char *author = get_modinfo(info, "author");
    info->info.author = author;
    logkd("    author: %s\n", author);
    
    const char *description = get_modinfo(info, "description");
    info->info.description = description;
    logkd("    description: %s\n", description);

    // 查找符号表和字符串表
    for (int i = 1; i < info->hdr->e_shnum; i++) {
        if (info->sechdrs[i].sh_type == SHT_SYMTAB) {
            info->index.sym = i;                                                               // 符号表段索引
            info->index.str = info->sechdrs[i].sh_link;                                       // 字符串表段索引
            info->strtab = (char *)info->hdr + info->sechdrs[info->index.str].sh_offset;     // 字符串表地址
            break;
        }
    }

    if (info->index.sym == 0) {
        logkd("module has no symbols (stripped?)\n");
        return -ENOEXEC;
    }
    return 0;
}

/**
 * @brief 检查ELF文件头的有效性
 * @param info 加载信息结构体指针
 * @return 成功返回0，失败返回-ENOEXEC
 */
static int elf_header_check(struct load_info *info)
{
    // 检查文件长度是否足够包含ELF头
    if (info->len <= sizeof(*(info->hdr))) return -ENOEXEC;
    
    // 检查ELF魔数、文件类型、架构和段头大小
    if (memcmp(info->hdr->e_ident, ELFMAG, SELFMAG) ||      // ELF魔数
        info->hdr->e_type != ET_REL ||                       // 可重定位文件类型
        !elf_check_arch(info->hdr) ||                        // ARM64架构检查
        info->hdr->e_shentsize != sizeof(Elf_Shdr))          // 段头大小检查
        return -ENOEXEC;
        
    // 检查段头表偏移和大小是否在文件范围内
    if (info->hdr->e_shoff >= info->len || 
        (info->hdr->e_shnum * sizeof(Elf_Shdr) > info->len - info->hdr->e_shoff))
        return -ENOEXEC;
        
    return 0;
}

struct module modules = { 0 };  // 全局模块链表头
static spinlock_t module_lock;   // 模块操作自旋锁

/**
 * @brief 加载KernelPatch模块
 * @param data ELF文件数据指针
 * @param len ELF文件数据长度
 * @param args 传递给模块的参数字符串
 * @param event 事件名称
 * @param reserved 保留参数
 * @return 成功返回0，失败返回错误码
 */
long load_module(const void *data, int len, const char *args, const char *event, void *__user reserved)
{
    struct load_info load_info = { .len = len, .hdr = data };  // 初始化加载信息
    struct load_info *info = &load_info;
    long rc = 0;

    // 检查ELF文件头有效性
    if ((rc = elf_header_check(info))) goto out;
    // 设置加载信息
    if ((rc = setup_load_info(info))) goto out;

    // 检查模块是否已存在
    if (find_module(info->info.name)) {
        logkfd("%s exist\n", info->info.name);
        rc = -EEXIST;
        goto out;
    }

    // 分配模块结构体内存
    struct module *mod = (struct module *)vmalloc(sizeof(struct module));
    if (!mod) return -ENOMEM;
    memset(mod, 0, sizeof(struct module));

    // 复制模块参数
    if (args) {
        mod->args = vmalloc(strlen(args) + 1);
        if (!mod->args) {
            rc = -ENOMEM;
            goto free1;
        }
        strcpy(mod->args, args);
    }

    // 布局模块段和符号表
    layout_sections(mod, info);
    layout_symtab(mod, info);

    // 移动模块到最终内存位置
    if ((rc = move_module(mod, info))) goto free;
    // 简化符号表（解析外部符号）
    if ((rc = simplify_symbols(mod, info))) goto free;
    // 应用重定位
    if ((rc = apply_relocations(mod, info))) goto free;

    // 刷新指令缓存，确保代码可执行
    flush_icache_all();

    // 调用模块初始化函数
    rc = (*mod->init)(mod->args, event, reserved);

    if (!rc) {
        // 初始化成功，将模块添加到全局链表
        logkfi("[%s] succeed with [%s] \n", mod->info.name, args);
        list_add_tail(&mod->list, &modules.list);
        goto out;
    } else {
        // 初始化失败，调用退出函数清理
        logkfi("[%s] failed with [%s] error: %d, try exit ...\n", mod->info.name, args, rc);
        (*mod->exit)(reserved);
    }

free:
    if (mod->args) kvfree(mod->args);  // 释放参数内存
    kp_free_exec(mod->start);          // 释放可执行内存
free1:
    kvfree(mod);  // 释放模块结构体
out:
    return rc;
}

/**
 * @brief 卸载指定名称的KernelPatch模块
 * @param name 要卸载的模块名称
 * @param reserved 保留参数
 * @return 成功返回0，失败返回错误码
 * 
 * TODO: 添加锁保护
 */
long unload_module(const char *name, void *__user reserved)
{
    if (!name) return -EINVAL;
    logkfe("name: %s\n", name);

    rcu_read_lock();  // 获取RCU读锁
    long rc = 0;

    // 查找要卸载的模块
    struct module *mod = find_module(name);
    if (!mod) {
        rc = -ENOENT;
        goto out;
    }
    
    list_del(&mod->list);  // 从模块链表中移除
    rc = (*mod->exit)(reserved);  // 调用模块退出函数

    // 释放模块占用的所有内存
    if (mod->args) kvfree(mod->args);          // 释放参数内存
    if (mod->ctl_args) kvfree(mod->ctl_args);  // 释放控制参数内存

    kp_free_exec(mod->start);  // 释放可执行内存
    kvfree(mod);               // 释放模块结构体

    logkfi("name: %s, rc: %d\n", name, rc);

out:
    rcu_read_unlock();  // 释放RCU读锁
    return rc;
}

/**
 * @brief 从文件路径加载KernelPatch模块
 * @param path 模块文件路径
 * @param args 传递给模块的参数字符串
 * @param reserved 保留参数
 * @return 成功返回0，失败返回错误码
 */
long load_module_path(const char *path, const char *args, void *__user reserved)
{
    long rc = 0;
    logkfd("%s\n", path);
    if (!path) return -EINVAL;

    // 打开模块文件
    struct file *filp = filp_open(path, O_RDONLY, 0);
    if (unlikely(!filp || IS_ERR(filp))) {
        logkfe("open module: %s error\n", path);
        rc = PTR_ERR(filp);
        goto out;
    }
    
    // 获取文件大小
    loff_t len = vfs_llseek(filp, 0, SEEK_END);
    logkfd("module size: %llx\n", len);
    vfs_llseek(filp, 0, SEEK_SET);  // 重置文件指针到开头

    // 分配内存用于存储文件内容
    void *data = vmalloc(len);
    if (!data) {
        rc = -ENOMEM;
        goto out;
    }
    memset(data, 0, len);

    // 读取文件内容到内存
    loff_t pos = 0;
    kernel_read(filp, data, len, &pos);
    filp_close(filp, 0);  // 关闭文件

    // 验证读取的字节数
    if (pos != len) {
        logkfe("read module: %s error\n", path);
        rc = -EIO;
        goto free;
    }

    // 调用内存加载函数
    rc = load_module(data, len, args, "load-file", reserved);
free:
    kvfree(data);  // 释放临时内存
out:
    return rc;
}

/**
 * @brief 模块控制接口0（字符串参数版本）
 * @param name 模块名称
 * @param ctl_args 控制参数字符串
 * @param out_msg 输出消息缓冲区
 * @param outlen 输出缓冲区长度
 * @return 成功返回0，失败返回错误码
 */
long module_control0(const char *name, const char *ctl_args, char *__user out_msg, int outlen)
{
    if (!name || !ctl_args) return -EINVAL;
    int args_len = strlen(ctl_args);
    if (args_len <= 0) return -EINVAL;

    logkfi("name %s, args: %s\n", name, ctl_args);

    long rc = 0;
    rcu_read_lock();  // 获取RCU读锁

    // 查找目标模块
    struct module *mod = find_module(name);
    if (!mod) {
        rc = -ENOENT;
        goto out;
    }

    // 检查模块是否实现了ctl0接口
    if (!*mod->ctl0) {
        logkfe("no ctl0\n");
        rc = -ENOSYS;
        goto out;
    }

    // 释放旧的控制参数内存
    if (mod->ctl_args) kvfree(mod->ctl_args);

    // 为新的控制参数分配内存
    mod->ctl_args = vmalloc(args_len + 1);
    if (!mod->ctl_args) {
        rc = -ENOMEM;
        goto out;
    }

    strcpy(mod->ctl_args, ctl_args);  // 复制控制参数

    // 调用模块的ctl0函数
    rc = (*mod->ctl0)(mod->ctl_args, out_msg, outlen);

    logkfi("name: %s, rc: %d\n", name, rc);
out:
    rcu_read_unlock();  // 释放RCU读锁
    return rc;
}

/**
 * @brief 模块控制接口1（指针参数版本）
 * @param name 模块名称
 * @param a1 第一个参数指针
 * @param a2 第二个参数指针
 * @param a3 第三个参数指针
 * @return 成功返回0，失败返回错误码
 */
long module_control1(const char *name, void *a1, void *a2, void *a3)
{
    logkfi("name %s, a1: %llx, a2: %llx, a3: %llx\n", name, a1, a2, a3);
    long rc = 0;
    rcu_read_lock();  // 获取RCU读锁

    // 查找目标模块
    struct module *mod = find_module(name);
    if (!mod) {
        rc = -ENOENT;
        goto out;
    }

    // 检查模块是否实现了ctl1接口
    if (!*mod->ctl1) {
        logkfe("no ctl1\n");
        rc = -ENOSYS;
        goto out;
    }

    // 调用模块的ctl1函数
    rc = (*mod->ctl1)(a1, a2, a3);

    logkfi("name: %s, rc: %d\n", name, rc);
out:
    rcu_read_unlock();  // 释放RCU读锁
    return rc;
}

/**
 * @brief 根据名称查找模块
 * @param name 模块名称
 * @return 找到的模块指针，未找到返回NULL
 */
struct module *find_module(const char *name)
{
    struct module *pos;
    // 遍历模块链表查找匹配的模块名称
    list_for_each_entry(pos, &modules.list, list)
    {
        if (!strcmp(name, pos->info.name)) {
            return pos;
        }
    }
    return 0;
}

/**
 * @brief 获取已加载模块的数量
 * @return 模块数量
 */
int get_module_nums()
{
    rcu_read_lock();  // 获取RCU读锁

    struct module *pos;
    int n = 0;
    // 遍历模块链表计数
    list_for_each_entry(pos, &modules.list, list)
    {
        n++;
    }
    rcu_read_unlock();  // 释放RCU读锁

    logkfd("%d\n", n);
    return n;
}

/**
 * @brief 列出所有已加载模块的名称
 * @param out_names 输出缓冲区，用于存储模块名称列表
 * @param size 缓冲区大小
 * @return 实际写入的字符数
 */
int list_modules(char *out_names, int size)
{
    rcu_read_lock();  // 获取RCU读锁

    struct module *pos;
    int off = 0;
    // 遍历模块链表，将模块名称写入输出缓冲区
    list_for_each_entry(pos, &modules.list, list)
    {
        off += snprintf(out_names + off, size - 1 - off, "%s\n", pos->info.name);
    }
    if (off > 0) out_names[off - 1] = '\0';  // 移除最后一个换行符

    rcu_read_unlock();  // 释放RCU读锁
    return off;
}

/**
 * @brief 获取指定模块的详细信息
 * @param name 模块名称
 * @param out_info 输出缓冲区，用于存储模块信息
 * @param size 缓冲区大小
 * @return 成功返回写入的字符数，失败返回错误码
 */
int get_module_info(const char *name, char *out_info, int size)
{
    if (size <= 0) return 0;
    rcu_read_lock();  // 获取RCU读锁

    // 查找指定模块
    struct module *mod = find_module(name);
    if (!mod) return -ENOENT;

    // 格式化模块信息到输出缓冲区
    int sz = snprintf(out_info, size - 1,
                      "name=%s\n"
                      "version=%s\n"
                      "license=%s\n"
                      "author=%s\n"
                      "description=%s\n"
                      "args=%s\n",
                      mod->info.name, mod->info.version, mod->info.license, mod->info.author, mod->info.description,
                      mod->args);

    if (sz > 0) out_info[sz - 1] = '\0';  // 确保字符串以null结尾
    logkfd("%s", out_info);

    rcu_read_unlock();  // 释放RCU读锁
    return sz;
}

/**
 * @brief 初始化模块管理系统
 * @details 初始化全局模块链表和自旋锁
 */
void module_init()
{
    INIT_LIST_HEAD(&modules.list);  // 初始化模块链表头
    spin_lock_init(&module_lock);    // 初始化自旋锁
}