/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * wxjump.c - W^X Shadow Page Jump Hook KPM
 *
 * 【核心原理】
 * 为目标代码页创建 shadow page，在 shadow page 写入用户提供的跳转指令
 * (MOVZ/MOVK/BR)，通过 execute-only 页面权限实现 CRC 防检测。
 *
 * 【与 wxshadow 的区别】
 * - 无 BRK 断点，无内核单步执行 → hook 命中时 0 内核异常
 * - 通用 PATCH/RELEASE 接口：写任意字节到 shadow page
 * - 更简单的状态机：只有 NONE / ORIG_R / SHADOW_X 三态
 *
 * 【工作流程】
 * 1. PATCH: 创建 shadow page → 写入跳转指令 → PTE 指向 shadow (--x)
 * 2. 执行: CPU 直接取指 shadow page → 跳转到用户 thunk → 0 异常
 * 3. CRC: 读 shadow page → Data Abort → 切到 orig page (r--) → 读原始字节
 * 4. 恢复: 执行 orig page → Insn Abort → 切回 shadow (--x)
 * 5. RELEASE: 还原 shadow page → 释放 → 恢复原始 PTE
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/ptrace.h>
#include <hook.h>
#include <ksyms.h>
#include <syscall.h>
#include <pgtable.h>
#include <common.h>
#include <stdint.h>

KPM_NAME("wxjump");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("XiaM");
KPM_DESCRIPTION("W^X Shadow Page Jump Hook - Zero kernel overhead inline hook with CRC protection");

/* ========== 常量 ========== */

/* prctl 命令码 (延续 wxshadow 0x575858xx 编号) */
#define WXJUMP_PRCTL_PATCH   0x57585804  /* 写入 shadow page */
#define WXJUMP_PRCTL_RELEASE 0x57585805  /* 释放 shadow page */

#define PRCTL_NR             167      /* ARM64 prctl 系统调用号 */
#define WX_PAGE_SIZE         4096

/* 页面状态机 (3 态，无 STEPPING) */
#define STATE_NONE           0   /* 未激活 */
#define STATE_ORIG_R         1   /* 原始页, r-- (不可执行, 供 CRC 读取) */
#define STATE_SHADOW_X       2   /* 影子页, --x (execute-only, 含跳转指令) */

/* ARM64 PTE 标志 */
#define PTE_BASE_FLAGS       0xF03ULL            /* Valid + AF + SH + nG (AttrIndx=0 → MT_NORMAL) */
#define PTE_USER_RDONLY      0xC0ULL              /* 用户只读+可执行 */
#define PTE_UXN_USER_RO      0x400000000000C0ULL  /* UXN + 用户只读 (不可执行) */

#define PFN_MASK             0xFFFFFFFFFULL
#define PAGE_MASK_4K         (~0xFFFULL)
#define PA_MASK              0xFFFFFFFFF000ULL

#define WX_GFP_KERNEL        3264

/* ========== 数据结构 ========== */

/*
 * 页面信息 (简化版，无 bp_entry/reg_mod/stepping)
 */
struct page_info {
    uint64_t orig_pfn;        /* 原始页帧号 */
    uint64_t orig_pte;        /* 原始完整 PTE 值 (用于精确恢复) */
    uint64_t shadow_pfn;      /* 影子页帧号 */
    uint64_t shadow_page_va;  /* 影子页内核虚拟地址 */
    uint32_t state;           /* STATE_NONE / ORIG_R / SHADOW_X */
    uint32_t patch_count;     /* 本页上的活跃 patch 数量 */
};

/*
 * W^X 区域 (管理一个 VMA 的所有页面)
 */
struct wx_region {
    struct list_head list;
    void            *mm;
    unsigned long    vm_start;
    unsigned long    vm_end;
    struct page_info *pages;
    int              nr_pages;
    int              refcount;
};

/* ========== 内核函数指针 ========== */

static void *(*kfn_find_vma)(void *mm, unsigned long addr);
static void *(*kfn_get_task_mm)(void *task);
static void  (*kfn_mmput)(void *mm);
static void *kfn_exit_mmap;

static unsigned long (*kfn___get_free_pages)(unsigned int gfp, unsigned int order);
static void  (*kfn_free_pages)(unsigned long addr, unsigned int order);

static void  (*kfn___flush_icache_range)(unsigned long start, unsigned long end);
static void  (*kfn___flush_tlb_range)(void *vma, unsigned long start, unsigned long end,
                                       unsigned long stride, int last_level, int tlb_level);
static void *(*kfn_flush_tlb_page)(void);

static void *(*kfn__raw_spin_lock)(void *lock);
static void *(*kfn__raw_spin_unlock)(void *lock);

static void *(*kfn_kzalloc)(unsigned long size, unsigned int flags);
static void *(*kfn_kcalloc)(unsigned long n, unsigned long size, unsigned int flags);
static void  (*kfn_kfree)(void *ptr);
static int   (*kfn_copy_from_kernel_nofault)(void *dst, const void *src, unsigned long size);

static void *kfn_do_page_fault;

/* 地址转换 */
static uint64_t *kvar_memstart_addr;
static uint64_t *kvar_physvirt_offset;
static uint64_t detected_physvirt_offset;
static int      physvirt_offset_valid;
static uint64_t wx_page_offset_base;
static int      wx_page_level;
static int      wx_page_shift;
static int      wx_mm_context_id_offset = -1;
static int      wx_mm_context_id_asid_shift = 0;
static int16_t  wx_vma_vm_mm_offset = 64;
static unsigned int wx_gfp_kernel = WX_GFP_KERNEL;

/* 全局状态 */
static LIST_HEAD(region_list);

/* Fault counters (debug, no locking needed for approximate counts) */
static unsigned long wx_fault_read_count;
static unsigned long wx_fault_exec_count;
static unsigned long wx_fault_el1_skip_count;
static uint64_t global_lock;

/* ========== PA ↔ VA 转换 ========== */

static inline uint64_t wx_pa_to_va(uint64_t pa)
{
    if (physvirt_offset_valid)
        return pa + detected_physvirt_offset;
    if (kvar_physvirt_offset)
        return pa + *kvar_physvirt_offset;
    return wx_page_offset_base - *kvar_memstart_addr + pa;
}

