先审阅 `fix_plan.md` 的具体结论，再对照 `wxjump` 里的实现判断哪些判断成立、哪些过度推断。


**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_plan.md**
```
# wxjump CRC 1000+ 次后黑屏重启 — 修复方案

## 一、问题根因

### 1.1 BBM (Break-Before-Make) 竞态

ARM64 架构要求：当修改 PTE 的输出地址 (OA) 时，必须先将 PTE 置为无效，执行 TLBI 刷新所有核心 TLB，然后才能写入新 PTE。这就是 Break-Before-Make (BBM)。

wxjump 的状态切换必须改变 OA（orig_pfn ↔ shadow_pfn），因此每次切换都要执行 BBM：

```c
// wxjump.c:377-382
*pte = 0;                          // Break: PTE 置零 ~0ns
wxjump_flush_tlb_page(vma, addr);  // TLBI + DSB ~100-500ns
*pte = make_pte(pfn, extra_flags); // Make: 写入新 PTE
```

**竞态窗口**：在 `*pte = 0` 到 `*pte = make_pte(...)` 的 100-500ns 内，其他 CPU 访问该地址会触发 **Translation Fault**（DFSC = 0x04/0x05/0x06/0x07），而非 Permission Fault（DFSC = 0x0C）。

当前 handler 只处理 Permission Fault：
```c
// wxjump.c:716
if ((esr & 0x3C) != 0x0C) return;  // 过滤掉 Translation Fault → 交给内核
```
内核对 Translation Fault 的默认处理 → 发送 SIGSEGV → 目标进程崩溃 → 黑屏。

### 1.2 累积概率

每次 CRC 循环产生 2 次 BBM（读切换 + 执行恢复切换），假设竞态命中概率 p ≈ 0.001（每次 BBM ~500ns 窗口，目标进程线程在执行），那么：

- 1000 次 CRC × 2 BBM × N 个 hook 页 = 2000N 次竞态窗口
- 1 - (1-p)^2000 ≈ 86.5%（N=1 时）

**1000+ 次 CRC 后黑屏几乎是必然的。**

### 1.3 全 SO CRC 的状态雪崩

CRC demo 的 `func_monitor_check()` 会扫描整个 SO r-xp 区域：
```cpp
// native-lib.cpp:189
uint32_t so_crc_now = crc32_compute((const void *)e->so_rx_start, so_size);
```

这会将所有被 hook 的页面**同时**切到 `ORIG_R`，目标线程恢复时产生 fault 风暴，大量 BBM 并发执行，竞态概率暴增。

---

## 二、修复方案

### 方案 1：处理 Translation Fault 重试（核心修复，优先级 P0）

**原理**：BBM 期间的 Translation Fault 是瞬态的（BBM 完成后 PTE 恢复有效）。只需让目标线程重试指令，BBM 完成后就能正常执行。

**实现**：

#### 1a. 添加 per-page 转换标志

```c
// page_info 新增字段
struct page_info {
    // ... 现有字段 ...
    volatile int transitioning;  /* 1 = BBM 进行中 */
};
```

#### 1b. wxjump_switch_mapping 设置标志

```c
static int wxjump_switch_mapping(void *vma, unsigned long addr,
                                  uint64_t pfn, uint64_t extra_flags,
                                  struct page_info *pi)  // 新增参数
{
    void *mm = NULL;
    uint64_t *pte;

    if (wx_vma_vm_mm_offset >= 0 && vma)
        mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);

    pte = get_user_pte(mm, addr, NULL);
    if (!pte)
        return -1;

    if (*pte & 1) {
        if (pi) pi->transitioning = 1;  // 标记 BBM 开始
        asm volatile("dmb ish" ::: "memory");  // 确保标志对其他 CPU 可见

        *pte = 0;
        wxjump_flush_tlb_page(vma, addr);
    }

    *pte = make_pte(pfn, extra_flags);

    if (pi) {
        asm volatile("dmb ish" ::: "memory");
        pi->transitioning = 0;  // 标记 BBM 完成
    }
    return 0;
}
```

#### 1c. do_page_fault_before 增加 Translation Fault 重试逻辑

在现有 Permission Fault 过滤**之前**，添加 Translation Fault 处理：

```c
static void do_page_fault_before(hook_fargs3_t *fargs, void *udata)
{
    unsigned long far = fargs->arg0;
    unsigned long esr = fargs->arg1;
    void *task = (void *)get_current();
    void *mm;

    int cpu = wx_cpu_id();
    if (wx_in_fault_handler[cpu]) {
        wx_fault_reentry_count++;
        return;
    }
    wx_in_fault_handler[cpu] = 1;

    mm = kfn_get_task_mm(task);
    if (!mm) {
        wx_in_fault_handler[cpu] = 0;
        return;
    }

    unsigned long dfsc_class = esr & 0x3C;  // DFSC[5:2]

    /* ===== 新增：Translation Fault 重试 ===== */
    if (dfsc_class != 0x0C) {
        // 不是 Permission Fault — 检查是否为 BBM 竞态导致的 Translation Fault
        // Translation Fault: DFSC[5:2] = 0b0001 (0x04) 至 0b0011 之前
        // 即 dfsc_class = 0x04, 0x08 (level 1/2/3 translation)
        unsigned long dfsc = esr & 0x3F;
        if (dfsc >= 0x04 && dfsc <= 0x07) {
            // Level 0-3 Translation Fault
            struct wx_region *region = wxjump_find_region(mm, far);
            if (region) {
                unsigned long page_addr = far & PAGE_MASK_4K;
                int idx = wxjump_page_index(region, page_addr);
                if (idx >= 0 && idx < region->nr_pages) {
                    struct page_info *pi = &region->pages[idx];
                    if (pi->transitioning) {
                        // BBM 进行中 — 重试指令
                        wxjump_put_region(region);
                        kfn_mmput(mm);
                        wx_in_fault_handler[cpu] = 0;
                        fargs->skip_origin = 1;
                        fargs->ret = 0;
                        return;
                    }
                }
                wxjump_put_region(region);
            }
        }
        // 非 BBM 导致的 Translation Fault → 交给内核处理
        kfn_mmput(mm);
        wx_in_fault_handler[cpu] = 0;
        return;
    }

    /* ===== 以下为现有的 Permission Fault 处理逻辑（不变）===== */
    struct wx_region *region = wxjump_find_region(mm, far);
    // ... 原有代码 ...
}
```

**安全性分析**：
- 如果 `transitioning=1`（BBM 进行中），重试指令。由于 fault → handler → return 的路径耗时 ~微秒级，远大于 BBM 窗口 ~500ns，重试时 BBM 已完成。
- 如果 `transitioning=0`，说明是内核 page reclaim 等正常 Translation Fault → 交给内核处理。
- 最坏情况：重试后 BBM 仍未完成 → 再次 Translation Fault → 再次重试，有限循环（BBM 时间有界）。

---

### 方案 2：CRC 端去除全 SO 扫描（优先级 P1）

全 SO CRC 扫描是最危险的操作，会同时触发所有 hook 页的状态切换。

**修改 native-lib.cpp**：移除或改为抽样扫描

```cpp
static std::string func_monitor_check() {
    // ...
    for (int i = 0; i < g_func_count; i++) {
        FuncCrcEntry *e = &g_func_monitors[i];
        if (!e->valid) { /* ... */ continue; }

        /* 保留：函数头 128 字节 CRC（只影响 1 个页面） */
        uint32_t func_crc_now = crc32_compute((const void *)e->addr, FUNC_CRC_SIZE);
        bool func_ok = (func_crc_now == e->func_crc_baseline);

        /* 移除或降频：全 SO 扫描
         * 原代码: so_crc_now = crc32_compute((const void *)e->so_rx_start, so_size);
         * 改为: 每 100 次 check 才做一次全 SO 扫描 */
        static int so_scan_counter = 0;
        uint32_t so_crc_now = e->so_crc_baseline;  // 默认通过
        bool so_ok = true;
        if (++so_scan_counter >= 100) {
            so_scan_counter = 0;
            size_t so_size = e->so_rx_end - e->so_rx_start;
            so_crc_now = crc32_compute((const void *)e->so_rx_start, so_size);
            so_ok = (so_crc_now == e->so_crc_baseline);
        }
        // ...
    }
}
```

---

### 方案 3：缩短 BBM 窗口（优先级 P2）

优化 `wxjump_switch_mapping()`，减小竞态窗口：

```c
static int wxjump_switch_mapping(void *vma, unsigned long addr,
                                  uint64_t pfn, uint64_t extra_flags,
                                  struct page_info *pi)
{
    // ... 获取 pte ...

