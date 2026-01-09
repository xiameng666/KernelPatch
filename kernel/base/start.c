/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

/**
 * @file start.c
 * @brief 内核补丁系统启动模块
 * @details 负责内核补丁的初始化、内存管理、符号解析和系统启动
 * 这是KernelPatch的核心启动模块，处理补丁系统的初始化流程
 */

#include <common.h>
#include <pgtable.h>
#include <ktypes.h>
#include <kallsyms.h>
#include <compiler.h>
#include <cache.h>
#include <symbol.h>
#include <predata.h>
#include <barrier.h>
#include <stdarg.h>

#include "../banner"
#include "start.h"
#include "hook.h"
#include "tlsf.h"
#include "hmem.h"
#include "setup.h"

// 位域操作宏：从64位数值中提取指定范围的位
#define bits(n, high, low) (((n) << (63u - (high))) >> (63u - (high) + (low)))
// 向下对齐到指定边界（清除低位）
#define align_floor(x, align) ((uint64_t)(x) & ~((uint64_t)(align) - 1))
// 向上对齐到指定边界（补齐到边界）
#define align_ceil(x, align) (((uint64_t)(x) + (uint64_t)(align) - 1) & ~((uint64_t)(align) - 1))

// 启动预设数据结构，存储在.start.data段中
// 包含补丁系统启动时需要的基本配置信息
start_preset_t start_preset __attribute__((section(".start.data")));

// 设置头部指针，指向补丁配置数据，导出给其他模块使用
setup_header_t *setup_header = 0;
KP_EXPORT_SYMBOL(setup_header);

// 内核符号表遍历函数指针 - 用于遍历内核中的所有符号
int (*kallsyms_on_each_symbol)(int (*fn)(void *data, const char *name, struct module *module, unsigned long addr),
                               void *data) = 0;
KP_EXPORT_SYMBOL(kallsyms_on_each_symbol);

// 内核符号查找函数指针 - 根据名称查找内核符号地址
unsigned long (*kallsyms_lookup_name)(const char *name) = 0;
KP_EXPORT_SYMBOL(kallsyms_lookup_name);

// 内核打印函数指针 - 用于内核级别的日志输出
void (*printk)(const char *fmt, ...) = 0;
KP_EXPORT_SYMBOL(printk);

// 格式化字符串函数指针 - 用于格式化输出
int (*vsnprintf)(char *buf, size_t size, const char *fmt, va_list args);

/**
 * @brief 虚拟内存区域结构体
 * @details 用于管理KernelPatch的内存区域，与内核VMA结构兼容
 */
static struct vm_struct
{
    struct vm_struct *next;     // 链表下一个节点
    void *addr;                 // 虚拟地址
    unsigned long size;         // 内存区域大小
    unsigned long flags;        // 标志位
    struct page **pages;        // 页面数组
#ifdef CONFIG_HAVE_ARCH_HUGE_VMALLOC
    unsigned int page_order;    // 页面阶数
#endif
    unsigned int nr_pages;      // 页面数量
    phys_addr_t phys_addr;      // 物理地址
    const void *caller;         // 调用者地址
} kp_vm = { 0 };

// 全局系统信息变量，导出给其他模块使用
uint32_t kver = 0;              // 内核版本号
KP_EXPORT_SYMBOL(kver);

uint32_t kpver = 0;             // KernelPatch版本号
KP_EXPORT_SYMBOL(kpver);

endian_t endian = little;       // 字节序类型（小端序）
KP_EXPORT_SYMBOL(endian);

// KernelPatch内存布局边界地址定义
uint64_t _kp_extra_start = 0;   // 额外数据段起始地址
uint64_t _kp_extra_end = 0;     // 额外数据段结束地址
uint64_t _kp_hook_start = 0;    // Hook代码段起始地址
uint64_t _kp_hook_end = 0;      // Hook代码段结束地址
uint64_t _kp_rox_start = 0;     // 只读可执行段起始地址
uint64_t _kp_rox_end = 0;       // 只读可执行段结束地址
uint64_t _kp_rw_start = 0;      // 读写数据段起始地址
uint64_t _kp_rw_end = 0;        // 读写数据段结束地址
uint64_t _kp_region_start = 0;  // KP区域起始地址
uint64_t _kp_region_end = 0;    // KP区域结束地址