static inline uint64_t wx_va_to_pa(uint64_t va)
{
    if (physvirt_offset_valid)
        return va - detected_physvirt_offset;
    if (kvar_physvirt_offset)
        return va - *kvar_physvirt_offset;
    return *kvar_memstart_addr - wx_page_offset_base + va;
}

/* ========== 自旋锁 ========== */

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

/* ========== 缓存/TLB ========== */

static inline void wx_flush_dcache_va(uint64_t va, unsigned long size)
{
    uint64_t addr;
    for (addr = va; addr < va + size; addr += 64)
        asm volatile("dc cvau, %0" :: "r"(addr) : "memory");
    dsb(ish);
}

static inline void wx_flush_icache(unsigned long addr)
{
    unsigned long page_start = addr & PAGE_MASK_4K;
    if (kfn___flush_icache_range) {
        kfn___flush_icache_range(page_start, page_start + WX_PAGE_SIZE);
    } else {
        asm volatile("ic ialluis" ::: "memory");
        dsb(ish);
    }
    isb();
}

static void wxjump_flush_tlb_page(void *vma, unsigned long addr)
{
    dsb(ishst);  /* 确保 PTE 写入对 TLB walker 可见后再执行 TLBI */
    if (kfn_flush_tlb_page) {
        ((void (*)(void *, unsigned long))kfn_flush_tlb_page)(vma, addr);
        return;
    }
    if (kfn___flush_tlb_range) {
        kfn___flush_tlb_range(vma, addr, addr + WX_PAGE_SIZE, WX_PAGE_SIZE, 1, 3);
        return;
    }

    /* TLBI 回退 */
    uint64_t tlbi_val = addr >> 12;
    if (vma && wx_vma_vm_mm_offset >= 0) {
        void *mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);
        if (mm && wx_mm_context_id_offset >= 0) {
            uint64_t ctx_val = *(uint64_t *)((uint64_t)mm + wx_mm_context_id_offset);
            uint16_t asid = (ctx_val >> wx_mm_context_id_asid_shift) & 0xFFFF;
            if (asid) {
                uint64_t val = tlbi_val | ((uint64_t)asid << 48);
                asm volatile("tlbi vale1is, %0" :: "r"(val));
                dsb(ish);
                isb();
                return;
            }
        }
    }
    asm volatile("tlbi vaale1is, %0" :: "r"(tlbi_val));
    dsb(ish);
    isb();
}

/* ========== 页表遍历 ========== */

static uint64_t *get_user_pte(void *mm, unsigned long addr, uint64_t *ptl_out)
{
    int shift = wx_page_shift;
    int level = wx_page_level;
    int bits_per_level = shift - 3;
    uint64_t mask = ~(-1ULL << bits_per_level);
    uint64_t pgd_val, pud_base, pud_val, pmd_base, pmd_val, pte_base;
    uint64_t *pgd_p, *pud_p, *pmd_p, *pte_p;
    int pgd_idx, pud_idx, pmd_idx, pte_idx;

    if (mm_struct_offset.pgd_offset < 0)
        return NULL;

    uint64_t pgd = *(uint64_t *)((uint64_t)mm + mm_struct_offset.pgd_offset);
    if (!pgd)
        return NULL;

    pgd_idx = (addr >> (shift + (level - 1) * bits_per_level)) & mask;
    pgd_p = (uint64_t *)(pgd + 8 * pgd_idx);
    pgd_val = *pgd_p;
    if (!pgd_val)
        return NULL;

    uint64_t next_pa = pgd_val & PA_MASK;

    if (level == 4) {
        pud_base = wx_pa_to_va(next_pa);
        pud_idx = (addr >> (shift + 2 * bits_per_level)) & mask;
        pud_p = (uint64_t *)(pud_base + 8 * pud_idx);
        if (!*pud_p)
            return NULL;
        pud_val = *pud_p;
        next_pa = pud_val & PA_MASK;
    }

    pmd_base = wx_pa_to_va(next_pa);
    pmd_idx = (addr >> (shift + bits_per_level)) & mask;
    pmd_p = (uint64_t *)(pmd_base + 8 * pmd_idx);
    if (!*pmd_p)
        return NULL;
    pmd_val = *pmd_p;

    if ((pmd_val & 3) == 1) {
        pr_warn("wxjump: 2MB section mapping not supported at %lx\n", addr);
        return NULL;
    }
    if ((pmd_val & 3) != 3)
        return NULL;

    pte_base = wx_pa_to_va(pmd_val & PA_MASK);
    pte_idx = (addr >> 12) & 0x1FF;
    pte_p = (uint64_t *)(pte_base + 8 * pte_idx);

    if (ptl_out)
        *ptl_out = 0;
    return pte_p;
}

/* ========== PTE 操作 ========== */

static inline uint64_t make_pte(uint64_t pfn, uint64_t flags)
{
    return (pfn << 12) | flags | PTE_BASE_FLAGS;
}

static int wxjump_switch_mapping(void *vma, unsigned long addr,
                                  uint64_t pfn, uint64_t extra_flags)
{
    void *mm = NULL;
    uint64_t *pte;

    if (wx_vma_vm_mm_offset >= 0 && vma)
        mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);

    pte = get_user_pte(mm, addr, NULL);
    if (!pte)
        return -1;

    *pte = make_pte(pfn, extra_flags);
    wxjump_flush_tlb_page(vma, addr);
    return 0;
}

/* ========== 区域管理 ========== */

static int wxjump_page_index(struct wx_region *r, unsigned long addr)
{
    return (unsigned int)((addr - r->vm_start) >> 12);
}

static struct wx_region *wxjump_find_region(void *mm, unsigned long addr)
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

static void wxjump_put_region(struct wx_region *r)
{
    int nr, i;
    struct page_info *pi;

    wx_spin_lock();
    r->refcount--;
    if (r->refcount > 0) {
        wx_spin_unlock();
        return;
    }
    list_del_init(&r->list);
    wx_spin_unlock();

    nr = r->nr_pages;
    pi = r->pages;
    if (nr > 0 && pi) {
        for (i = 0; i < nr; i++) {
            if (pi[i].shadow_page_va)
                kfn_free_pages(pi[i].shadow_page_va, 0);
        }
    }
    kfn_kfree(pi);
    kfn_kfree(r);
}

