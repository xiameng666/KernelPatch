/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * wxshadow.c - W^X 影子内存 KPM 模块
 *
 * 【核心原理】
 * 利用 ARM64 页表的 W^X (写 异或 执行) 机制实现隐藏断点。
 * 模块为目标内存页创建一份"影子副本"，在影子页中植入 BRK #7 指令。
 *
 * 【工作流程】
 * 1. 设置断点时：复制原始页 -> 在影子页写入 BRK #7 -> 将PTE指向影子页(可执行)
 * 2. 程序读取该地址时：触发页错误 -> 切换PTE到原始页(只读) -> 程序读到正常代码
 * 3. 程序执行该地址时：触发页错误 -> 切换PTE到影子页(可执行) -> 执行到 BRK #7
 * 4. 命中 BRK #7 时：打印寄存器 -> 应用寄存器修改 -> 切换到原始页 -> 单步执行原始指令
 * 5. 单步完成后：切换回影子页 -> 等待下次命中
 *
 * 【对抗CRC32检测的关键】
 * 当用户态程序尝试读取/校验代码段时，页错误处理器会自动切回原始页，
 * 使CRC32校验看到的是未修改的原始代码，从而绕过完整性检测。
 *
 * 由反编译二进制文件重建。
 */

/* KernelPatch 框架头文件 */
#include <compiler.h>        /* 编译器属性宏 */
#include <kpmodule.h>        /* KPM 模块生命周期宏 (KPM_INIT/KPM_EXIT 等) */
#include <linux/printk.h>    /* pr_info/pr_err 内核日志 */
#include <linux/string.h>    /* memcpy/memset/strcmp */
#include <linux/list.h>      /* 内核链表 list_head/list_for_each_entry */
#include <linux/mm_types.h>  /* mm_struct/vm_area_struct 类型定义 */
#include <linux/sched.h>     /* task_struct 相关 */
#include <asm/current.h>     /* get_current() 获取当前任务 */
#include <asm/ptrace.h>      /* struct pt_regs 寄存器上下文 */
#include <hook.h>            /* KP 框架的 hook_wrap/hook_unwrap_remove API */
#include <ksyms.h>           /* kallsyms_lookup_name 符号查找 */
#include <syscall.h>         /* hook_syscalln/unhook_syscalln 系统调用hook */
#include <pgtable.h>         /* mm_struct_offset 等框架提供的偏移量 */
#include <common.h>          /* 通用工具宏 */
#include <stdint.h>          /* uint8_t/uint32_t/uint64_t */

KPM_NAME("wxshadow");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("wxshadow");
KPM_DESCRIPTION("W^X Shadow Memory - Hidden Breakpoint Mechanism");

/* ========== 常量定义 ========== */

/*
 * 用户态通过 prctl() 系统调用与本模块通信的命令码
 * 格式: prctl(命令码, 目标PID, 地址, [寄存器号], [值])
 * PID=0 表示操作当前进程
 */
#define WXSHADOW_PRCTL_SET_BP   0x57585801  /* 设置隐藏断点 */
#define WXSHADOW_PRCTL_SET_REG  0x57585802  /* 设置断点命中时的寄存器修改 */
#define WXSHADOW_PRCTL_DEL_BP   0x57585803  /* 删除隐藏断点 */

#define BRK_IMM_NUM             7           /* BRK 指令的立即数编号 */
#define BRK_INSN                0xD42000E0u /* BRK #7 的机器码 (ARM64) */
#define PRCTL_NR                167         /* prctl 系统调用号 (ARM64) */

#define MAX_BP_PER_PAGE         16          /* 每个4KB页面最多支持16个断点 */
#define MAX_REG_MOD             4           /* 每个断点最多修改4个寄存器 */
#define WX_PAGE_SIZE            4096        /* 页面大小 4KB */

/*
 * 页面状态机 - 控制页面在原始/影子之间的切换
 *
 * NONE -> SHADOW_X (设置断点后，影子页可执行)
 * SHADOW_X -> ORIG_R (程序读取时，切到原始页只读，对抗CRC校验)
 * ORIG_R -> SHADOW_X (程序执行时，切回影子页)
 * SHADOW_X -> STEPPING (BRK命中，切到原始页单步执行)
 * STEPPING -> SHADOW_X (单步完成，切回影子页)
 */
#define STATE_NONE              0   /* 未激活 - 无影子页 */
#define STATE_ORIG_R            1   /* 原始页已映射，只读(r--)，不可执行 */
#define STATE_SHADOW_X          2   /* 影子页已映射，可执行(--x)，含BRK指令 */
#define STATE_STEPPING          3   /* 正在单步执行中，原始页映射 */

/*
 * ARM64 PTE (页表项) 标志位
 * 通过操控PTE实现页面权限的动态切换
 */
#define PTE_BASE_FLAGS          0xF03ULL            /* PTE基础标志: Valid + AF + SH + AP等 */
#define PTE_USER_RDONLY         0xC0ULL              /* 用户态只读，可执行 (用于单步状态) */
#define PTE_UXN_USER_RO         0x400000000000C0ULL  /* UXN(不可执行) + 用户只读 (用于读保护) */

/* 内核内存分配标志 GFP_KERNEL (常见内核值=3264) */
#define WX_GFP_KERNEL           3264

#define PFN_MASK                0xFFFFFFFFFULL       /* 页帧号掩码 (36位) */
#define PAGE_MASK_4K            (~0xFFFULL)          /* 4KB页面对齐掩码 */
#define PA_MASK                 0xFFFFFFFFF000ULL    /* 物理地址掩码 (去掉低12位) */

/* ========== 数据结构 ========== */

/*
 * 寄存器修改条目 (16字节)
 * 当断点命中时，可以自动修改指定寄存器的值
 * 用于实现无感知的函数参数/返回值篡改
 */
struct reg_mod {
    uint8_t  reg_num;   /* 寄存器编号: 0-30=x0~x30, 31=sp */
    uint8_t  active;    /* 是否启用: 1=启用, 0=禁用 */
    uint8_t  _pad[6];   /* 对齐填充 */
    uint64_t value;     /* 要设置的目标值 */
};

/*
 * 断点条目 (88字节)
 * 记录一个断点的地址、状态和关联的寄存器修改
 */
struct bp_entry {
    uint64_t       addr;              /* 断点虚拟地址 (用户态地址) */
    uint8_t        active;            /* 断点是否激活 */
    uint8_t        _pad[7];
    struct reg_mod mods[MAX_REG_MOD]; /* 最多4个寄存器修改 */
    uint32_t       mod_count;         /* 当前寄存器修改数量 */
    uint32_t       _pad2;
};

/*
 * 页面信息 (1464字节)
 * 管理单个4KB页面的影子映射状态
 * 每个页面可包含最多16个断点
 */
struct page_info {
    uint64_t        orig_pfn;         /* 原始页帧号 (PFN) */
    uint64_t        shadow_pfn;       /* 影子页帧号 */
    uint64_t        _reserved;
    uint64_t        shadow_page_va;   /* 影子页的内核虚拟地址 (用于读写影子页内容) */
    uint32_t        state;            /* 页面状态: STATE_NONE/ORIG_R/SHADOW_X/STEPPING */
    uint32_t        _pad;
    uint64_t        stepping_task;    /* 正在单步执行的 task_struct 指针 */
    struct bp_entry bps[MAX_BP_PER_PAGE]; /* 该页面上的断点数组 */
    uint32_t        bp_count;         /* 当前断点数量 */
    uint32_t        _pad2;
};

/*
 * W^X 区域 (56字节)
 * 对应一个 VMA (虚拟内存区域)，管理该区域内所有页面的影子映射
 * 所有区域通过链表连接，按 mm_struct 分组
 */
struct wx_region {
    struct list_head list;        /* 全局链表节点 */
    void            *mm;         /* 所属进程的 mm_struct */
    unsigned long    vm_start;   /* VMA 起始地址 */
    unsigned long    vm_end;     /* VMA 结束地址 */
    struct page_info *pages;     /* 页面信息数组，nr_pages个元素 */
    int              nr_pages;   /* 该VMA包含的页面数 */
    int              refcount;   /* 引用计数，防止使用中被释放 */
};

/* ========== 内核函数指针 ========== */
/*
 * 运行时通过 kallsyms_lookup_name() 动态解析的内核函数指针
 * 这些函数在内核中没有导出，必须通过符号表查找
 */

/* 内存管理函数 */
static void *(*kfn_find_vma)(void *mm, unsigned long addr);             /* 查找包含指定地址的VMA */
static void *(*kfn_get_task_mm)(void *task);                            /* 获取任务的mm_struct，增加引用计数 */
static void  (*kfn_mmput)(void *mm);                                    /* 释放mm_struct引用 */
static void *kfn_exit_mmap;                                             /* exit_mmap函数地址 (进程退出时清理) */

/* 页面分配/释放 */
static unsigned long (*kfn___get_free_pages)(unsigned int gfp, unsigned int order); /* 分配物理页 */
static void  (*kfn_free_pages)(unsigned long addr, unsigned int order);  /* 释放物理页 */
static void  (*kfn_put_page)(void *page);                               /* 减少页面引用计数 */
static void  (*kfn_get_page)(void *page);                               /* 增加页面引用计数 */

/* 单步调试控制 */
static void  (*kfn_user_enable_single_step)(void *task);                /* 启用用户态单步执行 */
static void  (*kfn_user_disable_single_step)(void *task);               /* 禁用用户态单步执行 */

/* 缓存/TLB 操作 */
static void  (*kfn___flush_icache_range)(unsigned long start, unsigned long end); /* 刷新指令缓存 */
static void  (*kfn_flush_dcache_page)(void *page);                      /* 刷新数据缓存页 */
static void  (*kfn___flush_tlb_range)(void *vma, unsigned long start, unsigned long end,
                                       unsigned long stride, int last_level, int tlb_level); /* TLB范围刷新 */
static void *(*kfn_flush_tlb_page)(void);                               /* TLB单页刷新 */
static void  (*kfn___sync_icache_dcache)(void *page);                   /* 同步指令/数据缓存 */

/* RCU 读写锁 (用于安全遍历任务列表) */
static void  (*kfn_rcu_read_lock)(void);
static void  (*kfn_rcu_read_unlock)(void);

/* 内核内存分配 */
static void *(*kfn_kzalloc)(unsigned long size, unsigned int flags);     /* 分配并清零内存 */
static void *(*kfn_kcalloc)(unsigned long n, unsigned long size, unsigned int flags); /* 分配数组 */
static void  (*kfn_kfree)(void *ptr);                                   /* 释放内核内存 */
static int   (*kfn_copy_from_kernel_nofault)(void *dst, const void *src, unsigned long size); /* 安全内存读取 */

/* hook目标函数地址 */
static void *kfn_do_page_fault;       /* 页错误处理器 (可选，用于读/执行错误拦截) */
static void *kfn_brk_handler;         /* BRK异常处理器 (必须，断点核心) */
static void *kfn_single_step_handler; /* 单步异常处理器 (必须，单步核心) */

/* 自旋锁 */
static void *(*kfn__raw_spin_lock)(void *lock);
static void *(*kfn__raw_spin_unlock)(void *lock);

/* 任务查找 */
static void *(*kfn_find_task_by_vpid)(int pid);                         /* 通过PID查找任务 */
static int   (*kfn___task_pid_nr_ns)(void *task, int type, void *ns);   /* 获取任务的PID/TGID */
static void *kfn_init_task;                                             /* init_task (swapper进程，PID=0) */

/* 内核变量指针 (用于物理地址↔虚拟地址转换) */
static uint64_t *kvar_memstart_addr;    /* 物理内存起始地址 */
static uint64_t *kvar_physvirt_offset;  /* 物理-虚拟地址偏移 (KASLR模式) */

/* 运行时检测的值 */
static uint64_t detected_physvirt_offset;  /* 通过AT指令检测的物理-虚拟偏移 */
static int      physvirt_offset_valid;     /* 上述偏移是否有效 */
static uint64_t wx_page_offset_base;       /* PAGE_OFFSET 基址 (从 TCR_EL1 计算) */
static int      wx_page_level;             /* 页表级数 (3或4) */
static int      wx_page_shift;             /* 页大小位移: 12=4KB, 14=16KB, 16=64KB */
static int      wx_mm_context_id_offset = -1; /* mm->context.id 在mm_struct中的偏移 */
static int      wx_mm_context_id_asid_shift = 0; /* ASID在context.id字段中的bit偏移 (0=标准, 48=MTK完整TTBR0) */
static int      wx_debug_mode = 0;              /* 调试模式: 加载时传参 'd' 启用详细dump */
static int16_t  wx_vma_vm_mm_offset = 64;  /* vm_area_struct.vm_mm 偏移 (默认64) */
static unsigned int wx_gfp_kernel = WX_GFP_KERNEL;

/* 前向声明 */
static int try_scan_mm_context_id_offset(void);