// 地址映射相关变量
uint64_t link_base_addr = (uint64_t)_link_base;    // 链接时的基地址
uint64_t runtime_base_addr = 0;                    // 运行时的基地址

// 内核地址空间布局相关变量
uint64_t kimage_voffset = 0;     // 内核镜像虚拟地址偏移量
uint64_t linear_voffset = 0;     // 线性映射虚拟地址偏移量
uint64_t kernel_va = 0;          // 内核虚拟地址起始
uint64_t kernel_pa = 0;          // 内核物理地址起始
int64_t kernel_size = 0;         // 内核镜像大小
int64_t page_shift = 0;          // 页面大小位移量（如12表示4KB页）
int64_t page_size = 0;           // 页面大小（字节）
int64_t va_bits = 0;             // 虚拟地址位数（如48位）
int64_t page_level;              // 页表级别数
uint64_t pgd_pa;                 // 页全局目录物理地址
uint64_t pgd_va;                 // 页全局目录虚拟地址
// int64_t pa_bits = 0;          // 物理地址位数（预留）

uint64_t kernel_stext_va = 0;    // 内核代码段起始虚拟地址

// KernelPatch内存分配器实例
tlsf_t kp_rw_mem = 0;            // 读写内存池分配器
tlsf_t kp_rox_mem = 0;           // 只读可执行内存池分配器

// 启动日志系统配置
#define BOOT_LOG_SIZE 0x2000     // 启动日志缓冲区大小（8KB）
static char boot_log[BOOT_LOG_SIZE] = { 0 };   // 启动日志缓冲区
static int boot_log_offset = 0;                // 当前写入偏移

/**
 * @brief 检查硬件是否支持脏页标记
 * @details 读取TCR_EL1寄存器检查硬件脏页支持
 * @return true表示支持，false表示不支持
 */
static inline bool hw_dirty()
{
    uint64_t tcr_el1;
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr_el1));
    return tcr_el1 & 0x10000000000;  // 检查HD位
}

/**
 * @brief 获取启动日志内容
 * @return 指向启动日志缓冲区的指针
 */
const char *get_boot_log()
{
    return boot_log;
}

/**
 * @brief 记录启动日志并通过printk输出
 * @details 格式化日志信息并存储到启动日志缓冲区，同时输出到内核日志
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void log_boot(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    // 将格式化字符串写入日志缓冲区的剩余空间
    int ret = vsnprintf(boot_log + boot_log_offset, sizeof(boot_log) - boot_log_offset, fmt, va);
    va_end(va);
    // 同时通过内核printk输出日志信息
    printk("KP %s", boot_log + boot_log_offset);
    boot_log_offset += ret;  // 更新缓冲区写入偏移
}

/**
 * @brief 获取指定虚拟地址在页表中的条目指针
 * @details 遍历多级页表结构，找到虚拟地址对应的页表条目
 * 支持4KB、16KB、64KB页面大小和不同的页表级别配置
 * @param pgd 页全局目录的虚拟地址
 * @param va 要查找的虚拟地址
 * @return 页表条目的虚拟地址指针，失败返回0
 */