/* ========== 映射验证 ========== */

static int wxjump_validate_mapping(void *mm, void *vma,
                                    struct page_info *pi, unsigned long addr)
{
    uint64_t *pte;
    uint64_t pte_val, current_pfn;

    if (!vma || addr < *(unsigned long *)vma ||
        *(unsigned long *)((uint64_t)vma + 8) <= addr)
        return 0;

    pte = get_user_pte(mm, addr, NULL);
    if (!pte)
        return 0;

    pte_val = *pte;
    if (!(pte_val & 1))
        return 0;

    current_pfn = (pte_val >> 12) & PFN_MASK;

    if (pi->shadow_pfn && pi->shadow_pfn == current_pfn)
        return 1;
    if (pi->orig_pfn && pi->orig_pfn == current_pfn &&
        pi->state >= STATE_ORIG_R && pi->state <= STATE_SHADOW_X)
        return 1;

    return 0;
}

/* ========== 自动清理 ========== */

static int wxjump_auto_cleanup(void *mm, struct wx_region *region,
                                unsigned int page_idx, const char *reason)
{
    struct page_info *pi;

    if (!region || (int)page_idx < 0 || (int)page_idx >= region->nr_pages)
        return -1;

    pi = &region->pages[page_idx];
    unsigned long addr = region->vm_start + ((unsigned long)page_idx << 12);

    pr_info("wxjump: auto_cleanup [%s] addr=%lx state=%d\n", reason, addr, pi->state);

    wx_spin_lock();
    if (pi->shadow_page_va) {
        uint64_t shadow_va = pi->shadow_page_va;
        pi->shadow_page_va = 0;
        pi->state = STATE_NONE;
        pi->orig_pfn = 0;
        pi->shadow_pfn = 0;
        pi->patch_count = 0;
        wx_spin_unlock();
        kfn_free_pages(shadow_va, 0);
    } else {
        pi->state = STATE_NONE;
        pi->orig_pfn = 0;
        pi->shadow_pfn = 0;
        pi->patch_count = 0;
        wx_spin_unlock();
    }
    return 0;
}

/* ========== 页错误处理 (W^X 状态机核心) ========== */

/* 写错误: 页面内容变更，影子页失效 */
static int wxjump_handle_write_fault(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    int idx;

    region = wxjump_find_region(mm, addr);
    if (!region)
        return -1;

    idx = wxjump_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages)
        goto out;

    pi = &region->pages[idx];
    if (pi->state == STATE_NONE)
        goto out;

    pr_info("wxjump: write fault at %lx, cleaning up\n", addr);
    wxjump_auto_cleanup(mm, region, idx, "Write Fault");

out:
    wxjump_put_region(region);
    return -1;
}

/*
 * 读错误: CRC 检测的核心防御
 * shadow page 是 execute-only, 读操作触发 Data Abort
 * → 切到 orig page (r--, UXN) → CRC 读到原始未修改的字节
 */
static int wxjump_handle_read_fault(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    int idx, ret;
    void *vma;

    region = wxjump_find_region(mm, addr);
    if (!region)
        return -1;

    idx = wxjump_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages) {
        wxjump_put_region(region);
        return -1;
    }

    wx_spin_lock();
    pi = &region->pages[idx];
    if (pi->state != STATE_SHADOW_X || !pi->orig_pfn) {
        wx_spin_unlock();
        wxjump_put_region(region);
        return -1;
    }
    wx_spin_unlock();

    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        wxjump_auto_cleanup(mm, region, idx, "VMA Gone (read)");
        wxjump_put_region(region);
        return -1;
    }

    if (!wxjump_validate_mapping(mm, vma, pi, page_addr)) {
        wxjump_auto_cleanup(mm, region, idx, "Mapping Changed (read)");
        wxjump_put_region(region);
        return -1;
    }

    /* 切到 orig page: UXN + 只读 (可读不可执行) */
    wx_fault_read_count++;
    ret = wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_UXN_USER_RO);
    if (ret) {
        wxjump_put_region(region);
        return -1;
    }

    wx_spin_lock();
    pi->state = STATE_ORIG_R;
    wx_spin_unlock();

    wxjump_put_region(region);
    return 0;
}

/*
 * 执行错误: CRC 恢复
 * orig page 是 r-- (不可执行), 执行操作触发 Insn Abort
 * → 切回 shadow page (--x, execute-only)
 */
static int wxjump_handle_exec_fault(void *mm, unsigned long addr)
{
    unsigned long page_addr = addr & PAGE_MASK_4K;
    struct wx_region *region;
    struct page_info *pi;
    int idx, ret;
    void *vma;

    region = wxjump_find_region(mm, addr);
    if (!region)
        return -1;

    idx = wxjump_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages) {
        wxjump_put_region(region);
        return -1;
    }

    wx_spin_lock();
    pi = &region->pages[idx];
    if (!pi->shadow_pfn || pi->state != STATE_ORIG_R) {
        wx_spin_unlock();
        wxjump_put_region(region);
        return -1;
    }
    wx_spin_unlock();

    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        wxjump_auto_cleanup(mm, region, idx, "VMA Gone (exec)");
        wxjump_put_region(region);
        return -1;
    }

    if (!wxjump_validate_mapping(mm, vma, pi, page_addr)) {
        wxjump_auto_cleanup(mm, region, idx, "Mapping Changed (exec)");
        wxjump_put_region(region);
        return -1;
    }

    if (pi->shadow_page_va)
        wx_flush_dcache_va(pi->shadow_page_va, WX_PAGE_SIZE);

    /* 切到 shadow page (--x, execute-only) */
    ret = wxjump_switch_mapping(vma, page_addr, pi->shadow_pfn, 0);
    if (ret) {
        wxjump_put_region(region);
        return -1;
    }

    wx_flush_icache(page_addr);

    wx_spin_lock();
    pi->state = STATE_SHADOW_X;
    wx_spin_unlock();

    wx_fault_exec_count++;
    wxjump_put_region(region);
    return 0;
}