/* 全局状态 */
static struct list_head region_list;  /* 所有 wx_region 的全局链表 */
static uint64_t global_lock;         /* 全局自旋锁，保护 region_list 和 page_info */

/* ========== 物理地址 ↔ 虚拟地址 转换 ========== */
/*
 * ARM64 内核使用线性映射 (linear mapping) 将物理内存映射到内核虚拟地址。
 * 转换方式取决于内核配置:
 *   1. physvirt_offset 模式 (KASLR): VA = PA + physvirt_offset
 *   2. memstart_addr 模式 (传统): VA = PA + PAGE_OFFSET - memstart_addr
 *
 * 我们需要这些转换来操控页表项(PTE)中的物理地址
 */

/* 物理地址 -> 内核虚拟地址 */
static inline uint64_t wx_pa_to_va(uint64_t pa)
{
    if (physvirt_offset_valid)
        return pa + detected_physvirt_offset;
    if (kvar_physvirt_offset)
        return pa + *kvar_physvirt_offset;
    return wx_page_offset_base - *kvar_memstart_addr + pa;
}

/* 内核虚拟地址 -> 物理地址 */
static inline uint64_t wx_va_to_pa(uint64_t va)
{
    if (physvirt_offset_valid)
        return va - detected_physvirt_offset;
    if (kvar_physvirt_offset)
        return va - *kvar_physvirt_offset;
    return *kvar_memstart_addr - wx_page_offset_base + va;
}

/* ========== 自旋锁 ========== */
/* 保护全局 region_list 和 page_info 的并发访问 */

static inline void wx_spin_lock(void)
{
    if (kfn__raw_spin_lock)
        kfn__raw_spin_lock(&global_lock);
}

static inline void wx_spin_unlock(void)
{
    if (kfn__raw_spin_unlock)
        kfn__raw_spin_unlock(&global_lock);
}

/* ========== 缓存 / TLB 操作 ========== */

/*
 * 刷新指令缓存 (I-Cache)
 * 切换页面映射后必须刷新，否则CPU可能继续执行旧的缓存指令
 * 优先使用内核函数，回退到直接执行 IC IALLUIS 指令
 */
static inline void wx_flush_icache(unsigned long addr)
{
    unsigned long page_start = addr & PAGE_MASK_4K;
    if (kfn___flush_icache_range) {
        kfn___flush_icache_range(page_start, page_start + WX_PAGE_SIZE);
    } else {
        /* 回退: 广播式指令缓存全无效化 (Inner Shareable) */
        asm volatile("ic ialluis" ::: "memory");
        dsb(ish); /* 数据同步屏障 */
    }
    isb(); /* 指令同步屏障 - 确保后续指令从刷新后的缓存取指 */
}

/* ========== 页表遍历 ========== */

/*
 * 获取用户态虚拟地址对应的 PTE (页表项) 指针
 *
 * ARM64 页表结构 (4KB页, 4级):
 *   PGD (L0) -> PUD (L1) -> PMD (L2) -> PTE (L3) -> 物理页
 *
 * 我们需要直接操控PTE来切换原始页/影子页的映射
 * 返回 PTE 指针，调用者可以直接修改 *pte 来更改映射
 */
static uint64_t *get_user_pte(void *mm, unsigned long addr, uint64_t *ptl_out)
{
    int shift = wx_page_shift;
    int level = wx_page_level;
    int bits_per_level = shift - 3;
    uint64_t mask = ~(-1ULL << bits_per_level);
    uint64_t pgd_val, pud_base, pud_val, pmd_base, pmd_val, pte_base;
    uint64_t *pgd_p, *pud_p, *pmd_p, *pte_p;
    int pgd_idx, pud_idx, pmd_idx, pte_idx;

    if (mm_struct_offset.pgd_offset < 0) {
        pr_err("wxshadow: mm_struct_offset.pgd_offset not initialized!\n");
        return NULL;
    }

    uint64_t pgd = *(uint64_t *)((uint64_t)mm + mm_struct_offset.pgd_offset);
    if (!pgd)
        return NULL;

    /* 第1级: PGD (页全局目录) 查找 */
    pgd_idx = (addr >> (shift + (level - 1) * bits_per_level)) & mask;
    pgd_p = (uint64_t *)(pgd + 8 * pgd_idx);
    pgd_val = *pgd_p;
    if (!pgd_val)
        return NULL;

    uint64_t next_pa = pgd_val & PA_MASK;

    if (level == 4) {
        /* 4级页表: PGD -> PUD -> PMD -> PTE (大多数内核配置) */
        pud_base = wx_pa_to_va(next_pa);
        pud_idx = (addr >> (shift + 2 * bits_per_level)) & mask;
        pud_p = (uint64_t *)(pud_base + 8 * pud_idx);
        if (!*pud_p)
            return NULL;
        pud_val = *pud_p;

        next_pa = pud_val & PA_MASK;
    }

    /* 第2/3级: PMD (页中间目录) 查找 */
    pmd_base = wx_pa_to_va(next_pa);
    pmd_idx = (addr >> (shift + bits_per_level)) & mask;
    pmd_p = (uint64_t *)(pmd_base + 8 * pmd_idx);
    if (!*pmd_p)
        return NULL;
    pmd_val = *pmd_p;

    /* 检查 PMD 类型 - 不支持 2MB 大页映射 (section mapping) */
    if ((pmd_val & 3) == 1) {
        pr_warn("wxshadow: address 0x%lx is in 2MB section mapping, not supported\n", addr);
        return NULL;
    }
    if ((pmd_val & 3) != 3) { /* 必须是 table descriptor (bit[1:0]=0b11) */
        pr_warn("wxshadow: invalid PMD type for address 0x%lx: 0x%llx\n", addr, pmd_val);
        return NULL;
    }

    /* 最后一级: PTE (页表项) 查找 - 实际指向物理页的条目 */
    pte_base = wx_pa_to_va(pmd_val & PA_MASK);
    pte_idx = (addr >> 12) & 0x1FF;
    pte_p = (uint64_t *)(pte_base + 8 * pte_idx);

    if (ptl_out)
        *ptl_out = 0;

    return pte_p;
}

/* ========== TLB 刷新 ========== */

/*
 * 刷新指定地址的 TLB (转换后备缓冲区)
 * 修改PTE后必须刷新TLB，否则CPU仍然使用旧的地址映射
 *
 * 三级回退策略:
 *   1. flush_tlb_page() - 内核函数，最可靠
 *   2. __flush_tlb_range() - 内核函数，范围刷新
 *   3. TLBI 指令 - 直接执行汇编，需要 ASID
 */
static void wxshadow_flush_tlb_page(void *vma, unsigned long addr)
{
    if (kfn_flush_tlb_page) {
        /* 策略1: 直接调用内核的 flush_tlb_page */
        ((void (*)(void *, unsigned long))kfn_flush_tlb_page)(vma, addr);
        return;
    }
    if (kfn___flush_tlb_range) {
        /* 策略2: 使用 __flush_tlb_range 刷新单页范围 */
        kfn___flush_tlb_range(vma, addr, addr + WX_PAGE_SIZE, WX_PAGE_SIZE, 1, 3);
        return;
    }

    /*
     * 策略3: 回退到直接执行 TLBI 指令
     * 尝试获取 ASID 以进行更精确的TLB无效化
     * ASID (Address Space ID) 用于区分不同进程的TLB条目
     */
    uint64_t tlbi_val = addr >> 12;
    if (vma && wx_vma_vm_mm_offset >= 0) {
        void *mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);
        if (mm && wx_mm_context_id_offset >= 0) {
            uint64_t ctx_val = *(uint64_t *)((uint64_t)mm + wx_mm_context_id_offset);
            uint16_t asid = (ctx_val >> wx_mm_context_id_asid_shift) & 0xFFFF;
            if (asid) {
                /* 有ASID: 用 VALE1IS 精确无效化指定进程的指定页 */
                uint64_t val = tlbi_val | ((uint64_t)asid << 48);
                asm volatile("tlbi vale1is, %0" :: "r"(val));
                dsb(ish);
                isb();
                return;
            }
        }
    }
    /* 无ASID: 用 VAALE1IS 无效化所有ASID的指定地址 (影响稍大但最安全) */
    asm volatile("tlbi vaale1is, %0" :: "r"(tlbi_val));
    dsb(ish);
    isb();
}

/* ========== PTE 辅助函数 ========== */

/* 直接写入PTE值 (无锁定，调用者负责同步) */
static inline void wxshadow_set_pte_at(void *mm, unsigned long addr,
                                         uint64_t *ptep, uint64_t val)
{
    *ptep = val;
}

/* 构造PTE值: 将页帧号(PFN)和标志位组合成完整的PTE */
static inline uint64_t make_pte(uint64_t pfn, uint64_t flags)
{
    return (pfn << 12) | flags | PTE_BASE_FLAGS;
}

/* ========== 区域管理 ========== */

/* 计算地址在区域内的页面索引 */
static int wxshadow_page_index(struct wx_region *region, unsigned long addr)
{
    return (unsigned int)((addr - region->vm_start) >> 12);
}

/*
 * 查找包含指定地址的 wx_region
 * 找到后自动增加引用计数，调用者用完后必须调用 wxshadow_put_region()
 */
static struct wx_region *wxshadow_find_region(void *mm, unsigned long addr)
{
    struct wx_region *r;

    wx_spin_lock();
    list_for_each_entry(r, &region_list, list) {
        if (r->mm == mm && addr >= r->vm_start && addr < r->vm_end) {
            r->refcount++;
            wx_spin_unlock();
            return r;
        }
    }
    wx_spin_unlock();
    return NULL;
}

/*
 * 释放区域引用
 * 引用计数到零时自动从全局链表移除并释放所有资源
 */
static void wxshadow_put_region(struct wx_region *r)
{
    int nr, i;
    struct page_info *pi;

    wx_spin_lock();
    r->refcount--;
    if (r->refcount > 0) {
        wx_spin_unlock();
        return;
    }
    /* 引用计数到零，从全局链表移除 */
    list_del_init(&r->list);
    wx_spin_unlock();

    /* 释放所有影子页 */
    nr = r->nr_pages;
    pi = r->pages;
    if (nr > 0 && pi) {
        for (i = 0; i < nr; i++) {
            if (pi[i].shadow_page_va) {
                kfn_free_pages(pi[i].shadow_page_va, 0);
            }
        }
    }
    kfn_kfree(pi);
    kfn_kfree(r);
}

/* ========== 断点查找 ========== */

/* 在页面的断点数组中查找指定地址的活跃断点 */
static struct bp_entry *wxshadow_find_bp(struct page_info *pi, unsigned long addr)
{
    int i;
    if (pi->bp_count <= 0)
        return NULL;
    for (i = 0; i < (int)pi->bp_count; i++) {
        if ((pi->bps[i].active & 1) && pi->bps[i].addr == addr)
            return &pi->bps[i];
    }
    return NULL;
}

/* ========== 页面映射验证 ========== */

/*
 * 验证页面映射是否仍然有效
 * 检查当前PTE中的PFN是否匹配我们记录的原始页或影子页
 * 如果PFN不匹配，说明页面发生了COW(写时复制)或remap，需要清理
 */
static int wxshadow_validate_page_mapping(void *mm, void *vma,
                                            struct page_info *pi,
                                            unsigned long addr)
{
    uint64_t *pte;
    uint64_t pte_val, current_pfn;

    if (!vma || addr < *(unsigned long *)vma ||
        *(unsigned long *)((uint64_t)vma + 8) <= addr) {
        pr_info("wxshadow: validate_mapping: VMA invalid for addr %lx\n", addr);
        return 0;
    }

    pte = get_user_pte(mm, addr, NULL);
    if (!pte) {
        pr_info("wxshadow: validate_mapping: PTE not found for addr %lx\n", addr);
        return 0;
    }

    pte_val = *pte;
    if (!(pte_val & 1)) {
        pr_info("wxshadow: validate_mapping: PTE invalid for addr %lx\n", addr);
        return 0;
    }

    current_pfn = (pte_val >> 12) & PFN_MASK;

    /* 检查当前PFN是否匹配影子页 */
    if (pi->shadow_pfn && pi->shadow_pfn == current_pfn)
        return 1;

    /* 检查当前PFN是否匹配原始页且状态有效 */
    if (pi->orig_pfn && pi->orig_pfn == current_pfn) {
        if (pi->state >= STATE_ORIG_R && pi->state <= STATE_STEPPING)
            return 1;
    }

    pr_info("wxshadow: validate_mapping: PFN mismatch for addr %lx: "
            "current=%lx, orig=%lx, shadow=%lx, state=%d\n",
            addr, (unsigned long)current_pfn, (unsigned long)pi->orig_pfn,
            (unsigned long)pi->shadow_pfn, pi->state);
    return 0;
}

/* ========== 自动清理 ========== */

/*
 * 自动清理一个页面的影子映射
 * 在VMA消失、PFN不匹配、写错误等异常情况下调用
 * 清理所有断点、释放影子页、重置状态
 */