uint64_t *pgtable_entry(uint64_t pgd, uint64_t va)
{
    // 计算页表索引位数：每个页表项8字节，页表大小为page_size
    uint64_t pxd_bits = page_shift - 3;  // 如4KB页时为9位
    uint64_t pxd_ptrs = 1u << pxd_bits;  // 每个页表的条目数
    uint64_t pxd_va = pgd;               // 当前页表虚拟地址
    uint64_t pxd_pa = virt_to_phys(pxd_va); // 当前页表物理地址
    uint64_t pxd_entry_va = 0;           // 页表条目虚拟地址
    uint64_t block_lv = 0;               // 块映射级别

    // ================
    // 必须调用某个函数（即使是空函数）才能正常工作
    // 原因不明，可能与缓存一致性或编译器优化有关
    // 如果有人知道原因请告知，非常感谢
    // ================
    __flush_dcache_area((void *)pxd_va, page_size);

    // 从最高级页表开始遍历，直到找到目标地址的页表条目
    for (int64_t lv = 4 - page_level; lv < 4; lv++) {
        // 计算当前级别的页表位移量
        uint64_t pxd_shift = (page_shift - 3) * (4 - lv) + 3;
        // 计算虚拟地址在当前级别页表中的索引
        uint64_t pxd_index = (va >> pxd_shift) & (pxd_ptrs - 1);
        pxd_entry_va = pxd_va + pxd_index * 8; // 每个条目8字节
        if (!pxd_entry_va) return 0;
        
        // 读取页表描述符内容
        uint64_t pxd_desc = *((uint64_t *)pxd_entry_va);
        
        if ((pxd_desc & 0b11) == 0b11) { // 表类型描述符（指向下级页表）
            // 提取下级页表的物理地址
            pxd_pa = pxd_desc & (((1ul << (48 - page_shift)) - 1) << page_shift);
        } else if ((pxd_desc & 0b11) == 0b01) { // 块类型描述符（大页映射）
            // 支持的块映射：4k页在lv1/lv2，16k和64k页仅在lv2
            uint64_t block_bits = (3 - lv) * pxd_bits + page_shift;
            pxd_pa = pxd_desc & (((1ul << (48 - block_bits)) - 1) << block_bits);
            block_lv = lv;  // 记录块映射级别
        } else { // 无效描述符
            return 0;
        }
        
        // 将物理地址转换为虚拟地址以访问下级页表
        pxd_va = phys_to_virt(pxd_pa);
        if (block_lv) {
            break;  // 遇到块映射则停止遍历
        }
    }
    
#if 0
    // 验证虚拟地址映射的正确性（调试代码，已禁用）
    // 计算页内偏移位数
    uint64_t left_bit = page_shift + (block_lv ? (3 - block_lv) * pxd_bits : 0);
    // 计算最终的物理地址
    uint64_t tpa = pxd_pa + (va & ((1u << left_bit) - 1));
    uint64_t tlva = phys_to_virt(tpa);  // 线性映射虚拟地址
    uint64_t tkimg = phys_to_kimg(tpa); // 内核镜像虚拟地址
    // 验证地址转换的一致性
    if (tlva != va && tkimg != va) {
        return 0;
    }
#endif
    return (uint64_t *)pxd_entry_va;
}
KP_EXPORT_SYMBOL(pgtable_entry);

/**
 * @brief 修改内核页表条目，处理连续页表条目的特殊情况
 * @details ARM64支持连续页表条目（Contiguous PTE）优化，
 * 当修改此类条目时需要特殊处理以保证TLB一致性
 * @param va 虚拟地址
 * @param entry 页表条目指针
 * @param value 新的页表条目值
 */
void modify_entry_kernel(uint64_t va, uint64_t *entry, uint64_t value)
{
    // 如果新旧值都不是有效的连续页表条目，使用标准修改流程
    if (!pte_valid_cont(*entry) && !pte_valid_cont(value)) {
        *entry = value;
        flush_tlb_kernel_page(va);  // 刷新单页TLB
        return;
    }

    // 处理连续页表条目的特殊情况
    uint64_t table_pa_mask = (((1ul << (48 - page_shift)) - 1) << page_shift);
    uint64_t prot = value & ~table_pa_mask;  // 提取保护位
    
    // 找到连续页表条目组的起始位置
    // 连续页表条目总是对齐到CONT_PTES边界
    uint64_t *p = (uint64_t *)((uintptr_t)entry & ~(sizeof(entry) * CONT_PTES - 1));
    
    // 更新所有连续页表条目的保护位，保持物理地址不变
    for (int i = 0; i < CONT_PTES; ++i, ++p)
        *p = (*p & table_pa_mask) | prot;

    *entry = value;  // 设置目标条目的完整值
    
    // 刷新连续页表条目对应的整个TLB范围
    va &= CONT_PTE_MASK;  // 对齐到连续条目边界
    flush_tlb_kernel_range(va, va + CONT_PTES * page_size);
}