/* do_page_fault 的 before hook */
static void do_page_fault_before(hook_fargs3_t *fargs, void *udata)
{
    unsigned long far = fargs->arg0;    /* x0: FAR_EL1 */
    unsigned long esr = fargs->arg1;    /* x1: ESR_EL1 */
    void *task = (void *)get_current();
    void *mm;

    mm = kfn_get_task_mm(task);
    if (!mm)
        return;

    /* 只处理 permission fault (AP 拒绝数据访问 / UXN 拒绝执行) */
    if ((esr & 0x3C) != 0x0C) {
        kfn_mmput(mm);
        return;
    }

    struct wx_region *region = wxjump_find_region(mm, far);
    if (!region) {
        kfn_mmput(mm);
        return;
    }
    wxjump_put_region(region);

    unsigned long ec = (esr >> 26) & 0x3FUL;
    unsigned long dfsc = esr & 0x3FUL;


    /* Only handle EL0 faults. EL1 faults (EC=0x21/0x25) are from kernel
     * (e.g. EPAN blocking EL1 access to UXN=0 pages) -> let kernel handle */
    if (ec != 0x24 && ec != 0x20) {
        wx_fault_el1_skip_count++;
        kfn_mmput(mm);
        return;
    }

    if (ec == 0x20) {
        /* Instruction Abort from EL0 */
        if (!wxjump_handle_exec_fault(mm, far)) {
            fargs->skip_origin = 1;
            fargs->ret = 0;
        }
    } else if (ec == 0x24 && !(esr & 0x40)) {
        /* Data Abort from EL0, WnR=0 → 读错误 (CRC 检测) */
        if (!wxjump_handle_read_fault(mm, far)) {
            fargs->skip_origin = 1;
            fargs->ret = 0;
        }
    } else {
        /* Data Abort, WnR=1 → 写错误 */
        wxjump_handle_write_fault(mm, far);
    }

    kfn_mmput(mm);
}

/* ========== 用户缓冲区读取 (通过页表遍历) ========== */

/*
 * 从用户态缓冲区复制数据到内核缓冲区
 * 通过遍历页表找到用户 buf 的物理页，然后经线性映射读取
 * 避免依赖 copy_from_user 内核符号
 */
static int wxjump_copy_from_user(void *mm, unsigned long user_addr,
                                  void *kernel_dst, size_t len)
{
    while (len > 0) {
        unsigned long page_addr = user_addr & PAGE_MASK_4K;
        unsigned long offset = user_addr & 0xFFF;
        size_t chunk = WX_PAGE_SIZE - offset;
        if (chunk > len) chunk = len;

        uint64_t *pte = get_user_pte(mm, page_addr, NULL);
        if (!pte || !(*pte & 1)) {
            pr_err("wxjump: copy_from_user: PTE not found for user addr %lx\n", user_addr);
            return -1;
        }

        uint64_t pfn = (*pte >> 12) & PFN_MASK;
        uint64_t kva = wx_pa_to_va(pfn << 12);
        memcpy(kernel_dst, (void *)(kva + offset), chunk);

        user_addr += chunk;
        kernel_dst = (uint8_t *)kernel_dst + chunk;
        len -= chunk;
    }
    return 0;
}

/* ========== PATCH 处理 ========== */

/*
 * prctl(0x5758, page_addr, buf_ptr, len, offset)
 * 在 shadow page 的 offset 处写入 len 字节
 */