static int wxshadow_auto_cleanup_page(void *mm, struct wx_region *region,
                                        unsigned int page_idx, const char *reason)
{
    struct page_info *pi;
    int had_mm, i, had_bp;

    if (!region || (int)page_idx < 0)
        return -1;
    if ((int)page_idx >= region->nr_pages)
        return -1;

    had_mm = (mm != NULL);
    pi = &region->pages[page_idx];
    unsigned long addr = region->vm_start + ((unsigned long)page_idx << 12);

    pr_info("wxshadow: ===============================================\n");
    pr_info("wxshadow: === AUTO CLEANUP: %s ===\n", reason);
    pr_info("wxshadow: ===============================================\n");
    pr_info("wxshadow:   Page addr:  0x%lx\n", addr);
    pr_info("wxshadow:   State:      %d\n", pi->state);
    pr_info("wxshadow:   PFN orig:   0x%lx\n", (unsigned long)pi->orig_pfn);
    pr_info("wxshadow:   PFN shadow: 0x%lx\n", (unsigned long)pi->shadow_pfn);

    wx_spin_lock();

    had_bp = 0;
    if ((int)pi->bp_count > 0) {
        for (i = 0; i < (int)pi->bp_count; i++) {
            if (pi->bps[i].active & 1) {
                pr_info("wxshadow:   Removing BP at 0x%lx\n", (unsigned long)pi->bps[i].addr);
                pi->bps[i].active = 0;
                memset(&pi->bps[i].mods, 0, sizeof(pi->bps[i].mods));
                pi->bps[i].mod_count = 0;
                had_bp = 1;
            }
        }
    }

    /* 如果正在单步执行，清除单步任务记录 */
    if (pi->stepping_task) {
        pr_info("wxshadow:   Clearing stepping task %px\n", (void *)pi->stepping_task);
        pi->stepping_task = 0;
    }

    had_bp = had_mm && had_bp;

    /* 释放影子页并重置所有状态 */
    if (pi->shadow_page_va) {
        uint64_t shadow_va = pi->shadow_page_va;
        pi->shadow_page_va = 0;
        pi->state = STATE_NONE;
        pi->orig_pfn = 0;
        pi->shadow_pfn = 0;
        pi->bp_count = 0;
        wx_spin_unlock();

        if (had_bp && mm)
            kfn_find_vma(mm, addr);  /* Touch VMA to keep it alive */

        kfn_free_pages(shadow_va, 0);
        pr_info("wxshadow:   Freed shadow page\n");
    } else {
        pi->state = STATE_NONE;
        pi->orig_pfn = 0;
        pi->shadow_pfn = 0;
        pi->bp_count = 0;
        wx_spin_unlock();

        if (had_bp && mm)
            kfn_find_vma(mm, addr);
    }

    pr_info("wxshadow: ===============================================\n");
    return 0;
}

/* ========== 切换映射 ========== */

/*
 * 核心操作: 切换页面映射
 * 将用户态地址 addr 的PTE指向新的物理页帧 pfn
 * extra_flags 控制页面权限:
 *   0 = 可执行 (用于影子页)
 *   PTE_USER_RDONLY = 只读+可执行 (用于单步)
 *   PTE_UXN_USER_RO = 只读+不可执行 (用于读保护，对抗CRC检测)
 */
static int wxshadow_switch_mapping(void *vma, unsigned long addr,
                                     uint64_t pfn, uint64_t extra_flags)
{
    void *mm = NULL;
    uint64_t *pte;

    if (wx_vma_vm_mm_offset >= 0 && vma)
        mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);

    pte = get_user_pte(mm, addr, NULL);
    if (!pte) {
        pr_err("wxshadow: [switch] FAILED get_user_pte addr=%lx\n", addr);
        return -1;
    }

    *pte = make_pte(pfn, extra_flags);
    wxshadow_flush_tlb_page(vma, addr);
    return 0;
}

/* ========== 页错误处理 ========== */
/*
 * 三种错误处理器配合实现 W^X 影子内存的核心逻辑:
 *   写错误 -> 清理 (页面内容变更，影子页失效)
 *   读错误 -> 切换到原始页 (让CRC校验看到原始代码)
 *   执行错误 -> 切换到影子页 (即将执行到BRK断点)
 */

/*
 * 写错误处理: 页面被写入时触发
 * 页面内容被修改，影子页与原始页不再一致，必须清理
 */
static int wxshadow_handle_write_fault(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    int idx;

    region = wxshadow_find_region(mm, addr);
    if (!region)
        return -1;

    idx = wxshadow_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages)
        goto out;

    pi = &region->pages[idx];
    if (pi->state == STATE_NONE)
        goto out;

    pr_info("wxshadow: write fault at %lx - page content changing\n", addr);
    wxshadow_auto_cleanup_page(mm, region, idx, "Write Fault (page modified)");

out:
    wxshadow_put_region(region);
    return -1;
}

/*
 * 读错误处理: 程序读取受保护页面时触发
 *
 * 【对抗CRC32检测的核心】
 * 当影子页(含BRK)已映射时(STATE_SHADOW_X)，影子页是可执行但不可读的。
 * 用户态程序尝试读取这个页面时(比如CRC32校验)，
 * 会触发读错误，我们在这里切换到原始页(只读+不可执行)。
 * 这样CRC32校验读到的是未修改的原始代码，检测不到断点的存在。
 */
static int wxshadow_handle_read_fault(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    int idx, ret;
    void *vma;

    region = wxshadow_find_region(mm, addr);
    if (!region)
        return -1;

    idx = wxshadow_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages) {
        wxshadow_put_region(region);
        return -1;
    }

    wx_spin_lock();
    pi = &region->pages[idx];
    if (pi->state != STATE_SHADOW_X || !pi->orig_pfn) {
        wx_spin_unlock();
        wxshadow_put_region(region);
        return -1;
    }
    wx_spin_unlock();

    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        pr_info("wxshadow: read_fault: VMA gone for addr=%lx, auto cleanup\n", addr);
        wxshadow_auto_cleanup_page(mm, region, idx, "VMA Gone (read fault)");
        wxshadow_put_region(region);
        return -1;
    }

    if (!wxshadow_validate_page_mapping(mm, vma, pi, page_addr)) {
        pr_info("wxshadow: read_fault: mapping invalid for addr=%lx, auto cleanup\n", addr);
        wxshadow_auto_cleanup_page(mm, region, idx, "Mapping Changed (read fault)");
        wxshadow_put_region(region);
        return -1;
    }

    /* 切换到原始页 (UXN+只读: 可读但不可执行，下次执行时会触发exec_fault) */
    ret = wxshadow_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_UXN_USER_RO);
    if (ret) {
        wxshadow_put_region(region);
        return -1;
    }

    wx_spin_lock();
    pi->state = STATE_ORIG_R;
    wx_spin_unlock();

    pr_info("wxshadow: read fault at %lx, switched to original (r--)\n", addr);
    wxshadow_put_region(region);
    return 0;
}

/*
 * 执行错误处理: 程序尝试执行受保护页面时触发
 * 当原始页已映射时(STATE_ORIG_R)，原始页是只读+不可执行的。
 * 程序尝试执行这个地址时，会触发执行错误，
 * 我们在这里切换回影子页(可执行)，程序将执行到BRK断点。
 */
static int wxshadow_handle_exec_fault(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    int idx, ret, state;
    void *vma;

    region = wxshadow_find_region(mm, addr);
    if (!region)
        return -1;

    idx = wxshadow_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages) {
        wxshadow_put_region(region);
        return -1;
    }

    wx_spin_lock();
    pi = &region->pages[idx];
    if (!pi->shadow_pfn) {
        wx_spin_unlock();
        wxshadow_put_region(region);
        return -1;
    }
    state = pi->state;
    if (state == STATE_SHADOW_X || state == STATE_STEPPING || state != STATE_ORIG_R) {
        wx_spin_unlock();
        wxshadow_put_region(region);
        return -1;
    }
    wx_spin_unlock();

    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        pr_info("wxshadow: exec_fault: VMA gone for addr=%lx, auto cleanup\n", addr);
        wxshadow_auto_cleanup_page(mm, region, idx, "VMA Gone (exec fault)");
        wxshadow_put_region(region);
        return -1;
    }

    if (!wxshadow_validate_page_mapping(mm, vma, pi, page_addr)) {
        pr_info("wxshadow: exec_fault: mapping invalid for addr=%lx, auto cleanup\n", addr);
        wxshadow_auto_cleanup_page(mm, region, idx, "Mapping Changed (exec fault)");
        wxshadow_put_region(region);
        return -1;
    }

    if (kfn_flush_dcache_page && pi->shadow_page_va)
        kfn_flush_dcache_page((void *)pi->shadow_page_va);

    /* 切换到影子页 (可执行，含BRK断点指令) */
    ret = wxshadow_switch_mapping(vma, page_addr, pi->shadow_pfn, 0);
    if (ret) {
        wxshadow_put_region(region);
        return -1;
    }

    wx_flush_icache(page_addr);

    wx_spin_lock();
    pi->state = STATE_SHADOW_X;
    wx_spin_unlock();

    pr_info("wxshadow: exec fault at %lx, switched to shadow (--x)\n", addr);
    wxshadow_put_region(region);
    return 0;
}

/* ========== 内核Hook回调 ========== */

/*
 * do_page_fault 的 before hook
 * 拦截页错误异常，判断是否是我们的W^X页面引起的
 * 如果是，根据错误类型(ESR)分发到对应的处理器
 */
static void do_page_fault_before(hook_fargs3_t *fargs, void *udata)
{
    unsigned long far = fargs->arg1;    /* FAR_EL1: 错误地址寄存器 */
    unsigned long esr = fargs->arg2;    /* ESR_EL1: 异常综合寄存器 */
    void *task = (void *)get_current();
    void *mm;

    mm = kfn_get_task_mm(task);
    if (!mm)
        return;

    /* 检查故障状态码: 只处理地址转换错误 (translation fault) */
    if ((esr & 0x3C) != 0x0C) {
        kfn_mmput(mm);
        return;
    }

    /* 快速检查: 这个地址是否在我们管理的区域内 */
    struct wx_region *region = wxshadow_find_region(mm, far);
    if (!region) {
        kfn_mmput(mm);
        return;
    }
    wxshadow_put_region(region);

    /* 根据 ESR 判断错误类型 */
    if ((esr >> 26) == 0x20) {
        /* 指令中止 (Instruction Abort from EL0) - 程序尝试执行不可执行的页 */
        if (!wxshadow_handle_exec_fault(mm, far)) {
            fargs->skip_origin = 1;
            fargs->ret = 0;
        }
    } else if (!(esr & 0x40)) {
        /* 数据中止, WnR=0 → 读错误 (程序读取受保护页, 比如CRC校验) */
        if (!wxshadow_handle_read_fault(mm, far)) {
            fargs->skip_origin = 1;
            fargs->ret = 0;
        }
    } else {
        /* 数据中止, WnR=1 → 写错误 (页面被修改, 影子页失效) */
        wxshadow_handle_write_fault(mm, far);
    }

    kfn_mmput(mm);
}

/* ========== 进程退出清理 ========== */

/*
 * exit_mmap 的 before hook
 * 进程退出时内核会调用 exit_mmap() 释放所有VMA
 * 我们必须在此之前恢复所有被修改的PTE，否则会触发"Bad page map"错误
 * 因为内核发现PTE指向的物理页不是它预期的
 */
static void exit_mmap_before(hook_fargs1_t *fargs, void *udata)
{
    void *mm = (void *)fargs->arg0;
    struct wx_region *regions[32];
    struct wx_region *r, *tmp;
    int count = 0;
    int i, j;

    if (!mm)
        return;

    /* 收集属于此 mm 的所有区域 */
    wx_spin_lock();
    list_for_each_entry_safe(r, tmp, &region_list, list) {
        if (r->mm == mm && count < 32) {
            list_del_init(&r->list);
            regions[count++] = r;
        }
    }
    wx_spin_unlock();

    if (!count)
        return;

    pr_info("wxshadow: [exit_mmap] mm=%px, restoring %d regions\n", mm, count);

    for (i = 0; i < count; i++) {
        r = regions[i];
        int nr = r->nr_pages;
        struct page_info *pages = r->pages;

        if (nr > 0 && pages) {
            for (j = 0; j < nr; j++) {
                struct page_info *pi = &pages[j];
                if (!pi->shadow_pfn || !pi->orig_pfn)
                    continue;

                unsigned long addr = r->vm_start + ((unsigned long)j << 12);
                uint64_t shadow_va = pi->shadow_page_va;

                void *vma = kfn_find_vma(mm, addr);
                if (vma && addr >= *(unsigned long *)vma) {
                    uint64_t *pte = get_user_pte(mm, addr, NULL);
                    if (pte && (*pte & 1)) {
                        /* 恢复PTE指向原始页 */
                        uint64_t new_pte = (pi->orig_pfn << 12) | 0xFC3ULL;
                        pr_info("wxshadow: [exit_mmap] restoring PTE at %lx: %llx -> %llx\n",
                                addr, *pte, new_pte);
                        *pte = new_pte;
                        wxshadow_flush_tlb_page(vma, addr);
                    }
                    if (shadow_va) {
                        pr_info("wxshadow: [exit_mmap] freeing shadow page at %lx\n", addr);
                        kfn_free_pages(shadow_va, 0);
                    }
                } else {
                    pr_info("wxshadow: [exit_mmap] VMA gone for %lx, freeing shadow\n", addr);
                    if (shadow_va)
                        kfn_free_pages(shadow_va, 0);
                }
            }
        }

        if (pages)
            kfn_kfree(pages);
        kfn_kfree(r);
    }

    pr_info("wxshadow: [exit_mmap] cleanup complete for mm=%px\n", mm);
}