// 设置KernelPatch自身的内存保护属性
static void prot_myself()
{
    // 获取内核代码段起始地址的页表条目
    uint64_t *kpte = pgtable_entry_kernel(kernel_stext_va);
    log_boot("Kernel stext prot: %llx\n", *kpte);

    // 计算KernelPatch整个内存区域的范围
    _kp_region_start = (uint64_t)_kp_text_start;
    _kp_region_end = (uint64_t)_kp_end + align_ceil(start_preset.extra_size, page_size) + HOOK_ALLOC_SIZE +
                     MEMORY_ROX_SIZE + MEMORY_RW_SIZE;
    log_boot("Region: %llx, %llx\n", _kp_region_start, _kp_region_end);

    uint64_t *kppte = pgtable_entry_kernel(_kp_region_start);
    log_boot("KernelPatch start prot: %llx\n", *kppte);

    // 设置代码段和只读数据段的保护属性
    uint64_t text_start = (uint64_t)_kp_text_start;
    uint64_t text_end = (uint64_t)_kp_text_end;
    uint64_t align_text_end = align_ceil(text_end, page_size);
    log_boot("Text: %llx, %llx\n", text_start, text_end);

    // 遍历代码段的每个页面，设置正确的内存保护属性
    for (uint64_t i = text_start; i < align_text_end; i += page_size) {
        uint64_t *pte = pgtable_entry_kernel(i);
        // 设置为共享页面，清除特权不可执行位，清除Guardian Page位
        *pte = (*pte | PTE_SHARED) & ~PTE_PXN & ~PTE_GP;
        if (has_vmalloc_area()) {
            // 在vmalloc区域中设置为只读，清除脏页标记位
            *pte = (*pte | PTE_RDONLY) & ~PTE_DBM;
        }
    }
    flush_tlb_kernel_range(text_start, align_text_end);

    // 设置数据段和BSS段的内存保护属性
    uint64_t data_start = (uint64_t)_kp_data_start;
    uint64_t data_end = (uint64_t)_kp_data_end;
    uint64_t align_data_end = align_ceil(data_end, page_size);
    log_boot("Data: %llx, %llx\n", data_start, data_end);

    // 遍历数据段的每个页面，设置为可读写、不可执行
    for (uint64_t i = data_start; i < align_data_end; i += page_size) {
        uint64_t *pte = pgtable_entry_kernel(i);
        // 设置脏页标记和共享位，清除只读位以允许写入
        *pte = (*pte | PTE_DBM | PTE_SHARED) & ~PTE_RDONLY;
        if (has_vmalloc_area()) {
            // 在vmalloc区域中设置特权不可执行位
            *pte |= PTE_PXN;
        }
    }
    flush_tlb_kernel_range(data_start, align_data_end);

    // 设置额外数据区域的内存保护属性
    _kp_extra_start = (uint64_t)_kp_end;
    _kp_extra_end = _kp_extra_start + start_preset.extra_size;
    uint64_t align_extra_end = align_ceil(_kp_extra_end, page_size);
    log_boot("Extra: %llx, %llx\n", _kp_extra_start, _kp_extra_end);

    // 设置额外数据区域为可读写、不可执行
    for (uint64_t i = _kp_extra_start; i < align_extra_end; i += page_size) {
        uint64_t *pte = pgtable_entry_kernel(i);
        // 允许读写访问，设置为共享页面
        *pte = (*pte | PTE_DBM | PTE_SHARED) & ~PTE_RDONLY;
        if (has_vmalloc_area()) {
            // 确保数据区域不可执行
            *pte |= PTE_PXN;
        }
    }
    flush_tlb_kernel_range(_kp_extra_start, align_extra_end);

    // 设置Hook区域为可读写可执行（用于动态代码注入和Hook功能）
    _kp_hook_start = (uint64_t)align_extra_end;
    _kp_hook_end = _kp_hook_start + HOOK_ALLOC_SIZE;
    log_boot("Hook: %llx, %llx\n", _kp_hook_start, _kp_hook_end);

    // Hook区域需要可执行权限以支持动态生成的Hook代码
    for (uint64_t i = _kp_hook_start; i < _kp_hook_end; i += page_size) {
        uint64_t *pte = pgtable_entry_kernel(i);
        *pte = (*pte | PTE_DBM | PTE_SHARED) & ~PTE_PXN & ~PTE_RDONLY & ~PTE_GP;
    }
    flush_tlb_kernel_range(_kp_hook_start, _kp_hook_end);
    // 将Hook区域添加到Hook内存管理器
    hook_mem_add(_kp_hook_start, HOOK_ALLOC_SIZE);

    // 设置读写内存区域
    _kp_rw_start = _kp_hook_end;
    _kp_rw_end = _kp_rw_start + MEMORY_RW_SIZE;
    log_boot("RW: %llx, %llx\n", _kp_rw_start, _kp_rw_end);

    // 设置读写内存区域为可读写、不可执行
    for (uint64_t i = _kp_rw_start; i < _kp_rw_end; i += page_size) {
        uint64_t *pte = pgtable_entry_kernel(i);
        *pte = (*pte | PTE_DBM | PTE_SHARED) & ~PTE_RDONLY;
        if (has_vmalloc_area()) {
            *pte |= PTE_PXN;
        }
    }
    flush_tlb_kernel_range(_kp_rw_start, _kp_rw_end);
    // 初始化读写内存分配器
    kp_rw_mem = tlsf_create_with_pool((void *)_kp_rw_start, MEMORY_RW_SIZE);

    // 设置只读可执行内存区域
    // 先在读写区域分配TLSF管理结构
    kp_rox_mem = tlsf_malloc(kp_rw_mem, tlsf_size());
    tlsf_create(kp_rox_mem);

    _kp_rox_start = _kp_rw_end;
    _kp_rox_end = _kp_rox_start + MEMORY_ROX_SIZE;
    log_boot("ROX: %llx, %llx\n", _kp_rox_start, _kp_rox_end);

    // 将只读可执行区域添加到分配器池中
    tlsf_add_pool(kp_rox_mem, (void *)_kp_rox_start, MEMORY_ROX_SIZE);

    // 设置只读可执行区域的保护属性
    for (uint64_t i = _kp_rox_start; i < _kp_rox_end; i += page_size) {
        uint64_t *pte = pgtable_entry_kernel(i);
        *pte = (*pte | PTE_SHARED) & ~PTE_PXN & ~PTE_GP;
        // TODO: TLSF的block_split会写入已分配的内存
        // 暂时不设置为只读，因为TLSF需要写入权限
        // if (has_vmalloc_area()) {
        // 注释：这里可以选择设置ROX区域为只读
        // *pte |= PTE_RDONLY;
        // *pte &= ~PTE_DBM;
        // }
    }
    flush_tlb_kernel_range(_kp_rox_start, _kp_rox_end);

    // 将KernelPatch区域添加到内核vmalloc区域管理系统
    // 这样可以让内核正确管理这块内存区域
    void (*vm_area_add_early)(struct vm_struct *vm) =
        (typeof(vm_area_add_early))kallsyms_lookup_name("vm_area_add_early");

    if (vm_area_add_early) {
        kp_vm.addr = (void *)_kp_region_start;                      // 虚拟地址起始
        kp_vm.phys_addr = kp_kimg_to_phys(_kp_region_start);       // 对应的物理地址
        kp_vm.size = _kp_region_end - _kp_region_start;            // 区域总大小
        kp_vm.flags = 0x00000044;                                  // vmalloc标志位
        kp_vm.caller = (void *)_kp_region_start;                   // 调用者标识
        vm_area_add_early(&kp_vm);  // 向内核注册这个vmalloc区域
        log_boot("add vmalloc area: %llx, %llx\n", kp_vm.addr, kp_vm.size);
    }
}

