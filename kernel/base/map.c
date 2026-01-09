/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 内存映射和重定位模块 - 负责内核补丁的内存布局管理和地址重定位

#include "setup.h"

#define NUMA_NO_NODE (-1)  // 无NUMA节点标识

// 内存块操作函数类型定义
typedef uint64_t phys_addr_t;  // 物理地址类型
typedef int (*memblock_reserve_f)(phys_addr_t base, phys_addr_t size);  // 内存块保留函数
typedef phys_addr_t (*memblock_phys_alloc_try_nid_f)(phys_addr_t size, phys_addr_t align, int nid);  // 物理内存分配函数
typedef void *(*memblock_virt_alloc_try_nid_f)(phys_addr_t size, phys_addr_t align, phys_addr_t min_addr,
                                               phys_addr_t max_addr, int nid);  // 虚拟内存分配函数
typedef int (*memblock_free_f)(phys_addr_t base, phys_addr_t size);  // 内存块释放函数
typedef int (*memblock_mark_nomap_f)(phys_addr_t base, phys_addr_t size);  // 内存块标记为非映射函数
typedef int (*printk_f)(const char *fmt, ...);  // 内核打印函数
typedef void (*paging_init_f)(void);  // 分页初始化函数

// 映射数据结构 - 存储在.map.data段中，按MAP_ALIGN对齐
map_data_t map_data __section(.map.data) __aligned(MAP_ALIGN) = {
#ifdef MAP_DEBUG
    .str_fmt_px = "KP: %x-%llx\n",  // 调试格式字符串
#endif
};

// 获取当前虚拟地址 - 存储在.map.text段中，按MAP_ALIGN对齐
uint64_t __section(.map.text) __noinline __aligned(MAP_ALIGN) get_myva()
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));  // 获取当前指令地址
    return this_va & ~((uint64_t)MAP_ALIGN - 1);  // 按对齐边界取整
}

// 获取映射数据结构指针
map_data_t *__noinline get_data()
{
    uint64_t va = get_myva() - sizeof(map_data_t);  // 计算数据结构位置
    return (map_data_t *)(va & ~((uint64_t)MAP_ALIGN - 1));  // 按对齐边界取整
}

// 获取内核虚拟地址
static uint64_t get_kva()
{
    map_data_t *data = get_data();
    uint64_t kernel_va = (uint64_t)data - data->map_offset;  // 计算内核虚拟地址
    return kernel_va;
}

// 物理地址到线性映射地址的转换
static inline uint64_t phys_to_lm(map_data_t *data, uint64_t phys)
{
    return phys + data->linear_voffset;  // 加上线性映射偏移
}

// 刷新所有TLB条目
static void flush_tlb_all()
{
    asm volatile("dsb ishst" : : : "memory");  // 数据同步屏障
    asm volatile("tlbi vmalle1is\n"            // 使TLB条目无效（内部共享）
                 "dsb ish\n"                   // 数据同步屏障（内部共享）
                 "tlbi vmalle1is\n");          // 再次使TLB条目无效
    asm volatile("dsb ish" : : : "memory");    // 数据同步屏障
    asm volatile("isb" : : : "memory");        // 指令同步屏障
}

// 刷新所有指令缓存
static void flush_icache_all(void)
{
    asm volatile("dsb ish" : : : "memory");  // 数据同步屏障
    asm volatile("ic ialluis");              // 使所有指令缓存无效（内部共享）
    asm volatile("dsb ish" : : : "memory");  // 数据同步屏障
    asm volatile("isb" : : : "memory");      // 指令同步屏障
}

// 内存处理和重定位函数
static map_data_t *mem_proc()
{
    map_data_t *data = get_data();
    uint64_t kernel_va = get_kva();

    // 地址重定位计算
    data->kimage_voffset = kernel_va - data->kernel_pa;  // 内核镜像虚拟偏移
    data->paging_init_relo += kernel_va;                 // 重定位分页初始化函数地址

    // 重定位映射符号表中的地址
    uint64_t map_symbol_addr = (uint64_t)&data->map_symbol;
    for (uint64_t addr = map_symbol_addr; addr < map_symbol_addr + MAP_SYMBOL_SIZE; addr += 8) {
        if (*(uint64_t *)addr) *(uint64_t *)addr += kernel_va;  // 非空地址加上内核虚拟偏移
    }

#ifdef MAP_DEBUG
    data->printk_relo += kernel_va;  // 重定位调试打印函数地址
#endif

    // 页表配置分析
    uint64_t tcr_el1;
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr_el1));  // 读取翻译控制寄存器
    uint64_t t1sz = tcr_el1 << 42 >> 58; // bits(tcr_el1, 21, 16) - 提取T1SZ字段
    uint64_t va1_bits = 64 - t1sz;        // 计算虚拟地址位数
    data->va1_bits = va1_bits;
    uint64_t tg1 = tcr_el1 << 32 >> 62; // bits(tcr_el1, 31, 30) - 提取TG1字段
    uint64_t page_shift = 12;           // 默认4KB页面（2^12）
    if (tg1 == 1) {
        page_shift = 14;  // 16KB页面（2^14）
    } else if (tg1 == 3) {
        page_shift = 16;  // 64KB页面（2^16）
    }
    data->page_shift = page_shift;

    // 线性映射偏移检测
    // 分配一小块物理内存用于检测线性映射偏移
    uint64_t detect_phys =
        ((memblock_phys_alloc_try_nid_f)data->map_symbol.memblock_phys_alloc_relo)(0, 0x10, NUMA_NO_NODE);
    // 分配对应的虚拟内存
    uint64_t detect_virt = (uint64_t)((memblock_virt_alloc_try_nid_f)data->map_symbol.memblock_virt_alloc_relo)(
        0, 0x10, detect_phys, detect_phys, NUMA_NO_NODE);
    // 计算线性映射的虚拟偏移
    data->linear_voffset = detect_virt - detect_phys;

    return data;
}