/* ========== BRK 断点处理器 ========== */

/*
 * BRK #7 断点命中时的核心处理逻辑
 *
 * 流程:
 * 1. 查找当前PC对应的区域和页面
 * 2. 打印寄存器快照 (x0-x7, fp, lr, sp, pstate)
 * 3. 应用用户配置的寄存器修改 (调参/返回值篡改)
 * 4. 切换到原始页 (可执行) + 启用单步
 * 5. 执行原始指令后触发单步异常 -> step_handler 切回影子页
 */
static int wxshadow_brk_handler(struct pt_regs *regs)
{
    unsigned long pc = regs->pc;
    void *task = (void *)get_current();
    void *mm;
    struct wx_region *r;
    struct page_info *pi;
    int idx;
    unsigned long page_addr;

    mm = kfn_get_task_mm(task);
    pr_info("wxshadow: BRK handler ENTER pc=%lx esr=%x mm=%px\n", pc, 0, mm);

    if (!mm)
        return 1;

    /* 查找当前PC对应的区域和页面 */
    wx_spin_lock();
    list_for_each_entry(r, &region_list, list) {
        if (pc < r->vm_start || pc >= r->vm_end)
            continue;

        idx = (int)((pc - r->vm_start) >> 12);
        if (idx >= r->nr_pages)
            continue;

        pi = &r->pages[idx];
        if (!pi->shadow_pfn)
            continue;

        int state = pi->state;
        if (state == STATE_SHADOW_X) {
            r->refcount++;
            wx_spin_unlock();
            goto found;
        }
        if (state == STATE_STEPPING) {
            /* 另一个线程正在单步，等待其完成 (STEPPING -> SHADOW_X) */
            unsigned int spins = 0;
            do {
                if (++spins == 10001) {
                    pr_info("wxshadow: find_by_addr: timeout waiting for STEPPING, state=%d\n", pi->state);
                    wx_spin_unlock();
                    goto not_ours;
                }
                wx_spin_unlock();
                asm volatile("yield" ::: "memory");
                wx_spin_lock();
                if (list_empty(&region_list))
                    goto unlock_not_ours;
                state = pi->state;
            } while (state == STATE_STEPPING);

            if (state == STATE_SHADOW_X) {
                pr_info("wxshadow: find_by_addr: waited %d iterations for STEPPING->SHADOW_X\n", spins);
                r->refcount++;
                wx_spin_unlock();
                goto found;
            }
            pr_info("wxshadow: find_by_addr: found region but state=%d (need SHADOW_X=%d)\n",
                    state, STATE_SHADOW_X);
            goto unlock_not_ours;
        }
    }

unlock_not_ours:
    wx_spin_unlock();
not_ours:
    pr_info("wxshadow: BRK: not our breakpoint at pc=%lx\n", pc);
    kfn_mmput(mm);
    return 1;

found:
    page_addr = pc & PAGE_MASK_4K;

    /* 验证VMA是否仍然存在 */
    void *vma = kfn_find_vma(mm, pc);
    if (!vma || pc < *(unsigned long *)vma) {
        pr_info("wxshadow: BRK handler: VMA not found for pc=%lx, auto cleanup\n", pc);
        wxshadow_auto_cleanup_page(mm, r, idx, "VMA Gone (process exit?)");
        wxshadow_put_region(r);
        kfn_mmput(mm);
        return 1;
    }

    if (!wxshadow_validate_page_mapping(mm, vma, pi, page_addr)) {
        pr_info("wxshadow: BRK handler: mapping invalid for pc=%lx, auto cleanup\n", pc);
        wxshadow_auto_cleanup_page(mm, r, idx, "Mapping Changed (COW/remap?)");
        wxshadow_put_region(r);
        kfn_mmput(mm);
        return 1;
    }

    /* 打印寄存器快照 - 这是hook工具读取函数参数/状态的关键信息 */
    pr_info("wxshadow: ======== Breakpoint Hit ========\n");
    pr_info("wxshadow: PC=%lx\n", pc);
    pr_info("wxshadow: x0=%016llx x1=%016llx x2=%016llx x3=%016llx\n",
            regs->regs[0], regs->regs[1], regs->regs[2], regs->regs[3]);
    pr_info("wxshadow: x4=%016llx x5=%016llx x6=%016llx x7=%016llx\n",
            regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
    pr_info("wxshadow: x29(fp)=%016llx x30(lr)=%016llx\n",
            regs->regs[29], regs->regs[30]);
    pr_info("wxshadow: sp=%016llx pstate=%016llx\n",
            regs->sp, regs->pstate);
    pr_info("wxshadow: ================================\n");

    /* 应用寄存器修改 - 实现无感知的函数参数/返回值篡改 */
    struct bp_entry *bp = wxshadow_find_bp(pi, pc);
    if (bp && bp->mod_count > 0) {
        int m;
        for (m = 0; m < (int)bp->mod_count; m++) {
            if (!(bp->mods[m].active & 1))
                continue;
            int reg = bp->mods[m].reg_num;
            if (reg <= 30) {
                pr_info("wxshadow: modifying x%d: %016llx -> %016llx\n",
                        reg, regs->regs[reg], bp->mods[m].value);
                regs->regs[reg] = bp->mods[m].value;
            } else if (reg == 31) {
                pr_info("wxshadow: modifying sp: %016llx -> %016llx\n",
                        regs->sp, bp->mods[m].value);
                regs->sp = bp->mods[m].value;
            }
        }
    }

    /* 切换到原始页 (只读+可执行) 以便单步执行原始指令 */
    pr_info("wxshadow: BRK switching to original: orig_pfn=%lx shadow_pfn=%lx\n",
            (unsigned long)pi->orig_pfn, (unsigned long)pi->shadow_pfn);

    if (wxshadow_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_USER_RDONLY)) {
        wxshadow_put_region(r);
        kfn_mmput(mm);
        regs->pc += 4;  /* 切换失败时跳过BRK指令继续执行 */
        return 0;
    }

    wx_flush_icache(page_addr);

    /* 设置状态为 STEPPING 并记录当前任务 (用于 step_handler 匹配) */
    wx_spin_lock();
    pi->state = STATE_STEPPING;
    pi->stepping_task = (uint64_t)get_current();
    wx_spin_unlock();

    wxshadow_put_region(r);
    kfn_mmput(mm);

    /* 启用单步执行 - CPU执行一条原始指令后会触发单步异常 */
    kfn_user_enable_single_step((void *)get_current());
    pr_info("wxshadow: BRK handler EXIT success, single-step enabled\n");
    return 0;
}

/*
 * brk_handler 的 before hook - 内核BRK异常入口
 * 过滤条件: 只处理 BRK #7 且来自 EL0 (用户态) 的异常
 */
static void brk_handler_before(hook_fargs3_t *fargs, void *udata)
{
    unsigned long esr = fargs->arg1;
    struct pt_regs *regs = (struct pt_regs *)fargs->arg2;

    /* 检查: 是否是 BRK #7 且来自 EL0 (用户态) */
    if ((esr & 0xFFFF) != BRK_IMM_NUM)
        return;
    if ((regs->pstate & 0xF) != 0) /* EL0 = pstate[3:0] == 0 */
        return;

    if (!wxshadow_brk_handler(regs)) {
        fargs->skip_origin = 1;
        fargs->ret = 0;
    }
}

/* ========== 单步异常处理器 ========== */

/*
 * 单步执行完成后的处理
 * BRK命中 -> 切换到原始页 -> 执行1条原始指令 -> 触发单步异常 -> 到这里
 * 在这里切换回影子页，等待下次BRK命中
 */
static int wxshadow_step_handler(struct pt_regs *regs)
{
    void *task = (void *)get_current();
    void *mm;
    struct wx_region *r;
    struct page_info *pi = NULL;
    unsigned int page_idx = 0;
    int found = 0;

    mm = kfn_get_task_mm(task);
    if (!mm)
        return 1;

    /* 查找当前任务正在单步的页面 (STATE_STEPPING + stepping_task匹配) */
    wx_spin_lock();
    list_for_each_entry(r, &region_list, list) {
        if (r->mm != mm)
            continue;
        int nr = r->nr_pages;
        int i;
        for (i = 0; i < nr; i++) {
            if (r->pages[i].state == STATE_STEPPING &&
                r->pages[i].stepping_task == (uint64_t)get_current()) {
                pi = &r->pages[i];
                page_idx = i;
                r->refcount++;
                found = 1;
                break;
            }
        }
        if (found)
            break;
    }

    if (!found) {
        wx_spin_unlock();
        pr_info("wxshadow: step handler: NOT FOUND! pc=%llx mm=%px current=%px\n",
                regs->pc, mm, task);
        kfn_mmput(mm);
        return 1;
    }

    unsigned long addr = r->vm_start + ((unsigned long)page_idx << 12);
    uint64_t shadow_pfn = pi->shadow_pfn;
    wx_spin_unlock();

    /* 验证VMA和映射是否仍然有效 */
    void *vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        pr_info("wxshadow: step handler: VMA gone for addr=%lx, auto cleanup\n", addr);
        wxshadow_auto_cleanup_page(mm, r, page_idx, "VMA Gone during step");
        goto done;
    }

    if (!wxshadow_validate_page_mapping(mm, vma, pi, addr)) {
        pr_info("wxshadow: step handler: mapping changed for addr=%lx, auto cleanup\n", addr);
        wxshadow_auto_cleanup_page(mm, r, page_idx, "Mapping changed during step");
        goto done;
    }

    /* 切换回影子页 (可执行, 含BRK) - 等待下次命中 */
    wxshadow_switch_mapping(vma, addr, shadow_pfn, 0);
    wx_flush_icache(addr & PAGE_MASK_4K);

    pr_info("wxshadow: step done at pc=%llx, switched back to shadow\n", regs->pc);

    /* 更新状态: STEPPING -> SHADOW_X */
    wx_spin_lock();
    if (pi->state == STATE_STEPPING && pi->stepping_task == (uint64_t)get_current()) {
        pi->state = STATE_SHADOW_X;
        pi->stepping_task = 0;
        pr_info("wxshadow: step: state updated to SHADOW_X\n");
    } else {
        pr_warn("wxshadow: step: state update SKIPPED! page_info=%px state=%d task=%px current=%px\n",
                pi, pi->state, (void *)pi->stepping_task, get_current());
    }
    wx_spin_unlock();

done:
    wxshadow_put_region(r);
    kfn_mmput(mm);

    /* Disable single step */
    kfn_user_disable_single_step((void *)get_current());
    return 0;
}

static void single_step_handler_before(hook_fargs3_t *fargs, void *udata)
{
    struct pt_regs *regs = (struct pt_regs *)fargs->arg2;

    /* Check EL0 */
    if ((regs->pstate & 0xF) != 0)
        return;

    if (!wxshadow_step_handler(regs)) {
        fargs->skip_origin = 1;
        fargs->ret = 0;
    }
}

/* ========== 断点设置/删除 ========== */

/*
 * 设置隐藏断点
 * 1. 如果区域已存在且已有影子页 -> 直接在影子页写入BRK
 * 2. 否则创建新区域 + 分配影子页 + 复制原始内容 + 写入BRK + 切换PTE
 */