    uint64_t new_pte = make_pte(pfn, extra_flags);

    if (*pte & 1) {
        if (pi) pi->transitioning = 1;
        asm volatile("dmb ish" ::: "memory");

        *pte = 0;

        /* 优化：内联 TLBI，避免函数调用开销 */
        uint64_t tlbi_val = addr >> 12;
        /* 如果有 ASID，使用精确 TLBI（只刷该进程） */
        if (vma && wx_vma_vm_mm_offset >= 0) {
            void *mm_ptr = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);
            if (mm_ptr && wx_mm_context_id_offset >= 0) {
                uint64_t ctx_val = *(uint64_t *)((uint64_t)mm_ptr + wx_mm_context_id_offset);
                uint16_t asid = (ctx_val >> wx_mm_context_id_asid_shift) & 0xFFFF;
                if (asid) {
                    uint64_t val = tlbi_val | ((uint64_t)asid << 48);
                    asm volatile(
                        "tlbi vale1is, %0\n"
                        "dsb ish\n"
                        : : "r"(val) : "memory"
                    );
                    goto write_new;
                }
            }
        }
        asm volatile(
            "tlbi vaale1is, %0\n"
            "dsb ish\n"
            : : "r"(tlbi_val) : "memory"
        );
    }

write_new:
    *pte = new_pte;                 /* 尽快写入新 PTE */
    asm volatile("dsb ishst" ::: "memory");  /* 确保 PTE store 对 page walker 可见 */

    if (pi) {
        pi->transitioning = 0;
        asm volatile("dmb ish" ::: "memory");
    }
    return 0;
}
```

关键优化点：
- 内联 TLBI 操作，消除 `wxjump_flush_tlb_page()` 的函数调用开销
- `*pte = new_pte` 后立即 `dsb ishst` 确保 page walker 可见
- 去掉 TLBI 后多余的 `isb()`（fault handler 路径不需要 isb）

---

### 方案 4：添加 fault 统计与自动熔断（优先级 P3）

当 fault 频率异常高时自动降级（临时禁用状态切换），避免系统崩溃：

```c
/* 滑动窗口 fault 频率统计 */
#define WX_FAULT_WINDOW_NS     1000000000ULL  /* 1 秒 */
#define WX_FAULT_THRESHOLD     5000           /* 1 秒内超过 5000 次 fault 则熔断 */

static volatile uint64_t wx_fault_window_start;
static volatile unsigned long wx_fault_window_count;
static volatile int wx_circuit_breaker;  /* 1 = 熔断中 */

/* 在 do_page_fault_before 入口处检查 */
static inline int wx_check_circuit_breaker(void)
{
    if (wx_circuit_breaker)
        return 1;  /* 熔断中，跳过所有处理 */

    wx_fault_window_count++;
    /* 简化：每 1024 次 fault 检查一次时间（减少 timer 开销） */
    if ((wx_fault_window_count & 0x3FF) == 0) {
        if (wx_fault_window_count > WX_FAULT_THRESHOLD) {
            wx_circuit_breaker = 1;
            pr_err("wxjump: CIRCUIT BREAKER - %lu faults in window, suspending\n",
                   wx_fault_window_count);
        }
    }
    return 0;
}
```

---

## 三、修改文件清单

| 文件 | 修改内容 |
|------|---------|
| `wxjump.c` | 方案 1（Translation Fault 重试）+ 方案 3（BBM 窗口优化）+ 方案 4（熔断） |
| `native-lib.cpp` | 方案 2（去除/降频全 SO CRC 扫描） |

## 四、测试计划

1. **基础验证**：安装 hook 后执行 CRC 检查 10000 次，确认无黑屏
2. **竞态压测**：多线程并发 CRC（4 线程同时扫描），确认 Translation Fault 重试正常
3. **功能验证**：确认 CRC 检查始终返回原始字节（hook 不被检测到）
4. **性能测试**：对比修复前后单次 CRC 耗时，评估重试开销
5. **熔断测试**：高频 CRC 触发熔断后，确认系统稳定、hook 自动恢复

## 五、风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| Translation Fault 重试误判（将 page reclaim 的 fault 重试） | 低 | 中 | transitioning 标志 + 重试次数上限 |
| BBM 窗口优化后 TLB 一致性问题 | 极低 | 高 | 保留 dsb ish 屏障，严格遵循 ARM ARM |
| 熔断后 hook 失效 | 中 | 低 | 熔断只暂停状态切换，hook 仍在 shadow page 上 |

```



接下来会核对 `fix_plan.md` 里引用的关键点：`wxjump.c` 的换页流程、fault handler、以及 CRC demo 的扫描方式，避免只按文档表述下判断。
UpdateTodos: 4 items


**Grep Results:**

- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump-tutorial.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_plan.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug--audit-glm5-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-oz-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_ec.py**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v3.md**




**File Glob Results:**

- **C:\Users\24151\Documents\GitHub\无痕hook\unixSocketDemo\app\src\main\cpp\native-lib.cpp**


已经定位到关键实现，下一步直接看 `wxjump.c` 里的真实流程，确认文档引用是否和代码一致。


**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
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

static int   (*kfn_down_read_trylock)(void *sem);
static void  (*kfn_down_read)(void *sem);
static void  (*kfn_up_read)(void *sem);

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
static int wx_mmap_lock_offset = -1;

/* 全局状态 */
static LIST_HEAD(region_list);

/* Fault counters (debug, no locking needed for approximate counts) */
static unsigned long wx_fault_read_count;
static unsigned long wx_fault_exec_count;
static unsigned long wx_fault_el1_skip_count;
static unsigned long wx_fault_total_count;  /* 所有进入 handler 的 fault */
static unsigned long wx_fault_reentry_count; /* 重入拦截计数 */
static uint64_t global_lock;

/* ========== Per-CPU 重入保护 ========== */
/*
 * 防止同一 CPU 嵌套进入 fault handler 导致 spinlock 死锁。
 * 场景: handler 持锁期间访问内存 → 触发嵌套 fault → 再次进入 handler → 死锁。
 * 用 MPIDR_EL1 推算 CPU 索引，无需内核符号依赖。
 */
#define WX_MAX_CPUS 16
static volatile int wx_in_fault_handler[WX_MAX_CPUS];