// TODO: 支持52位物理地址
// 获取或创建页表项 - 处理多级页表映射
static uint64_t __noinline get_or_create_pte(map_data_t *data, uint64_t va, uint64_t pa, uint64_t attr_indx)
{
    memblock_phys_alloc_try_nid_f memblock_phys_alloc_try_nid =
        (memblock_phys_alloc_try_nid_f)data->map_symbol.memblock_phys_alloc_relo;

    uint64_t page_shift = data->page_shift;   // 页面位移量
    uint64_t va_bits = data->va1_bits;        // 虚拟地址位数
    uint64_t page_level = (va_bits - 4) / (page_shift - 3);  // 页表级数
    uint64_t pxd_bits = page_shift - 3;       // 页表描述符位数
    uint64_t pxd_ptrs = 1u << pxd_bits;       // 每级页表指针数量

    uint64_t ttbr1_el1;
    asm volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1_el1));  // 读取翻译表基址寄存器
    uint64_t baddr = ttbr1_el1 & 0xFFFFFFFFFFFE;           // 提取基地址
    uint64_t page_size = 1 << page_shift;                  // 页面大小
    uint64_t page_size_mask = ~(page_size - 1);            // 页面大小掩码
    uint64_t attr_prot = 0x40000000000703 | attr_indx;     // 页面属性和保护位

    uint64_t pxd_pa = baddr & page_size_mask;  // 页表物理地址
    uint64_t pxd_va = phys_to_lm(data, pxd_pa);  // 转换为虚拟地址
    uint64_t pxd_entry_va = 0;  // 页表条目虚拟地址

    // 遍历页表层级（从顶级到最终级）
    for (uint64_t lv = 4 - page_level; lv < 4; lv++) {
        uint64_t pxd_shift = (page_shift - 3) * (4 - lv) + 3;  // 计算位移量
        uint64_t pxd_index = (va >> pxd_shift) & (pxd_ptrs - 1);  // 计算索引
        uint64_t alloc_flag = 0;  // 是否需要分配新页表
        uint64_t block_flag = 0;  // 是否为块映射

        pxd_entry_va = pxd_va + pxd_index * 8;  // 计算页表条目地址

        uint64_t pxd_desc = *((uint64_t *)pxd_entry_va);  // 读取页表描述符

        if ((pxd_desc & 0b11) == 0b11) { // 表描述符 - 指向下级页表
            pxd_pa = pxd_desc & (((1ul << (48 - page_shift)) - 1) << page_shift);
        } else if ((pxd_desc & 0b11) == 0b01) { // 块描述符 - 大页映射
            // 4k页面：lv1, lv2支持。16k和64k页面：仅lv2支持
            uint64_t block_bits = (3 - lv) * pxd_bits + page_shift;
            pxd_pa = pxd_desc & (((1ul << (48 - block_bits)) - 1) << block_bits);
            block_flag = 1;
        } else { // 无效描述符 - 需要分配新页表或页面
            if (lv != 3) {  // 非最终级别，分配新页表
                pxd_pa = memblock_phys_alloc_try_nid(page_size, page_size, 0);
                alloc_flag = 1;
            } else {  // 最终级别，使用目标物理地址
                pxd_pa = pa;
            }
            pxd_desc = (pxd_pa) | attr_prot;  // 组装页表描述符
            *((uint64_t *)pxd_entry_va) = pxd_desc;  // 写入页表条目
        }
        pxd_va = phys_to_lm(data, pxd_pa);  // 转换为虚拟地址
        if (alloc_flag) {  // 如果分配了新页表，需要清零
            for (uint64_t i = pxd_va; i < pxd_va + page_size; i += 8) {
                *(uint64_t *)i = 0;
            }
        }
        if (block_flag) {  // 如果是块映射，提前退出
            break;
        }
    }
    return pxd_entry_va;  // 返回页表条目地址
}

