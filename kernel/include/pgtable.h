// ARM64页表管理头文件
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#ifndef _KP_PGTABLE_H_
#define _KP_PGTABLE_H_

#include <ktypes.h>

// 内存类型定义宏（暂未实现）
#define MT_DEVICE_nGnRnE
#define MT_DEVICE_nGnRE
#define MT_DEVICE_GRE
#define MT_NORMAL_NC
#define MT_NORMAL
#define MT_NORMAL_WT

// 页表项标志位定义
#define PTE_VALID (1ul << 0)         // 页表项有效位
#define PTE_TYPE_MASK (3ul << 0)     // 页表项类型掩码
#define PTE_TYPE_PAGE (3ul << 0)     // 页表项类型：页面
#define PTE_TABLE_BIT (1ul << 1)     // 页表项表位
#define PTE_ATTRINDX(t) (t << 2)     // 属性索引[2:0]编码（映射MAIR_EL*寄存器中定义的属性）
#define PTE_NS (1ul << 5)            // 非安全访问控制
#define PTE_USER (1ul << 6)          // AP[1] 用户访问位
#define PTE_RDONLY (1ul << 7)        // AP[2] 只读位
#define PTE_SHARED (3ul << 8)        // SH[1:0] 内部可共享
#define PTE_AF (1ul << 10)           // 访问标志位
#define PTE_NG (1ul << 11)           // nG 非全局位
#define PTE_GP (1ul << 50)           // BTI保护位
#define PTE_DBM (1ul << 51)          // 脏位管理
#define PTE_CONT (1ul << 52)         // 连续范围位
#define PTE_PXN (1ul << 53)          // 特权执行禁止
#define PTE_UXN (1ul << 54)          // 用户执行禁止

// 页表项扩展标志位
#define PTE_WRITE (PTE_DBM)          // 写权限位（与DBM相同，位51）
#define PTE_SWP_EXCLUSIVE (1ul << 2) // 交换页独占位（仅用于交换pte）
#define PTE_DIRTY (1ul << 55)        // 软件脏位（某些版本中）
#define PTE_SPECIAL (1ul << 56)      // 特殊页位
#define PTE_DEVMAP (1ul << 57)       // 设备映射位
#define PTE_PROT_NONE (1ul << 58)    // 无保护位（仅当!PTE_VALID时）

// PMD相关标志位
#define PMD_PRESENT_INVALID (1ul << 59) // PMD存在但无效（仅当!PMD_SECT_VALID时）

// 页表属性标志位
#define PTATTR_PXN (1ul << 59)       // 特权执行禁止属性
#define PTATTR_XN (1ul << 60)        // 执行禁止属性
#define PTATTR_USER (1ul << 61)      // AP[1] 在el0不允许读取
#define PTATTR_RDONLY (1ul << 62)    // AP[2] 在任何异常级别都不允许写入
#define PTATTR_NS (1ul << 63)        // 指示表标识符是否位于安全PA空间

// 检查页表项是否为有效的连续页表项
#define pte_valid_cont(pte)	(((pte) & (PTE_VALID | PTE_TABLE_BIT | PTE_CONT)) == (PTE_VALID | PTE_TABLE_BIT | PTE_CONT))

// 连续页表项相关定义
#define CONT_PTE_SHIFT (4 + page_shift)      // 连续PTE偏移量
#define CONT_PTES (1 << (CONT_PTE_SHIFT - page_shift))  // 连续PTE数量
#define CONT_PTE_SIZE (CONT_PTES * page_size)           // 连续PTE大小
#define CONT_PTE_MASK (~(CONT_PTE_SIZE - 1))            // 连续PTE掩码

// 位掩码工具宏，生成从位l到位h的掩码
#define mask_ul(h, l) (((~0ul) << (l)) & (~0ul >> (63 - (h))))

// ARM64处理器指令宏
#define sev() asm volatile("sev" : : : "memory")    // 发送事件指令
#define wfe() asm volatile("wfe" : : : "memory")    // 等待事件指令
#define wfi() asm volatile("wfi" : : : "memory")    // 等待中断指令

// 内存屏障指令
#define isb() asm volatile("isb" : : : "memory")    // 指令同步屏障
#define dmb(opt) asm volatile("dmb " #opt : : : "memory")  // 数据内存屏障
#define dsb(opt) asm volatile("dsb " #opt : : : "memory")  // 数据同步屏障