static inline int wx_cpu_id(void)
{
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    /* Aff1 (cluster) * 4 + Aff0 (core) — 适用于 MTK DynamIQ 大小核布局 */
    int id = (int)(((mpidr >> 8) & 0xFF) * 4 + (mpidr & 0xFF));
    if ((unsigned)id >= WX_MAX_CPUS)
        id &= (WX_MAX_CPUS - 1);
    return id;
}

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
```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
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

    /* ARM64 Break-Before-Make (BBM):
     * When changing the output address of a valid PTE, we MUST invalidate
     * the old entry before writing the new one. Direct overwrite causes
     * TLB conflict — undefined behavior per ARM ARM D5.10.1.
     * Sequence: write invalid → TLBI+DSB → write new valid */
    if (*pte & 1) {
        *pte = 0;                          /* Break: invalidate old entry */
        wxjump_flush_tlb_page(vma, addr);  /* TLBI + DSB across all cores */
    }

    *pte = make_pte(pfn, extra_flags);     /* Make: install new mapping */
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

/* P0-2 fix: accept pfn values directly instead of reading from pi after unlock.
 * Callers snapshot orig_pfn / shadow_pfn inside spinlock, then pass the copies. */
static int wxjump_validate_mapping(void *mm, void *vma,
                                    uint64_t orig_pfn, uint64_t shadow_pfn,
                                    unsigned long addr)
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

    if (shadow_pfn && shadow_pfn == current_pfn)
        return 1;
    if (orig_pfn && orig_pfn == current_pfn)
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
```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
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

        wx_mmap_read_lock(mm);
        ret = wxjump_do_patch(mm, page_addr, user_buf, len, offset);
        wx_mmap_read_unlock(mm);
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

        wx_mmap_read_lock(mm);
        ret = wxjump_do_release(mm, page_addr, len, offset);
        wx_mmap_read_unlock(mm);
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
    int count, total = 0;
    int i, j;

    if (!mm)
        return;

    /* 循环收集，每轮最多 32 个，直到该 mm 的所有 region 全部处理 */
    for (;;) {
        count = 0;
        wx_spin_lock();
        list_for_each_entry_safe(r, tmp, &region_list, list) {
            if (r->mm == mm && count < 32) {
                list_del_init(&r->list);
                regions[count++] = r;
            }
        }
        wx_spin_unlock();

        if (!count)
            break;
        total += count;

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

    if (total)
        pr_info("wxjump: [exit_mmap] mm=%px, cleaned %d regions\n", mm, total);
}

/* ========== 符号解析 ========== */

#define RESOLVE_REQUIRED(name, var) \
    var = (typeof(var))kallsyms_lookup_name(name); \
    if (!var) { pr_err("wxjump: symbol not found: %s\n", name); return -1; }

#define RESOLVE_OPTIONAL(name, var) \
    var = (typeof(var))kallsyms_lookup_name(name);