/**
 * @brief 恢复内核映射备份数据
 * @details 在KernelPatch初始化过程中，某些内核数据可能被修改，
 * 这个函数负责从备份中恢复原始数据，确保内核的正常运行
 */
static void restore_map()
{
    // 计算需要恢复的内存区域范围
    uint64_t start = kernel_va + start_preset.map_offset;
    uint64_t end = start + start_preset.map_backup_len;
    log_boot("Restore: %llx, %llx\n", start, end);

    // 遍历需要恢复的每个页面，逐页进行数据恢复
    for (uint64_t i = start; i < align_ceil(end, page_size); i += page_size) {
        uint64_t *pte = pgtable_entry_kernel(i);
        uint64_t orig = *pte;  // 保存原始页表条目
        
        // 临时设置页面为可写，以便恢复数据
        *pte = (orig | PTE_DBM) & ~PTE_RDONLY;
        flush_tlb_kernel_page(i);
        
        // 从备份缓冲区中恢复数据，按8字节对齐进行复制
        for (uint64_t j = i; j >= start && j < end && j < i + page_size; j += 8) {
            *(uint64_t *)j = *(uint64_t *)(start_preset.map_backup + (j - start));
        }
        
        // 恢复页面的原始保护属性
        *pte = orig;
        flush_tlb_kernel_page(i);
    }
    
    // 刷新指令缓存确保所有修改都生效
    flush_icache_all();
}