static int wxshadow_do_set_bp(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    unsigned long offset = addr & 0xFFF;
    struct wx_region *region;
    struct page_info *pi;
    int idx;
    void *vma;

    pr_info("wxshadow: [set_bp] addr=%lx\n", addr);

    /* 检查TLB刷新方法是否可用 - 没有TLB刷新就无法切换页面 */
    if (!kfn_flush_tlb_page && !kfn___flush_tlb_range) {
        if (wx_mm_context_id_offset < 0) {
            pr_err("wxshadow: [set_bp] no TLB flush method available!\n");
            pr_err("wxshadow: [set_bp] need flush_tlb_page, __flush_tlb_range, or mm_context_id_offset\n");
            return -38;
        }
        pr_info("wxshadow: [set_bp] using TLBI instruction with ASID (context_id_offset=0x%x)\n",
                wx_mm_context_id_offset);
    }

    /* 检查区域是否已存在 (同一VMA的其他断点可能已创建了区域) */
    region = wxshadow_find_region(mm, addr);
    if (region) {
        idx = wxshadow_page_index(region, page_addr);
        pi = &region->pages[idx];

        if (pi->shadow_page_va) {
            /* 页面已有影子 - 直接在影子页添加新断点 */
            int bp_count = pi->bp_count;
            int i;

            /* Check if BP already exists */
            for (i = 0; i < bp_count; i++) {
                if (pi->bps[i].addr == addr)
                    break;
            }
            if (i == bp_count && bp_count <= 15) {
                /* Add new BP entry */
                pi->bp_count = bp_count + 1;
                memset(&pi->bps[bp_count], 0, sizeof(struct bp_entry));
                pi->bps[bp_count].addr = addr;
                pi->bps[bp_count].active = 1;
            }

            /* 在影子页的目标偏移处写入 BRK #7 指令 */
            *(uint32_t *)(pi->shadow_page_va + offset) = BRK_INSN;
            wx_flush_icache(page_addr);

            pr_info("wxshadow: bp at %lx (existing)\n", addr);
            wxshadow_put_region(region);
            return 0;
        }
        wxshadow_put_region(region);
    }

    /* 需要创建新区域和影子页 */
    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        pr_err("wxshadow: [set_bp] no vma for %lx\n", addr);
        return -1;
    }

    /* 并发检查: 在我们未持锁期间是否有其他线程创建了区域 */
    wx_spin_lock();
    struct wx_region *existing;
    list_for_each_entry(existing, &region_list, list) {
        if (existing->mm == mm && addr >= existing->vm_start && addr < existing->vm_end) {
            wx_spin_unlock();
            /* Retry */
            return wxshadow_do_set_bp(mm, addr);
        }
    }
    wx_spin_unlock();

    /* 创建新的 wx_region 和 page_info 数组 */
    unsigned long vm_start = *(unsigned long *)vma;
    unsigned long vm_end = *(unsigned long *)((uint64_t)vma + 8);
    int nr_pages = (int)((vm_end - vm_start) >> 12);

    struct wx_region *new_region = (struct wx_region *)kfn_kzalloc(sizeof(struct wx_region), wx_gfp_kernel);
    if (!new_region)
        return -12;

    struct page_info *pages;
    if (kfn_kcalloc)
        pages = (struct page_info *)kfn_kcalloc(nr_pages, sizeof(struct page_info), wx_gfp_kernel);
    else
        pages = (struct page_info *)kfn_kzalloc((unsigned long)nr_pages * sizeof(struct page_info), wx_gfp_kernel);

    if (!pages) {
        kfn_kfree(new_region);
        return -12;
    }

    INIT_LIST_HEAD(&new_region->list);
    new_region->mm = mm;
    new_region->vm_start = vm_start;
    new_region->vm_end = vm_end;
    new_region->pages = pages;
    new_region->nr_pages = nr_pages;
    new_region->refcount = 1;

    /* 插入全局链表 */
    wx_spin_lock();
    list_add(&new_region->list, &region_list);
    wx_spin_unlock();

    /* 设置影子页 */
    idx = wxshadow_page_index(new_region, page_addr);
    pi = &pages[idx];

    /* 获取当前PTE，提取原始页帧号 */
    uint64_t *pte = get_user_pte(mm, page_addr, NULL);
    if (!pte || !((*pte) & 1)) {
        pr_err("wxshadow: [set_bp] no pte for %lx\n", page_addr);
        return -14;
    }

    uint64_t pte_val = *pte;
    uint64_t orig_pfn = (pte_val >> 12) & PFN_MASK;
    pi->orig_pfn = orig_pfn;

    /* 将原始页帧号转换为内核虚拟地址 (用于复制原始内容) */
    uint64_t orig_va = wx_pa_to_va(orig_pfn << 12);

    /* 分配影子页 (1个物理页) */
    unsigned long shadow_va = kfn___get_free_pages(wx_gfp_kernel, 0);
    if (!shadow_va)
        return -12;

    uint64_t shadow_pa = wx_va_to_pa(shadow_va);
    uint64_t shadow_pfn = shadow_pa >> 12;

    pi->shadow_pfn = shadow_pfn;
    pi->shadow_page_va = shadow_va;

    /* 复制原始页内容到影子页 */
    memcpy((void *)shadow_va, (void *)orig_va, WX_PAGE_SIZE);

    /* 在影子页的目标偏移处写入 BRK #7 */
    *(uint32_t *)(shadow_va + offset) = BRK_INSN;

    /* 初始化页面信息和第一个断点 */
    pi->state = STATE_SHADOW_X;
    pi->bp_count = 1;
    memset(&pi->bps[0], 0, sizeof(struct bp_entry));
    pi->bps[0].addr = addr;
    pi->bps[0].active = 1;

    /* 切换PTE指向影子页 (可执行，含BRK) */
    int ret = wxshadow_switch_mapping(vma, page_addr, shadow_pfn, 0);
    if (ret) {
        pr_err("wxshadow: [set_bp] switch failed\n");
        kfn_free_pages(shadow_va, 0);
        pi->shadow_pfn = 0;
        pi->shadow_page_va = 0;
        return ret;
    }

    wx_flush_icache(page_addr);

    pr_info("wxshadow: bp at %lx orig_pfn=%lx shadow_pfn=%lx\n",
            addr, (unsigned long)orig_pfn, (unsigned long)shadow_pfn);
    return 0;
}

/*
 * 设置断点命中时的寄存器修改
 * 断点命中时自动将指定寄存器修改为目标值
 * 可用于: 修改函数参数(x0-x7), 修改返回地址(x30), 修改返回值(x0)等
 */
static int wxshadow_do_set_reg(void *mm, unsigned long addr,
                                unsigned int reg, uint64_t value)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    struct bp_entry *bp;
    int idx, i;

    if (reg > 31)
        return -22;

    region = wxshadow_find_region(mm, addr);
    if (!region)
        return -2;

    idx = wxshadow_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages) {
        wxshadow_put_region(region);
        return -2;
    }

    pi = &region->pages[idx];
    bp = wxshadow_find_bp(pi, addr);
    if (!bp) {
        wxshadow_put_region(region);
        return -2;
    }

    /* 检查是否已存在该寄存器的修改规则 (存在则更新) */
    for (i = 0; i < (int)bp->mod_count; i++) {
        if (bp->mods[i].reg_num == reg) {
            bp->mods[i].active = 1;
            bp->mods[i].value = value;
            pr_info("wxshadow: updated reg mod at %lx: x%d=%lx\n", addr, reg, (unsigned long)value);
            wxshadow_put_region(region);
            return 0;
        }
    }

    if (bp->mod_count >= MAX_REG_MOD) {
        wxshadow_put_region(region);
        return -28;
    }

    /* 添加新的寄存器修改规则 */
    i = bp->mod_count;
    bp->mod_count = i + 1;
    bp->mods[i].reg_num = reg;
    bp->mods[i].active = 1;
    bp->mods[i].value = value;

    pr_info("wxshadow: added reg mod at %lx: x%d=%lx\n", addr, reg, (unsigned long)value);
    wxshadow_put_region(region);
    return 0;
}

/*
 * 删除断点
 * 如果还有其他活跃断点 -> 只恢复该位置的原始指令
 * 如果无其他断点 -> 恢复原始映射 + 释放影子页
 */
static int wxshadow_do_del_bp(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    int idx, i, found_idx = -1, active_count = 0;

    region = wxshadow_find_region(mm, addr);
    if (!region)
        return -2;

    idx = wxshadow_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages) {
        wxshadow_put_region(region);
        return -2;
    }

    pi = &region->pages[idx];
    if ((int)pi->bp_count <= 0) {
        wxshadow_put_region(region);
        return -2;
    }

    /* 查找目标断点并统计剩余活跃断点数 */
    for (i = 0; i < (int)pi->bp_count; i++) {
        if ((pi->bps[i].active & 1) && pi->bps[i].addr == addr)
            found_idx = i;
        else if (pi->bps[i].active & 1)
            active_count++;
    }

    if (found_idx < 0) {
        wxshadow_put_region(region);
        return -2;
    }

    /* 清除断点条目 */
    pi->bps[found_idx].active = 0;
    memset(&pi->bps[found_idx].mods, 0, sizeof(pi->bps[found_idx].mods));
    pi->bps[found_idx].mod_count = 0;

    pr_info("wxshadow: del bp at %lx\n", addr);

    if (active_count > 0) {
        /* 仍有其他断点 - 只恢复该位置的原始指令 */
        if (pi->shadow_page_va && pi->orig_pfn) {
            uint64_t orig_va = wx_pa_to_va(pi->orig_pfn << 12);
            *(uint32_t *)(pi->shadow_page_va + (addr & 0xFFF)) =
                *(uint32_t *)(orig_va + (addr & 0xFFF));
            wx_flush_icache(page_addr);
        }
    } else {
        /* 无剩余断点 - 恢复原始映射并释放影子页 */
        void *vma = kfn_find_vma(mm, addr);
        if (vma && addr >= *(unsigned long *)vma && pi->state) {
            uint64_t shadow_va = pi->shadow_page_va;
            if (shadow_va) {
                wxshadow_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_USER_RDONLY);
                pi->shadow_page_va = 0;
                pi->orig_pfn = 0;
                pi->shadow_pfn = 0;
                pi->state = STATE_NONE;
                pi->bp_count = 0;
                kfn_free_pages(shadow_va, 0);
                pr_info("wxshadow: cleaned up shadow page for %lx\n", addr);
            }
        }
    }

    wxshadow_put_region(region);
    return 0;
}

/* ========== prctl 系统调用Hook ========== */

/*
 * prctl 系统调用的 before hook
 * 用户态通过 prctl() 与本模块通信:
 *   prctl(0x57585801, pid, addr)           -> 设置断点
 *   prctl(0x57585802, pid, addr, reg, val) -> 设置寄存器修改
 *   prctl(0x57585803, pid, addr)           -> 删除断点
 * pid=0 表示操作当前进程，否则通过find_task_by_vpid查找目标进程
 */
static void prctl_before(hook_fargs5_t *fargs, void *udata)
{
    uint64_t *args = syscall_args(fargs);
    uint64_t option = args[0];
    uint64_t pid = args[1];
    unsigned long addr = (unsigned long)args[2];
    uint64_t arg3 = args[3];
    uint64_t arg4 = args[4];

    /* Lazy detect context.id if needed */
    if (wx_mm_context_id_offset < 0)
        try_scan_mm_context_id_offset();

    void *mm = NULL;
    void *task;
    int ret;

    if ((uint32_t)option == WXSHADOW_PRCTL_SET_BP) {
        pr_info("wxshadow: [prctl] SET_BP pid=%d addr=%lx\n", (int)pid, addr);

        if (!(uint32_t)pid) {
            pr_info("wxshadow: [prctl] SET_BP using current task\n");
            mm = kfn_get_task_mm((void *)get_current());
            if (!mm) {
                pr_err("wxshadow: [prctl] SET_BP get_task_mm(current) failed\n");
                fargs->skip_origin = 1;
                fargs->ret = -3;
                return;
            }
        } else {
            pr_info("wxshadow: [prctl] SET_BP looking up pid=%d\n", (int)pid);
            kfn_rcu_read_lock();
            task = kfn_find_task_by_vpid((int)pid);
            if (!task) {
                pr_err("wxshadow: [prctl] SET_BP find_task_by_vpid failed\n");
                kfn_rcu_read_unlock();
                fargs->skip_origin = 1;
                fargs->ret = -3;
                return;
            }
            pr_info("wxshadow: [prctl] SET_BP found task=%px\n", task);
            mm = kfn_get_task_mm(task);
            kfn_rcu_read_unlock();
            if (!mm) {
                pr_err("wxshadow: [prctl] SET_BP get_task_mm failed\n");
                fargs->skip_origin = 1;
                fargs->ret = -3;
                return;
            }
        }

        pr_info("wxshadow: [prctl] SET_BP mm=%px, calling do_set_bp\n", mm);
        ret = wxshadow_do_set_bp(mm, addr);
        pr_info("wxshadow: [prctl] SET_BP do_set_bp returned %d\n", ret);
        kfn_mmput(mm);
        fargs->skip_origin = 1;
        fargs->ret = ret;
        return;
    }

    if ((uint32_t)option == WXSHADOW_PRCTL_SET_REG) {
        if (!(uint32_t)pid) {
            mm = kfn_get_task_mm((void *)get_current());
        } else {
            kfn_rcu_read_lock();
            task = kfn_find_task_by_vpid((int)pid);
            if (!task) {
                kfn_rcu_read_unlock();
                fargs->skip_origin = 1;
                fargs->ret = -3;
                return;
            }
            mm = kfn_get_task_mm(task);
            kfn_rcu_read_unlock();
        }
        if (!mm) {
            fargs->skip_origin = 1;
            fargs->ret = -3;
            return;
        }
        ret = wxshadow_do_set_reg(mm, addr, (unsigned int)arg3, arg4);
        kfn_mmput(mm);
        fargs->skip_origin = 1;
        fargs->ret = ret;
        return;
    }

    if ((uint32_t)option == WXSHADOW_PRCTL_DEL_BP) {
        if (!(uint32_t)pid) {
            mm = kfn_get_task_mm((void *)get_current());
        } else {
            kfn_rcu_read_lock();
            task = kfn_find_task_by_vpid((int)pid);
            if (!task) {
                kfn_rcu_read_unlock();
                fargs->skip_origin = 1;
                fargs->ret = -3;
                return;
            }
            mm = kfn_get_task_mm(task);
            kfn_rcu_read_unlock();
        }
        if (!mm) {
            fargs->skip_origin = 1;
            fargs->ret = -3;
            return;
        }
        ret = wxshadow_do_del_bp(mm, addr);
        kfn_mmput(mm);
        fargs->skip_origin = 1;
        fargs->ret = ret;
        return;
    }

    /* 不是我们的 prctl 命令，不处理 */
}