static int resolve_symbols(void)
{
    pr_info("wxjump: ====== resolve_symbols START ======\n");

    /* MM */
    RESOLVE_REQUIRED("find_vma", kfn_find_vma);
    pr_info("wxjump:   find_vma         = %px\n", kfn_find_vma);
    RESOLVE_REQUIRED("get_task_mm", kfn_get_task_mm);
    pr_info("wxjump:   get_task_mm      = %px\n", kfn_get_task_mm);
    RESOLVE_REQUIRED("mmput", kfn_mmput);
    pr_info("wxjump:   mmput            = %px\n", kfn_mmput);

    kfn_exit_mmap = (void *)kallsyms_lookup_name("exit_mmap");
    if (kfn_exit_mmap)
        pr_info("wxjump: exit_mmap at %px\n", kfn_exit_mmap);
    else
        pr_warn("wxjump: exit_mmap not found\n");

    /* Page alloc/free */
    RESOLVE_REQUIRED("__get_free_pages", kfn___get_free_pages);
    RESOLVE_REQUIRED("free_pages", kfn_free_pages);
    pr_info("wxjump:   __get_free_pages = %px\n", kfn___get_free_pages);
    pr_info("wxjump:   free_pages       = %px\n", kfn_free_pages);

    /* Address translation */
    kvar_memstart_addr = (uint64_t *)kallsyms_lookup_name("memstart_addr");
    if (!kvar_memstart_addr) {
        pr_err("wxjump: memstart_addr not found\n");
        return -1;
    }

    kvar_physvirt_offset = (uint64_t *)kallsyms_lookup_name("physvirt_offset");
    pr_info("wxjump:   memstart_addr    = %px (val=0x%llx)\n", kvar_memstart_addr, *kvar_memstart_addr);
    pr_info("wxjump:   physvirt_offset  = %px\n", kvar_physvirt_offset);

    uint64_t tcr = 0;
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr));
    int t1sz = (tcr >> 16) & 0x3F;
    wx_page_offset_base = -1ULL << (64 - t1sz);
    pr_info("wxjump:   TCR_EL1=0x%llx T1SZ=%d PAGE_OFFSET=0x%llx\n", tcr, t1sz, wx_page_offset_base);

    /* physvirt_offset 检测 */
    unsigned long test_page = kfn___get_free_pages(wx_gfp_kernel, 0);
    pr_info("wxjump:   test_page alloc  = 0x%lx\n", test_page);
    if (test_page) {
        uint64_t par;
        asm volatile("at s1e1r, %0" :: "r"(test_page));
        isb();
        asm volatile("mrs %0, par_el1" : "=r"(par));
        pr_info("wxjump:   PAR_EL1=0x%llx (fault=%d)\n", par, (int)(par & 1));
        if (!(par & 1)) {
            uint64_t pa = (par & PA_MASK) | (test_page & 0xFFF);
            if (pa) {
                detected_physvirt_offset = test_page - pa;
                physvirt_offset_valid = 1;
                pr_info("wxjump:   physvirt_offset  = 0x%llx (detected)\n", detected_physvirt_offset);
            }
        }
        kfn_free_pages(test_page, 0);
    } else {
        pr_err("wxjump:   test_page alloc FAILED\n");
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
    pr_info("wxjump:   _raw_spin_lock   = %px\n", kfn__raw_spin_lock);
    pr_info("wxjump:   _raw_spin_unlock = %px\n", kfn__raw_spin_unlock);

```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
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

    pr_info("wxjump: ====== INIT OK ======\n");
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
    pr_info("wxjump: stats: total=%lu read=%lu exec=%lu el1_skip=%lu reentry=%lu\n",
            wx_fault_total_count, wx_fault_read_count, wx_fault_exec_count,
            wx_fault_el1_skip_count, wx_fault_reentry_count);
    return 0;
}

KPM_INIT(wxjump_init);
KPM_CTL0(wxjump_control);
KPM_EXIT(wxjump_exit);
```

**C:\Users\24151\Documents\GitHub\无痕hook\unixSocketDemo\app\src\main\cpp\native-lib.cpp**
```

    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start, end;
        char perms[8] = {};
        char path[256] = {};
        int n = sscanf(line, "%lx-%lx %4s %*x %*x:%*x %*d %255[^\n]",
                       &start, &end, perms, path);
        if (n < 3) continue;
        if (strncmp(perms, "r-xp", 4) != 0) continue;

        char *p = path;
        while (*p == ' ' || *p == '\t') p++;
        if (strlen(so_path_out) > 0 && strcmp(p, so_path_out) == 0) {
            if (start < *rx_start) *rx_start = start;
            if (end > *rx_end)     *rx_end = end;
        }
    }
    fclose(fp);
    return true;
}

static bool compute_so_text_crc32(const char *so_name, SoTextInfo *out) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return false;

    char line[512];
    uintptr_t rx_start = 0, rx_end = 0;
    bool found_rx = false;

    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, so_name)) continue;
        if (!strstr(line, "r-xp")) continue;

        uintptr_t start, end;
        sscanf(line, "%lx-%lx", &start, &end);
        if (!found_rx) {
            rx_start = start;
            rx_end = end;
            found_rx = true;
        } else {
            if (start == rx_end) rx_end = end;
        }
    }
    fclose(fp);

    if (!found_rx || rx_end <= rx_start) return false;

    size_t sz = rx_end - rx_start;
    out->base = rx_start;
    out->text_addr = rx_start;
    out->text_size = sz;
    out->crc32 = crc32_compute((const void *)rx_start, sz);
    return true;
}

/* ========== Function-Level CRC32 ========== */

/*
 * Hook frameworks (Frida, XiaM, etc.) typically patch the first few
 * instructions of a function with a branch/trampoline. We CRC32 the
 * first FUNC_CRC_SIZE bytes of each monitored function to detect this.
 */
#define FUNC_CRC_SIZE       128   /* 32 instructions, covers any prologue hook */
#define MAX_FUNC_MONITORS   16

struct FuncCrcEntry {
    char        name[64];           /* symbol name */
    uintptr_t   addr;               /* resolved address */
    char        so_path[256];       /* SO that contains this function */
    uint32_t    func_crc_baseline;  /* CRC32 of first FUNC_CRC_SIZE bytes */
    uint32_t    so_crc_baseline;    /* CRC32 of entire r-xp region of the SO */
    uintptr_t   so_rx_start;
    uintptr_t   so_rx_end;
    bool        valid;
};

static FuncCrcEntry g_func_monitors[MAX_FUNC_MONITORS];
static int g_func_count = 0;

/*
 * Resolve a function by name, record baseline CRC32.
 * Uses dlsym(RTLD_DEFAULT, ...) which searches all loaded libraries.
 */
static void func_monitor_add(const char *sym_name) {
    if (g_func_count >= MAX_FUNC_MONITORS) return;

    FuncCrcEntry *e = &g_func_monitors[g_func_count];
    memset(e, 0, sizeof(*e));
    strncpy(e->name, sym_name, sizeof(e->name) - 1);

    /* Resolve via dlsym */
    void *p = dlsym(RTLD_DEFAULT, sym_name);
    if (!p) {
        LOGW("[func_crc] dlsym failed: %s", sym_name);
        e->valid = false;
        g_func_count++;
        return;
    }
    e->addr = (uintptr_t)p;

    /* Find which SO contains this address */
    uintptr_t rx_start = 0, rx_end = 0;
    if (!find_rx_region_for_addr(e->addr, e->so_path, sizeof(e->so_path),
                                  &rx_start, &rx_end)) {
        LOGW("[func_crc] cannot find r-xp region for %s @ %lx", sym_name, e->addr);
        e->valid = false;
        g_func_count++;
        return;
    }
    e->so_rx_start = rx_start;
    e->so_rx_end = rx_end;

    /* Baseline: function prologue */
    e->func_crc_baseline = crc32_compute((const void *)e->addr, FUNC_CRC_SIZE);

    /* Baseline: entire SO r-xp region */
    size_t so_size = rx_end - rx_start;
    e->so_crc_baseline = crc32_compute((const void *)rx_start, so_size);

    e->valid = true;
    g_func_count++;

    LOGI("[func_crc] %s @ 0x%lx  SO=%s [%lx-%lx]  func_crc=0x%08X  so_crc=0x%08X",
         sym_name, e->addr, e->so_path, rx_start, rx_end,
         e->func_crc_baseline, e->so_crc_baseline);
}

/*
 * Check all monitored functions. Returns formatted report:
 * "name|addr|so_path|func_baseline|func_current|func_ok|so_baseline|so_current|so_ok\n"
 */
static std::string func_monitor_check() {
    std::string result;
    char buf[512];

    for (int i = 0; i < g_func_count; i++) {
        FuncCrcEntry *e = &g_func_monitors[i];
        if (!e->valid) {
            snprintf(buf, sizeof(buf), "%s|0|N/A|0|0|unresolved|0|0|unresolved\n", e->name);
            result += buf;
            continue;
        }

        /* Re-compute function prologue CRC32 */
        uint32_t func_crc_now = crc32_compute((const void *)e->addr, FUNC_CRC_SIZE);
        bool func_ok = (func_crc_now == e->func_crc_baseline);

        /* Re-compute SO r-xp CRC32 */
        size_t so_size = e->so_rx_end - e->so_rx_start;
        uint32_t so_crc_now = crc32_compute((const void *)e->so_rx_start, so_size);
        bool so_ok = (so_crc_now == e->so_crc_baseline);

        if (!func_ok) {
            LOGW("[func_crc] HOOK DETECTED: %s @ 0x%lx  baseline=0x%08X current=0x%08X",
                 e->name, e->addr, e->func_crc_baseline, func_crc_now);
            /* Dump first 16 bytes for analysis */
            auto *insn = (const uint32_t *)e->addr;
            LOGW("[func_crc]   insn[0..3]: %08X %08X %08X %08X",
                 insn[0], insn[1], insn[2], insn[3]);
        }
        if (!so_ok) {
            LOGW("[func_crc] SO MODIFIED: %s  baseline=0x%08X current=0x%08X",
                 e->so_path, e->so_crc_baseline, so_crc_now);
        }

        snprintf(buf, sizeof(buf),
                 "%s|0x%lx|%s|0x%08X|0x%08X|%s|0x%08X|0x%08X|%s\n",
                 e->name, e->addr, e->so_path,
                 e->func_crc_baseline, func_crc_now, func_ok ? "true" : "false",
                 e->so_crc_baseline, so_crc_now, so_ok ? "true" : "false");
        result += buf;
    }
    return result;
}

/* ========== Disassembly Hint: dump first N instructions ========== */

static std::string dump_func_prologue(uintptr_t addr, int count) {
    std::string out;
    char buf[64];
    auto *insn = (const uint32_t *)addr;
    for (int i = 0; i < count; i++) {
        snprintf(buf, sizeof(buf), "  [%lx] %08X\n", addr + i * 4, insn[i]);
        out += buf;
    }
    return out;
}

/* ========== CRC counter query ========== */

static int get_crc_compute_count() { return g_crc_compute_count; }

/* ========== JNI Functions ========== */

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_unixsocketdemo_MainActivity_stringFromJNI(
        JNIEnv* env, jobject) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
```





**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
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
    /* P0-2 fix: snapshot fields while holding lock to prevent UAF race */
    uint64_t local_shadow_pfn = pi->shadow_pfn;
    uint64_t local_orig_pfn = pi->orig_pfn;
    uint64_t local_shadow_page_va = pi->shadow_page_va;
    wx_spin_unlock();

    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        wxjump_auto_cleanup(mm, region, idx, "VMA Gone (exec)");
        wxjump_put_region(region);
        return -1;
    }

    if (!wxjump_validate_mapping(mm, vma, local_orig_pfn, local_shadow_pfn, page_addr)) {
        wxjump_auto_cleanup(mm, region, idx, "Mapping Changed (exec)");
        wxjump_put_region(region);
        return -1;
    }

    if (local_shadow_page_va)
        wx_flush_dcache_va(local_shadow_page_va, WX_PAGE_SIZE);

    /* 切到 shadow page (--x, execute-only) */
    ret = wxjump_switch_mapping(vma, page_addr, local_shadow_pfn, 0);
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
    unsigned long cnt;

    /* ---- 重入保护: 同一 CPU 已在 handler 中则直接放行 ---- */
    int cpu = wx_cpu_id();
    if (wx_in_fault_handler[cpu]) {
        wx_fault_reentry_count++;
        return;  /* 交给内核默认处理 (copy_from_kernel_nofault 有 fixup) */
    }
    wx_in_fault_handler[cpu] = 1;

    cnt = ++wx_fault_total_count;
    /* 前 5 次 fault 打印完整诊断 */
    if (cnt <= 5)
        pr_info("wxjump: fault #%lu FAR=%lx ESR=%lx task=%px\n", cnt, far, esr, task);

    mm = kfn_get_task_mm(task);
    if (!mm) {
        wx_in_fault_handler[cpu] = 0;
        return;
    }

    /* 只处理 permission fault (DFSC[5:2]=0b0011) */
    if ((esr & 0x3C) != 0x0C) {
        kfn_mmput(mm);
        wx_in_fault_handler[cpu] = 0;
        return;
    }

    struct wx_region *region = wxjump_find_region(mm, far);
    if (!region) {
        kfn_mmput(mm);
        wx_in_fault_handler[cpu] = 0;
        return;
    }
    wxjump_put_region(region);

    unsigned long ec = (esr >> 26) & 0x3FUL;
    unsigned long dfsc = esr & 0x3FUL;

    if (cnt <= 5)
        pr_info("wxjump: fault #%lu EC=0x%lx DFSC=0x%lx matched_region FAR=%lx\n",
                cnt, ec, dfsc, far);

    /* Only handle EL0 faults. EL1 faults (EC=0x21/0x25) are from kernel
     * (e.g. EPAN blocking EL1 access to UXN=0 pages) -> let kernel handle */
    if (ec != 0x24 && ec != 0x20) {
        wx_fault_el1_skip_count++;
        kfn_mmput(mm);
        wx_in_fault_handler[cpu] = 0;
        return;
    }

    /* Acquire mmap_lock before VMA tree traversal in handlers.
     * Use trylock: we are in fault context (before kernel's do_page_fault
     * acquires its own lock). If contended, skip and let kernel handle. */
    if (!wx_mmap_read_trylock(mm)) {
        kfn_mmput(mm);
        wx_in_fault_handler[cpu] = 0;
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

    wx_mmap_read_unlock(mm);
    kfn_mmput(mm);
    wx_in_fault_handler[cpu] = 0;
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
            int pte_restored = 0;

            /* 精确恢复原始 PTE (保留 PXN 等所有原始标志) */
            {
                void *release_mm = NULL;
                if (wx_vma_vm_mm_offset >= 0 && vma)
                    release_mm = *(void **)((uint64_t)vma + wx_vma_vm_mm_offset);
                uint64_t *release_pte = get_user_pte(release_mm, page_addr, NULL);
                if (release_pte) {
                    *release_pte = pi->orig_pte;
                    wxjump_flush_tlb_page(vma, page_addr);
                    pte_restored = 1;
                } else {
                    pr_warn("wxjump: release: PTE lookup failed for %lx, keeping shadow\n", page_addr);
                }
            }

            /* 仅在 PTE 成功恢复后才释放 shadow page，避免悬垂映射 */
            if (pte_restored) {
                wx_flush_icache(page_addr);

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
            } else {
                /* PTE 恢复失败：保留 shadow 避免悬垂，但向调用方报错 */
                wxjump_put_region(region);
                return -11;  /* -EAGAIN */
            }
        } else {
            /* VMA 无效或状态已清除，无法恢复 PTE */
            pr_warn("wxjump: release: VMA invalid for %lx, keeping shadow\n", page_addr);
            wxjump_put_region(region);
            return -11;  /* -EAGAIN */
        }
    } else if (pi->state == STATE_SHADOW_X) {
        /* 还有其他 patch，刷新 icache 使恢复的字节生效 */
        wx_flush_icache(page_addr);
    }