// TLB无效化指令（无参数版本）
#define tlbi_0(op)       \
    asm("tlbi " #op "\n" \
        "dsb ish\n"      \
        "tlbi " #op "\n")

// TLB无效化指令（带参数版本）
#define tlbi_1(op, arg)      \
    asm("tlbi " #op ", %0\n" \
        "dsb ish\n"          \
        "tlbi " #op ", %0\n" \
        :                    \
        : "r"(arg))

// 刷新本地TLB的所有项
static inline void local_flush_tlb_all(void)
{
    dsb(nshst);      // 非共享存储同步屏障
    tlbi_0(vmalle1); // 无效化所有EL1虚拟地址TLB项
    dsb(nsh);        // 非共享数据同步屏障
    isb();           // 指令同步屏障
}

// 刷新全局TLB的所有项
static inline void flush_tlb_all(void)
{
    dsb(ishst);       // 内部共享存储同步屏障
    tlbi_0(vmalle1is); // 无效化所有EL1虚拟地址TLB项（内部共享）
    dsb(ish);         // 内部共享数据同步屏障
    isb();            // 指令同步屏障
}

// 构造TLB虚拟地址参数（__TLBI_VADDR宏的实现）
static inline uint64_t tlbi_vaddr(uint64_t addr, uint64_t asid)
{
    uint64_t x = addr >> 12;      // 页面地址右移12位
    x &= mask_ul(43, 0);          // 保留低44位
    x |= asid << 48;              // ASID左移48位后或运算
    return x;
}

// 全局变量声明
extern uint64_t kimage_voffset;  // 内核镜像虚拟偏移量
extern uint64_t linear_voffset;  // 线性映射虚拟偏移量
extern uint64_t kernel_va;       // 内核虚拟地址
extern uint64_t kernel_pa;       // 内核物理地址
extern int64_t kernel_size;      // 内核大小
extern int64_t page_shift;       // 页面偏移位数
extern int64_t page_size;        // 页面大小
extern int64_t va_bits;          // 虚拟地址位数
extern int64_t page_level;       // 页表级别
extern uint64_t pgd_pa;          // 页全局目录物理地址
extern uint64_t pgd_va;          // 页全局目录虚拟地址
// extern int64_t pa_bits;       // 物理地址位数（注释掉）

// 物理地址转换为虚拟地址
static inline uint64_t phys_to_virt(uint64_t phys)
{
    return phys + linear_voffset;
}

// 虚拟地址转换为物理地址
static inline uint64_t virt_to_phys(uint64_t virt)
{
    return virt - linear_voffset;
}

// 物理地址转换为内核镜像地址
static inline uint64_t phys_to_kimg(uint64_t phys)
{
    return phys + kimage_voffset;
}

// 内核镜像地址转换为物理地址
static inline uint64_t kimg_to_phys(uint64_t addr)
{
    return addr - kimage_voffset;
}

// 检查是否有vmalloc区域
static inline int has_vmalloc_area()
{
    return kimage_voffset != linear_voffset;
}

// KernelPatch内核镜像地址转物理地址
static inline uint64_t kp_kimg_to_phys(uint64_t addr)
{
    return addr - kimage_voffset;
}

// 刷新内核地址范围的TLB
static inline void flush_tlb_kernel_range(uint64_t start, uint64_t end)
{
    start = tlbi_vaddr(start, 0);  // 转换起始地址为TLB格式
    end = tlbi_vaddr(end, 0);      // 转换结束地址为TLB格式
    dsb(ishst);                    // 内部共享存储同步屏障
    // 遍历地址范围，按页面大小逐个无效化TLB项
    for (uint64_t addr = start; addr < end; addr += 1 << (page_shift - 12))
        tlbi_1(vaale1is, addr);    // 无效化虚拟地址所有EL1 TLB项（内部共享）
    dsb(ish);                      // 内部共享数据同步屏障
    isb();                         // 指令同步屏障
}

// 刷新内核单个页面的TLB
static inline void flush_tlb_kernel_page(uint64_t addr)
{
    addr = tlbi_vaddr(addr, 0);    // 转换地址为TLB格式
    dsb(ishst);                    // 内部共享存储同步屏障
    tlbi_1(vaale1is, addr);        // 无效化虚拟地址所有EL1 TLB项（内部共享）
    dsb(ish);                      // 内部共享数据同步屏障
    isb();                         // 指令同步屏障
}

// 检查地址是否在内核镜像范围内
static inline int is_kimg_range(uint64_t addr)
{
    return addr >= kernel_va && addr < (kernel_va + kernel_size);
}

// 获取页表项地址
uint64_t *pgtable_entry(uint64_t pgd, uint64_t va);

// 获取内核页表项地址
static inline uint64_t *pgtable_entry_kernel(uint64_t va)
{
    return pgtable_entry(pgd_va, va);
}

// 修改内核页表项
void modify_entry_kernel(uint64_t va, uint64_t *entry, uint64_t value);

#endif