static int wxjump_do_patch(void *mm, unsigned long page_addr,
                            unsigned long user_buf, size_t len, size_t offset)
{
    struct wx_region *region;
    struct page_info *pi;
    int idx;
    void *vma = NULL;
    uint8_t kbuf[64]; /* 足够容纳 20 字节的 MOVZ/MOVK/BR */

    if (offset + len > WX_PAGE_SIZE || len > sizeof(kbuf)) {
        pr_err("wxjump: patch: invalid offset=%zu len=%zu\n", offset, len);
        return -22;
    }

    page_addr &= PAGE_MASK_4K;

    pr_info("wxjump: [patch] page=%lx offset=%zu len=%zu\n", page_addr, offset, len);

    /* 先从用户空间复制 patch 字节到内核缓冲区 */
    if (wxjump_copy_from_user(mm, user_buf, kbuf, len)) {
        pr_err("wxjump: patch: failed to read user buf at %lx\n", user_buf);
        return -14;
    }

    /* 查找已有区域 */
    region = wxjump_find_region(mm, page_addr);
    if (region) {
        idx = wxjump_page_index(region, page_addr);
        pi = &region->pages[idx];

        if (pi->shadow_page_va) {
            /* 已有 shadow page → 直接写入 */
            memcpy((void *)(pi->shadow_page_va + offset), kbuf, len);

            /* 如果当前是 SHADOW_X 状态，刷新 icache */
            if (pi->state == STATE_SHADOW_X)
                wx_flush_icache(page_addr);

            wx_spin_lock();
            pi->patch_count++;
            wx_spin_unlock();

            pr_info("wxjump: patched existing shadow at %lx+%zu (%zu bytes), count=%d\n",
                    page_addr, offset, len, pi->patch_count);
            wxjump_put_region(region);
            return 0;
        }

        /* region 存在但 shadow_page_va=0 (stale page, 之前 release 过)
         * 直接在现有 region 上为此 page 重建 shadow，不递归也不 fall-through */
        goto setup_shadow_on_existing;
    }

    /* 需要创建新的 shadow page */
    vma = kfn_find_vma(mm, page_addr);
    if (!vma || page_addr < *(unsigned long *)vma) {
        pr_err("wxjump: patch: no VMA for %lx\n", page_addr);
        return -1;
    }

    /* 并发检查: 另一线程可能刚创建了 region */
    wx_spin_lock();
    struct wx_region *existing;
    list_for_each_entry(existing, &region_list, list) {
        if (existing->mm == mm &&
            page_addr >= existing->vm_start && page_addr < existing->vm_end) {
            existing->refcount++;
            wx_spin_unlock();
            region = existing;
            idx = wxjump_page_index(region, page_addr);
            pi = &region->pages[idx];
            goto setup_shadow_on_existing;
        }
    }
    wx_spin_unlock();

    /* 创建区域 */
    unsigned long vm_start = *(unsigned long *)vma;
    unsigned long vm_end = *(unsigned long *)((uint64_t)vma + 8);
    int nr_pages = (int)((vm_end - vm_start) >> 12);

    struct wx_region *new_region = (struct wx_region *)kfn_kzalloc(
        sizeof(struct wx_region), wx_gfp_kernel);
    if (!new_region)
        return -12;

    struct page_info *pages;
    if (kfn_kcalloc)
        pages = (struct page_info *)kfn_kcalloc(
            nr_pages, sizeof(struct page_info), wx_gfp_kernel);
    else
        pages = (struct page_info *)kfn_kzalloc(
            (unsigned long)nr_pages * sizeof(struct page_info), wx_gfp_kernel);
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
    new_region->refcount = 2;  /* 1 for list + 1 for current use */

    wx_spin_lock();
    list_add(&new_region->list, &region_list);
    wx_spin_unlock();

    region = new_region;
    idx = wxjump_page_index(new_region, page_addr);
    pi = &pages[idx];

setup_shadow_on_existing:
    /* 设置 shadow page (region/idx/pi 已确定) */
    if (!vma) {
        vma = kfn_find_vma(mm, page_addr);
        if (!vma || page_addr < *(unsigned long *)vma) {
            pr_err("wxjump: patch: no VMA for %lx\n", page_addr);
            wxjump_put_region(region);
            return -1;
        }
    }

    uint64_t *pte = get_user_pte(mm, page_addr, NULL);
    if (!pte || !((*pte) & 1)) {
        pr_err("wxjump: patch: no PTE for %lx\n", page_addr);
        wxjump_put_region(region);
        return -14;
    }

    uint64_t pte_val = *pte;
    uint64_t orig_pfn = (pte_val >> 12) & PFN_MASK;
    pi->orig_pfn = orig_pfn;
    pi->orig_pte = pte_val;

    uint64_t orig_va = wx_pa_to_va(orig_pfn << 12);

    /* 分配 shadow page */
    unsigned long shadow_va = kfn___get_free_pages(wx_gfp_kernel, 0);
    if (!shadow_va) {
        wxjump_put_region(region);
        return -12;
    }

    uint64_t shadow_pa = wx_va_to_pa(shadow_va);
    uint64_t shadow_pfn = shadow_pa >> 12;

    pi->shadow_pfn = shadow_pfn;
    pi->shadow_page_va = shadow_va;

    /* 复制原始页内容 + 应用 patch */
    memcpy((void *)shadow_va, (void *)orig_va, WX_PAGE_SIZE);
    memcpy((void *)(shadow_va + offset), kbuf, len);

    pi->state = STATE_SHADOW_X;
    pi->patch_count = 1;

    /* 切换 PTE → shadow page (execute-only: 只有 PTE_BASE_FLAGS, 无 RO/UXN) */
    int ret = wxjump_switch_mapping(vma, page_addr, shadow_pfn, 0);
    if (ret) {
        pr_err("wxjump: patch: switch failed\n");
        kfn_free_pages(shadow_va, 0);
        pi->shadow_pfn = 0;
        pi->shadow_page_va = 0;
        pi->state = STATE_NONE;
        pi->orig_pfn = 0;
        pi->orig_pte = 0;
        wxjump_put_region(region);
        return ret;
    }

    wx_flush_icache(page_addr);

    pr_info("wxjump: patched new shadow at %lx+%zu (%zu bytes) orig_pfn=%lx shadow_pfn=%lx\n",
            page_addr, offset, len, (unsigned long)orig_pfn, (unsigned long)shadow_pfn);
    wxjump_put_region(region);
    return 0;
}

/* ========== RELEASE 处理 ========== */

/*
 * prctl(0x5759, page_addr, len, offset, 0)
 * 释放 shadow page 上 offset+len 处的 patch
 */
static int wxjump_do_release(void *mm, unsigned long page_addr,
                              size_t len, size_t offset)
{
    struct wx_region *region;
    struct page_info *pi;
    int idx;

    page_addr &= PAGE_MASK_4K;

    region = wxjump_find_region(mm, page_addr);
    if (!region)
        return -2;

    idx = wxjump_page_index(region, page_addr);
    if (idx < 0 || idx >= region->nr_pages) {
        wxjump_put_region(region);
        return -2;
    }

    pi = &region->pages[idx];
    if (!pi->shadow_page_va || pi->state == STATE_NONE) {
        wxjump_put_region(region);
        return -2;
    }

    /* 从 orig page 恢复对应 offset 的原始字节到 shadow page */
    if (pi->orig_pfn && offset + len <= WX_PAGE_SIZE) {
        uint64_t orig_va = wx_pa_to_va(pi->orig_pfn << 12);
        memcpy((void *)(pi->shadow_page_va + offset),
               (void *)(orig_va + offset), len);
    }

    wx_spin_lock();
    if (pi->patch_count > 0)
        pi->patch_count--;
    uint32_t count = pi->patch_count;
    wx_spin_unlock();

    pr_info("wxjump: release at %lx+%zu (%zu bytes), remaining=%d\n",
            page_addr, offset, len, count);

    if (count == 0) {
        /* 无剩余 patch → 恢复原始页并释放 shadow */
        void *vma = kfn_find_vma(mm, page_addr);
        if (vma && page_addr >= *(unsigned long *)vma && pi->state != STATE_NONE) {
            uint64_t shadow_va = pi->shadow_page_va;

            /* 精确恢复原始 PTE (保留 PXN 等所有原始标志) */
            {
                void *release_mm = NULL;
                if (wx_vma_vm_mm_offset >= 0 && vma)
                    release_mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);
                uint64_t *release_pte = get_user_pte(release_mm, page_addr, NULL);
                if (release_pte) {
                    *release_pte = pi->orig_pte;
                    wxjump_flush_tlb_page(vma, page_addr);
                }
            }
            wx_flush_icache(page_addr);  /* 清除 shadow 指令的 I-cache 残留 */

            wx_spin_lock();
            pi->shadow_page_va = 0;
            pi->orig_pfn = 0;
            pi->orig_pte = 0;
            pi->shadow_pfn = 0;
            pi->state = STATE_NONE;
            wx_spin_unlock();

            if (shadow_va)
                kfn_free_pages(shadow_va, 0);
            pr_info("wxjump: shadow page freed for %lx\n", page_addr);
        }
    } else if (pi->state == STATE_SHADOW_X) {
        /* 还有其他 patch，刷新 icache 使恢复的字节生效 */
        wx_flush_icache(page_addr);
    }

    wxjump_put_region(region);
    return 0;
}