/**
 * @brief 寄存器日志记录宏
 * @details 用于调试时记录系统寄存器的值
 */
#define log_reg(regname)                                                   \
    do {                                                                   \
        uint64_t regname##_val = 0;                                        \
        asm volatile("mrs %[val], " #regname : [val] "+r"(regname##_val)); \
        log_boot("" #regname ": %llx\n", regname##_val);                   \
    } while (0)

// 记录重要的系统寄存器值（用于调试和分析）
static void log_regs()
{
    // 以下寄存器被注释掉以避免可能的访问权限问题：
    // log_reg(APDAKey_EL1); //      | R/W [1] | 数据指针认证密钥A（高低位对）
    // log_reg(APDBKey_EL1); //      | R/W [1] | 数据指针认证密钥B（高低位对）
    // log_reg(APGAKey_EL1); //      | R/W [1] | 通用指针认证密钥（高低位对）
    // log_reg(APIAKey_EL1); //      | R/W [1] | 指令指针认证密钥A（高低位对）
    // log_reg(APIBKey_EL1); //      | R/W [1] | 指令指针认证密钥B（高低位对）
    // log_reg(CTR_EL0); //          | R   [5] | 缓存类型寄存器
    // log_reg(HCR_EL2); //          | R   [2] | 管理程序配置寄存器
    log_reg(ID_AA64AFR0_EL1); //  | R       | AArch64辅助特性寄存器0
    log_reg(ID_AA64AFR1_EL1); //  | R       | AArch64辅助特性寄存器1
    log_reg(ID_AA64DFR0_EL1); //  | R       | AArch64调试特性寄存器0
    log_reg(ID_AA64DFR1_EL1); //  | R       | AArch64调试特性寄存器1
    // log_reg(ID_AA64ISAR0_EL1); // | R       | AArch64指令集属性寄存器0
    // log_reg(ID_AA64ISAR1_EL1); // | R       | AArch64指令集属性寄存器1
    // log_reg(ID_AA64ISAR2_EL1); // | R       | AArch64指令集属性寄存器2
    log_reg(ID_AA64MMFR0_EL1); // | R       | AArch64内存模型特性寄存器0
    log_reg(ID_AA64MMFR1_EL1); // | R       | AArch64内存模型特性寄存器1
    log_reg(ID_AA64MMFR2_EL1); // | R       | AArch64内存模型特性寄存器2
    // log_reg(ID_AA64MMFR3_EL1); // | R       | AArch64内存模型特性寄存器3
    // log_reg(ID_AA64MMFR4_EL1); // | R       | AArch64内存模型特性寄存器4
    log_reg(ID_AA64PFR0_EL1); //  | R       | AArch64处理器特性寄存器0
    log_reg(ID_AA64PFR1_EL1); //  | R       | AArch64处理器特性寄存器1
    // log_reg(ID_AA64PFR2_EL1); //  | R       | AArch64处理器特性寄存器2
    // log_reg(ID_AA64SMFR0_EL1); // | R       | SME特性ID寄存器0
    // log_reg(ID_AA64ZFR0_EL1); //  | R       | SVE特性ID寄存器0
    log_reg(MAIR_EL1); //         | R       | 内存属性间接寄存器（EL1）
    // log_reg(MAIR2_EL1); //        | R       | 扩展内存属性间接寄存器（EL1）
    log_reg(MIDR_EL1); //         | R       | 主要ID寄存器
    log_reg(MPIDR_EL1); //        | R       | 多处理器亲和性寄存器
    // log_reg(PIR_EL1); //          | R       | 权限间接寄存器1（EL1）
    // log_reg(PIRE0_EL1); //        | R       | 权限间接寄存器0（EL1）
    log_reg(REVIDR_EL1); //       | R       | 修订ID寄存器
    // log_reg(RNDR); //             | R       | 随机数
    // log_reg(RNDRRS); //           | R       | 重新种子随机数
    // log_reg(SCR_EL3); //          |     [3] | 安全配置寄存器（EL3）
    log_reg(SCTLR_EL1); //        | R/W     | 系统控制寄存器（EL1）
    // log_reg(SCTLR2_EL1); //       | R/W     | 系统控制寄存器2（EL1）
    // log_reg(SCXTNUM_EL0); //      | R/W     | EL0读写软件上下文号
    // log_reg(SCXTNUM_EL1); //      | R/W     | EL1读写软件上下文号
    log_reg(TCR_EL1); //          | R       | 转换控制寄存器（EL1）
    // log_reg(TCR2_EL1); //         | R       | 扩展转换控制寄存器（EL1）
    // log_reg(TPIDR_EL0); //        | R/W [5] | EL0读写软件线程ID寄存器
    // log_reg(TPIDR_EL1); //        | R/W [5] | EL1软件线程ID寄存器
    // log_reg(TPIDRRO_EL0); //      | R/W [5] | EL0只读软件线程ID寄存器
    // log_reg(TRCDEVARCH); //       | R       | 跟踪设备架构寄存器
    log_reg(TTBR0_EL1); //        | R       | 转换表基址寄存器0（EL1）
    log_reg(TTBR1_EL1); //        | R       | 转换表基址寄存器1（EL1）
    // log_reg(PMMIR_EL1); //        | R       | 性能监视器机器标识寄存器
    // log_reg(PMSIDR_EL1); //       | R   [4] | 采样性能分析ID寄存器
}

// KernelPatch启动初始化函数
static void start_init(uint64_t kimage_voff, uint64_t linear_voff)
{
    // 设置地址空间偏移量
    kimage_voffset = kimage_voff;
    linear_voffset = linear_voff;

    // 计算内核地址信息
    kernel_pa = start_preset.kernel_pa;
    kernel_va = kimage_voff + kernel_pa;
    kernel_size = start_preset.kernel_size;
    runtime_base_addr = (uint64_t)_link_base;

    // 初始化内核符号查找函数
    uint64_t kallsym_addr = kernel_va + start_preset.kallsyms_lookup_name_offset;
    kallsyms_lookup_name = (typeof(kallsyms_lookup_name))(kallsym_addr);
    // 查找重要的内核函数地址
    kernel_stext_va = kallsyms_lookup_name("_stext");
    printk = (typeof(printk))kallsyms_lookup_name("printk");
    if (!printk) printk = (typeof(printk))kallsyms_lookup_name("_printk");

    vsnprintf = (typeof(vsnprintf))kallsyms_lookup_name("vsnprintf");

    // 输出KernelPatch启动横幅
    log_boot(KERNEL_PATCH_BANNER);

    // 检测系统字节序
    endian = *(unsigned char *)&(uint16_t){ 1 } ? little : big;
    setup_header = &start_preset.header;
    // 构建版本号
    kver = VERSION(start_preset.kernel_version.major, start_preset.kernel_version.minor,
                   start_preset.kernel_version.patch);
    kpver = VERSION(setup_header->kp_version.major, setup_header->kp_version.minor, setup_header->kp_version.patch);

    // 记录关键地址信息
    log_boot("Kernel pa: %llx\n", kernel_pa);
    log_boot("Kernel va: %llx\n", kernel_va);

    log_boot("Kernel Version: %x\n", kver);
    log_boot("KernelPatch Version: %x\n", kpver);
    log_boot("KernelPatch Config: %llx\n", setup_header->config_flags);
    log_boot("KernelPatch Compile Time: %s\n", (uint64_t)setup_header->compile_time);

    log_boot("KernelPatch link base: %llx, runtime base: %llx\n", link_base_addr, runtime_base_addr);

    // 初始化符号表遍历函数
    kallsyms_on_each_symbol = (typeof(kallsyms_on_each_symbol))kallsyms_lookup_name("kallsyms_on_each_symbol");

    // 解析TCR_EL1寄存器获取地址空间配置
    uint64_t tcr_el1;
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr_el1));
    uint64_t t1sz = bits(tcr_el1, 21, 16);
    va_bits = 64 - t1sz;
    uint64_t tg1 = bits(tcr_el1, 31, 30);

    // 根据页面大小配置确定页面位移量
    page_shift = 12;
    if (tg1 == 1) {
        page_shift = 14;   // 16KB页面大小
    } else if (tg1 == 3) {
        page_shift = 16;   // 64KB页面大小
    }
    page_size = 1 << page_shift;  // 计算实际页面大小

    // 根据虚拟地址位数和页面大小计算页表级别数
    page_level = (va_bits - 4) / (page_shift - 3);

    // 获取页全局目录地址（从TTBR1_EL1寄存器）
    uint64_t ttbr1_el1;
    asm volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1_el1));
    uint64_t baddr = ttbr1_el1 & 0xFFFFFFFFFFFE;  // 清除低位标志位
    uint64_t page_size_mask = ~(page_size - 1);   // 页面大小对齐掩码
    pgd_pa = baddr & page_size_mask;              // 页目录物理地址
    pgd_va = phys_to_virt(pgd_pa);               // 转换为虚拟地址
}

// 前置函数声明
void symbol_init();  // 符号表初始化函数
int patch();         // 补丁应用函数

/**
 * @brief KernelPatch主启动函数
 * @details 这是KernelPatch的入口点，负责完整的初始化流程
 * 包括内存管理、符号解析、补丁应用等关键步骤
 * @param kimage_voff 内核镜像虚拟地址偏移
 * @param linear_voff 线性映射虚拟地址偏移
 * @return 返回启动结果，0表示成功，非0表示失败
 */
int __attribute__((section(".start.text"))) __noinline start(uint64_t kimage_voff, uint64_t linear_voff)
{
    int rc = 0;
    
    // 步骤1: 初始化基础配置和全局变量
    start_init(kimage_voff, linear_voff);
    
    // 步骤2: 设置KernelPatch自身的内存保护属性
    prot_myself();
    
    // 步骤3: 恢复被修改的内核映射数据
    restore_map();
    
    // 步骤4: 记录关键系统寄存器状态用于调试
    log_regs();
    
    // 步骤5: 初始化预设数据和配置
    predata_init();
    
    // 步骤6: 初始化内核符号表解析
    symbol_init();
    
    // 步骤7: 应用所有配置的补丁
    rc = patch();
    
    return rc;  // 返回整体启动结果
}