// TODO: 支持BTI（分支目标标识）
// 分页初始化函数 - 设置内核模块的页表映射
void __noinline _paging_init()
{
    map_data_t *data = mem_proc();  // 获取内存映射数据
#ifdef MAP_DEBUG
    printk_f printk = (printk_f)(data->printk_relo);
#define map_debug(idx, val) printk(data->str_fmt_px, idx, val)
    // 调试模式：打印映射数据结构内容
    for (int i = 0; i < sizeof(map_data_t); i += 8) {
        map_debug(i, *(uint64_t *)((uint64_t)data + i));
    }
#else
#define map_debug(idx, val)
#endif

    uint64_t page_size = 1 << data->page_shift;  // 计算页面大小
    uint64_t old_start_pa = data->start_offset + data->kernel_pa;  // 原始启动物理地址
    uint64_t reserve_size = data->start_img_size + data->extra_size;  // 预留大小
    uint64_t align_extra_size = (data->extra_size + page_size - 1) & ~(page_size - 1);  // 对齐额外大小
    uint64_t all_size = data->start_size + align_extra_size + data->alloc_size;  // 总大小

    // 预留原始启动区域，防止被覆盖
    ((memblock_reserve_f)data->map_symbol.memblock_reserve_relo)(old_start_pa, reserve_size);
    // 分配新的物理内存区域
    uint64_t start_pa =
        ((memblock_phys_alloc_try_nid_f)data->map_symbol.memblock_phys_alloc_relo)(all_size, page_size, 0);
    // 标记新分配区域为不可映射（如果支持）
    if (data->map_symbol.memblock_mark_nomap_relo)
        ((memblock_mark_nomap_f)(data->map_symbol.memblock_mark_nomap_relo))(start_pa, all_size);

    // 恢复并调用分页初始化函数
    uint64_t paging_init_va = data->paging_init_relo;
    *(uint32_t *)(paging_init_va) = data->paging_init_backup;  // 恢复原始指令
    flush_icache_all();  // 刷新指令缓存
    ((paging_init_f)(paging_init_va))();  // 调用分页初始化
    // 注意：此后无法写入数据到原始位置

    // 获取内核文本段的属性索引（AttrIndx[2:0] 编码）
    uint64_t ktext_pte = get_or_create_pte(data, data->paging_init_relo, 0, 0);
    uint64_t attrs = *(uint64_t *)ktext_pte;
    uint64_t attr_indx = attrs & 0b11100;

    // 清除WXN位（写执行从不）
    // TODO: 稍后恢复WXN设置
    uint64_t sctlr_el1 = 0;
    asm volatile("mrs %[reg], sctlr_el1" : [reg] "+r"(sctlr_el1));
    sctlr_el1 &= 0xFFFFFFFFFFF7FFFF;  // 清除WXN位
    asm volatile("msr sctlr_el1, %[reg]" : : [reg] "r"(sctlr_el1));

    // 准备内存移动操作
    uint64_t old_start_va = phys_to_lm(data, old_start_pa);  // 原始虚拟地址
    uint64_t start_va = start_pa + data->kimage_voffset;     // 新的虚拟地址

    // uint64_t vm_gurad_enough = page_size << 3;

    for (uint64_t off = 0; off < all_size; off += page_size) {
        uint64_t entry = get_or_create_pte(data, start_va + off, start_pa + off, attr_indx);
        // 设置页面权限：可写可执行，清除访问标志和dirty位
        *(uint64_t *)entry = (*(uint64_t *)entry | 0x8000000000000) & 0xFFDFFFFFFFFFFF7F;
    }
    flush_tlb_all();  // 刷新TLB

    // 复制内存内容到新位置
    for (uint64_t i = start_va; i < start_va + all_size; i += 8) {
        *(uint64_t *)i = 0;  // 先清零整个区域
    }
    // 复制原始镜像内容
    for (uint64_t i = 0; i < data->start_img_size; i += 8) {
        *(uint64_t *)(start_va + i) = *(uint64_t *)(old_start_va + i);
    }
    // 复制额外数据内容
    for (uint64_t i = 0; i < data->extra_size; i += 8) {
        *(uint64_t *)(start_va + data->start_size + i) = *(uint64_t *)(old_start_va + data->start_img_size + i);
    }

    flush_icache_all();  // 刷新指令缓存确保新代码可执行

    // 释放原始启动区域内存
    ((memblock_free_f)data->map_symbol.memblock_free_relo)(old_start_pa, reserve_size);

    // 启动新映射的内核代码
    ((start_f)start_va)(data->kimage_voffset, data->linear_voffset);
    // 注意：这个调用不会返回，控制权转移到新位置的内核代码
}