/* ========== prctl hook ========== */

static void prctl_before(hook_fargs5_t *fargs, void *udata)
{
    uint64_t *args = syscall_args(fargs);
    uint64_t option = args[0];
    void *mm;
    int ret;

    if ((uint32_t)option == WXJUMP_PRCTL_PATCH) {
        unsigned long page_addr = (unsigned long)args[1];
        unsigned long user_buf  = (unsigned long)args[2];
        size_t len              = (size_t)args[3];
        size_t offset           = (size_t)args[4];

        mm = kfn_get_task_mm((void *)get_current());
        if (!mm) {
            fargs->skip_origin = 1;
            fargs->ret = -3;
            return;
        }

        ret = wxjump_do_patch(mm, page_addr, user_buf, len, offset);
        kfn_mmput(mm);
        fargs->skip_origin = 1;
        fargs->ret = ret;
        return;
    }

    if ((uint32_t)option == WXJUMP_PRCTL_RELEASE) {
        unsigned long page_addr = (unsigned long)args[1];
        size_t len              = (size_t)args[2];
        size_t offset           = (size_t)args[3];

        mm = kfn_get_task_mm((void *)get_current());
        if (!mm) {
            fargs->skip_origin = 1;
            fargs->ret = -3;
            return;
        }

        ret = wxjump_do_release(mm, page_addr, len, offset);
        kfn_mmput(mm);
        fargs->skip_origin = 1;
        fargs->ret = ret;
        return;
    }
}

/* ========== 进程退出清理 ========== */

static void exit_mmap_before(hook_fargs1_t *fargs, void *udata)
{
    void *mm = (void *)fargs->arg0;
    struct wx_region *regions[32];
    struct wx_region *r, *tmp;
    int count = 0;
    int i, j;

    if (!mm)
        return;

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

    pr_info("wxjump: [exit_mmap] mm=%px, restoring %d regions\n", mm, count);

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
                    if (pte && (*pte & 1) && pi->orig_pte) {
                        *pte = pi->orig_pte;
                        wxjump_flush_tlb_page(vma, addr);
                    }
                }
                if (shadow_va)
                    kfn_free_pages(shadow_va, 0);
            }
        }

        if (pages)
            kfn_kfree(pages);
        kfn_kfree(r);
    }
}

/* ========== 符号解析 ========== */

#define RESOLVE_REQUIRED(name, var) \
    var = (typeof(var))kallsyms_lookup_name(name); \
    if (!var) { pr_err("wxjump: symbol not found: %s\n", name); return -1; }

#define RESOLVE_OPTIONAL(name, var) \
    var = (typeof(var))kallsyms_lookup_name(name);

static int resolve_symbols(void)
{
    pr_info("wxjump: resolving symbols...\n");

    /* MM */
    RESOLVE_REQUIRED("find_vma", kfn_find_vma);
    RESOLVE_REQUIRED("get_task_mm", kfn_get_task_mm);
    RESOLVE_REQUIRED("mmput", kfn_mmput);

    kfn_exit_mmap = (void *)kallsyms_lookup_name("exit_mmap");
    if (kfn_exit_mmap)
        pr_info("wxjump: exit_mmap at %px\n", kfn_exit_mmap);
    else
        pr_warn("wxjump: exit_mmap not found\n");

    /* Page alloc/free */
    RESOLVE_REQUIRED("__get_free_pages", kfn___get_free_pages);
    RESOLVE_REQUIRED("free_pages", kfn_free_pages);

    /* Address translation */
    kvar_memstart_addr = (uint64_t *)kallsyms_lookup_name("memstart_addr");
    if (!kvar_memstart_addr) {
        pr_err("wxjump: memstart_addr not found\n");
        return -1;
    }

    kvar_physvirt_offset = (uint64_t *)kallsyms_lookup_name("physvirt_offset");

    uint64_t tcr = 0;
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr));
    int t1sz = (tcr >> 16) & 0x3F;
    wx_page_offset_base = -1ULL << (64 - t1sz);

    /* physvirt_offset 检测 */
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
            }
        }
        kfn_free_pages(test_page, 0);
    }

    /* Page table config (TG1: bits[31:30], 内核空间页粒度) */
    uint64_t tg1 = (tcr >> 30) & 3;
    int bits_per_level;
    if (tg1 == 1) {
        wx_page_shift = 14; bits_per_level = 11;  /* TG1=0b01 → 16KB */
    } else if (tg1 == 3) {
        wx_page_shift = 16; bits_per_level = 13;  /* TG1=0b11 → 64KB */
    } else {
        wx_page_shift = 12; bits_per_level = 9;   /* TG1=0b10 → 4KB */
    }
    wx_page_level = (60 - t1sz) / bits_per_level;
    pr_info("wxjump: page_shift=%d page_level=%d\n", wx_page_shift, wx_page_level);

    /* Spinlock */
    RESOLVE_REQUIRED("_raw_spin_lock", kfn__raw_spin_lock);
    RESOLVE_REQUIRED("_raw_spin_unlock", kfn__raw_spin_unlock);

    /* TLB flush */
    kfn_flush_tlb_page = (void *)kallsyms_lookup_name("flush_tlb_page");
    if (!kfn_flush_tlb_page) {
        kfn___flush_tlb_range = (typeof(kfn___flush_tlb_range))kallsyms_lookup_name("__flush_tlb_range");
        if (!kfn___flush_tlb_range)
            pr_warn("wxjump: no TLB flush function, will use TLBI instruction\n");
    }

    /* Cache (flush_dcache_page 不再需要，已用 dc cvau 内联汇编替代) */
    kfn___flush_icache_range = (typeof(kfn___flush_icache_range))kallsyms_lookup_name("__flush_icache_range");
    if (!kfn___flush_icache_range)
        kfn___flush_icache_range = (typeof(kfn___flush_icache_range))kallsyms_lookup_name("flush_icache_range");
    if (!kfn___flush_icache_range)
        kfn___flush_icache_range = (typeof(kfn___flush_icache_range))kallsyms_lookup_name("__flush_cache_user_range");

    /* Memory alloc */
    kfn_kzalloc = (typeof(kfn_kzalloc))kallsyms_lookup_name("kzalloc");
    if (!kfn_kzalloc)
        kfn_kzalloc = (typeof(kfn_kzalloc))kallsyms_lookup_name("__kmalloc");
    if (!kfn_kzalloc) {
        pr_err("wxjump: kzalloc not found\n");
        return -1;
    }

    kfn_kcalloc = (typeof(kfn_kcalloc))kallsyms_lookup_name("kcalloc");
    if (!kfn_kcalloc)
        kfn_kcalloc = (typeof(kfn_kcalloc))kallsyms_lookup_name("kmalloc_array");

    RESOLVE_REQUIRED("kfree", kfn_kfree);

    kfn_copy_from_kernel_nofault = (typeof(kfn_copy_from_kernel_nofault))
        kallsyms_lookup_name("copy_from_kernel_nofault");
    if (!kfn_copy_from_kernel_nofault)
        kfn_copy_from_kernel_nofault = (typeof(kfn_copy_from_kernel_nofault))
            kallsyms_lookup_name("probe_kernel_read");

    /* Page fault handler */
    kfn_do_page_fault = (void *)kallsyms_lookup_name("do_page_fault");
    if (!kfn_do_page_fault)
        kfn_do_page_fault = (void *)kallsyms_lookup_name("__do_page_fault");
    if (!kfn_do_page_fault)
        kfn_do_page_fault = (void *)kallsyms_lookup_name("do_mem_abort");
    if (kfn_do_page_fault)
        pr_info("wxjump: page fault handler at %px\n", kfn_do_page_fault);
    else
        pr_warn("wxjump: page fault handler not found, CRC hiding disabled\n");

    pr_info("wxjump: symbols resolved\n");
    return 0;
}