```





**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
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
        uint64_t saved_orig_pte = pi->orig_pte;
        uint64_t saved_shadow_pfn = pi->shadow_pfn;
        pi->shadow_page_va = 0;
        pi->state = STATE_NONE;
        pi->orig_pfn = 0;
        pi->orig_pte = 0;
        pi->shadow_pfn = 0;
        pi->patch_count = 0;
        wx_spin_unlock();

        /* 仅当 PTE 仍指向我们的 shadow page 时才恢复 orig_pte，
         * 避免 Mapping Changed / VMA Gone 路径覆盖内核已设置的合法新映射 */
        if (saved_orig_pte && mm) {
            uint64_t *pte = get_user_pte(mm, addr, NULL);
            if (pte && (*pte & 1)) {
                uint64_t cur_pfn = (*pte >> 12) & PFN_MASK;
                if (cur_pfn == saved_shadow_pfn) {
                    *pte = saved_orig_pte;
                    void *vma = kfn_find_vma(mm, addr);
                    wxjump_flush_tlb_page(
                        (vma && addr >= *(unsigned long *)vma) ? vma : NULL, addr);
                }
            }
        }

        kfn_free_pages(shadow_va, 0);
    } else {
        pi->state = STATE_NONE;
        pi->orig_pfn = 0;
        pi->orig_pte = 0;
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
    /* P0-2 fix: snapshot fields while holding lock to prevent UAF race */
    uint64_t local_orig_pfn = pi->orig_pfn;
    uint64_t local_shadow_pfn = pi->shadow_pfn;
    wx_spin_unlock();

    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        wxjump_auto_cleanup(mm, region, idx, "VMA Gone (read)");
        wxjump_put_region(region);
        return -1;
    }

    if (!wxjump_validate_mapping(mm, vma, local_orig_pfn, local_shadow_pfn, page_addr)) {
        wxjump_auto_cleanup(mm, region, idx, "Mapping Changed (read)");
        wxjump_put_region(region);
        return -1;
    }

    /* 切到 orig page: UXN + 只读 (可读不可执行) */
    wx_fault_read_count++;
    ret = wxjump_switch_mapping(vma, page_addr, local_orig_pfn, PTE_UXN_USER_RO);
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
    /* P0-2 fix: snapshot fields while holding lock to prevent UAF race */
    uint64_t local_shadow_pfn = pi->shadow_pfn;
    uint64_t local_orig_pfn = pi->orig_pfn;
    uint64_t local_shadow_page_va = pi->shadow_page_va;
    wx_spin_unlock();

    vma = kfn_find_vma(mm, addr);
    if (!vma || addr < *(unsigned long *)vma) {
        wxjump_auto_cleanup(mm, region, idx, "VMA Gone (exec)");
        wxjump_put_region(region);
        return -1;
    }

    if (!wxjump_validate_mapping(mm, vma, local_orig_pfn, local_shadow_pfn, page_addr)) {
        wxjump_auto_cleanup(mm, region, idx, "Mapping Changed (exec)");
        wxjump_put_region(region);
        return -1;
    }

    if (local_shadow_page_va)
        wx_flush_dcache_va(local_shadow_page_va, WX_PAGE_SIZE);

    /* 切到 shadow page (--x, execute-only) */
    ret = wxjump_switch_mapping(vma, page_addr, local_shadow_pfn, 0);
```





**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
#define WX_MAX_CPUS 16
static volatile int wx_in_fault_handler[WX_MAX_CPUS];

static inline int wx_cpu_id(void)
{
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    /* Aff1 (cluster) * 4 + Aff0 (core) — 适用于 MTK DynamIQ 大小核布局 */
    int id = (int)(((mpidr >> 8) & 0xFF) * 4 + (mpidr & 0xFF));
    if ((unsigned)id >= WX_MAX_CPUS)
        id &= (WX_MAX_CPUS - 1);
    return id;
}

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

/* ========== mmap_lock (rw_semaphore) ========== */

static inline void *wx_mmap_lock_ptr(void *mm)
{
    if (wx_mmap_lock_offset < 0)
        return NULL;
    return (void *)((uint64_t)mm + wx_mmap_lock_offset);
}

static inline int wx_mmap_read_trylock(void *mm)
{
    void *sem = wx_mmap_lock_ptr(mm);
    if (!sem || !kfn_down_read_trylock)
        return 1;  /* no lock available — proceed without */
    return kfn_down_read_trylock(sem);
}

static inline void wx_mmap_read_lock(void *mm)
{
    void *sem = wx_mmap_lock_ptr(mm);
    if (!sem || !kfn_down_read)
        return;
    kfn_down_read(sem);
}