/* ========== 符号解析 ========== */

/* 必须解析的符号 (找不到则初始化失败) */
#define RESOLVE_REQUIRED(name, var) \
    var = (typeof(var))kallsyms_lookup_name(name); \
    if (!var) { pr_err("wxshadow: failed to find symbol: %s\n", name); return -1; }

/* 可选解析的符号 (找不到也不影响) */
#define RESOLVE_OPTIONAL(name, var) \
    var = (typeof(var))kallsyms_lookup_name(name);

/*
 * 解析所有需要的内核符号
 * 通过 KP 框架提供的 kallsyms_lookup_name() 动态查找内核函数地址
 * 还会检测页表配置和物理地址转换参数
 */
static int resolve_symbols(void)
{
    pr_info("wxshadow: resolving symbols...\n");

    /* [1/12] MM functions */
    pr_info("wxshadow: [1/12] mm functions...\n");
    RESOLVE_REQUIRED("find_vma", kfn_find_vma);
    RESOLVE_REQUIRED("get_task_mm", kfn_get_task_mm);
    RESOLVE_REQUIRED("mmput", kfn_mmput);

    kfn_exit_mmap = (void *)kallsyms_lookup_name("exit_mmap");
    if (kfn_exit_mmap)
        pr_info("wxshadow: exit_mmap found at %px\n", kfn_exit_mmap);
    else
        pr_warn("wxshadow: exit_mmap not found, process exit may cause Bad page map\n");

    /* [2/12] Page alloc */
    pr_info("wxshadow: [2/12] page alloc...\n");
    RESOLVE_REQUIRED("__get_free_pages", kfn___get_free_pages);

    /* [3/12] Page free */
    pr_info("wxshadow: [3/12] page free...\n");
    RESOLVE_REQUIRED("free_pages", kfn_free_pages);

    /* [4/12] Page refcount */
    pr_info("wxshadow: [4/12] page refcount...\n");
    RESOLVE_OPTIONAL("put_page", kfn_put_page);
    if (!kfn_put_page)
        RESOLVE_OPTIONAL("__put_page", kfn_put_page);
    RESOLVE_OPTIONAL("get_page", kfn_get_page);

    /* [5/12] Address translation */
    pr_info("wxshadow: [5/12] address translation...\n");
    kvar_memstart_addr = (uint64_t *)kallsyms_lookup_name("memstart_addr");
    if (!kvar_memstart_addr) {
        pr_err("wxshadow: memstart_addr not found\n");
        return -1;
    }
    pr_info("wxshadow: memstart_addr=%px, value=0x%llx\n", kvar_memstart_addr, *kvar_memstart_addr);

    kvar_physvirt_offset = (uint64_t *)kallsyms_lookup_name("physvirt_offset");
    if (kvar_physvirt_offset)
        pr_info("wxshadow: physvirt_offset=%px, value=0x%llx (KASLR mode)\n",
                kvar_physvirt_offset, *kvar_physvirt_offset);
    else
        pr_info("wxshadow: physvirt_offset not found, using traditional memstart_addr mode\n");

    /* 从 TCR_EL1 计算 PAGE_OFFSET (内核虚拟地址基址) */
    uint64_t tcr = 0;
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr));
    int t0sz = (tcr >> 16) & 0x3F;
    wx_page_offset_base = -1ULL << (63 - t0sz);

    /* 用 _stext 符号验证 PAGE_OFFSET 计算是否正确 */
    uint64_t stext = (uint64_t)kallsyms_lookup_name("_stext");
    if (stext) {
        uint64_t calculated = wx_page_offset_base & stext;
        if (calculated != wx_page_offset_base) {
            pr_warn("wxshadow: PAGE_OFFSET mismatch! calculated=0x%lx, from _stext=0x%lx\n",
                    (unsigned long)wx_page_offset_base, (unsigned long)calculated);
            wx_page_offset_base = calculated;
        }
    }
    pr_info("wxshadow: PAGE_OFFSET=0x%lx\n", (unsigned long)wx_page_offset_base);

    /* 通过 AT (Address Translation) 指令检测 physvirt_offset */
    unsigned long test_page = kfn___get_free_pages(wx_gfp_kernel, 0);
    if (test_page) {
        uint64_t par;
        asm volatile("at s1e1r, %0" :: "r"(test_page));
        isb();
        asm volatile("mrs %0, par_el1" : "=r"(par));

        if (!(par & 1)) {
            uint64_t pa = (par & PA_MASK) | (test_page & 0xFFF);
            if (pa) {
                detected_physvirt_offset = test_page - pa;
                physvirt_offset_valid = 1;
                pr_info("wxshadow: detected physvirt_offset = 0x%llx\n", detected_physvirt_offset);
            }
        }
        kfn_free_pages(test_page, 0);
    }

    /* [6/12] Page table ops */
    pr_info("wxshadow: [6/12] page table ops...\n");
    uint64_t tg0 = (tcr >> 30) & 3;
    int bits_per_level;
    if (tg0 == 1) {
        wx_page_shift = 14;
        bits_per_level = 11;
    } else if (tg0 == 3) {
        wx_page_shift = 16;
        bits_per_level = 13;
    } else {
        wx_page_shift = 12;
        bits_per_level = 9;
    }
    wx_page_level = (60 - t0sz) / bits_per_level;
    pr_info("wxshadow: TCR_EL1=0x%llx, va_bits=%lld, page_shift=%d, page_level=%d\n",
            tcr, (long long)(64 - t0sz), wx_page_shift, wx_page_level);

    RESOLVE_REQUIRED("_raw_spin_lock", kfn__raw_spin_lock);
    RESOLVE_REQUIRED("_raw_spin_unlock", kfn__raw_spin_unlock);

    kfn_find_task_by_vpid = (typeof(kfn_find_task_by_vpid))kallsyms_lookup_name("find_task_by_vpid");
    kfn___task_pid_nr_ns = (typeof(kfn___task_pid_nr_ns))kallsyms_lookup_name("__task_pid_nr_ns");
    if (!kfn_find_task_by_vpid || !kfn___task_pid_nr_ns) {
        pr_err("wxshadow: required kernel functions not found\n");
        return -1;
    }

    kfn_init_task = (void *)kallsyms_lookup_name("init_task");
    if (!kfn_init_task) {
        pr_err("wxshadow: init_task not found\n");
        return -1;
    }
    pr_info("wxshadow: wx_init_task at %px\n", kfn_init_task);

    /* TLB flush */
    kfn_flush_tlb_page = (void *)kallsyms_lookup_name("flush_tlb_page");
    if (kfn_flush_tlb_page) {
        pr_info("wxshadow: flush_tlb_page at %px\n", kfn_flush_tlb_page);
    } else {
        kfn___flush_tlb_range = (typeof(kfn___flush_tlb_range))kallsyms_lookup_name("__flush_tlb_range");
        if (kfn___flush_tlb_range) {
            pr_info("wxshadow: using __flush_tlb_range at %px (fallback)\n", kfn___flush_tlb_range);
        } else {
            pr_warn("wxshadow: neither flush_tlb_page nor __flush_tlb_range found\n");
            pr_info("wxshadow: will use TLBI instruction fallback (requires mm->context.id detection)\n");
        }
    }

    /* Cache ops */
    RESOLVE_OPTIONAL("__sync_icache_dcache", kfn___sync_icache_dcache);

    pr_info("wxshadow: [7/12] cache ops...\n");
    RESOLVE_REQUIRED("flush_dcache_page", kfn_flush_dcache_page);

    kfn___flush_icache_range = (typeof(kfn___flush_icache_range))kallsyms_lookup_name("__flush_icache_range");
    if (!kfn___flush_icache_range)
        kfn___flush_icache_range = (typeof(kfn___flush_icache_range))kallsyms_lookup_name("flush_icache_range");
    if (!kfn___flush_icache_range)
        kfn___flush_icache_range = (typeof(kfn___flush_icache_range))kallsyms_lookup_name("__flush_cache_user_range");
    if (!kfn___flush_icache_range)
        kfn___flush_icache_range = (typeof(kfn___flush_icache_range))kallsyms_lookup_name("invalidate_icache_range");
    if (kfn___flush_icache_range)
        pr_info("wxshadow: using kernel icache flush at %px\n", kfn___flush_icache_range);
    else
        pr_info("wxshadow: using built-in icache flush (dc cvau + ic ialluis)\n");

    /* [8/12] Debug/single-step */
    pr_info("wxshadow: [8/12] debug/single-step...\n");
    kfn_user_enable_single_step = (typeof(kfn_user_enable_single_step))kallsyms_lookup_name("user_enable_single_step");
    kfn_user_disable_single_step = (typeof(kfn_user_disable_single_step))kallsyms_lookup_name("user_disable_single_step");
    if (!kfn_user_enable_single_step || !kfn_user_disable_single_step) {
        pr_err("wxshadow: single step functions not found\n");
        return -1;
    }

    /* [9/12] BRK/step hooks */
    pr_info("wxshadow: [9/12] BRK/step hooks...\n");
    kfn_brk_handler = (void *)kallsyms_lookup_name("brk_handler");
    kfn_single_step_handler = (void *)kallsyms_lookup_name("single_step_handler");
    pr_info("wxshadow: brk_handler = %px\n", kfn_brk_handler);
    pr_info("wxshadow: single_step_handler = %px\n", kfn_single_step_handler);
    if (!kfn_brk_handler || !kfn_single_step_handler) {
        pr_err("wxshadow: brk_handler or single_step_handler not found\n");
        return -1;
    }

    /* [10/12] Locking (skipped - lockless) */
    pr_info("wxshadow: [10/12] locking... (skipped - lockless operation)\n");

    /* [11/12] RCU */
    pr_info("wxshadow: [11/12] RCU...\n");
    kfn_rcu_read_lock = (typeof(kfn_rcu_read_lock))kallsyms_lookup_name("__rcu_read_lock");
    kfn_rcu_read_unlock = (typeof(kfn_rcu_read_unlock))kallsyms_lookup_name("__rcu_read_unlock");
    if (!kfn_rcu_read_lock || !kfn_rcu_read_unlock) {
        pr_err("wxshadow: RCU functions not found\n");
        return -1;
    }

    /* [12/12] Memory alloc */
    pr_info("wxshadow: [12/12] memory alloc...\n");
    kfn_kzalloc = (typeof(kfn_kzalloc))kallsyms_lookup_name("kzalloc");
    if (!kfn_kzalloc)
        kfn_kzalloc = (typeof(kfn_kzalloc))kallsyms_lookup_name("__kmalloc");
    if (!kfn_kzalloc)
        kfn_kzalloc = (typeof(kfn_kzalloc))kallsyms_lookup_name("__kmalloc_node");
    if (!kfn_kzalloc)
        kfn_kzalloc = (typeof(kfn_kzalloc))kallsyms_lookup_name("kmalloc_trace");
    if (!kfn_kzalloc) {
        pr_err("wxshadow: kzalloc/__kmalloc not found\n");
        return -1;
    }
    pr_info("wxshadow: kzalloc resolved to %px\n", kfn_kzalloc);

    kfn_kcalloc = (typeof(kfn_kcalloc))kallsyms_lookup_name("kcalloc");
    if (!kfn_kcalloc)
        kfn_kcalloc = (typeof(kfn_kcalloc))kallsyms_lookup_name("kmalloc_array");
    if (kfn_kcalloc)
        pr_info("wxshadow: kcalloc resolved to %px\n", kfn_kcalloc);
    else
        pr_warn("wxshadow: kcalloc/kmalloc_array not found, will use kzalloc wrapper\n");

    RESOLVE_REQUIRED("kfree", kfn_kfree);

    kfn_copy_from_kernel_nofault = (typeof(kfn_copy_from_kernel_nofault))kallsyms_lookup_name("copy_from_kernel_nofault");
    if (!kfn_copy_from_kernel_nofault)
        kfn_copy_from_kernel_nofault = (typeof(kfn_copy_from_kernel_nofault))kallsyms_lookup_name("probe_kernel_read");
    if (kfn_copy_from_kernel_nofault)
        pr_info("wxshadow: safe memory access available at %px\n", kfn_copy_from_kernel_nofault);

    /* [13/14] Page fault handler */
    pr_info("wxshadow: [13/14] page fault handler (safe lookup)...\n");
    kfn_do_page_fault = (void *)kallsyms_lookup_name("do_page_fault");
    if (!kfn_do_page_fault)
        kfn_do_page_fault = (void *)kallsyms_lookup_name("__do_page_fault");
    if (!kfn_do_page_fault)
        kfn_do_page_fault = (void *)kallsyms_lookup_name("do_mem_abort");
    if (kfn_do_page_fault)
        pr_info("wxshadow: page fault handler found at %px\n", kfn_do_page_fault);
    else
        pr_warn("wxshadow: page fault handler not found, read hiding disabled\n");

    pr_info("wxshadow: all symbols resolved successfully\n");
    return 0;
}