/* ========== VMA offset 扫描 ========== */

static int scan_vma_offsets(void)
{
    void *mm, *vma;
    uint64_t vma_addr;
    int offset;

    mm = kfn_get_task_mm((void *)get_current());
    if (!mm) {
        wx_vma_vm_mm_offset = 64;
        return 0;
    }

    if (kfn_copy_from_kernel_nofault) {
        if (kfn_copy_from_kernel_nofault(&vma_addr, mm, 8)) {
            kfn_mmput(mm);
            wx_vma_vm_mm_offset = 64;
            return 0;
        }
    } else {
        vma_addr = *(uint64_t *)mm;
    }

    if (!vma_addr) {
        kfn_mmput(mm);
        wx_vma_vm_mm_offset = 64;
        return 0;
    }

    vma = (void *)vma_addr;
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
            pr_info("wxjump: vm_mm offset: 0x%x\n", offset);
            kfn_mmput(mm);
            return 0;
        }
    }

    kfn_mmput(mm);
    wx_vma_vm_mm_offset = 64;
    pr_info("wxjump: using default vm_mm offset: 0x40\n");
    return 0;
}

/* ========== context.id 扫描 ========== */

static int try_scan_context_id(void)
{
    void *task = (void *)get_current();
    uint64_t mm_val;
    uint64_t ttbr0, asid;
    int offset;

    if (task_struct_offset.mm_offset <= 0)
        return -1;

    uint64_t mm_ptr_addr = (uint64_t)task + task_struct_offset.mm_offset;
    if (kfn_copy_from_kernel_nofault) {
        if (kfn_copy_from_kernel_nofault(&mm_val, (void *)mm_ptr_addr, 8))
            return -1;
    } else {
        mm_val = *(uint64_t *)mm_ptr_addr;
    }
    if (!mm_val)
        return -1;

    asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0));
    asid = ttbr0 >> 48;

    for (offset = 256; offset < 1024; offset += 8) {
        uint64_t val;
        if (kfn_copy_from_kernel_nofault) {
            if (kfn_copy_from_kernel_nofault(&val, (void *)(mm_val + offset), 8))
                continue;
        } else {
            val = *(uint64_t *)(mm_val + offset);
        }

        if ((val & 0xFFFF) == asid) {
            wx_mm_context_id_offset = offset;
            wx_mm_context_id_asid_shift = 0;
            pr_info("wxjump: context.id at offset 0x%x (asid@bits[0:15])\n", offset);
            return 0;
        }
        if (((val >> 48) & 0xFFFF) == asid && asid != 0) {
            wx_mm_context_id_offset = offset;
            wx_mm_context_id_asid_shift = 48;
            pr_info("wxjump: context.id at offset 0x%x (asid@bits[48:63], MTK)\n", offset);
            return 0;
        }
    }
    return -1;
}

/* ========== mm_offset 推断 ========== */

static int detect_mm_offset(void)
{
    void *init_task = (void *)kallsyms_lookup_name("init_task");
    if (!init_task)
        return -1;

    if (task_struct_offset.mm_offset > 0)
        return 0;

    if (task_struct_offset.active_mm_offset > 0) {
        int candidate = task_struct_offset.active_mm_offset - 8;
        uint64_t mm_val = 0, active_mm_val = 0;

        if (kfn_copy_from_kernel_nofault) {
            kfn_copy_from_kernel_nofault(&mm_val, (void *)((uint64_t)init_task + candidate), 8);
            kfn_copy_from_kernel_nofault(&active_mm_val, (void *)((uint64_t)init_task + task_struct_offset.active_mm_offset), 8);
        } else {
            mm_val = *(uint64_t *)((uint64_t)init_task + candidate);
            active_mm_val = *(uint64_t *)((uint64_t)init_task + task_struct_offset.active_mm_offset);
        }

        if (mm_val == 0 && (active_mm_val >> 48) == 0xFFFF) {
            task_struct_offset.mm_offset = candidate;
            pr_info("wxjump: mm_offset=0x%x (active_mm-8)\n", candidate);
        } else {
            task_struct_offset.mm_offset = candidate;
            pr_warn("wxjump: mm_offset=0x%x (forced)\n", candidate);
        }
    }
    return 0;
}