static inline void wx_mmap_read_unlock(void *mm)
{
    void *sem = wx_mmap_lock_ptr(mm);
    if (!sem || !kfn_up_read)
        return;
    kfn_up_read(sem);
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
```



文档里“只处理 Permission Fault”和“全 SO CRC 确实在扫整个 r-xp”这两点已经被代码证实了；接下来重点判断的是：这是不是**足以**解释黑屏重启，以及 proposed fix 有没有隐患。


**Grep Results:**

- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump-tutorial.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v3.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\dmesg.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_plan.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**




**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\dmesg.md**
```
rubens:/ $ dmesg | grep wx
[ 4957.724899] [T423361] [+] KP D     name: wxjump
[ 4957.724941] [T423361] wxjump: ====== INIT START ======
[ 4957.724943] [T423361] wxjump: args=(null)
[ 4957.724944] [T423361] wxjump: ====== resolve_symbols START ======
[ 4957.725768] [T423361] wxjump:   find_vma         = ffffffe2a82b373c
[ 4957.725947] [T423361] wxjump:   get_task_mm      = ffffffe2a7f268c0
[ 4957.726116] [T423361] wxjump:   mmput            = ffffffe2a7f2684c
[ 4957.726897] [T423361] wxjump: exit_mmap at ffffffe2a82b4ed8
[ 4957.728505] [T423361] wxjump:   __get_free_pages = ffffffe2a82d6254
[ 4957.728506] [T423361] wxjump:   free_pages       = ffffffe2a82d5588
[ 4957.752832] [T623361] wxjump:   memstart_addr    = ffffffe2aa2d5b20 (val=0x40000000)
[ 4957.752840] [T623361] wxjump:   physvirt_offset  = 0000000000000000
[ 4957.752843] [T623361] wxjump:   TCR_EL1=0x1b2b5593519 T1SZ=25 PAGE_OFFSET=0xffffff8000000000
[ 4957.752846] [T623361] wxjump:   test_page alloc  = 0xffffff802ca74000
[ 4957.752848] [T623361] wxjump:   PAR_EL1=0xff0000006ca74b80 (fault=0)
[ 4957.752849] [T623361] wxjump:   physvirt_offset  = 0xffffff7fc0000000 (detected)
[ 4957.752852] [T623361] wxjump: page_shift=12 page_level=3
[ 4957.767433] [T623361] wxjump:   _raw_spin_lock   = ffffffe2a9a0df70
[ 4957.767442] [T623361] wxjump:   _raw_spin_unlock = ffffffe2a9a0db84
[ 4957.780379] [T623361] wxjump:   flush_tlb_page   = 0000000000000000
[ 4957.781161] [T623361] wxjump:   __flush_tlb_range= ffffffe2a829a5cc
[ 4957.829709] [T623361] wxjump:   down_read_trylock= ffffffe2a7fea320
[ 4957.829714] [T623361] wxjump:   up_read          = ffffffe2a7fe7dcc
[ 4957.836721] [T623361] wxjump: page fault handler at ffffffe2a9a10778
[ 4957.836724] [T623361] wxjump:   kzalloc          = ffffffe2a8300f64
[ 4957.836725] [T623361] wxjump:   kcalloc          = 0000000000000000
[ 4957.836727] [T623361] wxjump:   kfree            = ffffffe2a82fb2d4
[ 4957.836728] [T623361] wxjump:   icache_range     = ffffffe2a7e1590c
[ 4957.836729] [T623361] wxjump:   copy_nofault     = ffffffe2a8235258
[ 4957.836730] [T623361] wxjump: ====== resolve_symbols OK ======
[ 4957.836732] [T623361] wxjump: KP offsets: pgd=72 mm=-1 active_mm=1312
[ 4957.836733] [T623361] wxjump: scanning vma offsets...
[ 4957.836736] [T623361] wxjump: vm_mm offset: 0x40
[ 4957.836737] [T623361] wxjump: vma_vm_mm_offset=0x40
[ 4957.846973] [T623361] wxjump: mm_offset=0x518 (active_mm-8)
[ 4957.846976] [T623361] wxjump: context.id at offset 0x300 (asid@bits[0:15])
[ 4957.846978] [T623361] wxjump: context_id_offset=0x300 asid_shift=0
[ 4957.846980] [T623361] wxjump: mmap_lock at offset 112 (verified)
[ 4957.847009] [T623361] wxjump: hooked page fault handler
[ 4957.847018] [T623361] wxjump: hooked exit_mmap
[ 4957.847025] [T623361] wxjump: hooked prctl
[ 4957.847026] [T623361] wxjump: ====== INIT OK ======
[ 4957.847028] [T623361] [+] KP I load_module: [wxjump] succeed with [(null)]
[ 4957.847151] [T202613] wxjump: fault #1 FAR=12d4b000 ESR=92000047 task=ffffff801f7e2500
[ 4957.847234] [T723301] wxjump: fault #2 FAR=70ddccd0 ESR=82000007 task=ffffff8004b98000
[ 4957.847268] [T723301] wxjump: fault #3 FAR=12e88000 ESR=92000047 task=ffffff8004b98000
[ 4957.847542] [T723301] wxjump: fault #4 FAR=12e891e0 ESR=92000047 task=ffffff8004b98000
[ 4957.847827] [T723301] wxjump: fault #5 FAR=12e8a000 ESR=92000047 task=ffffff8004b98000
[ 4957.855880] [T723361] [+] KP D get_module_info: name=wxjump
[ 4976.826704] [T623739] wxjump: [patch] page=72d1f2f000 offset=256 len=20
[ 4976.826719] [T623739] wxjump: patched new shadow at 72d1f2f000+256 (20 bytes) orig_pfn=1d9e3a shadow_pfn=810c9
rubens:/ $
```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
```
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

    /* mmap_lock (rw_semaphore) operations */
    RESOLVE_OPTIONAL("down_read_trylock", kfn_down_read_trylock);
    RESOLVE_OPTIONAL("down_read", kfn_down_read);
    RESOLVE_OPTIONAL("up_read", kfn_up_read);
    pr_info("wxjump:   down_read_trylock= %px\n", kfn_down_read_trylock);
    pr_info("wxjump:   up_read          = %px\n", kfn_up_read);

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

    pr_info("wxjump:   kzalloc          = %px\n", kfn_kzalloc);
    pr_info("wxjump:   kcalloc          = %px\n", kfn_kcalloc);
    pr_info("wxjump:   kfree            = %px\n", kfn_kfree);
    pr_info("wxjump:   icache_range     = %px\n", kfn___flush_icache_range);
    pr_info("wxjump:   copy_nofault     = %px\n", kfn_copy_from_kernel_nofault);
    pr_info("wxjump: ====== resolve_symbols OK ======\n");
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

/* ========== mmap_lock 偏移检测 ========== */

static void detect_mmap_lock(void)
{
    /* GKI mmap_lock offsets (validated against xiaojia-hide) */
    if (kver >= VERSION(6, 6, 0))
        wx_mmap_lock_offset = 96;
    else if (kver >= VERSION(6, 1, 0))
        wx_mmap_lock_offset = 96;
    else if (kver >= VERSION(5, 15, 0))
        wx_mmap_lock_offset = 104;
    else if (kver >= VERSION(5, 10, 0))
        wx_mmap_lock_offset = 112;
    else if (kver >= VERSION(5, 4, 0))
        wx_mmap_lock_offset = 112;
    else if (kver >= VERSION(4, 19, 0))
        wx_mmap_lock_offset = 104;
    else {
        wx_mmap_lock_offset = -1;
        pr_warn("wxjump: unknown kernel version for mmap_lock, disabled\n");
        return;
    }

    /* Verify by trylock/unlock on current mm */
    if (kfn_down_read_trylock && kfn_up_read) {
        void *mm = kfn_get_task_mm((void *)get_current());
        if (mm) {
            void *sem = (void *)((uint64_t)mm + wx_mmap_lock_offset);
            int ok = kfn_down_read_trylock(sem);
            if (ok) {
                kfn_up_read(sem);
                pr_info("wxjump: mmap_lock at offset %d (verified)\n", wx_mmap_lock_offset);
            } else {
                pr_warn("wxjump: mmap_lock trylock failed at offset %d, disabled\n",
                        wx_mmap_lock_offset);
                wx_mmap_lock_offset = -1;
            }
            kfn_mmput(mm);
        }
    } else {
        pr_warn("wxjump: down_read_trylock/up_read not found, mmap_lock disabled\n");
        wx_mmap_lock_offset = -1;
    }
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
```





**Grep Results:**

- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_plan.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-oz-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug--audit-glm5-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_ec.py**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v3.md**




**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-oz-v1.md**
```
// wxjump.c:1065-1107 — exit_mmap_before: 无条件摘链 + free
wx_spin_lock();
list_for_each_entry_safe(r, tmp, &region_list, list) {
    if (r->mm == mm && count < 32) {
        list_del_init(&r->list);   // ← 不检查 refcount
        regions[count++] = r;
    }
}
wx_spin_unlock();
// ... 直接遍历 free shadow pages, kfn_kfree(pages), kfn_kfree(r)
```

```c
// wxjump.c:1513-1568 — wxjump_exit: 同样无条件摘链 + free
wx_spin_lock();
list_for_each_entry_safe(r, tmp, &region_list, list) {
    if (i < count) {
        list_del_init(&r->list);   // ← 不检查 refcount
        cleanup_list[i++] = r;
    }
}
wx_spin_unlock();
// ... 直接 free 所有 pages 和 region
```

### 触发路径

1. 线程 A 在 `read_fault` / `exec_fault` / `patch` / `release` 路径中通过 `wxjump_find_region()` 持有 `region` (refcount > 1)
2. 进程退出触发 `exit_mmap_before()`，或模块卸载触发 `wxjump_exit()`
3. 步骤 2 直接把 `region` 和 `region->pages` free 掉
4. 线程 A 继续访问 `region->pages[idx]` → **UAF**
5. 线程 A 后续调 `wxjump_put_region(region)` → **double free / 野指针**

### 触发条件

- 多线程 + 进程退出（Android 上非常常见：app 切后台被杀、zygote fork 的子进程退出）
- 模块热卸载
- 长时间运行时概率性触发

### 修复方向

`exit_mmap_before()` / `wxjump_exit()` 不能直接 free 活跃 region。方案：

1. 加 `dead` 标志位：摘链后标记 `r->dead = 1`，让后续 fault 路径检测到后走 cleanup
2. 等待 refcount 归零：spin-wait 或引入完成量 (completion)
3. 或至少用 `wxjump_put_region()` 代替直接 free，让 refcount 机制生效

---

## P0-2: fault 路径解锁后裸用 `page_info` 字段，与 cleanup/release 并发 UAF

**严重度**: P0 — 可直接内核 panic
**位置**: `wxjump.c (488-540)` `wxjump_handle_read_fault()`，`wxjump.c (549-607)` `wxjump_handle_exec_fault()`

### 问题描述

`read_fault` 和 `exec_fault` 只在开头短暂加锁检查状态，**解锁后继续使用 `pi` 的多个字段**：

```c
// wxjump.c:506-538 — read_fault
wx_spin_lock();
pi = &region->pages[idx];
if (pi->state != STATE_SHADOW_X || !pi->orig_pfn) {
    wx_spin_unlock();
    // ... return
}
wx_spin_unlock();
// ↓↓↓ 解锁后，pi->orig_pfn / pi->shadow_page_va 等可能随时被清零 ↓↓↓

vma = kfn_find_vma(mm, addr);                              // line 515
// ...
ret = wxjump_switch_mapping(vma, page_addr, pi->orig_pfn,  // line 530 ← 危险!
                            PTE_UXN_USER_RO);
wx_spin_lock();
pi->state = STATE_ORIG_R;                                   // line 537
wx_spin_unlock();
```

```c
// wxjump.c:567-603 — exec_fault
wx_spin_lock();
pi = &region->pages[idx];
if (!pi->shadow_pfn || pi->state != STATE_ORIG_R) {
    wx_spin_unlock();
    // ... return
}
wx_spin_unlock();
// ↓↓↓ 解锁后 ↓↓↓

if (pi->shadow_page_va)                                     // line 589 ← 危险!
    wx_flush_dcache_va(pi->shadow_page_va, WX_PAGE_SIZE);   // line 590 ← 危险!
ret = wxjump_switch_mapping(vma, page_addr, pi->shadow_pfn, // line 593 ← 危险!
                            0);
```

同时 `wxjump_auto_cleanup()` 和 `wxjump_do_release()` 会在锁内清零这些字段并释放 shadow page：

```c
// wxjump.c:413-441 — auto_cleanup
wx_spin_lock();
if (pi->shadow_page_va) {
    uint64_t shadow_va = pi->shadow_page_va;
    pi->shadow_page_va = 0;    // ← 清零
    pi->orig_pfn = 0;          // ← 清零
    pi->shadow_pfn = 0;        // ← 清零
    pi->state = STATE_NONE;
    wx_spin_unlock();
    // ...
    kfn_free_pages(shadow_va, 0);  // ← 释放 shadow page
}
```

### 具体危险点

- `read_fault` line 530: 用 `pi->orig_pfn` 做 `switch_mapping`，但 `orig_pfn` 可能已被并发清零 → **PTE 写入 PFN=0 → 映射到物理地址 0 → 内核 panic 或数据损坏**
- `exec_fault` line 590: 用 `pi->shadow_page_va` 做 `wx_flush_dcache_va`，但该 va 可能已被 free → **对已释放内存做 cache 操作**
- `exec_fault` line 593: 用 `pi->shadow_pfn` 做 `switch_mapping`，同理 → **PTE 指向已释放物理页**

### 修复方向

在持锁期间把需要的字段复制到局部变量（快照），之后用局部变量操作：

```c
wx_spin_lock();
pi = &region->pages[idx];
if (pi->state != STATE_SHADOW_X || !pi->orig_pfn) {
    wx_spin_unlock();
    return -1;
}
uint64_t local_orig_pfn = pi->orig_pfn;
uint64_t local_shadow_pfn = pi->shadow_pfn;
uint64_t local_shadow_va = pi->shadow_page_va;
// 可选: 设置 in-flight 标志防止 cleanup 释放 shadow page
wx_spin_unlock();

// 之后只用 local_* 变量
```

---

## P0-3: 对正在执行中的 shadow page 直接 `memcpy` 修改代码

**严重度**: P0 — 可导致 SIGILL / 跳飞 / 内核 panic (如果目标是关键进程)
**位置**: `wxjump.c (745-759)` `wxjump_do_patch()` 已有 shadow 路径，`wxjump.c (927-932)` `wxjump_do_release()`

### 问题描述

当 shadow page 已存在时，`do_patch` 直接往 shadow page memcpy：

```c
// wxjump.c:745-758
pi = &region->pages[idx];
if (pi->shadow_page_va) {
    /* 已有 shadow page → 直接写入 */
    memcpy((void *)(pi->shadow_page_va + offset), kbuf, len);  // ← 无任何同步!

    if (pi->state == STATE_SHADOW_X)
        wx_flush_icache(page_addr);

    wx_spin_lock();
    pi->patch_count++;
    wx_spin_unlock();
}
```

`do_release` 也类似：

```c
// wxjump.c:927-932
if (pi->orig_pfn && offset + len <= WX_PAGE_SIZE) {
    uint64_t orig_va = wx_pa_to_va(pi->orig_pfn << 12);
    memcpy((void *)(pi->shadow_page_va + offset),          // ← 无任何同步!
           (void *)(orig_va + offset), len);
}
```

### 问题

没有任何机制阻止其他 CPU 核心正在从这个 shadow page 取指执行。ARM64 的 I-cache 是 VIPT/PIPT，`memcpy` 改写后虽然刷了 icache，但在刷之前其他核心可能已经取了半写入的指令。

- 最好的情况：目标线程看到一条不合法指令 → SIGILL
- 最差的情况：看到一条合法但错误的指令 → 跳飞、栈破坏、数据损坏
- 如果目标进程是 system_server / surfaceflinger → 用户感知"手机重启"

### 修复方向

正确做法是**双缓冲 + 原子切页**：

1. 分配新 shadow page
2. 复制旧 shadow page 内容到新页
3. 在新页上应用 patch
4. 原子切换 PTE 到新 shadow page
5. TLB flush + icache flush
6. 释放旧 shadow page

或者简化方案：先把 PTE 切到 orig page（使页面不可执行），等待 quiesce，再修改 shadow page，再切回。

---

## P1-1: 回调 wrapper 持 registry mutex 时调用 `JS_Call`，可自锁死锁
```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v2.md**
```

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
```




_Grep error: 目录名称无效。 (os error 267) _



**Grep Results:**

- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump-tutorial.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_rcu.py**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v3.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-oz-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v2.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug--audit-glm5-v1.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**




**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v3.md**
```
  ```bash
  $repo='C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch'; $src=Join-Path $repo 'kpms\wxjump\wxjump.c'; $kp=Join-Path $repo 'kernel'; $incs=@('.', 'include', 'patch/include', 'linux/include', 'linux/arch/arm64/include', 'linux/tools/arch/arm64/include') | ForEach-Object { '-I' + (Join-Path $kp $_) }; & 'C:\Users\24151\AppData\Local\Android\Sdk\ndk\27.0.12077973\toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe' -target aarch64-linux-android21 -fsyntax-only @incs $src
  ```

  **Output:**
  ```
  
  ```

  MarkAsCompleted: 1 items

  这次的 3 个目标修复已经都落到代码里了。我再补查一件和可移植性有关的事：`auto_cleanup` 现在会在找不到有效 `vma` 时把 `NULL` 传给 `wxjump_flush_tlb_page`，我想确认这套内核里 `flush_tlb_page / __flush_tlb_range` 到底是不是可解析符号。

  ```bash
  Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common" -Recurse -File | Select-String -Pattern "EXPORT_SYMBOL\(flush_tlb_page\)|EXPORT_SYMBOL_GPL\(flush_tlb_page\)|EXPORT_SYMBOL\(__flush_tlb_range\)|EXPORT_SYMBOL_GPL\(__flush_tlb_range\)|\bflush_tlb_page\b|\b__flush_tlb_range\b" | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() }
  ```

  **Current Output:**
  ```
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\alpha\include\asm\tlbflush.h:117:flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\alpha\include\asm\tlbflush.h:140:extern void flush_tlb_page(struct vm_area_struct *, unsigned long);
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\alpha\kernel\smp.c:689:flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\alpha\kernel\smp.c:719:EXPORT_SYMBOL(flush_tlb_page);
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arc\include\asm\tlbflush.h:24:#define flush_tlb_page(vma, page)	local_flush_tlb_page(vma, page)
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arc\include\asm\tlbflush.h:34:extern void flush_tlb_page(struct vm_area_struct *vma, unsigned long page);
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arc\mm\tlb.c:444:void flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr)
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm\include\asm\tlbflush.h:264:*	flush_tlb_page(vaddr,vma)
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm\include\asm\tlbflush.h:610:#define flush_tlb_page		local_flush_tlb_page
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm\include\asm\tlbflush.h:618:extern void flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr);
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm\include\asm\tlbflush.h:658:extern void flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr);
  C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm\kernel\smp_tlb.c:196:void flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr)
  <|cursor|>
  ```

  

  **Grep Results:**

  - **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4.md**
  - **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v2.md**
  - **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
  - **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v2.md**
  - **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump-tutorial.md**
  - **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-claude-v1.md**


  Subagent: 79c63063-caeb-465b-a5bc-76bb6c0b6d2d

  

  **Files Read:**

  **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
  ```
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
```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit-codex5.4-v3.md**
```
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
          uint64_t saved_orig_pte = pi->orig_pte;
          pi->shadow_page_va = 0;
          pi->state = STATE_NONE;
          pi->orig_pfn = 0;
          pi->orig_pte = 0;
          pi->shadow_pfn = 0;
          pi->patch_count = 0;
          wx_spin_unlock();
  
          /* 先恢复原始 PTE 再释放 shadow page，避免悬垂 PTE 指向已释放页 */
          if (saved_orig_pte && mm) {
              uint64_t *pte = get_user_pte(mm, addr, NULL);
              if (pte && (*pte & 1)) {
                  *pte = saved_orig_pte;
                  void *vma = kfn_find_vma(mm, addr);
                  wxjump_flush_tlb_page(
                      (vma && addr >= *(unsigned long *)vma) ? vma : NULL, addr);
              }
          }
  
          kfn_free_pages(shadow_va, 0);
      } else {
          pi->state = STATE_NONE;
          pi->orig_pfn = 0;
          pi->orig_pte = 0;
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
  
```



MarkAsCompleted: 3 items
MarkAsCompleted: 1 items
结论：**方向对，但不能按 `fix_plan.md` 原样做。**  
我会给这份 plan 一个 **“核心判断 70% 对，修复细节还不够严谨”** 的评价。

### 对的部分
- `wxjump` 现在**确实在做 BBM**：`*pte = 0` → `TLB flush` → 写新 PTE，代码就在 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c:359-383`。
- `do_page_fault_before()` **现在只接管 permission fault**，其它 fault（包括 translation fault）会直接放给内核，见 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c:715-719`。
- CRC demo **确实每次都在扫整个 SO 的 `r-xp`**，而且这段逻辑在 `func_monitor_check()` 的循环体里，意味着**每个 monitor 都会重扫一次整个 SO**，见 `Documents/GitHub/无痕hook/unixSocketDemo/app/src/main/cpp/native-lib.cpp:231-246`。  
  这一点非常危险，甚至比 plan 里写得还严重。

### 不够准确的部分
- `fix_plan.md` 把“**translation fault → 一定 SIGSEGV → 黑屏**”说得太满了。  
  这是**很可能的原因之一**，但不是唯一已经被代码证实的根因。内核也可能把页重新 fault-in 或改写映射，后续 `wxjump` 再走 `Mapping Changed` 清理路径。
- 里面那个概率推导（`p≈0.001`、`86.5%`）更像**定性说明**，不是证据。

### 它漏掉的关键竞态
这个比它写的 BBM translation fault 更值得优先修。

当前代码里，**PTE 切换**和**`pi->state` 更新**不是原子完成的：

- 读 fault 路径：先切 PTE，再写 `STATE_ORIG_R`，见 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c:601-610`
- 执行 fault 路径：先切 PTE，再写 `STATE_SHADOW_X`，见 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c:669-679`

而另一个 CPU 的 handler 判断是否处理，是看 `pi->state`：

- 读 fault 检查 `STATE_SHADOW_X`，见 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c:576-581`
- 执行 fault 检查 `STATE_ORIG_R`，见 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c:640-645`

这会产生一个很直接的窗口：

1. CPU A 已经把 PTE 切到 `orig`  
2. 但 `pi->state` 还没来得及改成 `STATE_ORIG_R`
3. CPU B 这时执行该页，触发 permission fault
4. handler 看到的还是旧 state，于是直接 `return -1`
5. fault 落回内核默认路径，变成真实的 `SEGV_ACCERR`

所以：**不只是 BBM 的 invalid 窗口有问题，PTE 和 state 不一致的窗口也有问题。**

### 为什么我说 `transitioning` 方案还不够
`fix_plan.md` 里 proposed `transitioning` 是在 `wxjump_switch_mapping()` 里置 1、写完新 PTE 又清 0，见 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/fix_plan.md:83-96`。

问题是：**它清零得太早**。  
调用者后面才更新 `pi->state`，所以它**挡不住**上面那个“新 PTE + 旧 state”的窗口。

另外，`volatile int` 也不够稳，至少应换成 `READ_ONCE/WRITE_ONCE` 或 `atomic_t`/acquire-release 语义。

### 我不建议直接照做的部分
- **方案 3 里自己内联 raw TLBI / 去掉 `isb()`**：不建议直接上。  
  现在 `wxjump_flush_tlb_page()` 已经把 kernel/arch 适配封装起来了，见 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c:259-289`。  
  这类微优化收益不确定，但很容易引入新的 TLB 一致性问题。
- **熔断**可以做，但它是止损，不是根治。

### 我建议的优先级
1. **先砍掉或大幅降频 full-SO CRC**
   - 最好按 **unique SO 去重**
   - 不要在每个 `FuncCrcEntry` 上都扫一遍同一个 SO
2. **把“过渡态”做完整**
   - 要覆盖整个 `PTE 切换 + state 提交` 生命周期
   - handler 对 **translation fault 和 permission fault** 都要先检查“该页是否正在 transition”
   - 最好不是单纯 `transitioning=1/0`，而是 `TRANS_TO_ORIG` / `TRANS_TO_SHADOW` 这种显式目标态
3. **再加统计**
   - 分别统计 transition 命中的 translation fault / permission fault
   - 这样才能确认到底哪类 fault 在黑屏前暴涨
4. 最后再考虑 TLBI 微优化

### 一句话结论
**Claude 不是“说错了”，但这份 `fix_plan.md` 还不够准，尤其漏掉了 `PTE/state` 不一致这个更直接的竞态。**  
如果按我看，应该先把 P0 改成：

- **去掉/去重 full-SO CRC**
- **补完整 transition state，而不是只补 translation-fault retry**

如果你要，我下一步可以直接帮你把这份 plan 改成一版更可靠的 **P0/P1 修正版**。