/* ========== 偏移量扫描 ========== */
/*
 * 不同内核版本的结构体布局不同，需要动态检测关键字段的偏移
 * 这些偏移量对于正确操控页表、VMA、任务结构至关重要
 */

/* 检查 mm_struct.pgd 偏移 (由KP框架提供) */
static int scan_mm_struct_offsets(void)
{
    pr_info("wxshadow: using KP framework mm_struct_offset.pgd_offset = 0x%x\n",
            (unsigned int)mm_struct_offset.pgd_offset);
    if (mm_struct_offset.pgd_offset < 0) {
        pr_err("wxshadow: KP framework did not detect pgd_offset!\n");
        return -1;
    }
    return 0;
}

/*
 * 扫描 vm_area_struct.vm_mm 偏移
 * 通过在VMA结构体中搜索mm指针来确定偏移量
 * 这个偏移量用于从VMA获取对应的mm_struct (用于TLB刷新)
 */
static int scan_vma_struct_offsets(void)
{
    void *mm, *vma;
    uint64_t vma_addr;
    int offset;

    pr_info("wxshadow: scanning vm_area_struct offsets...\n");

    mm = kfn_get_task_mm((void *)get_current());
    if (!mm) {
        pr_warn("wxshadow: current task has no mm, using default vma offset\n");
        goto use_default;
    }

    /* 读取mm_struct的第一个VMA (mmap字段) */
    if (kfn_copy_from_kernel_nofault) {
        if (kfn_copy_from_kernel_nofault(&vma_addr, mm, 8)) {
            kfn_mmput(mm);
            goto use_default;
        }
    } else {
        vma_addr = *(uint64_t *)mm;
    }

    if (!vma_addr) {
        pr_warn("wxshadow: no VMA in current mm, using default offset\n");
        kfn_mmput(mm);
        goto use_default;
    }

    vma = (void *)vma_addr;
    pr_info("wxshadow: scanning VMA at %px for mm pointer %px\n", vma, mm);

    /* 在VMA结构体中搜索mm指针以确定vm_mm偏移 */
    for (offset = 16; offset < 128; offset += 8) {
        uint64_t val;
        if (kfn_copy_from_kernel_nofault) {
            if (kfn_copy_from_kernel_nofault(&val, (void *)(vma_addr + offset), 8))
                continue;
        } else {
            val = *(uint64_t *)(vma_addr + offset);
        }
        if (val == (uint64_t)mm) {
            wx_vma_vm_mm_offset = offset;
            pr_info("wxshadow: vm_area_struct.vm_mm offset: 0x%x\n", offset);
            kfn_mmput(mm);
            return 0;
        }
    }

    kfn_mmput(mm);

use_default:
    wx_vma_vm_mm_offset = 64;
    pr_info("wxshadow: using default vm_mm offset: 0x%x\n", wx_vma_vm_mm_offset);
    return 0;
}

/*
 * 检测 task_struct 偏移
 * 优先使用KP框架提供的偏移，否则在init_task中搜索"swapper"字符串
 */
static int detect_task_struct_offsets(void)
{
    pr_info("wxshadow: detecting task_struct offsets...\n");

    if (!kfn_init_task) {
        pr_err("wxshadow: wx_init_task is NULL\n");
        return -1;
    }

    /* 优先使用KP框架已检测的偏移 */
    if (task_struct_offset.comm_offset > 0) {
        pr_info("wxshadow: comm_offset = 0x%x (from framework)\n",
                (unsigned int)task_struct_offset.comm_offset);
    } else {
        /* 在init_task中扫描"swapper"字符串来定位comm字段 */
        int offset;
        for (offset = 1024; offset < 6144; offset += 4) {
            char buf[16] = {0};
            if (kfn_copy_from_kernel_nofault)
                kfn_copy_from_kernel_nofault(buf, (void *)((uint64_t)kfn_init_task + offset), 16);
            else
                memcpy(buf, (void *)((uint64_t)kfn_init_task + offset), 16);
            buf[7] = 0;
            if (buf[0] == 's' && buf[1] == 'w' && buf[2] == 'a' && buf[3] == 'p' &&
                buf[4] == 'p' && buf[5] == 'e' && buf[6] == 'r') {
                task_struct_offset.comm_offset = offset;
                pr_info("wxshadow: found comm at offset 0x%x: \"%.16s\"\n", offset, buf);
                break;
            }
        }
        if (task_struct_offset.comm_offset <= 0) {
            pr_err("wxshadow: failed to find comm_offset\n");
            return -1;
        }
    }

    /* Detect mm_offset if not provided by KP framework.
     * KP never detects mm_offset because init_task->mm == NULL (swapper is
     * a kernel thread), so there is no non-null pointer to search for.
     * active_mm_offset IS detected (init_mm is non-null for swapper).
     * In task_struct, mm always sits immediately before active_mm, so
     * mm_offset == active_mm_offset - 8. Verify this by checking that
     * *(init_task + candidate) == 0 (NULL mm) and active_mm is a kernel ptr.
     */
    if (task_struct_offset.mm_offset <= 0 && task_struct_offset.active_mm_offset > 0) {
        int active_mm_off = task_struct_offset.active_mm_offset;
        int candidate = active_mm_off - 8;
        uint64_t mm_val = 0, active_mm_val = 0;
        bool read_ok = false;

        if (kfn_copy_from_kernel_nofault) {
            uint64_t tmp;
            if (!kfn_copy_from_kernel_nofault(&tmp, (void *)((uint64_t)kfn_init_task + candidate), 8)) {
                mm_val = tmp;
                if (!kfn_copy_from_kernel_nofault(&tmp, (void *)((uint64_t)kfn_init_task + active_mm_off), 8)) {
                    active_mm_val = tmp;
                    read_ok = true;
                }
            }
        } else {
            mm_val = *(uint64_t *)((uint64_t)kfn_init_task + candidate);
            active_mm_val = *(uint64_t *)((uint64_t)kfn_init_task + active_mm_off);
            read_ok = true;
        }

        if (read_ok && mm_val == 0 && (active_mm_val >> 48) == 0xFFFF) {
            /* Primary: mm_offset = active_mm_offset - 8, verified */
            task_struct_offset.mm_offset = candidate;
            pr_info("wxshadow: mm_offset = 0x%x (inferred: active_mm_offset - 8, verified)\n",
                    candidate);
        } else {
            /* Fallback: scan [active_mm_off-32, active_mm_off) for a slot
             * where init_task (kernel thread) stores NULL */
            int scan;
            for (scan = active_mm_off - 32; scan < active_mm_off; scan += 8) {
                uint64_t val = 0;
                bool rok = false;
                if (scan <= 0) continue;
                if (kfn_copy_from_kernel_nofault) {
                    uint64_t tmp;
                    if (!kfn_copy_from_kernel_nofault(&tmp, (void *)((uint64_t)kfn_init_task + scan), 8)) {
                        val = tmp; rok = true;
                    }
                } else {
                    val = *(uint64_t *)((uint64_t)kfn_init_task + scan);
                    rok = true;
                }
                if (rok && val == 0) {
                    task_struct_offset.mm_offset = scan;
                    pr_info("wxshadow: mm_offset = 0x%x (scan fallback)\n", scan);
                    break;
                }
            }
            /* Hard fallback: just use active_mm_offset - 8 unconditionally */
            if (task_struct_offset.mm_offset <= 0) {
                task_struct_offset.mm_offset = candidate;
                pr_warn("wxshadow: mm_offset forced to 0x%x (active_mm_offset - 8, unverified)\n",
                        candidate);
            }
        }
    }

    pr_info("wxshadow: task_struct offsets: tasks=0x%x, mm=0x%x, active_mm=0x%x, comm=0x%x\n",
            (unsigned int)task_struct_offset.tasks_offset,
            (unsigned int)task_struct_offset.mm_offset,
            (unsigned int)task_struct_offset.active_mm_offset,
            (unsigned int)task_struct_offset.comm_offset);
    pr_info("wxshadow: pid/tgid: using wxfunc(__task_pid_nr_ns)\n");
    return 0;
}

/*
 * 扫描 mm->context.id 偏移
 * ASID (Address Space ID) 存储在 mm->context.id 中
 * 通过读取 TTBR0_EL1 获取当前ASID，然后在mm_struct中搜索匹配的值
 * 这个偏移量用于 TLBI 指令回退方式的TLB刷新
 */
static int try_scan_mm_context_id_offset(void)
{
    void *task = (void *)get_current();
    uint64_t mm_val;
    uint64_t ttbr0, asid;
    int offset;

    pr_info("wxshadow: trying to scan mm->context.id offset...\n");

    if (task_struct_offset.mm_offset <= 0) {
        pr_warn("wxshadow: mm_offset not detected, cannot scan context.id\n");
        return -1;
    }

    /* 从当前任务读取 mm_struct 指针 */
    uint64_t mm_ptr_addr = (uint64_t)task + task_struct_offset.mm_offset;
    if (kfn_copy_from_kernel_nofault) {
        if (kfn_copy_from_kernel_nofault(&mm_val, (void *)mm_ptr_addr, 8))
            return -1;
    } else {
        mm_val = *(uint64_t *)mm_ptr_addr;
    }

    if (!mm_val) {
        pr_info("wxshadow: current is kernel thread (mm=NULL), skip context.id scan\n");
        return -1;
    }

    /* 从TTBR0寄存器获取当前ASID (TTBR0高位存储ASID) */
    asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0));
    asid = ttbr0 >> 48;
    pr_info("wxshadow: TTBR0_EL1=0x%llx, ASID=%llu (0x%llx)\n", ttbr0, asid, asid);

    /* 在mm_struct中搜索与ASID匹配的值
     * context.id 在不同内核中有两种格式:
     *   标准 Linux: counter = ASID | (generation << ASID_BITS)  → ASID在bits[0:15]
     *   MTK定制:    counter = 完整TTBR0值 = phys_pgd|(ASID<<48)  → ASID在bits[48:63]
     */
    if (wx_debug_mode)
        pr_info("wxshadow: === mm_struct raw dump (mm=%px, offset 0x100~0x400) ===\n", (void *)mm_val);
    for (offset = 256; offset < 1024; offset += 8) {
        uint64_t val;
        if (kfn_copy_from_kernel_nofault) {
            if (kfn_copy_from_kernel_nofault(&val, (void *)(mm_val + offset), 8))
                continue;
        } else {
            val = *(uint64_t *)(mm_val + offset);
        }
        /* Debug: dump 每行: 偏移 + 值 + 4个16位段 */
        if (wx_debug_mode && offset < 0x400)
            pr_info("wxshadow:   [0x%03x] 0x%016llx  [%04llx|%04llx|%04llx|%04llx]\n",
                    offset, val,
                    (val >> 48) & 0xFFFF, (val >> 32) & 0xFFFF,
                    (val >> 16) & 0xFFFF, val & 0xFFFF);

        /* 标准 Linux: ASID在bits[0:15] */
        if ((val & 0xFFFF) == asid) {
            wx_mm_context_id_offset = offset;
            wx_mm_context_id_asid_shift = 0;
            pr_info("wxshadow: found mm->context.id at offset 0x%x, val=0x%llx (asid@bits[0:15])\n",
                    offset, val);
            return 0;
        }
        /* 中间位: bits[16:31] 或 bits[32:47] */
        if (((val >> 16) & 0xFFFF) == asid) {
            wx_mm_context_id_offset = offset;
            wx_mm_context_id_asid_shift = 16;
            pr_info("wxshadow: found mm->context.id at offset 0x%x, val=0x%llx (asid@bits[16:31])\n",
                    offset, val);
            return 0;
        }
        if (((val >> 32) & 0xFFFF) == asid) {
            wx_mm_context_id_offset = offset;
            wx_mm_context_id_asid_shift = 32;
            pr_info("wxshadow: found mm->context.id at offset 0x%x, val=0x%llx (asid@bits[32:47])\n",
                    offset, val);
            return 0;
        }
        /* MTK定制: context.id = 完整TTBR0, ASID在bits[48:63] */
        if (((val >> 48) & 0xFFFF) == asid) {
            wx_mm_context_id_offset = offset;
            wx_mm_context_id_asid_shift = 48;
            pr_info("wxshadow: found mm->context.id at offset 0x%x, val=0x%llx (asid@bits[48:63], MTK full-TTBR0)\n",
                    offset, val);
            return 0;
        }
    }

    pr_warn("wxshadow: mm->context.id offset not found (ASID=%llu)\n", asid);
    return -1;
}