/* ========== 模块初始化 ========== */

static long wxjump_init(const char *args, const char *event, void *reserved)
{
    int ret;

    pr_info("wxjump: initializing (args: %s)\n", args ? args : "(null)");

    INIT_LIST_HEAD(&region_list);
    global_lock = 0;

    /* 符号解析 */
    ret = resolve_symbols();
    if (ret) {
        pr_err("wxjump: symbol resolution failed\n");
        return ret;
    }

    /* mm_struct pgd offset 检查 */
    if (mm_struct_offset.pgd_offset < 0) {
        pr_err("wxjump: pgd_offset not detected by KP framework\n");
        return -1;
    }

    /* 偏移量扫描 */
    scan_vma_offsets();
    detect_mm_offset();
    try_scan_context_id();

    /* Hook page fault (W^X CRC 防护) */
    if (kfn_do_page_fault) {
        ret = hook_wrap(kfn_do_page_fault, 3, do_page_fault_before, NULL, NULL);
        if (ret) {
            pr_err("wxjump: failed to hook page fault: %d\n", ret);
            return -1;
        }
        pr_info("wxjump: hooked page fault handler\n");
    }

    /* Hook exit_mmap (进程退出清理) */
    if (kfn_exit_mmap) {
        ret = hook_wrap(kfn_exit_mmap, 1, exit_mmap_before, NULL, NULL);
        if (ret) {
            pr_err("wxjump: failed to hook exit_mmap: %d\n", ret);
            if (kfn_do_page_fault)
                hook_unwrap_remove(kfn_do_page_fault, do_page_fault_before, NULL, 1);
            return -1;
        }
        pr_info("wxjump: hooked exit_mmap\n");
    }

    /* Hook prctl (PATCH/RELEASE 接口) */
    ret = hook_syscalln(PRCTL_NR, 5, prctl_before, NULL, NULL);
    if (ret) {
        pr_err("wxjump: failed to hook prctl: %d\n", ret);
        if (kfn_do_page_fault)
            hook_unwrap_remove(kfn_do_page_fault, do_page_fault_before, NULL, 1);
        if (kfn_exit_mmap)
            hook_unwrap_remove(kfn_exit_mmap, exit_mmap_before, NULL, 1);
        return -1;
    }
    pr_info("wxjump: hooked prctl\n");

    pr_info("wxjump: module loaded successfully\n");
    return 0;
}

/* ========== 模块卸载 ========== */

static long wxjump_exit(void *reserved)
{
    struct wx_region *r, *tmp;
    struct wx_region **cleanup_list;
    int count = 0, i, j;

    pr_info("wxjump: unloading...\n");

    /* Unhook */
    unhook_syscalln(PRCTL_NR, prctl_before, NULL);
    if (kfn_exit_mmap)
        hook_unwrap_remove(kfn_exit_mmap, exit_mmap_before, NULL, 1);
    if (kfn_do_page_fault)
        hook_unwrap_remove(kfn_do_page_fault, do_page_fault_before, NULL, 1);

    /* 收集所有 regions */
    wx_spin_lock();
    list_for_each_entry(r, &region_list, list) {
        count++;
    }
    wx_spin_unlock();

    if (!count) {
        pr_info("wxjump: no active regions, unload complete\n");
        return 0;
    }

    cleanup_list = (struct wx_region **)kfn_kzalloc(
        count * sizeof(struct wx_region *), wx_gfp_kernel);
    if (!cleanup_list) {
        pr_err("wxjump: exit: cleanup alloc failed, leaking %d regions\n", count);
        return -12;
    }

    wx_spin_lock();
    i = 0;
    list_for_each_entry_safe(r, tmp, &region_list, list) {
        if (i < count) {
            list_del_init(&r->list);
            cleanup_list[i++] = r;
        }
    }
    wx_spin_unlock();

    /* 恢复所有映射并释放资源 */
    for (i = 0; i < count; i++) {
        r = cleanup_list[i];
        if (!r)
            continue;

        void *mm = r->mm;
        void *vma = NULL;
        if (mm && kfn_find_vma) {
            vma = kfn_find_vma(mm, r->vm_start);
            if (vma) {
                if (*(unsigned long *)vma > r->vm_start ||
                    *(unsigned long *)((uint64_t)vma + 8) < r->vm_end)
                    vma = NULL;
            }
        }

        for (j = 0; j < r->nr_pages; j++) {
            struct page_info *pi = &r->pages[j];
            if (!pi->state)
                continue;

            unsigned long addr = r->vm_start + ((unsigned long)j << 12);

            if (vma && pi->orig_pte &&
                (pi->state == STATE_SHADOW_X || pi->state == STATE_ORIG_R)) {
                void *exit_mm = NULL;
                if (wx_vma_vm_mm_offset >= 0 && vma)
                    exit_mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);
                uint64_t *exit_pte = get_user_pte(exit_mm, addr, NULL);
                if (exit_pte) {
                    *exit_pte = pi->orig_pte;
                    wxjump_flush_tlb_page(vma, addr);
                    wx_flush_icache(addr);
                }
            }

            if (pi->shadow_page_va) {
                kfn_free_pages(pi->shadow_page_va, 0);
                pi->shadow_page_va = 0;
            }
            pi->state = STATE_NONE;
        }

        kfn_kfree(r->pages);
        kfn_kfree(r);
    }

    if (cleanup_list)
        kfn_kfree(cleanup_list);

    pr_info("wxjump: unloaded (%d regions cleaned)\n", count);
    return 0;
}

static long wxjump_control(const char *args, char *__user out_msg, int outlen)
{
    pr_info("wxjump: control: %s\n", args ? args : "(null)");
    pr_info("wxjump: stats: read_faults=%lu exec_faults=%lu el1_skips=%lu\n",
            wx_fault_read_count, wx_fault_exec_count, wx_fault_el1_skip_count);
    return 0;
}

KPM_INIT(wxjump_init);
KPM_CTL0(wxjump_control);
KPM_EXIT(wxjump_exit);