/* 调试用: 打印前 N 个进程的信息，验证task_struct偏移量是否正确 */
static void debug_print_tasks_list(int max_count)
{
    int count = 0;
    uint64_t task_addr, next_addr;
    int tasks_off, comm_off, mm_off;

    pr_info("wxshadow: === DEBUG: tasks list (first %d processes) ===\n", max_count);

    tasks_off = task_struct_offset.tasks_offset;
    comm_off = task_struct_offset.comm_offset;
    mm_off = task_struct_offset.mm_offset;

    if (tasks_off < 0 || comm_off < 0) {
        pr_err("wxshadow: tasks_offset (%d) or comm_offset (%d) not initialized!\n",
               tasks_off, comm_off);
        return;
    }
    if (!kfn_init_task) {
        pr_err("wxshadow: wx_init_task is NULL!\n");
        return;
    }

    pr_info("wxshadow: wx_init_task (swapper) at %px\n", kfn_init_task);

    task_addr = *(uint64_t *)((uint64_t)kfn_init_task + tasks_off);
    task_addr -= tasks_off;

    while (task_addr != (uint64_t)kfn_init_task && count < max_count) {
        uint64_t mm_val = 0;
        int pid = 0, tgid = 0;

        pid = kfn___task_pid_nr_ns((void *)task_addr, 0, NULL);
        tgid = kfn___task_pid_nr_ns((void *)task_addr, 1, NULL);

        if (mm_off >= 0) {
            if (kfn_copy_from_kernel_nofault)
                kfn_copy_from_kernel_nofault(&mm_val, (void *)(task_addr + mm_off), 8);
            else
                mm_val = *(uint64_t *)(task_addr + mm_off);
        }

        pr_info("wxshadow: [%d] task=%px pid=%d tgid=%d mm=%px comm=\"%.16s\"\n",
                count, (void *)task_addr, pid, tgid, (void *)mm_val,
                (char *)(task_addr + comm_off));
        count++;

        next_addr = *(uint64_t *)(task_addr + tasks_off);
        task_addr = next_addr - tasks_off;
    }

    pr_info("wxshadow: === END tasks list (%d processes printed) ===\n", count);
}

/* ========== KPM 模块生命周期 ========== */

/*
 * 模块初始化 - KernelPatch 加载模块时调用
 * 步骤: 解析符号 -> 扫描偏移 -> 安装hook -> 就绪
 */
static long wxshadow_init(const char *args, const char *event, void *__user reserved)
{
    int ret;

    pr_info("wxshadow: initializing...\n");

    /* 解析参数: 包含 'd' 则启用调试模式 */
    if (args) {
        const char *p = args;
        while (*p) {
            if (*p == 'd' || *p == 'D') {
                wx_debug_mode = 1;
                pr_info("wxshadow: DEBUG MODE enabled\n");
                break;
            }
            p++;
        }
    }

    ret = resolve_symbols();
    if (ret < 0) {
        pr_err("wxshadow: failed to resolve symbols\n");
        return ret;
    }

    ret = scan_mm_struct_offsets();
    if (ret < 0) {
        pr_err("wxshadow: failed to scan mm_struct offsets\n");
        return ret;
    }

    ret = scan_vma_struct_offsets();
    if (ret < 0) {
        pr_err("wxshadow: failed to scan vma offsets\n");
        return ret;
    }

    ret = detect_task_struct_offsets();
    if (ret < 0) {
        pr_err("wxshadow: failed to detect task_struct offsets\n");
        return ret;
    }

    try_scan_mm_context_id_offset();

    /* 初始化全局区域链表 */
    INIT_LIST_HEAD(&region_list);

    /* Hook brk_handler - 断点核心 (必须) */
    pr_info("wxshadow: hooking brk_handler at %px...\n", kfn_brk_handler);
    ret = hook_wrap(kfn_brk_handler, 3, brk_handler_before, NULL, NULL);
    if (ret) {
        pr_err("wxshadow: failed to hook brk_handler: %d\n", ret);
        return -1;
    }
    pr_info("wxshadow: hooked brk_handler\n");

    /* Hook single_step_handler - 单步核心 (必须) */
    pr_info("wxshadow: hooking single_step_handler at %px...\n", kfn_single_step_handler);
    ret = hook_wrap(kfn_single_step_handler, 3, single_step_handler_before, NULL, NULL);
    if (ret) {
        pr_err("wxshadow: failed to hook single_step_handler: %d\n", ret);
        hook_unwrap_remove(kfn_brk_handler, brk_handler_before, NULL, 1);
        return -1;
    }
    pr_info("wxshadow: hooked single_step_handler\n");

    /* Hook prctl 系统调用 - 用户态通信接口 (必须) */
    ret = hook_syscalln(PRCTL_NR, 5, prctl_before, NULL, NULL);
    if (ret) {
        pr_err("wxshadow: failed to hook prctl: %d\n", ret);
        hook_unwrap_remove(kfn_brk_handler, brk_handler_before, NULL, 1);
        hook_unwrap_remove(kfn_single_step_handler, single_step_handler_before, NULL, 1);
        return -1;
    }
    pr_info("wxshadow: hooked prctl syscall\n");

    /* Hook do_page_fault - 页错误拦截 (可选，用于读/执行错误处理和对抗CRC检测) */
    if (kfn_do_page_fault) {
        ret = hook_wrap(kfn_do_page_fault, 3, (void *)do_page_fault_before, NULL, NULL);
        if (ret) {
            pr_warn("wxshadow: failed to hook do_page_fault: %d\n", ret);
            pr_warn("wxshadow: read hiding will be disabled\n");
            kfn_do_page_fault = NULL;
        } else {
            pr_info("wxshadow: hooked do_page_fault for read/exec fault handling\n");
        }
    }

    /* Hook exit_mmap - 进程退出清理 (可选，防止Bad page map错误) */
    if (kfn_exit_mmap) {
        ret = hook_wrap(kfn_exit_mmap, 1, (void *)exit_mmap_before, NULL, NULL);
        if (ret) {
            pr_warn("wxshadow: failed to hook exit_mmap: %d\n", ret);
            pr_warn("wxshadow: process exit may cause Bad page map errors\n");
            kfn_exit_mmap = NULL;
        } else {
            pr_info("wxshadow: hooked exit_mmap for proper cleanup\n");
        }
    }

    pr_info("wxshadow: W^X shadow memory module loaded\n");
    pr_info("wxshadow: use prctl(0x%x, pid, addr) to set breakpoint\n", WXSHADOW_PRCTL_SET_BP);
    pr_info("wxshadow: use prctl(0x%x, pid, addr, reg, val) to set reg mod\n", WXSHADOW_PRCTL_SET_REG);
    pr_info("wxshadow: use prctl(0x%x, pid, addr) to delete breakpoint\n", WXSHADOW_PRCTL_DEL_BP);

    if (kfn_do_page_fault)
        pr_info("wxshadow: read hiding ENABLED (do_page_fault hooked)\n");
    else
        pr_info("wxshadow: read hiding DISABLED\n");

    if (wx_debug_mode)
        debug_print_tasks_list(10);
    return 0;
}

/*
 * 模块卸载 - KernelPatch 卸载模块时调用
 * 步骤: 移除所有hook -> 恢复所有PTE -> 释放所有影子页 -> 释放所有区域
 */
static long wxshadow_exit(void *__user reserved)
{
    struct wx_region *r, *tmp;
    struct wx_region **cleanup_list;
    int count = 0, i, j;

    pr_info("wxshadow: unloading...\n");

    /* 移除所有hook (先移除hook再清理数据，防止卸载过程中触发hook) */
    if (kfn_do_page_fault) {
        hook_unwrap_remove(kfn_do_page_fault, (void *)do_page_fault_before, NULL, 1);
        pr_info("wxshadow: unhooked do_page_fault\n");
    }
    if (kfn_exit_mmap) {
        hook_unwrap_remove(kfn_exit_mmap, (void *)exit_mmap_before, NULL, 1);
        pr_info("wxshadow: unhooked exit_mmap\n");
    }
    unhook_syscalln(PRCTL_NR, prctl_before, NULL);
    hook_unwrap_remove(kfn_single_step_handler, single_step_handler_before, NULL, 1);
    hook_unwrap_remove(kfn_brk_handler, brk_handler_before, NULL, 1);
    pr_info("wxshadow: unhooked brk_handler and single_step_handler\n");

    /* 统计待清理的区域数 */
    wx_spin_lock();
    list_for_each_entry(r, &region_list, list)
        count++;

    if (!count) {
        wx_spin_unlock();
        pr_info("wxshadow: module unloaded (cleaned 0 regions)\n");
        return 0;
    }
    wx_spin_unlock();

    /* 分配清理数组，用于在锁外处理每个区域 */
    cleanup_list = (struct wx_region **)kfn_kzalloc(8UL * count, wx_gfp_kernel);

    wx_spin_lock();
    if (!cleanup_list) {
        pr_warn("wxshadow: failed to allocate cleanup array, leaking memory\n");
        /* Just detach all regions */
        list_for_each_entry_safe(r, tmp, &region_list, list) {
            list_del_init(&r->list);
        }
        wx_spin_unlock();
        count = 0;
    } else {
        int idx = 0;
        list_for_each_entry_safe(r, tmp, &region_list, list) {
            list_del_init(&r->list);
            if (idx < count)
                cleanup_list[idx++] = r;
        }
        count = idx;
        wx_spin_unlock();
    }

    /* 逐个清理区域: 禁用单步 -> 恢复原始映射 -> 释放影子页 */
    for (i = 0; i < count; i++) {
        r = cleanup_list[i];
        if (!r)
            continue;

        void *mm = r->mm;
        pr_info("wxshadow: cleanup region mm=%px vm_start=%lx vm_end=%lx nr_pages=%d\n",
                mm, r->vm_start, r->vm_end, r->nr_pages);

        void *vma = NULL;
        if (mm && kfn_find_vma) {
            vma = kfn_find_vma(mm, r->vm_start);
            if (vma) {
                if (*(unsigned long *)vma > r->vm_start ||
                    *(unsigned long *)((uint64_t)vma + 8) < r->vm_end) {
                    pr_warn("wxshadow: VMA mismatch during cleanup, skip mapping restore\n");
                    vma = NULL;
                }
            }
        }

        /* Cleanup pages */
        for (j = 0; j < r->nr_pages; j++) {
            struct page_info *pi = &r->pages[j];
            if (!pi->state)
                continue;

            unsigned long addr = r->vm_start + ((unsigned long)j << 12);
            pr_info("wxshadow: cleanup page[%d] addr=%lx state=%d stepping_task=%px\n",
                    j, addr, pi->state, (void *)pi->stepping_task);

            /* 禁用可能正在进行的单步执行 */
            if (pi->stepping_task && kfn_user_disable_single_step) {
                pr_info("wxshadow: disabling single step for task %px\n", (void *)pi->stepping_task);
                kfn_user_disable_single_step((void *)pi->stepping_task);
                pi->stepping_task = 0;
            }

            /* 恢复原始页面映射 */
            if (vma && pi->orig_pfn && (pi->state == STATE_SHADOW_X || pi->state == STATE_STEPPING)) {
                if (!wxshadow_switch_mapping(vma, addr, pi->orig_pfn, PTE_USER_RDONLY)) {
                    pr_info("wxshadow: restored original mapping for addr=%lx pfn=%lx\n",
                            addr, (unsigned long)pi->orig_pfn);
                    wx_flush_icache(addr);
                } else {
                    pr_warn("wxshadow: failed to restore mapping for addr=%lx\n", addr);
                }
            }

            /* 释放影子页 */
            if (pi->shadow_page_va) {
                kfn_free_pages(pi->shadow_page_va, 0);
                pr_info("wxshadow: freed shadow page for addr=%lx\n", addr);
                pi->shadow_page_va = 0;
            }
            pi->state = STATE_NONE;
        }

        kfn_kfree(r->pages);
        kfn_kfree(r);
    }

    if (cleanup_list)
        kfn_kfree(cleanup_list);

    pr_info("wxshadow: module unloaded (cleaned %d regions)\n", count);
    return 0;
}

/* 模块控制接口 - 通过 KernelPatch 管理器发送命令 (当前未使用) */
static long wxshadow_control(const char *args, char *__user out_msg, int outlen)
{
    const char *a = args ? args : "(null)";
    pr_info("wxshadow: control called with args: %s\n", a);
    return 0;
}

/* 注册 KPM 生命周期回调 */
KPM_INIT(wxshadow_init);       /* 加载时调用 wxshadow_init */
KPM_CTL0(wxshadow_control);    /* 控制命令调用 wxshadow_control */
KPM_EXIT(wxshadow_exit);       /* 卸载时调用 wxshadow_exit */
