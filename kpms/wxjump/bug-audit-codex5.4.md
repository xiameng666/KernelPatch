USER:
@C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit.md 审计这份报告有没有说错

AGENT:
先读取这份审计报告，再到同目录代码里核对它引用的函数、偏移和结论，确认哪些判断成立、哪些地方可能说错了。
UpdateTodos: 3 items


**Files Read:**

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit.md**
```
# wxjump.c Bug 审计报告

审计时间: 2026-03-11
验证内核: Linux 5.10.101-android12-9 (GKI)
内核源码: `vmware-shared-gki-source/common/`

---

## 🔴 致命 Bug: PTE_BASE_FLAGS 内存类型错误

**文件**: wxjump.c 第 59 行
**代码**:
​```c
#define PTE_BASE_FLAGS  0xF13ULL
```

**问题**: 该常量的 AttrIndx 字段与你的 5.10 GKI 内核**不兼容**，会导致所有 wxjump 管理的页面被映射为**设备内存 (Device Memory)**。

### 内核源码证据

你的 5.10 GKI 内核的 MAIR 索引顺序 (**新版**, 支持 MTE):

```c
// common/arch/arm64/include/asm/memory.h:134-141
#define MT_NORMAL           0   // ← 索引 0
#define MT_NORMAL_TAGGED    1
#define MT_NORMAL_NC        2
#define MT_NORMAL_WT        3
#define MT_DEVICE_nGnRnE    4   // ← 索引 4
#define MT_DEVICE_nGnRE     5
```

wxjump 代码假设的是**旧版**顺序 (MT_NORMAL=4):

```
旧版内核: MT_DEVICE_nGnRnE=0, ..., MT_NORMAL=4
新版内核: MT_NORMAL=0, ..., MT_DEVICE_nGnRnE=4   ← 你的内核
```

### PTE_BASE_FLAGS 解码

```
0xF13 = 0b 1111_0001_0011

bit[1:0]  = 0b11   → PTE_TYPE_PAGE (Valid + Page)  ✓
bit[4:2]  = 0b100  → AttrIndx = 4
bit[9:8]  = 0b11   → Inner Shareable                ✓
bit[10]   = 1      → AF (Access Flag)               ✓
bit[11]   = 1      → nG (non-Global)                ✓
```

**AttrIndx=4 在你的内核上 = MT_DEVICE_nGnRnE (设备内存!)**

### 后果

所有通过 `make_pte()` 构造的 PTE 都会包含设备内存属性:
- `wxjump_do_patch` → shadow page 映射为 Device memory
- `wxjump_handle_read_fault` → orig page 切换后映射为 Device memory
- `wxjump_handle_exec_fault` → shadow page 恢复映射为 Device memory

设备内存上**执行代码是 UNPREDICTABLE 行为** (ARM 架构手册)。可能导致:
- 进程直接崩溃
- CPU 指令获取异常
- 缓存行为异常 (Device memory 不可缓存)

### 正确值

```c
// 你的内核: MT_NORMAL=0 → PTE_ATTRINDX(0) = 0
// 正确的 PTE_BASE_FLAGS:
#define PTE_BASE_FLAGS  0xF03ULL  // AttrIndx=0 → MT_NORMAL

// 对比:
// 0xF13 = 0b1111_0001_0011  (AttrIndx=4, 错误)
// 0xF03 = 0b1111_0000_0011  (AttrIndx=0, 正确)
//                ^--- 区别: bit[4]
```

### 内核验证

内核中 `_PAGE_DEFAULT` 的定义证实了 MT_NORMAL=0:

```c
// common/arch/arm64/include/asm/pgtable-prot.h:66
#define _PAGE_DEFAULT  (_PROT_DEFAULT | PTE_ATTRINDX(MT_NORMAL))
//                                      PTE_ATTRINDX(0) = 0

// _PROT_DEFAULT = PTE_TYPE_PAGE | PTE_AF | PTE_SHARED
//               = 0x3 | 0x400 | 0x300 = 0x703

// 所以 _PAGE_DEFAULT = 0x703 (AttrIndx=0)
```

### 推荐修复

不要硬编码 MAIR 索引，而是运行时从内核读取:

```c
// 方案 1: 从 _PAGE_DEFAULT 动态获取
uint64_t page_default = kallsyms_lookup_name("_PAGE_DEFAULT");  // 可能不导出
// 方案 2: 从现有 PTE 提取 AttrIndx
uint64_t existing_pte = *get_user_pte(mm, some_known_addr, NULL);
uint64_t attrindx = existing_pte & PTE_ATTRINDX_MASK;  // bits[4:2]
// 方案 3: 简单修复，直接改为 0xF03
#define PTE_BASE_FLAGS  0xF03ULL
```

---

## 🔴 Bug 2: flush_dcache_page 传参类型错误

**文件**: wxjump.c 第 559-560 行
**代码**:
```c
if (kfn_flush_dcache_page && pi->shadow_page_va)
    kfn_flush_dcache_page((void *)pi->shadow_page_va);
```

### 内核源码证据

```c
// common/arch/arm64/mm/flush.c:70-74
void flush_dcache_page(struct page *page)
{
    if (test_bit(PG_dcache_clean, &page->flags))
        clear_bit(PG_dcache_clean, &page->flags);
}
```

**函数签名**: `void flush_dcache_page(struct page *page)`
- 参数应为 `struct page *` 指针 (指向 `struct page` 描述符)
- 代码传入的是 `shadow_page_va` = `__get_free_pages()` 返回的**内核虚拟地址**

### 后果

函数将虚拟地址当作 `struct page *` 解读，访问 `page->flags`:
- `page->flags` 是 struct page 的第一个字段，偏移量为 0
- 实际读取的是 shadow page 的**前 8 字节** (即被复制的用户代码的前 8 字节)
- `test_bit(PG_dcache_clean, ...)` 检查该值的某一位
- 如果恰好为 1，`clear_bit` 会修改 shadow page 的代码内容!

### 推荐修复

```c
// 方案 1: 使用内联汇编直接操作缓存
static inline void wx_flush_dcache_shadow(uint64_t va)
{
    // 按缓存行大小遍历整个页
    for (uint64_t addr = va; addr < va + WX_PAGE_SIZE; addr += 64)
        asm volatile("dc cvau, %0" :: "r"(addr) : "memory");
    dsb(ish);
}

// 方案 2: 用 virt_to_page 转换 (需额外解析符号)
struct page *pg = kfn_virt_to_page(pi->shadow_page_va);
kfn_flush_dcache_page(pg);
```

---

## 🔴 Bug 3: PAGE_OFFSET 回退公式差一

**文件**: wxjump.c 第 1055-1056 行
**代码**:
```c
int t0sz = (tcr >> 16) & 0x3F;
wx_page_offset_base = -1ULL << (63 - t0sz);
```

### 内核源码证据

```c
// common/arch/arm64/include/asm/memory.h:44-45
#define _PAGE_OFFSET(va)  (-(UL(1) << (va)))
#define PAGE_OFFSET       (_PAGE_OFFSET(VA_BITS))
```

展开: `PAGE_OFFSET = -(1 << VA_BITS) = -1ULL << VA_BITS = -1ULL << (64 - T1SZ)`

wxjump 的公式: `-1ULL << (63 - T1SZ)`

| T1SZ | VA_BITS | 代码结果                             | 正确 PAGE_OFFSET                     |
| ---- | ------- | ------------------------------------ | ------------------------------------ |
| 25   | 39      | `-1ULL << 38` = `0xFFFFFFC000000000` | `-1ULL << 39` = `0xFFFFFF8000000000` |
| 16   | 48      | `-1ULL << 47` = `0xFFFF800000000000` | `-1ULL << 48` = `0xFFFF000000000000` |

**差距**: 始终差一位 (应为 `64 - t0sz` 而非 `63 - t0sz`)

### 影响范围

仅在以下两个条件都满足时触发:
1. `physvirt_offset` 符号不存在 (kallsyms 查找失败)
2. AT S1E1R 地址翻译检测失败

在你的 5.10 内核上 AT S1E1R 检测正常工作，因此**当前不会走到此路径**。

---

## 🟡 Bug 4: TCR_EL1 提取了错误的字段

**文件**: wxjump.c 第 1055, 1076 行
**代码**:
```c
int t0sz = (tcr >> 16) & 0x3F;    // 变量名 t0sz，实际提取 T1SZ
uint64_t tg0 = (tcr >> 30) & 3;   // 变量名 tg0，实际提取 TG1
```

### 内核源码证据

```c
// common/arch/arm64/include/asm/pgtable-hwdef.h:180-181,243-253
#define TCR_T0SZ_OFFSET   0      // T0SZ: bits[5:0]
#define TCR_T1SZ_OFFSET   16     // T1SZ: bits[21:16]

#define TCR_TG0_SHIFT     14     // TG0: bits[15:14]
#define TCR_TG1_SHIFT     30     // TG1: bits[31:30]
```

TG0 和 TG1 的编码不同:

```c
// TG0 (用户空间, bits[15:14]):
#define TCR_TG0_4K    (UL(0) << 14)   // 0b00
#define TCR_TG0_64K   (UL(1) << 14)   // 0b01
#define TCR_TG0_16K   (UL(2) << 14)   // 0b10

// TG1 (内核空间, bits[31:30]):
#define TCR_TG1_16K   (UL(1) << 30)   // 0b01
#define TCR_TG1_4K    (UL(2) << 30)   // 0b10
#define TCR_TG1_64K   (UL(3) << 30)   // 0b11
```

代码提取 TG1(bits[31:30]) 但用 TG0 的解码逻辑:
- `tg0==1` → 16KB (TG1 编码 ✓, TG0 编码应为 64KB ✗)
- `tg0==3` → 64KB (TG1 编码 ✓, TG0 编码为 Reserved ✗)
- else → 4KB (TG1=0b10 时正确，TG1=0b00 时为 Reserved 但也落入 4KB)

### 影响

Android 内核配置 T0SZ=T1SZ 且 TG0=TG1 (均为 4KB)，因此**实际不影响**。
但变量命名误导性极强，且在非标准配置上会出错。

---

## 🟡 Bug 5: RELEASE 不恢复 PXN 位

**文件**: wxjump.c 第 883 行
**代码**:
```c
wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_USER_RDONLY);
// PTE_USER_RDONLY = 0xC0，最终 PTE = make_pte(pfn, 0xC0) = (pfn << 12) | 0xC0 | 0xF13
// 注: 由于 Bug 1，0xF13 本身就有问题。但即使修为 0xF03，仍缺 PXN
```

### 内核源码证据

正常的用户态可执行只读页面:

```c
// common/arch/arm64/include/asm/pgtable-prot.h:89
#define PAGE_READONLY_EXEC  __pgprot(_PAGE_DEFAULT | PTE_USER | PTE_RDONLY | PTE_NG | PTE_PXN)
//                                                                                    ^^^^^^
```

内核为所有用户页面设置 PXN (Privileged Execute-Never):

```c
// common/arch/arm64/include/asm/pgtable-hwdef.h:147
#define PTE_PXN  (_AT(pteval_t, 1) << 53)
```

wxjump RELEASE 后的 PTE **缺少 PXN (bit 53)**，意味着内核态可以执行该用户页面代码，削弱 ret2usr 防护。

### 根本原因

`page_info` 只保存了 `orig_pfn`，没有保存**完整的原始 PTE 值**。恢复时只能重新构造权限位，无法精确还原。

### 推荐修复

```c
struct page_info {
    uint64_t orig_pfn;
    uint64_t orig_pte;       // ← 新增: 保存完整原始 PTE
    uint64_t shadow_pfn;
    uint64_t shadow_page_va;
    uint32_t state;
    uint32_t patch_count;
};

// PATCH 时保存:
pi->orig_pte = *pte;

// RELEASE 时精确恢复:
*pte = pi->orig_pte;
```

---

## 🟡 Bug 6: do_patch 错误路径资源泄漏

**文件**: wxjump.c 第 776-798 行

在第 776 行 `list_add(&new_region->list, &region_list)` 将 region 加入全局链表后，如果后续操作失败 (第 784 行 `get_user_pte` 返回 NULL 或第 797 行 `__get_free_pages` 返回 0)，直接 return 不清理:

```c
wx_spin_lock();
list_add(&new_region->list, &region_list);  // ← region 已入链表
wx_spin_unlock();

...

uint64_t *pte = get_user_pte(mm, page_addr, NULL);
if (!pte || !((*pte) & 1)) {
    return -14;  // ← 泄漏! region 仍在链表中
}

unsigned long shadow_va = kfn___get_free_pages(wx_gfp_kernel, 0);
if (!shadow_va)
    return -12;  // ← 泄漏! region 仍在链表中
```

### 推荐修复

失败时从链表移除并释放 region:

```c
if (!pte || !((*pte) & 1)) {
    wx_spin_lock();
    list_del_init(&new_region->list);
    wx_spin_unlock();
    kfn_kfree(pages);
    kfn_kfree(new_region);
    return -14;
}
```

---

## 🟢 Bug 7: exit_mmap 的 PTE 恢复值 0xFC3 (仅旧内核有问题)

**文件**: wxjump.c 第 1000 行
**代码**:
```c
uint64_t new_pte = (pi->orig_pfn << 12) | 0xFC3ULL;
```

### 分析

```
0xFC3 = 0b 1111_1100_0011
AttrIndx = bits[4:2] = 0b000 = 0
```

- **在你的 5.10 GKI 内核上**: AttrIndx=0 = MT_NORMAL → **正确** ✓
- **在旧版内核上 (MT_NORMAL=4)**: AttrIndx=0 = MT_DEVICE_nGnRnE → **错误** ✗

与 PTE_BASE_FLAGS (0xF13) 矛盾: 一个假设 MT_NORMAL=4，另一个假设 MT_NORMAL=0。说明代码中 PTE 常量是在不同内核版本上分别编写/测试的，存在不一致。

---

## 🟢 Bug 8: wx_region 在所有 patch 释放后不回收

`wxjump_do_release` 在 `patch_count` 归零后释放了 shadow page，但 `wx_region` 结构体及其 `page_info` 数组仍留在全局链表中直到进程退出。

对于大 VMA (如 10MB 的 `.text` 段):
- `nr_pages` = 10MB / 4KB = 2560
- `page_info` 数组 = 2560 × 40 bytes = ~100KB 内存泄漏

---

## 总结

| #    | 严重度 | Bug                                         | 你的 5.10 GKI 上触发?       | 行号       |
| ---- | ------ | ------------------------------------------- | --------------------------- | ---------- |
| 1    | 🔴致命  | PTE_BASE_FLAGS AttrIndx=4 映射为设备内存    | **是! 所有 PTE 操作受影响** | 59         |
| 2    | 🔴严重  | flush_dcache_page 传入 VA 而非 struct page* | 是，可能静默损坏代码        | 559        |
| 3    | 🔴中等  | PAGE_OFFSET 公式差一位                      | 否 (AT S1E1R 正常)          | 1056       |
| 4    | 🟡中等  | TCR 提取 T1SZ/TG1 而非 T0SZ/TG0             | 否 (Android 配置相同)       | 1055,1076  |
| 5    | 🟡中等  | RELEASE 后缺失 PXN 保护                     | 是，安全性降低              | 883        |
| 6    | 🟡低    | do_patch 错误路径 region 泄漏               | 仅在分配失败时              | 784,797    |
| 7    | 🟢信息  | exit_mmap 0xFC3 与 PTE_BASE_FLAGS 不一致    | 0xFC3 在你内核上恰好正确    | 1000       |
| 8    | 🟢信息  | wx_region 释放后不回收                      | 是，轻微内存泄漏            | do_release |

**最优先修复**: Bug 1 (PTE_BASE_FLAGS)。在你的内核上，这会导致 wxjump **完全无法工作** — 所有页面被映射为不可缓存的设备内存，代码执行行为未定义。

```



报告里的结论点已经提出来了，下一步我会直接对照 `wxjump.c` 和它引用的内核宏/接口，逐项验证是不是和代码一致。
MarkAsCompleted: 1 items


**File Glob Results:**

- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump-tutorial.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**




**File Glob Results:**



USER:
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source内核源码在这

AGENT:
内核源码路径已经有了，我会直接核对本地 `vmware-shared-gki-source` 里的头文件和实现，避免靠记忆判断。


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
#define PTE_BASE_FLAGS       0xF13ULL            /* Valid + AF + SH + AP 基础标志 */
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
static void  (*kfn_flush_dcache_page)(void *page);
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

    if (kfn_flush_dcache_page && pi->shadow_page_va)
        kfn_flush_dcache_page((void *)pi->shadow_page_va);

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
    void *vma;
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
        wxjump_put_region(region);
    }

    /* 需要创建新的 shadow page */
    vma = kfn_find_vma(mm, page_addr);
    if (!vma || page_addr < *(unsigned long *)vma) {
        pr_err("wxjump: patch: no VMA for %lx\n", page_addr);
        return -1;
    }

    /* 并发检查 */
    wx_spin_lock();
    struct wx_region *existing;
    list_for_each_entry(existing, &region_list, list) {
        if (existing->mm == mm &&
            page_addr >= existing->vm_start && page_addr < existing->vm_end) {
            wx_spin_unlock();
            return wxjump_do_patch(mm, page_addr, user_buf, len, offset);
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
    new_region->refcount = 1;

    wx_spin_lock();
    list_add(&new_region->list, &region_list);
    wx_spin_unlock();

    /* 设置 shadow page */
    idx = wxjump_page_index(new_region, page_addr);
    pi = &pages[idx];

    uint64_t *pte = get_user_pte(mm, page_addr, NULL);
    if (!pte || !((*pte) & 1)) {
        pr_err("wxjump: patch: no PTE for %lx\n", page_addr);
        return -14;
    }

    uint64_t pte_val = *pte;
    uint64_t orig_pfn = (pte_val >> 12) & PFN_MASK;
    pi->orig_pfn = orig_pfn;

    uint64_t orig_va = wx_pa_to_va(orig_pfn << 12);

    /* 分配 shadow page */
    unsigned long shadow_va = kfn___get_free_pages(wx_gfp_kernel, 0);
    if (!shadow_va)
        return -12;

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
        return ret;
    }

    wx_flush_icache(page_addr);

    pr_info("wxjump: patched new shadow at %lx+%zu (%zu bytes) orig_pfn=%lx shadow_pfn=%lx\n",
            page_addr, offset, len, (unsigned long)orig_pfn, (unsigned long)shadow_pfn);
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

            wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_USER_RDONLY);
            wx_flush_icache(page_addr);  /* 清除 shadow 指令的 I-cache 残留 */
    
            wx_spin_lock();
            pi->shadow_page_va = 0;
            pi->orig_pfn = 0;
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
                    if (pte && (*pte & 1)) {
                        uint64_t new_pte = (pi->orig_pfn << 12) | 0xFC3ULL;
                        *pte = new_pte;
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
    int t0sz = (tcr >> 16) & 0x3F;
    wx_page_offset_base = -1ULL << (63 - t0sz);
    
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
    
    /* Page table config */
    uint64_t tg0 = (tcr >> 30) & 3;
    int bits_per_level;
    if (tg0 == 1) {
        wx_page_shift = 14; bits_per_level = 11;
    } else if (tg0 == 3) {
        wx_page_shift = 16; bits_per_level = 13;
    } else {
        wx_page_shift = 12; bits_per_level = 9;
    }
    wx_page_level = (60 - t0sz) / bits_per_level;
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
    
    /* Cache */
    RESOLVE_REQUIRED("flush_dcache_page", kfn_flush_dcache_page);
    
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
    
    if (!count) {
        wx_spin_unlock();
        pr_info("wxjump: no active regions, unload complete\n");
        return 0;
    }
    
    cleanup_list = (struct wx_region **)kfn_kzalloc(
        count * sizeof(struct wx_region *), wx_gfp_kernel);
    
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
    
            if (vma && pi->orig_pfn &&
                (pi->state == STATE_SHADOW_X || pi->state == STATE_ORIG_R)) {
                if (!wxjump_switch_mapping(vma, addr, pi->orig_pfn, PTE_USER_RDONLY)) {
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

```

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\memory.h**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/memory.h
 *
 * Copyright (C) 2000-2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * Note: this file should not be included by non-asm/.h files
 */
#ifndef __ASM_MEMORY_H
#define __ASM_MEMORY_H

#include <linux/const.h>
#include <linux/sizes.h>
#include <asm/page-def.h>

/*
 * Size of the PCI I/O space. This must remain a power of two so that
 * IO_SPACE_LIMIT acts as a mask for the low bits of I/O addresses.
 */
#define PCI_IO_SIZE		SZ_16M

/*
 * VMEMMAP_SIZE - allows the whole linear region to be covered by
 *                a struct page array
 *
 * If we are configured with a 52-bit kernel VA then our VMEMMAP_SIZE
 * needs to cover the memory region from the beginning of the 52-bit
 * PAGE_OFFSET all the way to PAGE_END for 48-bit. This allows us to
 * keep a constant PAGE_OFFSET and "fallback" to using the higher end
 * of the VMEMMAP where 52-bit support is not available in hardware.
 */
#define VMEMMAP_SIZE ((_PAGE_END(VA_BITS_MIN) - PAGE_OFFSET) \
			>> (PAGE_SHIFT - STRUCT_PAGE_MAX_SHIFT))

/*
 * PAGE_OFFSET - the virtual address of the start of the linear map, at the
 *               start of the TTBR1 address space.
 * PAGE_END - the end of the linear map, where all other kernel mappings begin.
 * KIMAGE_VADDR - the virtual address of the start of the kernel image.
 * VA_BITS - the maximum number of bits for virtual addresses.
 */
#define VA_BITS			(CONFIG_ARM64_VA_BITS)
#define _PAGE_OFFSET(va)	(-(UL(1) << (va)))
#define PAGE_OFFSET		(_PAGE_OFFSET(VA_BITS))
#define KIMAGE_VADDR		(MODULES_END)
#define BPF_JIT_REGION_START	(KASAN_SHADOW_END)
#define BPF_JIT_REGION_SIZE	(SZ_128M)
#define BPF_JIT_REGION_END	(BPF_JIT_REGION_START + BPF_JIT_REGION_SIZE)
#define MODULES_END		(MODULES_VADDR + MODULES_VSIZE)
#define MODULES_VADDR		(BPF_JIT_REGION_END)
#define MODULES_VSIZE		(SZ_128M)
#define VMEMMAP_START		(-VMEMMAP_SIZE - SZ_2M)
#define VMEMMAP_END		(VMEMMAP_START + VMEMMAP_SIZE)
#define PCI_IO_END		(VMEMMAP_START - SZ_2M)
#define PCI_IO_START		(PCI_IO_END - PCI_IO_SIZE)
#define FIXADDR_TOP		(PCI_IO_START - SZ_2M)

#if VA_BITS > 48
#define VA_BITS_MIN		(48)
#else
#define VA_BITS_MIN		(VA_BITS)
#endif

#define _PAGE_END(va)		(-(UL(1) << ((va) - 1)))

#define KERNEL_START		_text
#define KERNEL_END		_end

/*
 * Generic and tag-based KASAN require 1/8th and 1/16th of the kernel virtual
 * address space for the shadow region respectively. They can bloat the stack
 * significantly, so double the (minimum) stack size when they are in use.
 */
#if defined(CONFIG_KASAN_GENERIC) || defined(CONFIG_KASAN_SW_TAGS)
#define KASAN_SHADOW_OFFSET	_AC(CONFIG_KASAN_SHADOW_OFFSET, UL)
#define KASAN_SHADOW_END	((UL(1) << (64 - KASAN_SHADOW_SCALE_SHIFT)) \
					+ KASAN_SHADOW_OFFSET)
#define KASAN_THREAD_SHIFT	1
#else
#define KASAN_THREAD_SHIFT	0
#define KASAN_SHADOW_END	(_PAGE_END(VA_BITS_MIN))
#endif /* CONFIG_KASAN */

#define MIN_THREAD_SHIFT	(14 + KASAN_THREAD_SHIFT)

/*
 * VMAP'd stacks are allocated at page granularity, so we must ensure that such
 * stacks are a multiple of page size.
 */
#if defined(CONFIG_VMAP_STACK) && (MIN_THREAD_SHIFT < PAGE_SHIFT)
#define THREAD_SHIFT		PAGE_SHIFT
#else
#define THREAD_SHIFT		MIN_THREAD_SHIFT
#endif

#if THREAD_SHIFT >= PAGE_SHIFT
#define THREAD_SIZE_ORDER	(THREAD_SHIFT - PAGE_SHIFT)
#endif

#define THREAD_SIZE		(UL(1) << THREAD_SHIFT)

/*
 * By aligning VMAP'd stacks to 2 * THREAD_SIZE, we can detect overflow by
 * checking sp & (1 << THREAD_SHIFT), which we can do cheaply in the entry
 * assembly.
 */
#ifdef CONFIG_VMAP_STACK
#define THREAD_ALIGN		(2 * THREAD_SIZE)
#else
#define THREAD_ALIGN		THREAD_SIZE
#endif

#define IRQ_STACK_SIZE		THREAD_SIZE

#define OVERFLOW_STACK_SIZE	SZ_4K

/*
 * Alignment of kernel segments (e.g. .text, .data).
 *
 *  4 KB granule:  16 level 3 entries, with contiguous bit
 * 16 KB granule:   4 level 3 entries, without contiguous bit
 * 64 KB granule:   1 level 3 entry
 */
#define SEGMENT_ALIGN		SZ_64K

/*
 * Memory types available.
 *
 * IMPORTANT: MT_NORMAL must be index 0 since vm_get_page_prot() may 'or' in
 *	      the MT_NORMAL_TAGGED memory type for PROT_MTE mappings. Note
 *	      that protection_map[] only contains MT_NORMAL attributes.
 */
#define MT_NORMAL		0
#define MT_NORMAL_TAGGED	1
#define MT_NORMAL_NC		2
#define MT_NORMAL_WT		3
#define MT_DEVICE_nGnRnE	4
#define MT_DEVICE_nGnRE		5
#define MT_DEVICE_GRE		6
#define MT_NORMAL_iNC_oWB	7

/*
 * Memory types for Stage-2 translation
 */
#define MT_S2_NORMAL		0xf
#define MT_S2_DEVICE_nGnRE	0x1

/*
 * Memory types for Stage-2 translation when ID_AA64MMFR2_EL1.FWB is 0001
 * Stage-2 enforces Normal-WB and Device-nGnRE
 */
#define MT_S2_FWB_NORMAL	6
#define MT_S2_FWB_DEVICE_nGnRE	1

#ifdef CONFIG_ARM64_4K_PAGES
#define IOREMAP_MAX_ORDER	(PUD_SHIFT)
#else
#define IOREMAP_MAX_ORDER	(PMD_SHIFT)
#endif

#ifndef __ASSEMBLY__

#include <linux/bitops.h>
#include <linux/compiler.h>
#include <linux/mmdebug.h>
#include <linux/types.h>
#include <asm/bug.h>

extern u64			vabits_actual;
#define PAGE_END		(_PAGE_END(vabits_actual))

extern s64			memstart_addr;
/* PHYS_OFFSET - the physical address of the start of memory. */
#define PHYS_OFFSET		({ VM_BUG_ON(memstart_addr & 1); memstart_addr; })

/* the virtual base of the kernel image */
extern u64			kimage_vaddr;

/* the offset between the kernel virtual and physical mappings */
extern u64			kimage_voffset;

static inline unsigned long kaslr_offset(void)
{
	return kimage_vaddr - KIMAGE_VADDR;
}

/*
 * Allow all memory at the discovery stage. We will clip it later.
 */
#define MIN_MEMBLOCK_ADDR	0
#define MAX_MEMBLOCK_ADDR	U64_MAX

/*
 * PFNs are used to describe any physical page; this means
 * PFN 0 == physical address 0.
 *
 * This is the PFN of the first RAM page in the kernel
 * direct-mapped view.  We assume this is the first page
 * of RAM in the mem_map as well.
 */
#define PHYS_PFN_OFFSET	(PHYS_OFFSET >> PAGE_SHIFT)

/*
 * When dealing with data aborts, watchpoints, or instruction traps we may end
 * up with a tagged userland pointer. Clear the tag to get a sane pointer to
 * pass on to access_ok(), for instance.
 */
#define __untagged_addr(addr)	\
	((__force __typeof__(addr))sign_extend64((__force u64)(addr), 55))

#define untagged_addr(addr)	({					\
	u64 __addr = (__force u64)(addr);					\
	__addr &= __untagged_addr(__addr);				\
	(__force __typeof__(addr))__addr;				\
})

#if defined(CONFIG_KASAN_SW_TAGS) || defined(CONFIG_KASAN_HW_TAGS)
#define __tag_shifted(tag)	((u64)(tag) << 56)
#define __tag_reset(addr)	__untagged_addr(addr)
#define __tag_get(addr)		(__u8)((u64)(addr) >> 56)
#else
#define __tag_shifted(tag)	0UL
#define __tag_reset(addr)	(addr)
#define __tag_get(addr)		0
#endif /* CONFIG_KASAN_SW_TAGS || CONFIG_KASAN_HW_TAGS */

static inline const void *__tag_set(const void *addr, u8 tag)
{
	u64 __addr = (u64)addr & ~__tag_shifted(0xff);
	return (const void *)(__addr | __tag_shifted(tag));
}

#ifdef CONFIG_KASAN_HW_TAGS
#define arch_enable_tagging_sync()		mte_enable_kernel_sync()
#define arch_enable_tagging_async()		mte_enable_kernel_async()
#define arch_set_tagging_report_once(state)	mte_set_report_once(state)
#define arch_force_async_tag_fault()		mte_check_tfsr_exit()
#define arch_get_random_tag()			mte_get_random_tag()
#define arch_get_mem_tag(addr)			mte_get_mem_tag(addr)
#define arch_set_mem_tag_range(addr, size, tag, init)	\
			mte_set_mem_tag_range((addr), (size), (tag), (init))
#endif /* CONFIG_KASAN_HW_TAGS */

/*
 * Physical vs virtual RAM address space conversion.  These are
 * private definitions which should NOT be used outside memory.h
 * files.  Use virt_to_phys/phys_to_virt/__pa/__va instead.
 */


/*
 * Check whether an arbitrary address is within the linear map, which
 * lives in the [PAGE_OFFSET, PAGE_END) interval at the bottom of the
 * kernel's TTBR1 address range.
 */
#define __is_lm_address(addr)	(((u64)(addr) ^ PAGE_OFFSET) < (PAGE_END - PAGE_OFFSET))

#define __lm_to_phys(addr)	(((addr) & ~PAGE_OFFSET) + PHYS_OFFSET)
#define __kimg_to_phys(addr)	((addr) - kimage_voffset)

#define __virt_to_phys_nodebug(x) ({					\
	phys_addr_t __x = (phys_addr_t)(__tag_reset(x));		\
	__is_lm_address(__x) ? __lm_to_phys(__x) : __kimg_to_phys(__x);	\
})

#define __pa_symbol_nodebug(x)	__kimg_to_phys((phys_addr_t)(x))

#ifdef CONFIG_DEBUG_VIRTUAL
extern phys_addr_t __virt_to_phys(unsigned long x);
extern phys_addr_t __phys_addr_symbol(unsigned long x);
#else
#define __virt_to_phys(x)	__virt_to_phys_nodebug(x)
#define __phys_addr_symbol(x)	__pa_symbol_nodebug(x)
#endif /* CONFIG_DEBUG_VIRTUAL */

#define __phys_to_virt(x)	((unsigned long)((x) - PHYS_OFFSET) | PAGE_OFFSET)
#define __phys_to_kimg(x)	((unsigned long)((x) + kimage_voffset))

/*
 * Convert a page to/from a physical address
 */
#define page_to_phys(page)	(__pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys)	(pfn_to_page(__phys_to_pfn(phys)))

/*
 * Note: Drivers should NOT use these.  They are the wrong
 * translation for translating DMA addresses.  Use the driver
 * DMA support - see dma-mapping.h.
 */
#define virt_to_phys virt_to_phys
static inline phys_addr_t virt_to_phys(const volatile void *x)
{
	return __virt_to_phys((unsigned long)(x));
}

#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(phys_addr_t x)
{
	return (void *)(__phys_to_virt(x));
}

/*
 * Drivers should NOT use these either.
 */
#define __pa(x)			__virt_to_phys((unsigned long)(x))
#define __pa_symbol(x)		__phys_addr_symbol(RELOC_HIDE((unsigned long)(x), 0))
#define __pa_nodebug(x)		__virt_to_phys_nodebug((unsigned long)(x))
#define __va(x)			((void *)__phys_to_virt((phys_addr_t)(x)))
#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)
#define virt_to_pfn(x)		__phys_to_pfn(__virt_to_phys((unsigned long)(x)))
#define sym_to_pfn(x)		__phys_to_pfn(__pa_symbol(x))

/*
 * With non-canonical CFI jump tables, the compiler replaces function
 * address references with the address of the function's CFI jump
 * table entry. This results in __pa_symbol(function) returning the
 * physical address of the jump table entry, which can lead to address
 * space confusion since the jump table points to the function's
 * virtual address. Therefore, use inline assembly to ensure we are
 * always taking the address of the actual function.
 */
#define __va_function(x) ({						\
	void *addr;							\
	asm("adrp %0, " __stringify(x) "\n\t"				\
	    "add  %0, %0, :lo12:" __stringify(x) : "=r" (addr));	\
	addr;								\
})

#define __pa_function(x) 	__pa_symbol(__va_function(x))

/*
 *  virt_to_page(x)	convert a _valid_ virtual address to struct page *
 *  virt_addr_valid(x)	indicates whether a virtual address is valid
 */
#define ARCH_PFN_OFFSET		((unsigned long)PHYS_PFN_OFFSET)

#if !defined(CONFIG_SPARSEMEM_VMEMMAP) || defined(CONFIG_DEBUG_VIRTUAL)
#define page_to_virt(x)	({						\
	__typeof__(x) __page = x;					\
	void *__addr = __va(page_to_phys(__page));			\
	(void *)__tag_set((const void *)__addr, page_kasan_tag(__page));\
})
#define virt_to_page(x)		pfn_to_page(virt_to_pfn(x))
#else
#define page_to_virt(x)	({						\
	__typeof__(x) __page = x;					\
	u64 __idx = ((u64)__page - VMEMMAP_START) / sizeof(struct page);\
	u64 __addr = PAGE_OFFSET + (__idx * PAGE_SIZE);			\
	(void *)__tag_set((const void *)__addr, page_kasan_tag(__page));\
})

#define virt_to_page(x)	({						\
	u64 __idx = (__tag_reset((u64)x) - PAGE_OFFSET) / PAGE_SIZE;	\
	u64 __addr = VMEMMAP_START + (__idx * sizeof(struct page));	\
	(struct page *)__addr;						\
})
#endif /* !CONFIG_SPARSEMEM_VMEMMAP || CONFIG_DEBUG_VIRTUAL */

#define virt_addr_valid(addr)	({					\
	__typeof__(addr) __addr = __tag_reset(addr);			\
	__is_lm_address(__addr) && pfn_valid(virt_to_pfn(__addr));	\
})

void dump_mem_limit(void);
#endif /* !ASSEMBLY */

/*
 * Given that the GIC architecture permits ITS implementations that can only be
 * configured with a LPI table address once, GICv3 systems with many CPUs may
 * end up reserving a lot of different regions after a kexec for their LPI
 * tables (one per CPU), as we are forced to reuse the same memory after kexec
 * (and thus reserve it persistently with EFI beforehand)
 */
#if defined(CONFIG_EFI) && defined(CONFIG_ARM_GIC_V3_ITS)
# define INIT_MEMBLOCK_RESERVED_REGIONS	(INIT_MEMBLOCK_REGIONS + NR_CPUS + 1)
#endif

#include <asm-generic/memory_model.h>

#endif /* __ASM_MEMORY_H */

```

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-prot.h**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016 ARM Ltd.
 */
#ifndef __ASM_PGTABLE_PROT_H
#define __ASM_PGTABLE_PROT_H

#include <asm/memory.h>
#include <asm/pgtable-hwdef.h>

#include <linux/const.h>

/*
 * Software defined PTE bits definition.
 */
#define PTE_WRITE		(PTE_DBM)		 /* same as DBM (51) */
#define PTE_DIRTY		(_AT(pteval_t, 1) << 55)
#define PTE_SPECIAL		(_AT(pteval_t, 1) << 56)
#define PTE_DEVMAP		(_AT(pteval_t, 1) << 57)
#define PTE_PROT_NONE		(_AT(pteval_t, 1) << 58) /* only when !PTE_VALID */

/*
 * This bit indicates that the entry is present i.e. pmd_page()
 * still points to a valid huge page in memory even if the pmd
 * has been invalidated.
 */
#define PMD_PRESENT_INVALID	(_AT(pteval_t, 1) << 59) /* only when !PMD_SECT_VALID */

#ifndef __ASSEMBLY__

#include <asm/cpufeature.h>
#include <asm/pgtable-types.h>

extern bool arm64_use_ng_mappings;

#define _PROT_DEFAULT		(PTE_TYPE_PAGE | PTE_AF | PTE_SHARED)
#define _PROT_SECT_DEFAULT	(PMD_TYPE_SECT | PMD_SECT_AF | PMD_SECT_S)

#define PTE_MAYBE_NG		(arm64_use_ng_mappings ? PTE_NG : 0)
#define PMD_MAYBE_NG		(arm64_use_ng_mappings ? PMD_SECT_NG : 0)

/*
 * If we have userspace only BTI we don't want to mark kernel pages
 * guarded even if the system does support BTI.
 */
#ifdef CONFIG_ARM64_BTI_KERNEL
#define PTE_MAYBE_GP		(system_supports_bti() ? PTE_GP : 0)
#else
#define PTE_MAYBE_GP		0
#endif

#define PROT_DEFAULT		(_PROT_DEFAULT | PTE_MAYBE_NG)
#define PROT_SECT_DEFAULT	(_PROT_SECT_DEFAULT | PMD_MAYBE_NG)

#define PROT_DEVICE_nGnRnE	(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_WRITE | PTE_ATTRINDX(MT_DEVICE_nGnRnE))
#define PROT_DEVICE_nGnRE	(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_WRITE | PTE_ATTRINDX(MT_DEVICE_nGnRE))
#define PROT_NORMAL_NC		(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL_NC))
#define PROT_NORMAL_WT		(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL_WT))
#define PROT_NORMAL		(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL))
#define PROT_NORMAL_TAGGED	(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL_TAGGED))

#define PROT_SECT_DEVICE_nGnRE	(PROT_SECT_DEFAULT | PMD_SECT_PXN | PMD_SECT_UXN | PMD_ATTRINDX(MT_DEVICE_nGnRE))
#define PROT_SECT_NORMAL	(PROT_SECT_DEFAULT | PMD_SECT_PXN | PMD_SECT_UXN | PMD_ATTRINDX(MT_NORMAL))
#define PROT_SECT_NORMAL_EXEC	(PROT_SECT_DEFAULT | PMD_SECT_UXN | PMD_ATTRINDX(MT_NORMAL))

#define _PAGE_DEFAULT		(_PROT_DEFAULT | PTE_ATTRINDX(MT_NORMAL))

#define PAGE_KERNEL		__pgprot(PROT_NORMAL)
#define PAGE_KERNEL_RO		__pgprot((PROT_NORMAL & ~PTE_WRITE) | PTE_RDONLY)
#define PAGE_KERNEL_ROX		__pgprot((PROT_NORMAL & ~(PTE_WRITE | PTE_PXN)) | PTE_RDONLY)
#define PAGE_KERNEL_EXEC	__pgprot(PROT_NORMAL & ~PTE_PXN)
#define PAGE_KERNEL_EXEC_CONT	__pgprot((PROT_NORMAL & ~PTE_PXN) | PTE_CONT)

#define PAGE_S2_MEMATTR(attr, has_fwb)					\
	({								\
		u64 __val;						\
		if (has_fwb)						\
			__val = PTE_S2_MEMATTR(MT_S2_FWB_ ## attr);	\
		else							\
			__val = PTE_S2_MEMATTR(MT_S2_ ## attr);		\
		__val;							\
	 })

#define PAGE_NONE		__pgprot(((_PAGE_DEFAULT) & ~PTE_VALID) | PTE_PROT_NONE | PTE_RDONLY | PTE_NG | PTE_PXN | PTE_UXN)
/* shared+writable pages are clean by default, hence PTE_RDONLY|PTE_WRITE */
#define PAGE_SHARED		__pgprot(_PAGE_DEFAULT | PTE_USER | PTE_RDONLY | PTE_NG | PTE_PXN | PTE_UXN | PTE_WRITE)
#define PAGE_SHARED_EXEC	__pgprot(_PAGE_DEFAULT | PTE_USER | PTE_RDONLY | PTE_NG | PTE_PXN | PTE_WRITE)
#define PAGE_READONLY		__pgprot(_PAGE_DEFAULT | PTE_USER | PTE_RDONLY | PTE_NG | PTE_PXN | PTE_UXN)
#define PAGE_READONLY_EXEC	__pgprot(_PAGE_DEFAULT | PTE_USER | PTE_RDONLY | PTE_NG | PTE_PXN)

#define __P000  PAGE_NONE
#define __P001  PAGE_READONLY
#define __P010  PAGE_READONLY
#define __P011  PAGE_READONLY
#define __P100  PAGE_READONLY_EXEC
#define __P101  PAGE_READONLY_EXEC
#define __P110  PAGE_READONLY_EXEC
#define __P111  PAGE_READONLY_EXEC

#define __S000  PAGE_NONE
#define __S001  PAGE_READONLY
#define __S010  PAGE_SHARED
#define __S011  PAGE_SHARED
#define __S100  PAGE_READONLY_EXEC
#define __S101  PAGE_READONLY_EXEC
#define __S110  PAGE_SHARED_EXEC
#define __S111  PAGE_SHARED_EXEC

#endif /* __ASSEMBLY__ */

#endif /* __ASM_PGTABLE_PROT_H */

```

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_PGTABLE_HWDEF_H
#define __ASM_PGTABLE_HWDEF_H

#include <asm/memory.h>

/*
 * Number of page-table levels required to address 'va_bits' wide
 * address, without section mapping. We resolve the top (va_bits - PAGE_SHIFT)
 * bits with (PAGE_SHIFT - 3) bits at each page table level. Hence:
 *
 *  levels = DIV_ROUND_UP((va_bits - PAGE_SHIFT), (PAGE_SHIFT - 3))
 *
 * where DIV_ROUND_UP(n, d) => (((n) + (d) - 1) / (d))
 *
 * We cannot include linux/kernel.h which defines DIV_ROUND_UP here
 * due to build issues. So we open code DIV_ROUND_UP here:
 *
 *	((((va_bits) - PAGE_SHIFT) + (PAGE_SHIFT - 3) - 1) / (PAGE_SHIFT - 3))
 *
 * which gets simplified as :
 */
#define ARM64_HW_PGTABLE_LEVELS(va_bits) (((va_bits) - 4) / (PAGE_SHIFT - 3))

/*
 * Size mapped by an entry at level n ( 0 <= n <= 3)
 * We map (PAGE_SHIFT - 3) at all translation levels and PAGE_SHIFT bits
 * in the final page. The maximum number of translation levels supported by
 * the architecture is 4. Hence, starting at level n, we have further
 * ((4 - n) - 1) levels of translation excluding the offset within the page.
 * So, the total number of bits mapped by an entry at level n is :
 *
 *  ((4 - n) - 1) * (PAGE_SHIFT - 3) + PAGE_SHIFT
 *
 * Rearranging it a bit we get :
 *   (4 - n) * (PAGE_SHIFT - 3) + 3
 */
#define ARM64_HW_PGTABLE_LEVEL_SHIFT(n)	((PAGE_SHIFT - 3) * (4 - (n)) + 3)

#define PTRS_PER_PTE		(1 << (PAGE_SHIFT - 3))

/*
 * PMD_SHIFT determines the size a level 2 page table entry can map.
 */
#if CONFIG_PGTABLE_LEVELS > 2
#define PMD_SHIFT		ARM64_HW_PGTABLE_LEVEL_SHIFT(2)
#define PMD_SIZE		(_AC(1, UL) << PMD_SHIFT)
#define PMD_MASK		(~(PMD_SIZE-1))
#define PTRS_PER_PMD		PTRS_PER_PTE
#endif

/*
 * PUD_SHIFT determines the size a level 1 page table entry can map.
 */
#if CONFIG_PGTABLE_LEVELS > 3
#define PUD_SHIFT		ARM64_HW_PGTABLE_LEVEL_SHIFT(1)
#define PUD_SIZE		(_AC(1, UL) << PUD_SHIFT)
#define PUD_MASK		(~(PUD_SIZE-1))
#define PTRS_PER_PUD		PTRS_PER_PTE
#endif

/*
 * PGDIR_SHIFT determines the size a top-level page table entry can map
 * (depending on the configuration, this level can be 0, 1 or 2).
 */
#define PGDIR_SHIFT		ARM64_HW_PGTABLE_LEVEL_SHIFT(4 - CONFIG_PGTABLE_LEVELS)
#define PGDIR_SIZE		(_AC(1, UL) << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE-1))
#define PTRS_PER_PGD		(1 << (VA_BITS - PGDIR_SHIFT))

/*
 * Section address mask and size definitions.
 */
#define SECTION_SHIFT		PMD_SHIFT
#define SECTION_SIZE		(_AC(1, UL) << SECTION_SHIFT)
#define SECTION_MASK		(~(SECTION_SIZE-1))

/*
 * Contiguous page definitions.
 */
#define CONT_PTE_SHIFT		(CONFIG_ARM64_CONT_PTE_SHIFT + PAGE_SHIFT)
#define CONT_PTES		(1 << (CONT_PTE_SHIFT - PAGE_SHIFT))
#define CONT_PTE_SIZE		(CONT_PTES * PAGE_SIZE)
#define CONT_PTE_MASK		(~(CONT_PTE_SIZE - 1))

#define CONT_PMD_SHIFT		(CONFIG_ARM64_CONT_PMD_SHIFT + PMD_SHIFT)
#define CONT_PMDS		(1 << (CONT_PMD_SHIFT - PMD_SHIFT))
#define CONT_PMD_SIZE		(CONT_PMDS * PMD_SIZE)
#define CONT_PMD_MASK		(~(CONT_PMD_SIZE - 1))

/*
 * Hardware page table definitions.
 *
 * Level 1 descriptor (PUD).
 */
#define PUD_TYPE_TABLE		(_AT(pudval_t, 3) << 0)
#define PUD_TABLE_BIT		(_AT(pudval_t, 1) << 1)
#define PUD_TYPE_MASK		(_AT(pudval_t, 3) << 0)
#define PUD_TYPE_SECT		(_AT(pudval_t, 1) << 0)
#define PUD_SECT_RDONLY		(_AT(pudval_t, 1) << 7)		/* AP[2] */

/*
 * Level 2 descriptor (PMD).
 */
#define PMD_TYPE_MASK		(_AT(pmdval_t, 3) << 0)
#define PMD_TYPE_TABLE		(_AT(pmdval_t, 3) << 0)
#define PMD_TYPE_SECT		(_AT(pmdval_t, 1) << 0)
#define PMD_TABLE_BIT		(_AT(pmdval_t, 1) << 1)

/*
 * Section
 */
#define PMD_SECT_VALID		(_AT(pmdval_t, 1) << 0)
#define PMD_SECT_USER		(_AT(pmdval_t, 1) << 6)		/* AP[1] */
#define PMD_SECT_RDONLY		(_AT(pmdval_t, 1) << 7)		/* AP[2] */
#define PMD_SECT_S		(_AT(pmdval_t, 3) << 8)
#define PMD_SECT_AF		(_AT(pmdval_t, 1) << 10)
#define PMD_SECT_NG		(_AT(pmdval_t, 1) << 11)
#define PMD_SECT_CONT		(_AT(pmdval_t, 1) << 52)
#define PMD_SECT_PXN		(_AT(pmdval_t, 1) << 53)
#define PMD_SECT_UXN		(_AT(pmdval_t, 1) << 54)

/*
 * AttrIndx[2:0] encoding (mapping attributes defined in the MAIR* registers).
 */
#define PMD_ATTRINDX(t)		(_AT(pmdval_t, (t)) << 2)
#define PMD_ATTRINDX_MASK	(_AT(pmdval_t, 7) << 2)

/*
 * Level 3 descriptor (PTE).
 */
#define PTE_VALID		(_AT(pteval_t, 1) << 0)
#define PTE_TYPE_MASK		(_AT(pteval_t, 3) << 0)
#define PTE_TYPE_PAGE		(_AT(pteval_t, 3) << 0)
#define PTE_TABLE_BIT		(_AT(pteval_t, 1) << 1)
#define PTE_USER		(_AT(pteval_t, 1) << 6)		/* AP[1] */
#define PTE_RDONLY		(_AT(pteval_t, 1) << 7)		/* AP[2] */
#define PTE_SHARED		(_AT(pteval_t, 3) << 8)		/* SH[1:0], inner shareable */
#define PTE_AF			(_AT(pteval_t, 1) << 10)	/* Access Flag */
#define PTE_NG			(_AT(pteval_t, 1) << 11)	/* nG */
#define PTE_GP			(_AT(pteval_t, 1) << 50)	/* BTI guarded */
#define PTE_DBM			(_AT(pteval_t, 1) << 51)	/* Dirty Bit Management */
#define PTE_CONT		(_AT(pteval_t, 1) << 52)	/* Contiguous range */
#define PTE_PXN			(_AT(pteval_t, 1) << 53)	/* Privileged XN */
#define PTE_UXN			(_AT(pteval_t, 1) << 54)	/* User XN */

#define PTE_ADDR_LOW		(((_AT(pteval_t, 1) << (48 - PAGE_SHIFT)) - 1) << PAGE_SHIFT)
#ifdef CONFIG_ARM64_PA_BITS_52
#define PTE_ADDR_HIGH		(_AT(pteval_t, 0xf) << 12)
#define PTE_ADDR_MASK		(PTE_ADDR_LOW | PTE_ADDR_HIGH)
#else
#define PTE_ADDR_MASK		PTE_ADDR_LOW
#endif

/*
 * AttrIndx[2:0] encoding (mapping attributes defined in the MAIR* registers).
 */
#define PTE_ATTRINDX(t)		(_AT(pteval_t, (t)) << 2)
#define PTE_ATTRINDX_MASK	(_AT(pteval_t, 7) << 2)

/*
 * Memory Attribute override for Stage-2 (MemAttr[3:0])
 */
#define PTE_S2_MEMATTR(t)	(_AT(pteval_t, (t)) << 2)

/*
 * Highest possible physical address supported.
 */
#define PHYS_MASK_SHIFT		(CONFIG_ARM64_PA_BITS)
#define PHYS_MASK		((UL(1) << PHYS_MASK_SHIFT) - 1)

#define TTBR_CNP_BIT		(UL(1) << 0)

/*
 * TCR flags.
 */
#define TCR_T0SZ_OFFSET		0
#define TCR_T1SZ_OFFSET		16
#define TCR_T0SZ(x)		((UL(64) - (x)) << TCR_T0SZ_OFFSET)
#define TCR_T1SZ(x)		((UL(64) - (x)) << TCR_T1SZ_OFFSET)
#define TCR_TxSZ(x)		(TCR_T0SZ(x) | TCR_T1SZ(x))
#define TCR_TxSZ_WIDTH		6
#define TCR_T0SZ_MASK		(((UL(1) << TCR_TxSZ_WIDTH) - 1) << TCR_T0SZ_OFFSET)
#define TCR_T1SZ_MASK		(((UL(1) << TCR_TxSZ_WIDTH) - 1) << TCR_T1SZ_OFFSET)

#define TCR_EPD0_SHIFT		7
#define TCR_EPD0_MASK		(UL(1) << TCR_EPD0_SHIFT)
#define TCR_IRGN0_SHIFT		8
#define TCR_IRGN0_MASK		(UL(3) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_NC		(UL(0) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WBWA		(UL(1) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WT		(UL(2) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WBnWA		(UL(3) << TCR_IRGN0_SHIFT)

#define TCR_EPD1_SHIFT		23
#define TCR_EPD1_MASK		(UL(1) << TCR_EPD1_SHIFT)
#define TCR_IRGN1_SHIFT		24
#define TCR_IRGN1_MASK		(UL(3) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_NC		(UL(0) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WBWA		(UL(1) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WT		(UL(2) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WBnWA		(UL(3) << TCR_IRGN1_SHIFT)

#define TCR_IRGN_NC		(TCR_IRGN0_NC | TCR_IRGN1_NC)
#define TCR_IRGN_WBWA		(TCR_IRGN0_WBWA | TCR_IRGN1_WBWA)
#define TCR_IRGN_WT		(TCR_IRGN0_WT | TCR_IRGN1_WT)
#define TCR_IRGN_WBnWA		(TCR_IRGN0_WBnWA | TCR_IRGN1_WBnWA)
#define TCR_IRGN_MASK		(TCR_IRGN0_MASK | TCR_IRGN1_MASK)


#define TCR_ORGN0_SHIFT		10
#define TCR_ORGN0_MASK		(UL(3) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_NC		(UL(0) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WBWA		(UL(1) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WT		(UL(2) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WBnWA		(UL(3) << TCR_ORGN0_SHIFT)

#define TCR_ORGN1_SHIFT		26
#define TCR_ORGN1_MASK		(UL(3) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_NC		(UL(0) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WBWA		(UL(1) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WT		(UL(2) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WBnWA		(UL(3) << TCR_ORGN1_SHIFT)

#define TCR_ORGN_NC		(TCR_ORGN0_NC | TCR_ORGN1_NC)
#define TCR_ORGN_WBWA		(TCR_ORGN0_WBWA | TCR_ORGN1_WBWA)
#define TCR_ORGN_WT		(TCR_ORGN0_WT | TCR_ORGN1_WT)
#define TCR_ORGN_WBnWA		(TCR_ORGN0_WBnWA | TCR_ORGN1_WBnWA)
#define TCR_ORGN_MASK		(TCR_ORGN0_MASK | TCR_ORGN1_MASK)

#define TCR_SH0_SHIFT		12
#define TCR_SH0_MASK		(UL(3) << TCR_SH0_SHIFT)
#define TCR_SH0_INNER		(UL(3) << TCR_SH0_SHIFT)

#define TCR_SH1_SHIFT		28
#define TCR_SH1_MASK		(UL(3) << TCR_SH1_SHIFT)
#define TCR_SH1_INNER		(UL(3) << TCR_SH1_SHIFT)
#define TCR_SHARED		(TCR_SH0_INNER | TCR_SH1_INNER)

#define TCR_TG0_SHIFT		14
#define TCR_TG0_MASK		(UL(3) << TCR_TG0_SHIFT)
#define TCR_TG0_4K		(UL(0) << TCR_TG0_SHIFT)
#define TCR_TG0_64K		(UL(1) << TCR_TG0_SHIFT)
#define TCR_TG0_16K		(UL(2) << TCR_TG0_SHIFT)

#define TCR_TG1_SHIFT		30
#define TCR_TG1_MASK		(UL(3) << TCR_TG1_SHIFT)
#define TCR_TG1_16K		(UL(1) << TCR_TG1_SHIFT)
#define TCR_TG1_4K		(UL(2) << TCR_TG1_SHIFT)
#define TCR_TG1_64K		(UL(3) << TCR_TG1_SHIFT)

#define TCR_IPS_SHIFT		32
#define TCR_IPS_MASK		(UL(7) << TCR_IPS_SHIFT)
#define TCR_A1			(UL(1) << 22)
#define TCR_ASID16		(UL(1) << 36)
#define TCR_TBI0		(UL(1) << 37)
#define TCR_TBI1		(UL(1) << 38)
#define TCR_HA			(UL(1) << 39)
#define TCR_HD			(UL(1) << 40)
#define TCR_TBID1		(UL(1) << 52)
#define TCR_NFD0		(UL(1) << 53)
#define TCR_NFD1		(UL(1) << 54)
#define TCR_E0PD0		(UL(1) << 55)
#define TCR_E0PD1		(UL(1) << 56)

/*
 * TTBR.
 */
#ifdef CONFIG_ARM64_PA_BITS_52
/*
 * This should be GENMASK_ULL(47, 2).
 * TTBR_ELx[1] is RES0 in this configuration.
 */
#define TTBR_BADDR_MASK_52	(((UL(1) << 46) - 1) << 2)
#endif

#ifdef CONFIG_ARM64_VA_BITS_52
/* Must be at least 64-byte aligned to prevent corruption of the TTBR */
#define TTBR1_BADDR_4852_OFFSET	(((UL(1) << (52 - PGDIR_SHIFT)) - \
				 (UL(1) << (48 - PGDIR_SHIFT))) * 8)
#endif

#endif

```

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\flush.c**
```
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Based on arch/arm/mm/flush.c
 *
 * Copyright (C) 1995-2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 */

#include <linux/export.h>
#include <linux/mm.h>
#include <linux/pagemap.h>

#include <asm/cacheflush.h>
#include <asm/cache.h>
#include <asm/tlbflush.h>

void sync_icache_aliases(void *kaddr, unsigned long len)
{
	unsigned long addr = (unsigned long)kaddr;

	if (icache_is_aliasing()) {
		__clean_dcache_area_pou(kaddr, len);
		__flush_icache_all();
	} else {
		/*
		 * Don't issue kick_all_cpus_sync() after I-cache invalidation
		 * for user mappings.
		 */
		__flush_icache_range(addr, addr + len);
	}
}

static void flush_ptrace_access(struct vm_area_struct *vma, struct page *page,
				unsigned long uaddr, void *kaddr,
				unsigned long len)
{
	if (vma->vm_flags & VM_EXEC)
		sync_icache_aliases(kaddr, len);
}

/*
 * Copy user data from/to a page which is mapped into a different processes
 * address space.  Really, we want to allow our "user space" model to handle
 * this.
 */
void copy_to_user_page(struct vm_area_struct *vma, struct page *page,
		       unsigned long uaddr, void *dst, const void *src,
		       unsigned long len)
{
	memcpy(dst, src, len);
	flush_ptrace_access(vma, page, uaddr, dst, len);
}

void __sync_icache_dcache(pte_t pte)
{
	struct page *page = pte_page(pte);

	if (!test_bit(PG_dcache_clean, &page->flags)) {
		sync_icache_aliases(page_address(page), page_size(page));
		set_bit(PG_dcache_clean, &page->flags);
	}
}
EXPORT_SYMBOL_GPL(__sync_icache_dcache);

/*
 * This function is called when a page has been modified by the kernel. Mark
 * it as dirty for later flushing when mapped in user space (if executable,
 * see __sync_icache_dcache).
 */
void flush_dcache_page(struct page *page)
{
	if (test_bit(PG_dcache_clean, &page->flags))
		clear_bit(PG_dcache_clean, &page->flags);
}
EXPORT_SYMBOL(flush_dcache_page);

/*
 * Additional functions defined in assembly.
 */
EXPORT_SYMBOL(__flush_icache_range);

#ifdef CONFIG_ARCH_HAS_PMEM_API
void arch_wb_cache_pmem(void *addr, size_t size)
{
	/* Ensure order against any prior non-cacheable writes */
	dmb(osh);
	__clean_dcache_area_pop(addr, size);
}
EXPORT_SYMBOL_GPL(arch_wb_cache_pmem);

void arch_invalidate_pmem(void *addr, size_t size)
{
	__inval_dcache_area(addr, size);
}
EXPORT_SYMBOL_GPL(arch_invalidate_pmem);
#endif

```





**Grep Results:**

- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_bugs.py**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump.c**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\bug-audit.md**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_ec.py**
- **C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\wxjump-tutorial.md**


代码和内核头文件都拿到了。接下来我在看 `make_pte`、页表切换逻辑和 fault 路径，确认报告里对 PTE 位、缓存刷新和恢复路径的解释有没有偏差。


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
#define PTE_BASE_FLAGS       0xF13ULL            /* Valid + AF + SH + AP 基础标志 */
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
static void  (*kfn_flush_dcache_page)(void *page);
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

    if (kfn_flush_dcache_page && pi->shadow_page_va)
        kfn_flush_dcache_page((void *)pi->shadow_page_va);

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
    void *vma;
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
        wxjump_put_region(region);
    }

    /* 需要创建新的 shadow page */
    vma = kfn_find_vma(mm, page_addr);
    if (!vma || page_addr < *(unsigned long *)vma) {
        pr_err("wxjump: patch: no VMA for %lx\n", page_addr);
        return -1;
    }

    /* 并发检查 */
    wx_spin_lock();
    struct wx_region *existing;
    list_for_each_entry(existing, &region_list, list) {
        if (existing->mm == mm &&
            page_addr >= existing->vm_start && page_addr < existing->vm_end) {
            wx_spin_unlock();
            return wxjump_do_patch(mm, page_addr, user_buf, len, offset);
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
    new_region->refcount = 1;

    wx_spin_lock();
    list_add(&new_region->list, &region_list);
    wx_spin_unlock();

    /* 设置 shadow page */
    idx = wxjump_page_index(new_region, page_addr);
    pi = &pages[idx];

    uint64_t *pte = get_user_pte(mm, page_addr, NULL);
    if (!pte || !((*pte) & 1)) {
        pr_err("wxjump: patch: no PTE for %lx\n", page_addr);
        return -14;
    }

    uint64_t pte_val = *pte;
    uint64_t orig_pfn = (pte_val >> 12) & PFN_MASK;
    pi->orig_pfn = orig_pfn;

    uint64_t orig_va = wx_pa_to_va(orig_pfn << 12);

    /* 分配 shadow page */
    unsigned long shadow_va = kfn___get_free_pages(wx_gfp_kernel, 0);
    if (!shadow_va)
        return -12;

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
        return ret;
    }

    wx_flush_icache(page_addr);

    pr_info("wxjump: patched new shadow at %lx+%zu (%zu bytes) orig_pfn=%lx shadow_pfn=%lx\n",
            page_addr, offset, len, (unsigned long)orig_pfn, (unsigned long)shadow_pfn);
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

            wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_USER_RDONLY);
            wx_flush_icache(page_addr);  /* 清除 shadow 指令的 I-cache 残留 */
    
            wx_spin_lock();
            pi->shadow_page_va = 0;
            pi->orig_pfn = 0;
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
                    if (pte && (*pte & 1)) {
                        uint64_t new_pte = (pi->orig_pfn << 12) | 0xFC3ULL;
                        *pte = new_pte;
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
    int t0sz = (tcr >> 16) & 0x3F;
    wx_page_offset_base = -1ULL << (63 - t0sz);
    
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
    
    /* Page table config */
    uint64_t tg0 = (tcr >> 30) & 3;
    int bits_per_level;
    if (tg0 == 1) {
        wx_page_shift = 14; bits_per_level = 11;
    } else if (tg0 == 3) {
        wx_page_shift = 16; bits_per_level = 13;
    } else {
        wx_page_shift = 12; bits_per_level = 9;
    }
    wx_page_level = (60 - t0sz) / bits_per_level;
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
    
    /* Cache */
    RESOLVE_REQUIRED("flush_dcache_page", kfn_flush_dcache_page);
    
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
    
    if (!count) {
        wx_spin_unlock();
        pr_info("wxjump: no active regions, unload complete\n");
        return 0;
    }
    
    cleanup_list = (struct wx_region **)kfn_kzalloc(
        count * sizeof(struct wx_region *), wx_gfp_kernel);
    
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
    
            if (vma && pi->orig_pfn &&
                (pi->state == STATE_SHADOW_X || pi->state == STATE_ORIG_R)) {
                if (!wxjump_switch_mapping(vma, addr, pi->orig_pfn, PTE_USER_RDONLY)) {
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

```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_bugs.py**
```
import sys

path = r'C:\Users\24151\Documents\GitHub\xiaojia-hide\KernelPatch\kpms\wxjump\wxjump.c'
with open(path, 'rb') as f:
    data = f.read()

# Fix 1: PTE_BASE_FLAGS AttrIndx=0 -> AttrIndx=4 (MT_NORMAL)
# 0xF03 bits[4:2]=000 -> 0xF13 bits[4:2]=100
old1 = b'#define PTE_BASE_FLAGS       0xF03ULL'
new1 = b'#define PTE_BASE_FLAGS       0xF13ULL'
assert old1 in data, 'PTE_BASE_FLAGS not found'
data = data.replace(old1, new1, 1)
print('Fix 1: PTE_BASE_FLAGS 0xF03 -> 0xF13 (AttrIndx=4 MT_NORMAL)')

# Fix 2: fault handler arg mapping: arg0=far(x0), arg1=esr(x1)
old2 = b'unsigned long far = fargs->arg1;\r\n    unsigned long esr = fargs->arg2;'
new2 = b'unsigned long far = fargs->arg0;    /* x0: FAR_EL1 */\r\n    unsigned long esr = fargs->arg1;    /* x1: ESR_EL1 */'
assert old2 in data, 'fault handler args not found'
data = data.replace(old2, new2, 1)
print('Fix 2: fault handler arg1/arg2 -> arg0/arg1')

# Fix 3: Add debug log after ESR check passes
old3 = b'    wxjump_put_region(region);\r\n\r\n    if ((esr >> 26) == 0x20) {'
new3 = (b'    wxjump_put_region(region);\r\n\r\n'
        b'    pr_info("wxjump: [fault] far=%lx esr=%lx EC=%lx DFSC=%lx\\n",\r\n'
        b'            far, esr, (esr >> 26) & 0x3FUL, esr & 0x3FUL);\r\n\r\n'
        b'    if ((esr >> 26) == 0x20) {')
assert old3 in data, 'fault dispatch block not found'
data = data.replace(old3, new3, 1)
print('Fix 3: Added fault handler debug logging')

with open(path, 'wb') as f:
    f.write(data)
print('All fixes applied successfully')

```

**C:\Users\24151\Documents\GitHub\无痕hook\KernelPatch\kpms\wxjump\fix_ec.py**
```
path = r'C:\Users\24151\Documents\GitHub\xiaojia-hide\KernelPatch\kpms\wxjump\wxjump.c'
with open(path, 'rb') as f:
    data = f.read()

# Replace the fault handler dispatch logic:
# After ESR permission fault check, add EC filter for EL0 only
# EC=0x24: Data Abort from lower EL (EL0)
# EC=0x20: Instruction Abort from lower EL (EL0)
# EC=0x25/0x21: from current EL (EL1) -> skip, let kernel handle

old = (b'    pr_info("wxjump: [fault] far=%lx esr=%lx EC=%lx DFSC=%lx\\n",\r\n'
       b'            far, esr, (esr >> 26) & 0x3FUL, esr & 0x3FUL);\r\n'
       b'\r\n'
       b'    if ((esr >> 26) == 0x20) {')

new = (b'    unsigned long ec = (esr >> 26) & 0x3FUL;\r\n'
       b'    unsigned long dfsc = esr & 0x3FUL;\r\n'
       b'\r\n'
       b'    pr_info("wxjump: [fault] far=%lx esr=%lx EC=%lx DFSC=%lx\\n",\r\n'
       b'            far, esr, ec, dfsc);\r\n'
       b'\r\n'
       b'    /* Only handle EL0 faults. EL1 faults (EC=0x21/0x25) are from kernel\r\n'
       b'     * (e.g. EPAN blocking EL1 access to UXN=0 pages) -> let kernel handle */\r\n'
       b'    if (ec != 0x24 && ec != 0x20) {\r\n'
       b'        pr_info("wxjump: [fault] skipping EL1 fault (EC=%lx)\\n", ec);\r\n'
       b'        kfn_mmput(mm);\r\n'
       b'        return;\r\n'
       b'    }\r\n'
       b'\r\n'
       b'    if (ec == 0x20) {')

assert old in data, 'fault dispatch block not found'
data = data.replace(old, new, 1)
print('Fix: Added EC filter for EL0-only fault handling')

# Also fix the second branch: the old code checked (esr >> 26) == 0x20
# but now we use ec variable, and the data abort check changes
old2 = (b'    } else if (!(esr & 0x40)) {\r\n'
        b'        /* Data Abort, WnR=0')
new2 = (b'    } else if (ec == 0x24 && !(esr & 0x40)) {\r\n'
        b'        /* Data Abort from EL0, WnR=0')
assert old2 in data, 'data abort branch not found'
data = data.replace(old2, new2, 1)
print('Fix: Updated data abort branch to check EC=0x24')

# Also add logging to read/exec fault handlers
old3 = b'    /* switch to orig page: UXN + read-only */'
if old3 not in data:
    old3 = b'    ret = wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_UXN_USER_RO);'
    new3 = (b'    pr_info("wxjump: [read_fault] switching %lx to ORIG_R\\n", page_addr);\r\n'
            b'    ret = wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_UXN_USER_RO);')
    assert old3 in data, 'read fault switch not found'
    data = data.replace(old3, new3, 1)
else:
    # Insert before the comment
    new3 = (b'    pr_info("wxjump: [read_fault] switching %lx to ORIG_R\\n", page_addr);\r\n'
            b'    /* switch to orig page: UXN + read-only */')
    data = data.replace(old3, new3, 1)
print('Fix: Added read_fault logging')

with open(path, 'wb') as f:
    f.write(data)
print('All fixes applied')

```




_Grep error: Grep command exited with non-zero exit code, output:
Select-String : �Ҳ�����������ơ�NoEmphasis��ƥ��Ĳ�����
����λ�� ��:1 �ַ�: 118
+ ... gki-source\common" -Recurse -File | Select-String -NoEmphasis -CaseSe ...
+                                                       ~~~~~~~~~~~
    + CategoryInfo          : InvalidArgument: (:) [Select-String]��ParameterBindingException
    + FullyQualifiedErrorId : NamedParameterNotFound,Microsoft.PowerShell.Commands.SelectStringCommand
 
 _



**Files Read:**

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\memory.h**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/memory.h
 *
 * Copyright (C) 2000-2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * Note: this file should not be included by non-asm/.h files
 */
#ifndef __ASM_MEMORY_H
#define __ASM_MEMORY_H

#include <linux/const.h>
#include <linux/sizes.h>
#include <asm/page-def.h>

/*
 * Size of the PCI I/O space. This must remain a power of two so that
 * IO_SPACE_LIMIT acts as a mask for the low bits of I/O addresses.
 */
#define PCI_IO_SIZE		SZ_16M

/*
 * VMEMMAP_SIZE - allows the whole linear region to be covered by
 *                a struct page array
 *
 * If we are configured with a 52-bit kernel VA then our VMEMMAP_SIZE
 * needs to cover the memory region from the beginning of the 52-bit
 * PAGE_OFFSET all the way to PAGE_END for 48-bit. This allows us to
 * keep a constant PAGE_OFFSET and "fallback" to using the higher end
 * of the VMEMMAP where 52-bit support is not available in hardware.
 */
#define VMEMMAP_SIZE ((_PAGE_END(VA_BITS_MIN) - PAGE_OFFSET) \
			>> (PAGE_SHIFT - STRUCT_PAGE_MAX_SHIFT))

/*
 * PAGE_OFFSET - the virtual address of the start of the linear map, at the
 *               start of the TTBR1 address space.
 * PAGE_END - the end of the linear map, where all other kernel mappings begin.
 * KIMAGE_VADDR - the virtual address of the start of the kernel image.
 * VA_BITS - the maximum number of bits for virtual addresses.
 */
#define VA_BITS			(CONFIG_ARM64_VA_BITS)
#define _PAGE_OFFSET(va)	(-(UL(1) << (va)))
#define PAGE_OFFSET		(_PAGE_OFFSET(VA_BITS))
#define KIMAGE_VADDR		(MODULES_END)
#define BPF_JIT_REGION_START	(KASAN_SHADOW_END)
#define BPF_JIT_REGION_SIZE	(SZ_128M)
#define BPF_JIT_REGION_END	(BPF_JIT_REGION_START + BPF_JIT_REGION_SIZE)
#define MODULES_END		(MODULES_VADDR + MODULES_VSIZE)
#define MODULES_VADDR		(BPF_JIT_REGION_END)
#define MODULES_VSIZE		(SZ_128M)
#define VMEMMAP_START		(-VMEMMAP_SIZE - SZ_2M)
#define VMEMMAP_END		(VMEMMAP_START + VMEMMAP_SIZE)
#define PCI_IO_END		(VMEMMAP_START - SZ_2M)
#define PCI_IO_START		(PCI_IO_END - PCI_IO_SIZE)
#define FIXADDR_TOP		(PCI_IO_START - SZ_2M)

#if VA_BITS > 48
#define VA_BITS_MIN		(48)
#else
#define VA_BITS_MIN		(VA_BITS)
#endif

#define _PAGE_END(va)		(-(UL(1) << ((va) - 1)))

#define KERNEL_START		_text
#define KERNEL_END		_end

/*
 * Generic and tag-based KASAN require 1/8th and 1/16th of the kernel virtual
 * address space for the shadow region respectively. They can bloat the stack
 * significantly, so double the (minimum) stack size when they are in use.
 */
#if defined(CONFIG_KASAN_GENERIC) || defined(CONFIG_KASAN_SW_TAGS)
#define KASAN_SHADOW_OFFSET	_AC(CONFIG_KASAN_SHADOW_OFFSET, UL)
#define KASAN_SHADOW_END	((UL(1) << (64 - KASAN_SHADOW_SCALE_SHIFT)) \
					+ KASAN_SHADOW_OFFSET)
#define KASAN_THREAD_SHIFT	1
#else
#define KASAN_THREAD_SHIFT	0
#define KASAN_SHADOW_END	(_PAGE_END(VA_BITS_MIN))
#endif /* CONFIG_KASAN */

#define MIN_THREAD_SHIFT	(14 + KASAN_THREAD_SHIFT)

/*
 * VMAP'd stacks are allocated at page granularity, so we must ensure that such
 * stacks are a multiple of page size.
 */
#if defined(CONFIG_VMAP_STACK) && (MIN_THREAD_SHIFT < PAGE_SHIFT)
#define THREAD_SHIFT		PAGE_SHIFT
#else
#define THREAD_SHIFT		MIN_THREAD_SHIFT
#endif

#if THREAD_SHIFT >= PAGE_SHIFT
#define THREAD_SIZE_ORDER	(THREAD_SHIFT - PAGE_SHIFT)
#endif

#define THREAD_SIZE		(UL(1) << THREAD_SHIFT)

/*
 * By aligning VMAP'd stacks to 2 * THREAD_SIZE, we can detect overflow by
 * checking sp & (1 << THREAD_SHIFT), which we can do cheaply in the entry
 * assembly.
 */
#ifdef CONFIG_VMAP_STACK
#define THREAD_ALIGN		(2 * THREAD_SIZE)
#else
#define THREAD_ALIGN		THREAD_SIZE
#endif

#define IRQ_STACK_SIZE		THREAD_SIZE

#define OVERFLOW_STACK_SIZE	SZ_4K

/*
 * Alignment of kernel segments (e.g. .text, .data).
 *
 *  4 KB granule:  16 level 3 entries, with contiguous bit
 * 16 KB granule:   4 level 3 entries, without contiguous bit
 * 64 KB granule:   1 level 3 entry
 */
#define SEGMENT_ALIGN		SZ_64K

/*
 * Memory types available.
 *
 * IMPORTANT: MT_NORMAL must be index 0 since vm_get_page_prot() may 'or' in
 *	      the MT_NORMAL_TAGGED memory type for PROT_MTE mappings. Note
 *	      that protection_map[] only contains MT_NORMAL attributes.
 */
#define MT_NORMAL		0
#define MT_NORMAL_TAGGED	1
#define MT_NORMAL_NC		2
#define MT_NORMAL_WT		3
#define MT_DEVICE_nGnRnE	4
#define MT_DEVICE_nGnRE		5
#define MT_DEVICE_GRE		6
#define MT_NORMAL_iNC_oWB	7

/*
 * Memory types for Stage-2 translation
 */
#define MT_S2_NORMAL		0xf
#define MT_S2_DEVICE_nGnRE	0x1

/*
 * Memory types for Stage-2 translation when ID_AA64MMFR2_EL1.FWB is 0001
 * Stage-2 enforces Normal-WB and Device-nGnRE
 */
#define MT_S2_FWB_NORMAL	6
#define MT_S2_FWB_DEVICE_nGnRE	1

#ifdef CONFIG_ARM64_4K_PAGES
#define IOREMAP_MAX_ORDER	(PUD_SHIFT)
#else
#define IOREMAP_MAX_ORDER	(PMD_SHIFT)
#endif

#ifndef __ASSEMBLY__

#include <linux/bitops.h>
#include <linux/compiler.h>
#include <linux/mmdebug.h>
#include <linux/types.h>
#include <asm/bug.h>

extern u64			vabits_actual;
#define PAGE_END		(_PAGE_END(vabits_actual))

extern s64			memstart_addr;
/* PHYS_OFFSET - the physical address of the start of memory. */
#define PHYS_OFFSET		({ VM_BUG_ON(memstart_addr & 1); memstart_addr; })

/* the virtual base of the kernel image */
extern u64			kimage_vaddr;

/* the offset between the kernel virtual and physical mappings */
extern u64			kimage_voffset;

static inline unsigned long kaslr_offset(void)
{
	return kimage_vaddr - KIMAGE_VADDR;
}

/*
 * Allow all memory at the discovery stage. We will clip it later.
 */
#define MIN_MEMBLOCK_ADDR	0
#define MAX_MEMBLOCK_ADDR	U64_MAX

/*
 * PFNs are used to describe any physical page; this means
 * PFN 0 == physical address 0.
 *
 * This is the PFN of the first RAM page in the kernel
 * direct-mapped view.  We assume this is the first page
 * of RAM in the mem_map as well.
 */
#define PHYS_PFN_OFFSET	(PHYS_OFFSET >> PAGE_SHIFT)

/*
 * When dealing with data aborts, watchpoints, or instruction traps we may end
 * up with a tagged userland pointer. Clear the tag to get a sane pointer to
 * pass on to access_ok(), for instance.
 */
#define __untagged_addr(addr)	\
	((__force __typeof__(addr))sign_extend64((__force u64)(addr), 55))

#define untagged_addr(addr)	({					\
	u64 __addr = (__force u64)(addr);					\
	__addr &= __untagged_addr(__addr);				\
	(__force __typeof__(addr))__addr;				\
})

#if defined(CONFIG_KASAN_SW_TAGS) || defined(CONFIG_KASAN_HW_TAGS)
#define __tag_shifted(tag)	((u64)(tag) << 56)
#define __tag_reset(addr)	__untagged_addr(addr)
#define __tag_get(addr)		(__u8)((u64)(addr) >> 56)
#else
#define __tag_shifted(tag)	0UL
#define __tag_reset(addr)	(addr)
#define __tag_get(addr)		0
#endif /* CONFIG_KASAN_SW_TAGS || CONFIG_KASAN_HW_TAGS */

static inline const void *__tag_set(const void *addr, u8 tag)
{
	u64 __addr = (u64)addr & ~__tag_shifted(0xff);
	return (const void *)(__addr | __tag_shifted(tag));
}

#ifdef CONFIG_KASAN_HW_TAGS
#define arch_enable_tagging_sync()		mte_enable_kernel_sync()
#define arch_enable_tagging_async()		mte_enable_kernel_async()
#define arch_set_tagging_report_once(state)	mte_set_report_once(state)
#define arch_force_async_tag_fault()		mte_check_tfsr_exit()
#define arch_get_random_tag()			mte_get_random_tag()
#define arch_get_mem_tag(addr)			mte_get_mem_tag(addr)
#define arch_set_mem_tag_range(addr, size, tag, init)	\
			mte_set_mem_tag_range((addr), (size), (tag), (init))
#endif /* CONFIG_KASAN_HW_TAGS */

/*
 * Physical vs virtual RAM address space conversion.  These are
 * private definitions which should NOT be used outside memory.h
 * files.  Use virt_to_phys/phys_to_virt/__pa/__va instead.
 */


/*
 * Check whether an arbitrary address is within the linear map, which
 * lives in the [PAGE_OFFSET, PAGE_END) interval at the bottom of the
 * kernel's TTBR1 address range.
 */
#define __is_lm_address(addr)	(((u64)(addr) ^ PAGE_OFFSET) < (PAGE_END - PAGE_OFFSET))

#define __lm_to_phys(addr)	(((addr) & ~PAGE_OFFSET) + PHYS_OFFSET)
#define __kimg_to_phys(addr)	((addr) - kimage_voffset)

#define __virt_to_phys_nodebug(x) ({					\
	phys_addr_t __x = (phys_addr_t)(__tag_reset(x));		\
	__is_lm_address(__x) ? __lm_to_phys(__x) : __kimg_to_phys(__x);	\
})

#define __pa_symbol_nodebug(x)	__kimg_to_phys((phys_addr_t)(x))

#ifdef CONFIG_DEBUG_VIRTUAL
extern phys_addr_t __virt_to_phys(unsigned long x);
extern phys_addr_t __phys_addr_symbol(unsigned long x);
#else
#define __virt_to_phys(x)	__virt_to_phys_nodebug(x)
#define __phys_addr_symbol(x)	__pa_symbol_nodebug(x)
#endif /* CONFIG_DEBUG_VIRTUAL */

#define __phys_to_virt(x)	((unsigned long)((x) - PHYS_OFFSET) | PAGE_OFFSET)
#define __phys_to_kimg(x)	((unsigned long)((x) + kimage_voffset))

/*
 * Convert a page to/from a physical address
 */
#define page_to_phys(page)	(__pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys)	(pfn_to_page(__phys_to_pfn(phys)))

/*
 * Note: Drivers should NOT use these.  They are the wrong
 * translation for translating DMA addresses.  Use the driver
 * DMA support - see dma-mapping.h.
 */
#define virt_to_phys virt_to_phys
static inline phys_addr_t virt_to_phys(const volatile void *x)
{
	return __virt_to_phys((unsigned long)(x));
}

#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(phys_addr_t x)
{
	return (void *)(__phys_to_virt(x));
}

/*
 * Drivers should NOT use these either.
 */
#define __pa(x)			__virt_to_phys((unsigned long)(x))
#define __pa_symbol(x)		__phys_addr_symbol(RELOC_HIDE((unsigned long)(x), 0))
#define __pa_nodebug(x)		__virt_to_phys_nodebug((unsigned long)(x))
#define __va(x)			((void *)__phys_to_virt((phys_addr_t)(x)))
#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)
#define virt_to_pfn(x)		__phys_to_pfn(__virt_to_phys((unsigned long)(x)))
#define sym_to_pfn(x)		__phys_to_pfn(__pa_symbol(x))

/*
 * With non-canonical CFI jump tables, the compiler replaces function
 * address references with the address of the function's CFI jump
 * table entry. This results in __pa_symbol(function) returning the
 * physical address of the jump table entry, which can lead to address
 * space confusion since the jump table points to the function's
 * virtual address. Therefore, use inline assembly to ensure we are
 * always taking the address of the actual function.
 */
#define __va_function(x) ({						\
	void *addr;							\
	asm("adrp %0, " __stringify(x) "\n\t"				\
	    "add  %0, %0, :lo12:" __stringify(x) : "=r" (addr));	\
	addr;								\
})

#define __pa_function(x) 	__pa_symbol(__va_function(x))

/*
 *  virt_to_page(x)	convert a _valid_ virtual address to struct page *
 *  virt_addr_valid(x)	indicates whether a virtual address is valid
 */
#define ARCH_PFN_OFFSET		((unsigned long)PHYS_PFN_OFFSET)

#if !defined(CONFIG_SPARSEMEM_VMEMMAP) || defined(CONFIG_DEBUG_VIRTUAL)
#define page_to_virt(x)	({						\
	__typeof__(x) __page = x;					\
	void *__addr = __va(page_to_phys(__page));			\
	(void *)__tag_set((const void *)__addr, page_kasan_tag(__page));\
})
#define virt_to_page(x)		pfn_to_page(virt_to_pfn(x))
#else
#define page_to_virt(x)	({						\
	__typeof__(x) __page = x;					\
	u64 __idx = ((u64)__page - VMEMMAP_START) / sizeof(struct page);\
	u64 __addr = PAGE_OFFSET + (__idx * PAGE_SIZE);			\
	(void *)__tag_set((const void *)__addr, page_kasan_tag(__page));\
})

#define virt_to_page(x)	({						\
	u64 __idx = (__tag_reset((u64)x) - PAGE_OFFSET) / PAGE_SIZE;	\
	u64 __addr = VMEMMAP_START + (__idx * sizeof(struct page));	\
	(struct page *)__addr;						\
})
#endif /* !CONFIG_SPARSEMEM_VMEMMAP || CONFIG_DEBUG_VIRTUAL */

#define virt_addr_valid(addr)	({					\
	__typeof__(addr) __addr = __tag_reset(addr);			\
	__is_lm_address(__addr) && pfn_valid(virt_to_pfn(__addr));	\
})

void dump_mem_limit(void);
#endif /* !ASSEMBLY */

/*
 * Given that the GIC architecture permits ITS implementations that can only be
 * configured with a LPI table address once, GICv3 systems with many CPUs may
 * end up reserving a lot of different regions after a kexec for their LPI
 * tables (one per CPU), as we are forced to reuse the same memory after kexec
 * (and thus reserve it persistently with EFI beforehand)
 */
#if defined(CONFIG_EFI) && defined(CONFIG_ARM_GIC_V3_ITS)
# define INIT_MEMBLOCK_RESERVED_REGIONS	(INIT_MEMBLOCK_REGIONS + NR_CPUS + 1)
#endif

#include <asm-generic/memory_model.h>

#endif /* __ASM_MEMORY_H */

```



我还在核对 fallback 分支是否真“当前不会触发”。这需要确认 `physvirt_offset` 在这套源码里是否存在，以及代码是不是优先走了别的路径。

**Command Executed:**
​```bash
rg -n "physvirt_offset|memstart_addr|CONFIG_ARM64_VA_BITS" "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common"
```

**Output:**
```
rg : 无法将“rg”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。请检查名称的拼写，如果包括路径，请确保路径正确，然后再试一次。
所在位置 行:1 字符: 1
+ rg -n "physvirt_offset|memstart_addr|CONFIG_ARM64_VA_BITS" "C:\Users\ ...
+ ~~
    + CategoryInfo          : ObjectNotFound: (rg:String) [], CommandNotFoundException
    + FullyQualifiedErrorId : CommandNotFoundException
 
```


```bash
Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common" -Recurse -File | Select-String -Pattern "physvirt_offset|memstart_addr|CONFIG_ARM64_VA_BITS" | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() }
```

**Current Output:**
```

<|cursor|>
```

Subagent: 1eb52a1b-d316-44aa-8e34-cd5a4d482834


_Read files error: These files do not exist: C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\generated\autoconf.h _



**Files Read:**

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\init.c**
```
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Based on arch/arm/mm/init.c
 *
 * Copyright (C) 1995-2005 Russell King
 * Copyright (C) 2012 ARM Ltd.
 */

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/swap.h>
#include <linux/init.h>
#include <linux/cache.h>
#include <linux/mman.h>
#include <linux/nodemask.h>
#include <linux/initrd.h>
#include <linux/gfp.h>
#include <linux/memblock.h>
#include <linux/sort.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/dma-direct.h>
#include <linux/dma-map-ops.h>
#include <linux/efi.h>
#include <linux/swiotlb.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/kexec.h>
#include <linux/crash_dump.h>
#include <linux/hugetlb.h>
#include <linux/acpi_iort.h>

#include <asm/boot.h>
#include <asm/fixmap.h>
#include <asm/kasan.h>
#include <asm/kernel-pgtable.h>
#include <asm/kvm_host.h>
#include <asm/memory.h>
#include <asm/numa.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <linux/sizes.h>
#include <asm/tlb.h>
#include <asm/alternative.h>

/*
 * We need to be able to catch inadvertent references to memstart_addr
 * that occur (potentially in generic code) before arm64_memblock_init()
 * executes, which assigns it its actual value. So use a default value
 * that cannot be mistaken for a real physical address.
 */
s64 memstart_addr __ro_after_init = -1;
EXPORT_SYMBOL(memstart_addr);

/*
 * If the corresponding config options are enabled, we create both ZONE_DMA
 * and ZONE_DMA32. By default ZONE_DMA covers the 32-bit addressable memory
 * unless restricted on specific platforms (e.g. 30-bit on Raspberry Pi 4).
 * In such case, ZONE_DMA32 covers the rest of the 32-bit addressable memory,
 * otherwise it is empty.
 */
phys_addr_t arm64_dma_phys_limit __ro_after_init;

/*
 * Provide a run-time mean of disabling ZONE_DMA32 if it is enabled via
 * CONFIG_ZONE_DMA32.
 */
static bool disable_dma32 __ro_after_init;

#ifdef CONFIG_KEXEC_CORE
/*
 * reserve_crashkernel() - reserves memory for crash kernel
 *
 * This function reserves memory area given in "crashkernel=" kernel command
 * line parameter. The memory reserved is used by dump capture kernel when
 * primary kernel is crashing.
 */
static void __init reserve_crashkernel(void)
{
	unsigned long long crash_base, crash_size;
	int ret;

	ret = parse_crashkernel(boot_command_line, memblock_phys_mem_size(),
				&crash_size, &crash_base);
	/* no crashkernel= or invalid value specified */
	if (ret || !crash_size)
		return;

	crash_size = PAGE_ALIGN(crash_size);

	if (crash_base == 0) {
		/* Current arm64 boot protocol requires 2MB alignment */
		crash_base = memblock_find_in_range(0, arm64_dma_phys_limit,
				crash_size, SZ_2M);
		if (crash_base == 0) {
			pr_warn("cannot allocate crashkernel (size:0x%llx)\n",
				crash_size);
			return;
		}
	} else {
		/* User specifies base address explicitly. */
		if (!memblock_is_region_memory(crash_base, crash_size)) {
			pr_warn("cannot reserve crashkernel: region is not memory\n");
			return;
		}

		if (memblock_is_region_reserved(crash_base, crash_size)) {
			pr_warn("cannot reserve crashkernel: region overlaps reserved memory\n");
			return;
		}

		if (!IS_ALIGNED(crash_base, SZ_2M)) {
			pr_warn("cannot reserve crashkernel: base address is not 2MB aligned\n");
			return;
		}
	}
	memblock_reserve(crash_base, crash_size);

	pr_info("crashkernel reserved: 0x%016llx - 0x%016llx (%lld MB)\n",
		crash_base, crash_base + crash_size, crash_size >> 20);

	crashk_res.start = crash_base;
	crashk_res.end = crash_base + crash_size - 1;
}
#else
static void __init reserve_crashkernel(void)
{
}
#endif /* CONFIG_KEXEC_CORE */

#ifdef CONFIG_CRASH_DUMP
static int __init early_init_dt_scan_elfcorehdr(unsigned long node,
		const char *uname, int depth, void *data)
{
	const __be32 *reg;
	int len;

	if (depth != 1 || strcmp(uname, "chosen") != 0)
		return 0;

	reg = of_get_flat_dt_prop(node, "linux,elfcorehdr", &len);
	if (!reg || (len < (dt_root_addr_cells + dt_root_size_cells)))
		return 1;

	elfcorehdr_addr = dt_mem_next_cell(dt_root_addr_cells, &reg);
	elfcorehdr_size = dt_mem_next_cell(dt_root_size_cells, &reg);

	return 1;
}

/*
 * reserve_elfcorehdr() - reserves memory for elf core header
 *
 * This function reserves the memory occupied by an elf core header
 * described in the device tree. This region contains all the
 * information about primary kernel's core image and is used by a dump
 * capture kernel to access the system memory on primary kernel.
 */
static void __init reserve_elfcorehdr(void)
{
	of_scan_flat_dt(early_init_dt_scan_elfcorehdr, NULL);

	if (!elfcorehdr_size)
		return;

	if (memblock_is_region_reserved(elfcorehdr_addr, elfcorehdr_size)) {
		pr_warn("elfcorehdr is overlapped\n");
		return;
	}

	memblock_reserve(elfcorehdr_addr, elfcorehdr_size);

	pr_info("Reserving %lldKB of memory at 0x%llx for elfcorehdr\n",
		elfcorehdr_size >> 10, elfcorehdr_addr);
}
#else
static void __init reserve_elfcorehdr(void)
{
}
#endif /* CONFIG_CRASH_DUMP */

/*
 * Return the maximum physical address for a zone accessible by the given bits
 * limit. If DRAM starts above 32-bit, expand the zone to the maximum
 * available memory, otherwise cap it at 32-bit.
 */
static phys_addr_t __init max_zone_phys(unsigned int zone_bits)
{
	phys_addr_t zone_mask = DMA_BIT_MASK(zone_bits);
	phys_addr_t phys_start = memblock_start_of_DRAM();

	if (phys_start > U32_MAX)
		zone_mask = PHYS_ADDR_MAX;
	else if (phys_start > zone_mask)
		zone_mask = U32_MAX;

	return min(zone_mask, memblock_end_of_DRAM() - 1) + 1;
}

static void __init zone_sizes_init(unsigned long min, unsigned long max)
{
	unsigned long max_zone_pfns[MAX_NR_ZONES]  = {0};
	unsigned int __maybe_unused acpi_zone_dma_bits;
	unsigned int __maybe_unused dt_zone_dma_bits;
	phys_addr_t __maybe_unused dma32_phys_limit = max_zone_phys(32);

#ifdef CONFIG_ZONE_DMA
	acpi_zone_dma_bits = fls64(acpi_iort_dma_get_max_cpu_address());
	dt_zone_dma_bits = fls64(of_dma_get_max_cpu_address(NULL));
	zone_dma_bits = min3(32U, dt_zone_dma_bits, acpi_zone_dma_bits);
	arm64_dma_phys_limit = max_zone_phys(zone_dma_bits);
	max_zone_pfns[ZONE_DMA] = PFN_DOWN(arm64_dma_phys_limit);
#endif
#ifdef CONFIG_ZONE_DMA32
	max_zone_pfns[ZONE_DMA32] = disable_dma32 ? 0 : PFN_DOWN(dma32_phys_limit);
	if (!arm64_dma_phys_limit)
		arm64_dma_phys_limit = dma32_phys_limit;
#endif
	if (!arm64_dma_phys_limit)
		arm64_dma_phys_limit = PHYS_MASK + 1;
	max_zone_pfns[ZONE_NORMAL] = max;

	free_area_init(max_zone_pfns);
}

static int __init early_disable_dma32(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (!strcmp(buf, "on"))
		disable_dma32 = true;

	return 0;
}
early_param("disable_dma32", early_disable_dma32);

int pfn_valid(unsigned long pfn)
{
	phys_addr_t addr = pfn << PAGE_SHIFT;

	if ((addr >> PAGE_SHIFT) != pfn)
		return 0;

#ifdef CONFIG_SPARSEMEM
	if (pfn_to_section_nr(pfn) >= NR_MEM_SECTIONS)
		return 0;

	if (!valid_section(__pfn_to_section(pfn)))
		return 0;

	/*
	 * ZONE_DEVICE memory does not have the memblock entries.
	 * memblock_is_map_memory() check for ZONE_DEVICE based
	 * addresses will always fail. Even the normal hotplugged
	 * memory will never have MEMBLOCK_NOMAP flag set in their
	 * memblock entries. Skip memblock search for all non early
	 * memory sections covering all of hotplug memory including
	 * both normal and ZONE_DEVICE based.
	 */
	if (!early_section(__pfn_to_section(pfn)))
		return pfn_section_valid(__pfn_to_section(pfn), pfn);
#endif
	return memblock_is_map_memory(addr);
}
EXPORT_SYMBOL(pfn_valid);

static phys_addr_t memory_limit = PHYS_ADDR_MAX;

/*
 * Limit the memory size that was specified via FDT.
 */
static int __init early_mem(char *p)
{
	if (!p)
		return 1;

	memory_limit = memparse(p, &p) & PAGE_MASK;
	pr_notice("Memory limited to %lldMB\n", memory_limit >> 20);

	return 0;
}
early_param("mem", early_mem);

static int __init early_init_dt_scan_usablemem(unsigned long node,
		const char *uname, int depth, void *data)
{
	struct memblock_region *usablemem = data;
	const __be32 *reg;
	int len;

	if (depth != 1 || strcmp(uname, "chosen") != 0)
		return 0;

	reg = of_get_flat_dt_prop(node, "linux,usable-memory-range", &len);
	if (!reg || (len < (dt_root_addr_cells + dt_root_size_cells)))
		return 1;

	usablemem->base = dt_mem_next_cell(dt_root_addr_cells, &reg);
	usablemem->size = dt_mem_next_cell(dt_root_size_cells, &reg);

	return 1;
}

static void __init fdt_enforce_memory_region(void)
{
	struct memblock_region reg = {
		.size = 0,
	};

	of_scan_flat_dt(early_init_dt_scan_usablemem, &reg);

	if (reg.size)
		memblock_cap_memory_range(reg.base, reg.size);
}

void __init arm64_memblock_init(void)
{
	const s64 linear_region_size = BIT(vabits_actual - 1);

	/* Handle linux,usable-memory-range property */
	fdt_enforce_memory_region();

	/* Remove memory above our supported physical address size */
	memblock_remove(1ULL << PHYS_MASK_SHIFT, ULLONG_MAX);

	/*
	 * Select a suitable value for the base of physical memory.
	 */
	memstart_addr = round_down(memblock_start_of_DRAM(),
				   ARM64_MEMSTART_ALIGN);

	/*
	 * Remove the memory that we will not be able to cover with the
	 * linear mapping. Take care not to clip the kernel which may be
	 * high in memory.
	 */
	memblock_remove(max_t(u64, memstart_addr + linear_region_size,
			__pa_symbol(_end)), ULLONG_MAX);
	if (memstart_addr + linear_region_size < memblock_end_of_DRAM()) {
		/* ensure that memstart_addr remains sufficiently aligned */
		memstart_addr = round_up(memblock_end_of_DRAM() - linear_region_size,
					 ARM64_MEMSTART_ALIGN);
		memblock_remove(0, memstart_addr);
	}

	/*
	 * If we are running with a 52-bit kernel VA config on a system that
	 * does not support it, we have to place the available physical
	 * memory in the 48-bit addressable part of the linear region, i.e.,
	 * we have to move it upward. Since memstart_addr represents the
	 * physical address of PAGE_OFFSET, we have to *subtract* from it.
	 */
	if (IS_ENABLED(CONFIG_ARM64_VA_BITS_52) && (vabits_actual != 52))
		memstart_addr -= _PAGE_OFFSET(48) - _PAGE_OFFSET(52);

	/*
	 * Apply the memory limit if it was set. Since the kernel may be loaded
	 * high up in memory, add back the kernel region that must be accessible
	 * via the linear mapping.
	 */
	if (memory_limit != PHYS_ADDR_MAX) {
		memblock_mem_limit_remove_map(memory_limit);
		memblock_add(__pa_symbol(_text), (u64)(_end - _text));
	}

	if (IS_ENABLED(CONFIG_BLK_DEV_INITRD) && phys_initrd_size) {
		/*
		 * Add back the memory we just removed if it results in the
		 * initrd to become inaccessible via the linear mapping.
		 * Otherwise, this is a no-op
		 */
		u64 base = phys_initrd_start & PAGE_MASK;
		u64 size = PAGE_ALIGN(phys_initrd_start + phys_initrd_size) - base;

		/*
		 * We can only add back the initrd memory if we don't end up
		 * with more memory than we can address via the linear mapping.
		 * It is up to the bootloader to position the kernel and the
		 * initrd reasonably close to each other (i.e., within 32 GB of
		 * each other) so that all granule/#levels combinations can
		 * always access both.
		 */
		if (WARN(base < memblock_start_of_DRAM() ||
			 base + size > memblock_start_of_DRAM() +
				       linear_region_size,
			"initrd not fully accessible via the linear mapping -- please check your bootloader ...\n")) {
			phys_initrd_size = 0;
		} else {
			memblock_remove(base, size); /* clear MEMBLOCK_ flags */
			memblock_add(base, size);
			memblock_reserve(base, size);
		}
	}

	if (IS_ENABLED(CONFIG_RANDOMIZE_BASE)) {
		extern u16 memstart_offset_seed;
		u64 mmfr0 = read_cpuid(ID_AA64MMFR0_EL1);
		int parange = cpuid_feature_extract_unsigned_field(
					mmfr0, ID_AA64MMFR0_PARANGE_SHIFT);
		s64 range = linear_region_size -
			    BIT(id_aa64mmfr0_parange_to_phys_shift(parange));

		/*
		 * If the size of the linear region exceeds, by a sufficient
		 * margin, the size of the region that the physical memory can
		 * span, randomize the linear region as well.
		 */
		if (memstart_offset_seed > 0 && range >= (s64)ARM64_MEMSTART_ALIGN) {
			range /= ARM64_MEMSTART_ALIGN;
			memstart_addr -= ARM64_MEMSTART_ALIGN *
					 ((range * memstart_offset_seed) >> 16);
		}
	}

	/*
	 * Register the kernel text, kernel data, initrd, and initial
	 * pagetables with memblock.
	 */
	memblock_reserve(__pa_symbol(_text), _end - _text);
	if (IS_ENABLED(CONFIG_BLK_DEV_INITRD) && phys_initrd_size) {
		/* the generic initrd code expects virtual addresses */
		initrd_start = __phys_to_virt(phys_initrd_start);
		initrd_end = initrd_start + phys_initrd_size;
	}

	early_init_fdt_scan_reserved_mem();

	reserve_elfcorehdr();

	high_memory = __va(memblock_end_of_DRAM() - 1) + 1;
}

void __init bootmem_init(void)
{
	unsigned long min, max;

	min = PFN_UP(memblock_start_of_DRAM());
	max = PFN_DOWN(memblock_end_of_DRAM());

	early_memtest(min << PAGE_SHIFT, max << PAGE_SHIFT);

	max_pfn = max_low_pfn = max;
	min_low_pfn = min;

	arm64_numa_init();

	/*
	 * must be done after arm64_numa_init() which calls numa_init() to
	 * initialize node_online_map that gets used in hugetlb_cma_reserve()
	 * while allocating required CMA size across online nodes.
	 */
#if defined(CONFIG_HUGETLB_PAGE) && defined(CONFIG_CMA)
	arm64_hugetlb_cma_reserve();
#endif

	dma_pernuma_cma_reserve();

	kvm_hyp_reserve();

	/*
	 * sparse_init() tries to allocate memory from memblock, so must be
	 * done after the fixed reservations
	 */
	sparse_init();
	zone_sizes_init(min, max);

	/*
	 * Reserve the CMA area after arm64_dma_phys_limit was initialised.
	 */
	dma_contiguous_reserve(arm64_dma_phys_limit);

	/*
	 * request_standard_resources() depends on crashkernel's memory being
	 * reserved, so do it here.
	 */
	reserve_crashkernel();

	memblock_dump_all();
}

#ifndef CONFIG_SPARSEMEM_VMEMMAP
static inline void free_memmap(unsigned long start_pfn, unsigned long end_pfn)
{
	struct page *start_pg, *end_pg;
	unsigned long pg, pgend;

	/*
	 * Convert start_pfn/end_pfn to a struct page pointer.
	 */
	start_pg = pfn_to_page(start_pfn - 1) + 1;
	end_pg = pfn_to_page(end_pfn - 1) + 1;

	/*
	 * Convert to physical addresses, and round start upwards and end
	 * downwards.
	 */
	pg = (unsigned long)PAGE_ALIGN(__pa(start_pg));
	pgend = (unsigned long)__pa(end_pg) & PAGE_MASK;

	/*
	 * If there are free pages between these, free the section of the
	 * memmap array.
	 */
	if (pg < pgend)
		memblock_free(pg, pgend - pg);
}

/*
 * The mem_map array can get very big. Free the unused area of the memory map.
 */
static void __init free_unused_memmap(void)
{
	unsigned long start, end, prev_end = 0;
	int i;

	for_each_mem_pfn_range(i, MAX_NUMNODES, &start, &end, NULL) {
#ifdef CONFIG_SPARSEMEM
		/*
		 * Take care not to free memmap entries that don't exist due
		 * to SPARSEMEM sections which aren't present.
		 */
		start = min(start, ALIGN(prev_end, PAGES_PER_SECTION));
#endif
		/*
		 * If we had a previous bank, and there is a space between the
		 * current bank and the previous, free it.
		 */
		if (prev_end && prev_end < start)
			free_memmap(prev_end, start);

		/*
		 * Align up here since the VM subsystem insists that the
		 * memmap entries are valid from the bank end aligned to
		 * MAX_ORDER_NR_PAGES.
		 */
		prev_end = ALIGN(end, MAX_ORDER_NR_PAGES);
	}

#ifdef CONFIG_SPARSEMEM
	if (!IS_ALIGNED(prev_end, PAGES_PER_SECTION))
		free_memmap(prev_end, ALIGN(prev_end, PAGES_PER_SECTION));
#endif
}
#endif	/* !CONFIG_SPARSEMEM_VMEMMAP */

/*
 * mem_init() marks the free areas in the mem_map and tells us how much memory
 * is free.  This is done after various parts of the system have claimed their
 * memory after the kernel image.
 */
void __init mem_init(void)
{
	if (swiotlb_force == SWIOTLB_FORCE ||
	    max_pfn > PFN_DOWN(arm64_dma_phys_limit))
		swiotlb_init(1);
	else
		swiotlb_force = SWIOTLB_NO_FORCE;

	set_max_mapnr(max_pfn - PHYS_PFN_OFFSET);

#ifndef CONFIG_SPARSEMEM_VMEMMAP
	free_unused_memmap();
#endif
	/* this will put all unused low memory onto the freelists */
	memblock_free_all();

	mem_init_print_info(NULL);

	/*
	 * Check boundaries twice: Some fundamental inconsistencies can be
	 * detected at build time already.
	 */
#ifdef CONFIG_COMPAT
	BUILD_BUG_ON(TASK_SIZE_32 > DEFAULT_MAP_WINDOW_64);
#endif

	if (PAGE_SIZE >= 16384 && get_num_physpages() <= 128) {
		extern int sysctl_overcommit_memory;
		/*
		 * On a machine this small we won't get anywhere without
		 * overcommit, so turn it on by default.
		 */
		sysctl_overcommit_memory = OVERCOMMIT_ALWAYS;
	}
}

void free_initmem(void)
{
	free_reserved_area(lm_alias(__init_begin),
			   lm_alias(__init_end),
			   POISON_FREE_INITMEM, "unused kernel");
	/*
	 * Unmap the __init region but leave the VM area in place. This
	 * prevents the region from being reused for kernel modules, which
	 * is not supported by kallsyms.
	 */
	unmap_kernel_range((u64)__init_begin, (u64)(__init_end - __init_begin));
}

void dump_mem_limit(void)
{
	if (memory_limit != PHYS_ADDR_MAX) {
		pr_emerg("Memory Limit: %llu MB\n", memory_limit >> 20);
	} else {
		pr_emerg("Memory Limit: none\n");
	}
}

```

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable.h**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_PGTABLE_H
#define __ASM_PGTABLE_H

#include <asm/bug.h>
#include <asm/proc-fns.h>

#include <asm/memory.h>
#include <asm/mte.h>
#include <asm/pgtable-hwdef.h>
#include <asm/pgtable-prot.h>
#include <asm/tlbflush.h>

/*
 * VMALLOC range.
 *
 * VMALLOC_START: beginning of the kernel vmalloc space
 * VMALLOC_END: extends to the available space below vmemmap, PCI I/O space
 *	and fixed mappings
 */
#define VMALLOC_START		(MODULES_END)
#define VMALLOC_END		(- PUD_SIZE - VMEMMAP_SIZE - SZ_64K)

#define vmemmap			((struct page *)VMEMMAP_START - (memstart_addr >> PAGE_SHIFT))

#define FIRST_USER_ADDRESS	0UL

#ifndef __ASSEMBLY__

#include <asm/cmpxchg.h>
#include <asm/fixmap.h>
#include <linux/mmdebug.h>
#include <linux/mm_types.h>
#include <linux/sched.h>

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define __HAVE_ARCH_FLUSH_PMD_TLB_RANGE

/* Set stride and tlb_level in flush_*_tlb_range */
#define flush_pmd_tlb_range(vma, addr, end)	\
	__flush_tlb_range(vma, addr, end, PMD_SIZE, false, 2)
#define flush_pud_tlb_range(vma, addr, end)	\
	__flush_tlb_range(vma, addr, end, PUD_SIZE, false, 1)
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

/*
 * Outside of a few very special situations (e.g. hibernation), we always
 * use broadcast TLB invalidation instructions, therefore a spurious page
 * fault on one CPU which has been handled concurrently by another CPU
 * does not need to perform additional invalidation.
 */
#define flush_tlb_fix_spurious_fault(vma, address) do { } while (0)

/*
 * ZERO_PAGE is a global shared page that is always zero: used
 * for zero-mapped memory areas etc..
 */
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)];
#define ZERO_PAGE(vaddr)	phys_to_page(__pa_symbol(empty_zero_page))

#define pte_ERROR(e)	\
	pr_err("%s:%d: bad pte %016llx.\n", __FILE__, __LINE__, pte_val(e))

/*
 * Macros to convert between a physical address and its placement in a
 * page table entry, taking care of 52-bit addresses.
 */
#ifdef CONFIG_ARM64_PA_BITS_52
#define __pte_to_phys(pte)	\
	((pte_val(pte) & PTE_ADDR_LOW) | ((pte_val(pte) & PTE_ADDR_HIGH) << 36))
#define __phys_to_pte_val(phys)	(((phys) | ((phys) >> 36)) & PTE_ADDR_MASK)
#else
#define __pte_to_phys(pte)	(pte_val(pte) & PTE_ADDR_MASK)
#define __phys_to_pte_val(phys)	(phys)
#endif

#define pte_pfn(pte)		(__pte_to_phys(pte) >> PAGE_SHIFT)
#define pfn_pte(pfn,prot)	\
	__pte(__phys_to_pte_val((phys_addr_t)(pfn) << PAGE_SHIFT) | pgprot_val(prot))

#define pte_none(pte)		(!pte_val(pte))
#define pte_clear(mm,addr,ptep)	set_pte(ptep, __pte(0))
#define pte_page(pte)		(pfn_to_page(pte_pfn(pte)))

/*
 * The following only work if pte_present(). Undefined behaviour otherwise.
 */
#define pte_present(pte)	(!!(pte_val(pte) & (PTE_VALID | PTE_PROT_NONE)))
#define pte_young(pte)		(!!(pte_val(pte) & PTE_AF))
#define pte_special(pte)	(!!(pte_val(pte) & PTE_SPECIAL))
#define pte_write(pte)		(!!(pte_val(pte) & PTE_WRITE))
#define pte_user_exec(pte)	(!(pte_val(pte) & PTE_UXN))
#define pte_cont(pte)		(!!(pte_val(pte) & PTE_CONT))
#define pte_devmap(pte)		(!!(pte_val(pte) & PTE_DEVMAP))
#define pte_tagged(pte)		((pte_val(pte) & PTE_ATTRINDX_MASK) == \
				 PTE_ATTRINDX(MT_NORMAL_TAGGED))

#define pte_cont_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + CONT_PTE_SIZE) & CONT_PTE_MASK;	\
	(__boundary - 1 < (end) - 1) ? __boundary : (end);			\
})

#define pmd_cont_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + CONT_PMD_SIZE) & CONT_PMD_MASK;	\
	(__boundary - 1 < (end) - 1) ? __boundary : (end);			\
})

#define pte_hw_dirty(pte)	(pte_write(pte) && !(pte_val(pte) & PTE_RDONLY))
#define pte_sw_dirty(pte)	(!!(pte_val(pte) & PTE_DIRTY))
#define pte_dirty(pte)		(pte_sw_dirty(pte) || pte_hw_dirty(pte))

#define pte_valid(pte)		(!!(pte_val(pte) & PTE_VALID))
#define pte_valid_not_user(pte) \
	((pte_val(pte) & (PTE_VALID | PTE_USER)) == PTE_VALID)
#define pte_valid_user(pte) \
	((pte_val(pte) & (PTE_VALID | PTE_USER)) == (PTE_VALID | PTE_USER))

/*
 * Could the pte be present in the TLB? We must check mm_tlb_flush_pending
 * so that we don't erroneously return false for pages that have been
 * remapped as PROT_NONE but are yet to be flushed from the TLB.
 * Note that we can't make any assumptions based on the state of the access
 * flag, since ptep_clear_flush_young() elides a DSB when invalidating the
 * TLB.
 */
#define pte_accessible(mm, pte)	\
	(mm_tlb_flush_pending(mm) ? pte_present(pte) : pte_valid(pte))

/*
 * p??_access_permitted() is true for valid user mappings (subject to the
 * write permission check). PROT_NONE mappings do not have the PTE_VALID bit
 * set.
 */
#define pte_access_permitted(pte, write) \
	(pte_valid_user(pte) && (!(write) || pte_write(pte)))
#define pmd_access_permitted(pmd, write) \
	(pte_access_permitted(pmd_pte(pmd), (write)))
#define pud_access_permitted(pud, write) \
	(pte_access_permitted(pud_pte(pud), (write)))

static inline pte_t clear_pte_bit(pte_t pte, pgprot_t prot)
{
	pte_val(pte) &= ~pgprot_val(prot);
	return pte;
}

static inline pte_t set_pte_bit(pte_t pte, pgprot_t prot)
{
	pte_val(pte) |= pgprot_val(prot);
	return pte;
}

static inline pmd_t clear_pmd_bit(pmd_t pmd, pgprot_t prot)
{
	pmd_val(pmd) &= ~pgprot_val(prot);
	return pmd;
}

static inline pmd_t set_pmd_bit(pmd_t pmd, pgprot_t prot)
{
	pmd_val(pmd) |= pgprot_val(prot);
	return pmd;
}

static inline pte_t pte_mkwrite(pte_t pte)
{
	pte = set_pte_bit(pte, __pgprot(PTE_WRITE));
	pte = clear_pte_bit(pte, __pgprot(PTE_RDONLY));
	return pte;
}

static inline pte_t pte_mkclean(pte_t pte)
{
	pte = clear_pte_bit(pte, __pgprot(PTE_DIRTY));
	pte = set_pte_bit(pte, __pgprot(PTE_RDONLY));

	return pte;
}

static inline pte_t pte_mkdirty(pte_t pte)
{
	pte = set_pte_bit(pte, __pgprot(PTE_DIRTY));

	if (pte_write(pte))
		pte = clear_pte_bit(pte, __pgprot(PTE_RDONLY));

	return pte;
}

static inline pte_t pte_wrprotect(pte_t pte)
{
	/*
	 * If hardware-dirty (PTE_WRITE/DBM bit set and PTE_RDONLY
	 * clear), set the PTE_DIRTY bit.
	 */
	if (pte_hw_dirty(pte))
		pte = pte_mkdirty(pte);

	pte = clear_pte_bit(pte, __pgprot(PTE_WRITE));
	pte = set_pte_bit(pte, __pgprot(PTE_RDONLY));
	return pte;
}

static inline pte_t pte_mkold(pte_t pte)
{
	return clear_pte_bit(pte, __pgprot(PTE_AF));
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	return set_pte_bit(pte, __pgprot(PTE_AF));
}

static inline pte_t pte_mkspecial(pte_t pte)
{
	return set_pte_bit(pte, __pgprot(PTE_SPECIAL));
}

static inline pte_t pte_mkcont(pte_t pte)
{
	pte = set_pte_bit(pte, __pgprot(PTE_CONT));
	return set_pte_bit(pte, __pgprot(PTE_TYPE_PAGE));
}

static inline pte_t pte_mknoncont(pte_t pte)
{
	return clear_pte_bit(pte, __pgprot(PTE_CONT));
}

static inline pte_t pte_mkpresent(pte_t pte)
{
	return set_pte_bit(pte, __pgprot(PTE_VALID));
}

static inline pmd_t pmd_mkcont(pmd_t pmd)
{
	return __pmd(pmd_val(pmd) | PMD_SECT_CONT);
}

static inline pte_t pte_mkdevmap(pte_t pte)
{
	return set_pte_bit(pte, __pgprot(PTE_DEVMAP | PTE_SPECIAL));
}

static inline void set_pte(pte_t *ptep, pte_t pte)
{
	WRITE_ONCE(*ptep, pte);

	/*
	 * Only if the new pte is valid and kernel, otherwise TLB maintenance
	 * or update_mmu_cache() have the necessary barriers.
	 */
	if (pte_valid_not_user(pte)) {
		dsb(ishst);
		isb();
	}
}

extern void __sync_icache_dcache(pte_t pteval);

/*
 * PTE bits configuration in the presence of hardware Dirty Bit Management
 * (PTE_WRITE == PTE_DBM):
 *
 * Dirty  Writable | PTE_RDONLY  PTE_WRITE  PTE_DIRTY (sw)
 *   0      0      |   1           0          0
 *   0      1      |   1           1          0
 *   1      0      |   1           0          1
 *   1      1      |   0           1          x
 *
 * When hardware DBM is not present, the sofware PTE_DIRTY bit is updated via
 * the page fault mechanism. Checking the dirty status of a pte becomes:
 *
 *   PTE_DIRTY || (PTE_WRITE && !PTE_RDONLY)
 */

static inline void __check_racy_pte_update(struct mm_struct *mm, pte_t *ptep,
					   pte_t pte)
{
	pte_t old_pte;

	if (!IS_ENABLED(CONFIG_DEBUG_VM))
		return;

	old_pte = READ_ONCE(*ptep);

	if (!pte_valid(old_pte) || !pte_valid(pte))
		return;
	if (mm != current->active_mm && atomic_read(&mm->mm_users) <= 1)
		return;

	/*
	 * Check for potential race with hardware updates of the pte
	 * (ptep_set_access_flags safely changes valid ptes without going
	 * through an invalid entry).
	 */
	VM_WARN_ONCE(!pte_young(pte),
		     "%s: racy access flag clearing: 0x%016llx -> 0x%016llx",
		     __func__, pte_val(old_pte), pte_val(pte));
	VM_WARN_ONCE(pte_write(old_pte) && !pte_dirty(pte),
		     "%s: racy dirty state clearing: 0x%016llx -> 0x%016llx",
		     __func__, pte_val(old_pte), pte_val(pte));
}

static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pte)
{
	if (pte_present(pte) && pte_user_exec(pte) && !pte_special(pte))
		__sync_icache_dcache(pte);

	if (system_supports_mte() &&
	    pte_present(pte) && pte_tagged(pte) && !pte_special(pte))
		mte_sync_tags(ptep, pte);

	__check_racy_pte_update(mm, ptep, pte);

	set_pte(ptep, pte);
}

/*
 * Huge pte definitions.
 */
#define pte_mkhuge(pte)		(__pte(pte_val(pte) & ~PTE_TABLE_BIT))

/*
 * Hugetlb definitions.
 */
#define HUGE_MAX_HSTATE		4
#define HPAGE_SHIFT		PMD_SHIFT
#define HPAGE_SIZE		(_AC(1, UL) << HPAGE_SHIFT)
#define HPAGE_MASK		(~(HPAGE_SIZE - 1))
#define HUGETLB_PAGE_ORDER	(HPAGE_SHIFT - PAGE_SHIFT)

static inline pte_t pgd_pte(pgd_t pgd)
{
	return __pte(pgd_val(pgd));
}

static inline pte_t p4d_pte(p4d_t p4d)
{
	return __pte(p4d_val(p4d));
}

static inline pte_t pud_pte(pud_t pud)
{
	return __pte(pud_val(pud));
}

static inline pud_t pte_pud(pte_t pte)
{
	return __pud(pte_val(pte));
}

static inline pmd_t pud_pmd(pud_t pud)
{
	return __pmd(pud_val(pud));
}

static inline pte_t pmd_pte(pmd_t pmd)
{
	return __pte(pmd_val(pmd));
}

static inline pmd_t pte_pmd(pte_t pte)
{
	return __pmd(pte_val(pte));
}

static inline pgprot_t mk_pud_sect_prot(pgprot_t prot)
{
	return __pgprot((pgprot_val(prot) & ~PUD_TABLE_BIT) | PUD_TYPE_SECT);
}

static inline pgprot_t mk_pmd_sect_prot(pgprot_t prot)
{
	return __pgprot((pgprot_val(prot) & ~PMD_TABLE_BIT) | PMD_TYPE_SECT);
}

#ifdef CONFIG_NUMA_BALANCING
/*
 * See the comment in include/linux/pgtable.h
 */
static inline int pte_protnone(pte_t pte)
{
	return (pte_val(pte) & (PTE_VALID | PTE_PROT_NONE)) == PTE_PROT_NONE;
}

static inline int pmd_protnone(pmd_t pmd)
{
	return pte_protnone(pmd_pte(pmd));
}
#endif

#define pmd_present_invalid(pmd)     (!!(pmd_val(pmd) & PMD_PRESENT_INVALID))

static inline int pmd_present(pmd_t pmd)
{
	return pte_present(pmd_pte(pmd)) || pmd_present_invalid(pmd);
}

/*
 * THP definitions.
 */

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
static inline int pmd_trans_huge(pmd_t pmd)
{
	return pmd_val(pmd) && pmd_present(pmd) && !(pmd_val(pmd) & PMD_TABLE_BIT);
}
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

#define pmd_dirty(pmd)		pte_dirty(pmd_pte(pmd))
#define pmd_young(pmd)		pte_young(pmd_pte(pmd))
#define pmd_valid(pmd)		pte_valid(pmd_pte(pmd))
#define pmd_wrprotect(pmd)	pte_pmd(pte_wrprotect(pmd_pte(pmd)))
#define pmd_mkold(pmd)		pte_pmd(pte_mkold(pmd_pte(pmd)))
#define pmd_mkwrite(pmd)	pte_pmd(pte_mkwrite(pmd_pte(pmd)))
#define pmd_mkclean(pmd)	pte_pmd(pte_mkclean(pmd_pte(pmd)))
#define pmd_mkdirty(pmd)	pte_pmd(pte_mkdirty(pmd_pte(pmd)))
#define pmd_mkyoung(pmd)	pte_pmd(pte_mkyoung(pmd_pte(pmd)))

static inline pmd_t pmd_mkinvalid(pmd_t pmd)
{
	pmd = set_pmd_bit(pmd, __pgprot(PMD_PRESENT_INVALID));
	pmd = clear_pmd_bit(pmd, __pgprot(PMD_SECT_VALID));

	return pmd;
}

#define pmd_thp_or_huge(pmd)	(pmd_huge(pmd) || pmd_trans_huge(pmd))

#define pmd_write(pmd)		pte_write(pmd_pte(pmd))

#define pmd_mkhuge(pmd)		(__pmd(pmd_val(pmd) & ~PMD_TABLE_BIT))

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define pmd_devmap(pmd)		pte_devmap(pmd_pte(pmd))
#endif
static inline pmd_t pmd_mkdevmap(pmd_t pmd)
{
	return pte_pmd(set_pte_bit(pmd_pte(pmd), __pgprot(PTE_DEVMAP)));
}

#define __pmd_to_phys(pmd)	__pte_to_phys(pmd_pte(pmd))
#define __phys_to_pmd_val(phys)	__phys_to_pte_val(phys)
#define pmd_pfn(pmd)		((__pmd_to_phys(pmd) & PMD_MASK) >> PAGE_SHIFT)
#define pfn_pmd(pfn,prot)	__pmd(__phys_to_pmd_val((phys_addr_t)(pfn) << PAGE_SHIFT) | pgprot_val(prot))
#define mk_pmd(page,prot)	pfn_pmd(page_to_pfn(page),prot)

#define pud_young(pud)		pte_young(pud_pte(pud))
#define pud_mkyoung(pud)	pte_pud(pte_mkyoung(pud_pte(pud)))
#define pud_write(pud)		pte_write(pud_pte(pud))

#define pud_mkhuge(pud)		(__pud(pud_val(pud) & ~PUD_TABLE_BIT))

#define __pud_to_phys(pud)	__pte_to_phys(pud_pte(pud))
#define __phys_to_pud_val(phys)	__phys_to_pte_val(phys)
#define pud_pfn(pud)		((__pud_to_phys(pud) & PUD_MASK) >> PAGE_SHIFT)
#define pfn_pud(pfn,prot)	__pud(__phys_to_pud_val((phys_addr_t)(pfn) << PAGE_SHIFT) | pgprot_val(prot))

#define set_pmd_at(mm, addr, pmdp, pmd)	set_pte_at(mm, addr, (pte_t *)pmdp, pmd_pte(pmd))
#define set_pud_at(mm, addr, pudp, pud)	set_pte_at(mm, addr, (pte_t *)pudp, pud_pte(pud))

#define __p4d_to_phys(p4d)	__pte_to_phys(p4d_pte(p4d))
#define __phys_to_p4d_val(phys)	__phys_to_pte_val(phys)

#define __pgd_to_phys(pgd)	__pte_to_phys(pgd_pte(pgd))
#define __phys_to_pgd_val(phys)	__phys_to_pte_val(phys)

#define __pgprot_modify(prot,mask,bits) \
	__pgprot((pgprot_val(prot) & ~(mask)) | (bits))

#define pgprot_nx(prot) \
	__pgprot_modify(prot, PTE_MAYBE_GP, PTE_PXN)

/*
 * Mark the prot value as uncacheable and unbufferable.
 */
#define pgprot_noncached(prot) \
	__pgprot_modify(prot, PTE_ATTRINDX_MASK, PTE_ATTRINDX(MT_DEVICE_nGnRnE) | PTE_PXN | PTE_UXN)
#define pgprot_writecombine(prot) \
	__pgprot_modify(prot, PTE_ATTRINDX_MASK, PTE_ATTRINDX(MT_NORMAL_NC) | PTE_PXN | PTE_UXN)
#define pgprot_device(prot) \
	__pgprot_modify(prot, PTE_ATTRINDX_MASK, PTE_ATTRINDX(MT_DEVICE_nGnRE) | PTE_PXN | PTE_UXN)
#define pgprot_tagged(prot) \
	__pgprot_modify(prot, PTE_ATTRINDX_MASK, PTE_ATTRINDX(MT_NORMAL_TAGGED))
#define pgprot_mhp	pgprot_tagged
/*
 * DMA allocations for non-coherent devices use what the Arm architecture calls
 * "Normal non-cacheable" memory, which permits speculation, unaligned accesses
 * and merging of writes.  This is different from "Device-nGnR[nE]" memory which
 * is intended for MMIO and thus forbids speculation, preserves access size,
 * requires strict alignment and can also force write responses to come from the
 * endpoint.
 */
#define pgprot_dmacoherent(prot) \
	__pgprot_modify(prot, PTE_ATTRINDX_MASK, \
			PTE_ATTRINDX(MT_NORMAL_NC) | PTE_PXN | PTE_UXN)

/*
 * Mark the prot value as outer cacheable and inner non-cacheable. Non-coherent
 * devices on a system with support for a system or last level cache use these
 * attributes to cache allocations in the system cache.
 */
#define pgprot_syscached(prot) \
	__pgprot_modify(prot, PTE_ATTRINDX_MASK, \
			PTE_ATTRINDX(MT_NORMAL_iNC_oWB) | PTE_PXN | PTE_UXN)

#define __HAVE_PHYS_MEM_ACCESS_PROT
struct file;
extern pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot);

#define pmd_none(pmd)		(!pmd_val(pmd))

#define pmd_bad(pmd)		(!(pmd_val(pmd) & PMD_TABLE_BIT))

#define pmd_table(pmd)		((pmd_val(pmd) & PMD_TYPE_MASK) == \
				 PMD_TYPE_TABLE)
#define pmd_sect(pmd)		((pmd_val(pmd) & PMD_TYPE_MASK) == \
				 PMD_TYPE_SECT)
#define pmd_leaf(pmd)		pmd_sect(pmd)

#if defined(CONFIG_ARM64_64K_PAGES) || CONFIG_PGTABLE_LEVELS < 3
static inline bool pud_sect(pud_t pud) { return false; }
static inline bool pud_table(pud_t pud) { return true; }
#else
#define pud_sect(pud)		((pud_val(pud) & PUD_TYPE_MASK) == \
				 PUD_TYPE_SECT)
#define pud_table(pud)		((pud_val(pud) & PUD_TYPE_MASK) == \
				 PUD_TYPE_TABLE)
#endif

extern pgd_t init_pg_dir[PTRS_PER_PGD];
extern pgd_t init_pg_end[];
extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
extern pgd_t idmap_pg_dir[PTRS_PER_PGD];
extern pgd_t idmap_pg_end[];
extern pgd_t tramp_pg_dir[PTRS_PER_PGD];
extern pgd_t reserved_pg_dir[PTRS_PER_PGD];

extern void set_swapper_pgd(pgd_t *pgdp, pgd_t pgd);

#ifdef CONFIG_MEMORY_HOTPLUG
extern int populate_range_driver_managed(u64 start, u64 size,
		const char *resource_name);
extern int depopulate_range_driver_managed(u64 start, u64 size,
		const char *resource_name);
#endif

static inline bool in_swapper_pgdir(void *addr)
{
	return ((unsigned long)addr & PAGE_MASK) ==
	        ((unsigned long)swapper_pg_dir & PAGE_MASK);
}

static inline void set_pmd(pmd_t *pmdp, pmd_t pmd)
{
#ifdef __PAGETABLE_PMD_FOLDED
	if (in_swapper_pgdir(pmdp)) {
		set_swapper_pgd((pgd_t *)pmdp, __pgd(pmd_val(pmd)));
		return;
	}
#endif /* __PAGETABLE_PMD_FOLDED */

	WRITE_ONCE(*pmdp, pmd);

	if (pmd_valid(pmd)) {
		dsb(ishst);
		isb();
	}
}

static inline void pmd_clear(pmd_t *pmdp)
{
	set_pmd(pmdp, __pmd(0));
}

static inline phys_addr_t pmd_page_paddr(pmd_t pmd)
{
	return __pmd_to_phys(pmd);
}

static inline unsigned long pmd_page_vaddr(pmd_t pmd)
{
	return (unsigned long)__va(pmd_page_paddr(pmd));
}

/* Find an entry in the third-level page table. */
#define pte_offset_phys(dir,addr)	(pmd_page_paddr(READ_ONCE(*(dir))) + pte_index(addr) * sizeof(pte_t))

#define pte_set_fixmap(addr)		((pte_t *)set_fixmap_offset(FIX_PTE, addr))
#define pte_set_fixmap_offset(pmd, addr)	pte_set_fixmap(pte_offset_phys(pmd, addr))
#define pte_clear_fixmap()		clear_fixmap(FIX_PTE)

#define pmd_page(pmd)			phys_to_page(__pmd_to_phys(pmd))

/* use ONLY for statically allocated translation tables */
#define pte_offset_kimg(dir,addr)	((pte_t *)__phys_to_kimg(pte_offset_phys((dir), (addr))))

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */
#define mk_pte(page,prot)	pfn_pte(page_to_pfn(page),prot)

#if CONFIG_PGTABLE_LEVELS > 2

#define pmd_ERROR(e)	\
	pr_err("%s:%d: bad pmd %016llx.\n", __FILE__, __LINE__, pmd_val(e))

#define pud_none(pud)		(!pud_val(pud))
#define pud_bad(pud)		(!(pud_val(pud) & PUD_TABLE_BIT))
#define pud_present(pud)	pte_present(pud_pte(pud))
#define pud_leaf(pud)		pud_sect(pud)
#define pud_valid(pud)		pte_valid(pud_pte(pud))

static inline void set_pud(pud_t *pudp, pud_t pud)
{
#ifdef __PAGETABLE_PUD_FOLDED
	if (in_swapper_pgdir(pudp)) {
		set_swapper_pgd((pgd_t *)pudp, __pgd(pud_val(pud)));
		return;
	}
#endif /* __PAGETABLE_PUD_FOLDED */

	WRITE_ONCE(*pudp, pud);

	if (pud_valid(pud)) {
		dsb(ishst);
		isb();
	}
}

static inline void pud_clear(pud_t *pudp)
{
	set_pud(pudp, __pud(0));
}

static inline phys_addr_t pud_page_paddr(pud_t pud)
{
	return __pud_to_phys(pud);
}

static inline unsigned long pud_page_vaddr(pud_t pud)
{
	return (unsigned long)__va(pud_page_paddr(pud));
}

/* Find an entry in the second-level page table. */
#define pmd_offset_phys(dir, addr)	(pud_page_paddr(READ_ONCE(*(dir))) + pmd_index(addr) * sizeof(pmd_t))

#define pmd_set_fixmap(addr)		((pmd_t *)set_fixmap_offset(FIX_PMD, addr))
#define pmd_set_fixmap_offset(pud, addr)	pmd_set_fixmap(pmd_offset_phys(pud, addr))
#define pmd_clear_fixmap()		clear_fixmap(FIX_PMD)

#define pud_page(pud)			phys_to_page(__pud_to_phys(pud))

/* use ONLY for statically allocated translation tables */
#define pmd_offset_kimg(dir,addr)	((pmd_t *)__phys_to_kimg(pmd_offset_phys((dir), (addr))))

#else

#define pud_page_paddr(pud)	({ BUILD_BUG(); 0; })

/* Match pmd_offset folding in <asm/generic/pgtable-nopmd.h> */
#define pmd_set_fixmap(addr)		NULL
#define pmd_set_fixmap_offset(pudp, addr)	((pmd_t *)pudp)
#define pmd_clear_fixmap()

#define pmd_offset_kimg(dir,addr)	((pmd_t *)dir)

#endif	/* CONFIG_PGTABLE_LEVELS > 2 */

#if CONFIG_PGTABLE_LEVELS > 3

#define pud_ERROR(e)	\
	pr_err("%s:%d: bad pud %016llx.\n", __FILE__, __LINE__, pud_val(e))

#define p4d_none(p4d)		(!p4d_val(p4d))
#define p4d_bad(p4d)		(!(p4d_val(p4d) & 2))
#define p4d_present(p4d)	(p4d_val(p4d))

static inline void set_p4d(p4d_t *p4dp, p4d_t p4d)
{
	if (in_swapper_pgdir(p4dp)) {
		set_swapper_pgd((pgd_t *)p4dp, __pgd(p4d_val(p4d)));
		return;
	}

	WRITE_ONCE(*p4dp, p4d);
	dsb(ishst);
	isb();
}

static inline void p4d_clear(p4d_t *p4dp)
{
	set_p4d(p4dp, __p4d(0));
}

static inline phys_addr_t p4d_page_paddr(p4d_t p4d)
{
	return __p4d_to_phys(p4d);
}

static inline unsigned long p4d_page_vaddr(p4d_t p4d)
{
	return (unsigned long)__va(p4d_page_paddr(p4d));
}

/* Find an entry in the frst-level page table. */
#define pud_offset_phys(dir, addr)	(p4d_page_paddr(READ_ONCE(*(dir))) + pud_index(addr) * sizeof(pud_t))

#define pud_set_fixmap(addr)		((pud_t *)set_fixmap_offset(FIX_PUD, addr))
#define pud_set_fixmap_offset(p4d, addr)	pud_set_fixmap(pud_offset_phys(p4d, addr))
#define pud_clear_fixmap()		clear_fixmap(FIX_PUD)

#define p4d_page(p4d)		pfn_to_page(__phys_to_pfn(__p4d_to_phys(p4d)))

/* use ONLY for statically allocated translation tables */
#define pud_offset_kimg(dir,addr)	((pud_t *)__phys_to_kimg(pud_offset_phys((dir), (addr))))

#else

#define p4d_page_paddr(p4d)	({ BUILD_BUG(); 0;})
#define pgd_page_paddr(pgd)	({ BUILD_BUG(); 0;})

/* Match pud_offset folding in <asm/generic/pgtable-nopud.h> */
#define pud_set_fixmap(addr)		NULL
#define pud_set_fixmap_offset(pgdp, addr)	((pud_t *)pgdp)
#define pud_clear_fixmap()

#define pud_offset_kimg(dir,addr)	((pud_t *)dir)

#endif  /* CONFIG_PGTABLE_LEVELS > 3 */

#define pgd_ERROR(e)	\
	pr_err("%s:%d: bad pgd %016llx.\n", __FILE__, __LINE__, pgd_val(e))

#define pgd_set_fixmap(addr)	((pgd_t *)set_fixmap_offset(FIX_PGD, addr))
#define pgd_clear_fixmap()	clear_fixmap(FIX_PGD)

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	/*
	 * Normal and Normal-Tagged are two different memory types and indices
	 * in MAIR_EL1. The mask below has to include PTE_ATTRINDX_MASK.
	 */
	const pteval_t mask = PTE_USER | PTE_PXN | PTE_UXN | PTE_RDONLY |
			      PTE_PROT_NONE | PTE_VALID | PTE_WRITE | PTE_GP |
			      PTE_ATTRINDX_MASK;
	/* preserve the hardware dirty information */
	if (pte_hw_dirty(pte))
		pte = pte_mkdirty(pte);
	pte_val(pte) = (pte_val(pte) & ~mask) | (pgprot_val(newprot) & mask);
	return pte;
}

static inline pmd_t pmd_modify(pmd_t pmd, pgprot_t newprot)
{
	return pte_pmd(pte_modify(pmd_pte(pmd), newprot));
}

#define __HAVE_ARCH_PTEP_SET_ACCESS_FLAGS
extern int ptep_set_access_flags(struct vm_area_struct *vma,
				 unsigned long address, pte_t *ptep,
				 pte_t entry, int dirty);

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define __HAVE_ARCH_PMDP_SET_ACCESS_FLAGS
static inline int pmdp_set_access_flags(struct vm_area_struct *vma,
					unsigned long address, pmd_t *pmdp,
					pmd_t entry, int dirty)
{
	return ptep_set_access_flags(vma, address, (pte_t *)pmdp, pmd_pte(entry), dirty);
}

static inline int pud_devmap(pud_t pud)
{
	return 0;
}

static inline int pgd_devmap(pgd_t pgd)
{
	return 0;
}
#endif

/*
 * Atomic pte/pmd modifications.
 */
#define __HAVE_ARCH_PTEP_TEST_AND_CLEAR_YOUNG
static inline int __ptep_test_and_clear_young(pte_t *ptep)
{
	pte_t old_pte, pte;

	pte = READ_ONCE(*ptep);
	do {
		old_pte = pte;
		pte = pte_mkold(pte);
		pte_val(pte) = cmpxchg_relaxed(&pte_val(*ptep),
					       pte_val(old_pte), pte_val(pte));
	} while (pte_val(pte) != pte_val(old_pte));

	return pte_young(pte);
}

static inline int ptep_test_and_clear_young(struct vm_area_struct *vma,
					    unsigned long address,
					    pte_t *ptep)
{
	return __ptep_test_and_clear_young(ptep);
}

#define __HAVE_ARCH_PTEP_CLEAR_YOUNG_FLUSH
static inline int ptep_clear_flush_young(struct vm_area_struct *vma,
					 unsigned long address, pte_t *ptep)
{
	int young = ptep_test_and_clear_young(vma, address, ptep);

	if (young) {
		/*
		 * We can elide the trailing DSB here since the worst that can
		 * happen is that a CPU continues to use the young entry in its
		 * TLB and we mistakenly reclaim the associated page. The
		 * window for such an event is bounded by the next
		 * context-switch, which provides a DSB to complete the TLB
		 * invalidation.
		 */
		flush_tlb_page_nosync(vma, address);
	}

	return young;
}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define __HAVE_ARCH_PMDP_TEST_AND_CLEAR_YOUNG
static inline int pmdp_test_and_clear_young(struct vm_area_struct *vma,
					    unsigned long address,
					    pmd_t *pmdp)
{
	return ptep_test_and_clear_young(vma, address, (pte_t *)pmdp);
}
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

#define __HAVE_ARCH_PTEP_GET_AND_CLEAR
static inline pte_t ptep_get_and_clear(struct mm_struct *mm,
				       unsigned long address, pte_t *ptep)
{
	return __pte(xchg_relaxed(&pte_val(*ptep), 0));
}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define __HAVE_ARCH_PMDP_HUGE_GET_AND_CLEAR
static inline pmd_t pmdp_huge_get_and_clear(struct mm_struct *mm,
					    unsigned long address, pmd_t *pmdp)
{
	return pte_pmd(ptep_get_and_clear(mm, address, (pte_t *)pmdp));
}
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

/*
 * ptep_set_wrprotect - mark read-only while trasferring potential hardware
 * dirty status (PTE_DBM && !PTE_RDONLY) to the software PTE_DIRTY bit.
 */
#define __HAVE_ARCH_PTEP_SET_WRPROTECT
static inline void ptep_set_wrprotect(struct mm_struct *mm, unsigned long address, pte_t *ptep)
{
	pte_t old_pte, pte;

	pte = READ_ONCE(*ptep);
	do {
		old_pte = pte;
		pte = pte_wrprotect(pte);
		pte_val(pte) = cmpxchg_relaxed(&pte_val(*ptep),
					       pte_val(old_pte), pte_val(pte));
	} while (pte_val(pte) != pte_val(old_pte));
}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define __HAVE_ARCH_PMDP_SET_WRPROTECT
static inline void pmdp_set_wrprotect(struct mm_struct *mm,
				      unsigned long address, pmd_t *pmdp)
{
	ptep_set_wrprotect(mm, address, (pte_t *)pmdp);
}

#define pmdp_establish pmdp_establish
static inline pmd_t pmdp_establish(struct vm_area_struct *vma,
		unsigned long address, pmd_t *pmdp, pmd_t pmd)
{
	return __pmd(xchg_relaxed(&pmd_val(*pmdp), pmd_val(pmd)));
}
#endif

/*
 * Encode and decode a swap entry:
 *	bits 0-1:	present (must be zero)
 *	bits 2-7:	swap type
 *	bits 8-57:	swap offset
 *	bit  58:	PTE_PROT_NONE (must be zero)
 */
#define __SWP_TYPE_SHIFT	2
#define __SWP_TYPE_BITS		6
#define __SWP_OFFSET_BITS	50
#define __SWP_TYPE_MASK		((1 << __SWP_TYPE_BITS) - 1)
#define __SWP_OFFSET_SHIFT	(__SWP_TYPE_BITS + __SWP_TYPE_SHIFT)
#define __SWP_OFFSET_MASK	((1UL << __SWP_OFFSET_BITS) - 1)

#define __swp_type(x)		(((x).val >> __SWP_TYPE_SHIFT) & __SWP_TYPE_MASK)
#define __swp_offset(x)		(((x).val >> __SWP_OFFSET_SHIFT) & __SWP_OFFSET_MASK)
#define __swp_entry(type,offset) ((swp_entry_t) { ((type) << __SWP_TYPE_SHIFT) | ((offset) << __SWP_OFFSET_SHIFT) })

#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(swp)	((pte_t) { (swp).val })

#ifdef CONFIG_ARCH_ENABLE_THP_MIGRATION
#define __pmd_to_swp_entry(pmd)		((swp_entry_t) { pmd_val(pmd) })
#define __swp_entry_to_pmd(swp)		__pmd((swp).val)
#endif /* CONFIG_ARCH_ENABLE_THP_MIGRATION */

/*
 * Ensure that there are not more swap files than can be encoded in the kernel
 * PTEs.
 */
#define MAX_SWAPFILES_CHECK() BUILD_BUG_ON(MAX_SWAPFILES_SHIFT > __SWP_TYPE_BITS)

extern int kern_addr_valid(unsigned long addr);

#ifdef CONFIG_ARM64_MTE

#define __HAVE_ARCH_PREPARE_TO_SWAP
static inline int arch_prepare_to_swap(struct page *page)
{
	if (system_supports_mte())
		return mte_save_tags(page);
	return 0;
}

#define __HAVE_ARCH_SWAP_INVALIDATE
static inline void arch_swap_invalidate_page(int type, pgoff_t offset)
{
	if (system_supports_mte())
		mte_invalidate_tags(type, offset);
}

static inline void arch_swap_invalidate_area(int type)
{
	if (system_supports_mte())
		mte_invalidate_tags_area(type);
}

#define __HAVE_ARCH_SWAP_RESTORE
static inline void arch_swap_restore(swp_entry_t entry, struct page *page)
{
	if (system_supports_mte() && mte_restore_tags(entry, page))
		set_bit(PG_mte_tagged, &page->flags);
}

#endif /* CONFIG_ARM64_MTE */

/*
 * On AArch64, the cache coherency is handled via the set_pte_at() function.
 */
static inline void update_mmu_cache(struct vm_area_struct *vma,
				    unsigned long addr, pte_t *ptep)
{
	/*
	 * We don't do anything here, so there's a very small chance of
	 * us retaking a user fault which we just fixed up. The alternative
	 * is doing a dsb(ishst), but that penalises the fastpath.
	 */
}

#define update_mmu_cache_pmd(vma, address, pmd) do { } while (0)

#ifdef CONFIG_ARM64_PA_BITS_52
#define phys_to_ttbr(addr)	(((addr) | ((addr) >> 46)) & TTBR_BADDR_MASK_52)
#else
#define phys_to_ttbr(addr)	(addr)
#endif

/*
 * On arm64 without hardware Access Flag, copying from user will fail because
 * the pte is old and cannot be marked young. So we always end up with zeroed
 * page after fork() + CoW for pfn mappings. We don't always have a
 * hardware-managed access flag on arm64.
 */
static inline bool arch_faults_on_old_pte(void)
{
	WARN_ON(preemptible());

	return !cpu_has_hw_af();
}
#define arch_faults_on_old_pte		arch_faults_on_old_pte

/*
 * Experimentally, it's cheap to set the access flag in hardware and we
 * benefit from prefaulting mappings as 'old' to start with.
 */
static inline bool arch_wants_old_prefaulted_pte(void)
{
	return !arch_faults_on_old_pte();
}
#define arch_wants_old_prefaulted_pte	arch_wants_old_prefaulted_pte

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_PGTABLE_H */

```




**Command Executed:**
```bash
$matches = Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64" -Recurse -File | Select-String -Pattern "\bphysvirt_offset\b"; if ($matches) { $matches | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() } } else { "NO_MATCH" }
```

**Output:**
```
NO_MATCH
```


```bash
$matches = Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common" -Recurse -File | Select-String -Pattern "\bphysvirt_offset\b"; if ($matches) { $matches | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() } } else { "NO_MATCH" }
```

**Current Output:**
```

<|cursor|>
```

Subagent: d7798391-4ddd-46f8-99ce-c5405fd96df1


**Command Executed:**
```bash
Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64" -Recurse -File | Select-String -Pattern "TCR_TxSZ\(|TCR_T0SZ\(|TCR_T1SZ\(|TCR_TG0_4K|TCR_TG1_4K|TCR_TG0_16K|TCR_TG1_16K|TCR_TG0_64K|TCR_TG1_64K" | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() }
```

**Output:**
```
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\kvm_arm.h:107:#define VTCR_EL2_TG0_4K		TCR_TG0_4K
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\kvm_arm.h:108:#define VTCR_EL2_TG0_16K	TCR_TG0_16K
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\kvm_arm.h:109:#define VTCR_EL2_TG0_64K	TCR_TG0_64K
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\kvm_arm.h:123:#define VTCR_EL2_T0SZ(x)	TCR_T0SZ(x)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h:60:* TCR_T0SZ(VA_BITS), unless system RAM is positioned very high in
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h:68:return unlikely(idmap_t0sz != TCR_T0SZ(vabits_actual));
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h:83:static inline void __cpu_set_tcr_t0sz(unsigned long t0sz)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h:97:#define cpu_set_default_tcr_t0sz()__cpu_set_tcr_t0sz(TCR_T0SZ(vabits_actual))
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h:98:#define cpu_set_idmap_tcr_t0sz()	_cpu_set_tcr_t0sz(idmap_t0sz)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h:118:cpu_set_default_tcr_t0sz();
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h:128:cpu_set_idmap_tcr_t0sz();
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:182:#define TCR_T0SZ(x)		((UL(64) - (x)) << TCR_T0SZ_OFFSET)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:183:#define TCR_T1SZ(x)		((UL(64) - (x)) << TCR_T1SZ_OFFSET)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:184:#define TCR_TxSZ(x)		(TCR_T0SZ(x) | TCR_T1SZ(x))
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:245:#define TCR_TG0_4K		(UL(0) << TCR_TG0_SHIFT)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:246:#define TCR_TG0_64K		(UL(1) << TCR_TG0_SHIFT)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:247:#define TCR_TG0_16K		(UL(2) << TCR_TG0_SHIFT)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:251:#define TCR_TG1_16K		(UL(1) << TCR_TG1_SHIFT)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:252:#define TCR_TG1_4K		(UL(2) << TCR_TG1_SHIFT)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\pgtable-hwdef.h:253:#define TCR_TG1_64K		(UL(3) << TCR_TG1_SHIFT)
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\kernel\head.S:337:cmp	x5, TCR_T0SZ(VA_BITS_MIN) // default T0SZ small enough?
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\mmu.c:43:u64 idmap_t0sz = TCR_T0SZ(VA_BITS_MIN);
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\proc.S:24:#define TCR_TG_FLAGS	TCR_TG0_64K | TCR_TG1_64K
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\proc.S:26:#define TCR_TG_FLAGS	TCR_TG0_16K | TCR_TG1_16K
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\proc.S:28:#define TCR_TG_FLAGS	TCR_TG0_4K | TCR_TG1_4K
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\proc.S:476:mov_q	x10, TCR_TxSZ(VA_BITS) | TCR_CACHE_FLAGS | TCR_SMP_FLAGS | \
```



**Files Read:**

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\mm\proc.S**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/mm/proc.S
 *
 * Copyright (C) 2001 Deep Blue Solutions Ltd.
 * Copyright (C) 2012 ARM Ltd.
 * Author: Catalin Marinas <catalin.marinas@arm.com>
 */

#include <linux/init.h>
#include <linux/linkage.h>
#include <linux/pgtable.h>
#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#include <asm/asm_pointer_auth.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/cpufeature.h>
#include <asm/alternative.h>
#include <asm/smp.h>
#include <asm/sysreg.h>

#ifdef CONFIG_ARM64_64K_PAGES
#define TCR_TG_FLAGS	TCR_TG0_64K | TCR_TG1_64K
#elif defined(CONFIG_ARM64_16K_PAGES)
#define TCR_TG_FLAGS	TCR_TG0_16K | TCR_TG1_16K
#else /* CONFIG_ARM64_4K_PAGES */
#define TCR_TG_FLAGS	TCR_TG0_4K | TCR_TG1_4K
#endif

#ifdef CONFIG_RANDOMIZE_BASE
#define TCR_KASLR_FLAGS	TCR_NFD1
#else
#define TCR_KASLR_FLAGS	0
#endif

#define TCR_SMP_FLAGS	TCR_SHARED

/* PTWs cacheable, inner/outer WBWA */
#define TCR_CACHE_FLAGS	TCR_IRGN_WBWA | TCR_ORGN_WBWA

#ifdef CONFIG_KASAN_SW_TAGS
#define TCR_KASAN_SW_FLAGS TCR_TBI1 | TCR_TBID1
#else
#define TCR_KASAN_SW_FLAGS 0
#endif

#ifdef CONFIG_KASAN_HW_TAGS
#define TCR_MTE_FLAGS SYS_TCR_EL1_TCMA1 | TCR_TBI1 | TCR_TBID1
#else
/*
 * The mte_zero_clear_page_tags() implementation uses DC GZVA, which relies on
 * TBI being enabled at EL1.
 */
#define TCR_MTE_FLAGS TCR_TBI1 | TCR_TBID1
#endif

/*
 * Default MAIR_EL1. MT_NORMAL_TAGGED is initially mapped as Normal memory and
 * changed during __cpu_setup to Normal Tagged if the system supports MTE.
 */
#define MAIR_EL1_SET							\
	(MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRnE, MT_DEVICE_nGnRnE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRE, MT_DEVICE_nGnRE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_DEVICE_GRE, MT_DEVICE_GRE) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_NC, MT_NORMAL_NC) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL) |			\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_WT, MT_NORMAL_WT) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL_TAGGED) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_iNC_oWB, MT_NORMAL_iNC_oWB))

#ifdef CONFIG_CPU_PM
/**
 * cpu_do_suspend - save CPU registers context
 *
 * x0: virtual address of context pointer
 *
 * This must be kept in sync with struct cpu_suspend_ctx in <asm/suspend.h>.
 */
SYM_FUNC_START(cpu_do_suspend)
	mrs	x2, tpidr_el0
	mrs	x3, tpidrro_el0
	mrs	x4, contextidr_el1
	mrs	x5, osdlr_el1
	mrs	x6, cpacr_el1
	mrs	x7, tcr_el1
	mrs	x8, vbar_el1
	mrs	x9, mdscr_el1
	mrs	x10, oslsr_el1
	mrs	x11, sctlr_el1
alternative_if_not ARM64_HAS_VIRT_HOST_EXTN
	mrs	x12, tpidr_el1
alternative_else
	mrs	x12, tpidr_el2
alternative_endif
	mrs	x13, sp_el0
	stp	x2, x3, [x0]
	stp	x4, x5, [x0, #16]
	stp	x6, x7, [x0, #32]
	stp	x8, x9, [x0, #48]
	stp	x10, x11, [x0, #64]
	stp	x12, x13, [x0, #80]
	/*
	 * Save x18 as it may be used as a platform register, e.g. by shadow
	 * call stack.
	 */
	str	x18, [x0, #96]
	ret
SYM_FUNC_END(cpu_do_suspend)

/**
 * cpu_do_resume - restore CPU register context
 *
 * x0: Address of context pointer
 */
	.pushsection ".idmap.text", "awx"
SYM_FUNC_START(cpu_do_resume)
	ldp	x2, x3, [x0]
	ldp	x4, x5, [x0, #16]
	ldp	x6, x8, [x0, #32]
	ldp	x9, x10, [x0, #48]
	ldp	x11, x12, [x0, #64]
	ldp	x13, x14, [x0, #80]
	/*
	 * Restore x18, as it may be used as a platform register, and clear
	 * the buffer to minimize the risk of exposure when used for shadow
	 * call stack.
	 */
	ldr	x18, [x0, #96]
	str	xzr, [x0, #96]
	msr	tpidr_el0, x2
	msr	tpidrro_el0, x3
	msr	contextidr_el1, x4
	msr	cpacr_el1, x6

	/* Don't change t0sz here, mask those bits when restoring */
	mrs	x7, tcr_el1
	bfi	x8, x7, TCR_T0SZ_OFFSET, TCR_TxSZ_WIDTH

	msr	tcr_el1, x8
	msr	vbar_el1, x9

	/*
	 * __cpu_setup() cleared MDSCR_EL1.MDE and friends, before unmasking
	 * debug exceptions. By restoring MDSCR_EL1 here, we may take a debug
	 * exception. Mask them until local_daif_restore() in cpu_suspend()
	 * resets them.
	 */
	disable_daif
	msr	mdscr_el1, x10

	msr	sctlr_el1, x12
alternative_if_not ARM64_HAS_VIRT_HOST_EXTN
	msr	tpidr_el1, x13
alternative_else
	msr	tpidr_el2, x13
alternative_endif
	msr	sp_el0, x14
	/*
	 * Restore oslsr_el1 by writing oslar_el1
	 */
	msr	osdlr_el1, x5
	ubfx	x11, x11, #1, #1
	msr	oslar_el1, x11
	reset_pmuserenr_el0 x0			// Disable PMU access from EL0
	reset_amuserenr_el0 x0			// Disable AMU access from EL0

alternative_if ARM64_HAS_RAS_EXTN
	msr_s	SYS_DISR_EL1, xzr
alternative_else_nop_endif

	ptrauth_keys_install_kernel_nosync x14, x1, x2, x3
	isb
	ret
SYM_FUNC_END(cpu_do_resume)
	.popsection
#endif

	.pushsection ".idmap.text", "awx"

.macro	__idmap_cpu_set_reserved_ttbr1, tmp1, tmp2
	adrp	\tmp1, reserved_pg_dir
	phys_to_ttbr \tmp2, \tmp1
	offset_ttbr1 \tmp2, \tmp1
	msr	ttbr1_el1, \tmp2
	isb
	tlbi	vmalle1
	dsb	nsh
	isb
.endm

/*
 * void idmap_cpu_replace_ttbr1(phys_addr_t ttbr1)
 *
 * This is the low-level counterpart to cpu_replace_ttbr1, and should not be
 * called by anything else. It can only be executed from a TTBR0 mapping.
 */
SYM_FUNC_START(idmap_cpu_replace_ttbr1)
	save_and_disable_daif flags=x2

	__idmap_cpu_set_reserved_ttbr1 x1, x3

	offset_ttbr1 x0, x3
	msr	ttbr1_el1, x0
	isb

	restore_daif x2

	ret
SYM_FUNC_END(idmap_cpu_replace_ttbr1)
	.popsection

#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
	.pushsection ".idmap.text", "awx"

	.macro	__idmap_kpti_get_pgtable_ent, type
	dc	cvac, cur_\()\type\()p		// Ensure any existing dirty
	dmb	sy				// lines are written back before
	ldr	\type, [cur_\()\type\()p]	// loading the entry
	tbz	\type, #0, skip_\()\type	// Skip invalid and
	tbnz	\type, #11, skip_\()\type	// non-global entries
	.endm

	.macro __idmap_kpti_put_pgtable_ent_ng, type
	orr	\type, \type, #PTE_NG		// Same bit for blocks and pages
	str	\type, [cur_\()\type\()p]	// Update the entry and ensure
	dmb	sy				// that it is visible to all
	dc	civac, cur_\()\type\()p		// CPUs.
	.endm

/*
 * void __kpti_install_ng_mappings(int cpu, int num_cpus, phys_addr_t swapper)
 *
 * Called exactly once from stop_machine context by each CPU found during boot.
 */
__idmap_kpti_flag:
	.long	1
SYM_FUNC_START(idmap_kpti_install_ng_mappings)
	cpu		.req	w0
	num_cpus	.req	w1
	swapper_pa	.req	x2
	swapper_ttb	.req	x3
	flag_ptr	.req	x4
	cur_pgdp	.req	x5
	end_pgdp	.req	x6
	pgd		.req	x7
	cur_pudp	.req	x8
	end_pudp	.req	x9
	pud		.req	x10
	cur_pmdp	.req	x11
	end_pmdp	.req	x12
	pmd		.req	x13
	cur_ptep	.req	x14
	end_ptep	.req	x15
	pte		.req	x16

	mrs	swapper_ttb, ttbr1_el1
	restore_ttbr1	swapper_ttb
	adr	flag_ptr, __idmap_kpti_flag

	cbnz	cpu, __idmap_kpti_secondary

	/* We're the boot CPU. Wait for the others to catch up */
	sevl
1:	wfe
	ldaxr	w17, [flag_ptr]
	eor	w17, w17, num_cpus
	cbnz	w17, 1b

	/* We need to walk swapper, so turn off the MMU. */
	pre_disable_mmu_workaround
	mrs	x17, sctlr_el1
	bic	x17, x17, #SCTLR_ELx_M
	msr	sctlr_el1, x17
	isb

	/* Everybody is enjoying the idmap, so we can rewrite swapper. */
	/* PGD */
	mov	cur_pgdp, swapper_pa
	add	end_pgdp, cur_pgdp, #(PTRS_PER_PGD * 8)
do_pgd:	__idmap_kpti_get_pgtable_ent	pgd
	tbnz	pgd, #1, walk_puds
next_pgd:
	__idmap_kpti_put_pgtable_ent_ng	pgd
skip_pgd:
	add	cur_pgdp, cur_pgdp, #8
	cmp	cur_pgdp, end_pgdp
	b.ne	do_pgd

	/* Publish the updated tables and nuke all the TLBs */
	dsb	sy
	tlbi	vmalle1is
	dsb	ish
	isb

	/* We're done: fire up the MMU again */
	mrs	x17, sctlr_el1
	orr	x17, x17, #SCTLR_ELx_M
	set_sctlr_el1	x17

	/* Set the flag to zero to indicate that we're all done */
	str	wzr, [flag_ptr]
	ret

	/* PUD */
walk_puds:
	.if CONFIG_PGTABLE_LEVELS > 3
	pte_to_phys	cur_pudp, pgd
	add	end_pudp, cur_pudp, #(PTRS_PER_PUD * 8)
do_pud:	__idmap_kpti_get_pgtable_ent	pud
	tbnz	pud, #1, walk_pmds
next_pud:
	__idmap_kpti_put_pgtable_ent_ng	pud
skip_pud:
	add	cur_pudp, cur_pudp, 8
	cmp	cur_pudp, end_pudp
	b.ne	do_pud
	b	next_pgd
	.else /* CONFIG_PGTABLE_LEVELS <= 3 */
	mov	pud, pgd
	b	walk_pmds
next_pud:
	b	next_pgd
	.endif

	/* PMD */
walk_pmds:
	.if CONFIG_PGTABLE_LEVELS > 2
	pte_to_phys	cur_pmdp, pud
	add	end_pmdp, cur_pmdp, #(PTRS_PER_PMD * 8)
do_pmd:	__idmap_kpti_get_pgtable_ent	pmd
	tbnz	pmd, #1, walk_ptes
next_pmd:
	__idmap_kpti_put_pgtable_ent_ng	pmd
skip_pmd:
	add	cur_pmdp, cur_pmdp, #8
	cmp	cur_pmdp, end_pmdp
	b.ne	do_pmd
	b	next_pud
	.else /* CONFIG_PGTABLE_LEVELS <= 2 */
	mov	pmd, pud
	b	walk_ptes
next_pmd:
	b	next_pud
	.endif

	/* PTE */
walk_ptes:
	pte_to_phys	cur_ptep, pmd
	add	end_ptep, cur_ptep, #(PTRS_PER_PTE * 8)
do_pte:	__idmap_kpti_get_pgtable_ent	pte
	__idmap_kpti_put_pgtable_ent_ng	pte
skip_pte:
	add	cur_ptep, cur_ptep, #8
	cmp	cur_ptep, end_ptep
	b.ne	do_pte
	b	next_pmd

	.unreq	cpu
	.unreq	num_cpus
	.unreq	swapper_pa
	.unreq	cur_pgdp
	.unreq	end_pgdp
	.unreq	pgd
	.unreq	cur_pudp
	.unreq	end_pudp
	.unreq	pud
	.unreq	cur_pmdp
	.unreq	end_pmdp
	.unreq	pmd
	.unreq	cur_ptep
	.unreq	end_ptep
	.unreq	pte

	/* Secondary CPUs end up here */
__idmap_kpti_secondary:
	/* Uninstall swapper before surgery begins */
	__idmap_cpu_set_reserved_ttbr1 x16, x17

	/* Increment the flag to let the boot CPU we're ready */
1:	ldxr	w16, [flag_ptr]
	add	w16, w16, #1
	stxr	w17, w16, [flag_ptr]
	cbnz	w17, 1b

	/* Wait for the boot CPU to finish messing around with swapper */
	sevl
1:	wfe
	ldxr	w16, [flag_ptr]
	cbnz	w16, 1b

	/* All done, act like nothing happened */
	offset_ttbr1 swapper_ttb, x16
	msr	ttbr1_el1, swapper_ttb
	isb
	ret

	.unreq	swapper_ttb
	.unreq	flag_ptr
SYM_FUNC_END(idmap_kpti_install_ng_mappings)
	.popsection
#endif

/*
 *	__cpu_setup
 *
 *	Initialise the processor for turning the MMU on.
 *
 * Output:
 *	Return in x0 the value of the SCTLR_EL1 register.
 */
	.pushsection ".idmap.text", "awx"
SYM_FUNC_START(__cpu_setup)
	tlbi	vmalle1				// Invalidate local TLB
	dsb	nsh

	mov	x1, #3 << 20
	msr	cpacr_el1, x1			// Enable FP/ASIMD
	mov	x1, #1 << 12			// Reset mdscr_el1 and disable
	msr	mdscr_el1, x1			// access to the DCC from EL0
	isb					// Unmask debug exceptions now,
	enable_dbg				// since this is per-cpu
	reset_pmuserenr_el0 x1			// Disable PMU access from EL0
	reset_amuserenr_el0 x1			// Disable AMU access from EL0

	/*
	 * Memory region attributes
	 */
	mov_q	x5, MAIR_EL1_SET
#ifdef CONFIG_ARM64_MTE
	mte_tcr	.req	x20

	mov	mte_tcr, #0

	/*
	 * Update MAIR_EL1, GCR_EL1 and TFSR*_EL1 if MTE is supported
	 * (ID_AA64PFR1_EL1[11:8] > 1).
	 */
	mrs	x10, ID_AA64PFR1_EL1
	ubfx	x10, x10, #ID_AA64PFR1_MTE_SHIFT, #4
	cmp	x10, #ID_AA64PFR1_MTE
	b.lt	1f

	/* Normal Tagged memory type at the corresponding MAIR index */
	mov	x10, #MAIR_ATTR_NORMAL_TAGGED
	bfi	x5, x10, #(8 *  MT_NORMAL_TAGGED), #8

	mov	x10, #KERNEL_GCR_EL1
	msr_s	SYS_GCR_EL1, x10

	/*
	 * If GCR_EL1.RRND=1 is implemented the same way as RRND=0, then
	 * RGSR_EL1.SEED must be non-zero for IRG to produce
	 * pseudorandom numbers. As RGSR_EL1 is UNKNOWN out of reset, we
	 * must initialize it.
	 */
	mrs	x10, CNTVCT_EL0
	ands	x10, x10, #SYS_RGSR_EL1_SEED_MASK
	csinc	x10, x10, xzr, ne
	lsl	x10, x10, #SYS_RGSR_EL1_SEED_SHIFT
	msr_s	SYS_RGSR_EL1, x10

	/* clear any pending tag check faults in TFSR*_EL1 */
	msr_s	SYS_TFSR_EL1, xzr
	msr_s	SYS_TFSRE0_EL1, xzr

	/* set the TCR_EL1 bits */
	mov_q	mte_tcr, TCR_MTE_FLAGS
1:
#endif
	msr	mair_el1, x5
	/*
	 * Set/prepare TCR and TTBR. TCR_EL1.T1SZ gets further
	 * adjusted if the kernel is compiled with 52bit VA support.
	 */
	mov_q	x10, TCR_TxSZ(VA_BITS) | TCR_CACHE_FLAGS | TCR_SMP_FLAGS | \
			TCR_TG_FLAGS | TCR_KASLR_FLAGS | TCR_ASID16 | \
			TCR_TBI0 | TCR_A1 | TCR_KASAN_SW_FLAGS
#ifdef CONFIG_ARM64_MTE
	orr	x10, x10, mte_tcr
	.unreq	mte_tcr
#endif
	tcr_clear_errata_bits x10, x9, x5

#ifdef CONFIG_ARM64_VA_BITS_52
	ldr_l		x9, vabits_actual
	sub		x9, xzr, x9
	add		x9, x9, #64
	tcr_set_t1sz	x10, x9
#else
	ldr_l		x9, idmap_t0sz
#endif
	tcr_set_t0sz	x10, x9

	/*
	 * Set the IPS bits in TCR_EL1.
	 */
	tcr_compute_pa_size x10, #TCR_IPS_SHIFT, x5, x6
#ifdef CONFIG_ARM64_HW_AFDBM
	/*
	 * Enable hardware update of the Access Flags bit.
	 * Hardware dirty bit management is enabled later,
	 * via capabilities.
	 */
	mrs	x9, ID_AA64MMFR1_EL1
	and	x9, x9, #0xf
	cbz	x9, 1f
	orr	x10, x10, #TCR_HA		// hardware Access flag update
1:
#endif	/* CONFIG_ARM64_HW_AFDBM */
	msr	tcr_el1, x10
	/*
	 * Prepare SCTLR
	 */
	mov_q	x0, INIT_SCTLR_EL1_MMU_ON
	ret					// return to head.S
SYM_FUNC_END(__cpu_setup)

```

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\mmu_context.h**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/mmu_context.h
 *
 * Copyright (C) 1996 Russell King.
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_MMU_CONTEXT_H
#define __ASM_MMU_CONTEXT_H

#ifndef __ASSEMBLY__

#include <linux/compiler.h>
#include <linux/sched.h>
#include <linux/sched/hotplug.h>
#include <linux/mm_types.h>
#include <linux/pgtable.h>

#include <asm/cacheflush.h>
#include <asm/cpufeature.h>
#include <asm/proc-fns.h>
#include <asm-generic/mm_hooks.h>
#include <asm/cputype.h>
#include <asm/sysreg.h>
#include <asm/tlbflush.h>

extern bool rodata_full;

static inline void contextidr_thread_switch(struct task_struct *next)
{
	if (!IS_ENABLED(CONFIG_PID_IN_CONTEXTIDR))
		return;

	write_sysreg(task_pid_nr(next), contextidr_el1);
	isb();
}

/*
 * Set TTBR0 to reserved_pg_dir. No translations will be possible via TTBR0.
 */
static inline void cpu_set_reserved_ttbr0(void)
{
	unsigned long ttbr = phys_to_ttbr(__pa_symbol(reserved_pg_dir));

	write_sysreg(ttbr, ttbr0_el1);
	isb();
}

void cpu_do_switch_mm(phys_addr_t pgd_phys, struct mm_struct *mm);

static inline void cpu_switch_mm(pgd_t *pgd, struct mm_struct *mm)
{
	BUG_ON(pgd == swapper_pg_dir);
	cpu_set_reserved_ttbr0();
	cpu_do_switch_mm(virt_to_phys(pgd),mm);
}

/*
 * TCR.T0SZ value to use when the ID map is active. Usually equals
 * TCR_T0SZ(VA_BITS), unless system RAM is positioned very high in
 * physical memory, in which case it will be smaller.
 */
extern u64 idmap_t0sz;
extern u64 idmap_ptrs_per_pgd;

static inline bool __cpu_uses_extended_idmap(void)
{
	return unlikely(idmap_t0sz != TCR_T0SZ(vabits_actual));
}

/*
 * True if the extended ID map requires an extra level of translation table
 * to be configured.
 */
static inline bool __cpu_uses_extended_idmap_level(void)
{
	return ARM64_HW_PGTABLE_LEVELS(64 - idmap_t0sz) > CONFIG_PGTABLE_LEVELS;
}

/*
 * Set TCR.T0SZ to its default value (based on VA_BITS)
 */
static inline void __cpu_set_tcr_t0sz(unsigned long t0sz)
{
	unsigned long tcr;

	if (!__cpu_uses_extended_idmap())
		return;

	tcr = read_sysreg(tcr_el1);
	tcr &= ~TCR_T0SZ_MASK;
	tcr |= t0sz << TCR_T0SZ_OFFSET;
	write_sysreg(tcr, tcr_el1);
	isb();
}

#define cpu_set_default_tcr_t0sz()	__cpu_set_tcr_t0sz(TCR_T0SZ(vabits_actual))
#define cpu_set_idmap_tcr_t0sz()	__cpu_set_tcr_t0sz(idmap_t0sz)

/*
 * Remove the idmap from TTBR0_EL1 and install the pgd of the active mm.
 *
 * The idmap lives in the same VA range as userspace, but uses global entries
 * and may use a different TCR_EL1.T0SZ. To avoid issues resulting from
 * speculative TLB fetches, we must temporarily install the reserved page
 * tables while we invalidate the TLBs and set up the correct TCR_EL1.T0SZ.
 *
 * If current is a not a user task, the mm covers the TTBR1_EL1 page tables,
 * which should not be installed in TTBR0_EL1. In this case we can leave the
 * reserved page tables in place.
 */
static inline void cpu_uninstall_idmap(void)
{
	struct mm_struct *mm = current->active_mm;

	cpu_set_reserved_ttbr0();
	local_flush_tlb_all();
	cpu_set_default_tcr_t0sz();

	if (mm != &init_mm && !system_uses_ttbr0_pan())
		cpu_switch_mm(mm->pgd, mm);
}

static inline void cpu_install_idmap(void)
{
	cpu_set_reserved_ttbr0();
	local_flush_tlb_all();
	cpu_set_idmap_tcr_t0sz();

	cpu_switch_mm(lm_alias(idmap_pg_dir), &init_mm);
}

/*
 * Atomically replaces the active TTBR1_EL1 PGD with a new VA-compatible PGD,
 * avoiding the possibility of conflicting TLB entries being allocated.
 */
static inline void __nocfi cpu_replace_ttbr1(pgd_t *pgdp)
{
	typedef void (ttbr_replace_func)(phys_addr_t);
	extern ttbr_replace_func idmap_cpu_replace_ttbr1;
	ttbr_replace_func *replace_phys;

	/* phys_to_ttbr() zeros lower 2 bits of ttbr with 52-bit PA */
	phys_addr_t ttbr1 = phys_to_ttbr(virt_to_phys(pgdp));

	if (system_supports_cnp() && !WARN_ON(pgdp != lm_alias(swapper_pg_dir))) {
		/*
		 * cpu_replace_ttbr1() is used when there's a boot CPU
		 * up (i.e. cpufeature framework is not up yet) and
		 * latter only when we enable CNP via cpufeature's
		 * enable() callback.
		 * Also we rely on the cpu_hwcap bit being set before
		 * calling the enable() function.
		 */
		ttbr1 |= TTBR_CNP_BIT;
	}

	replace_phys = (void *)__pa_function(idmap_cpu_replace_ttbr1);

	cpu_install_idmap();
	replace_phys(ttbr1);
	cpu_uninstall_idmap();
}

/*
 * It would be nice to return ASIDs back to the allocator, but unfortunately
 * that introduces a race with a generation rollover where we could erroneously
 * free an ASID allocated in a future generation. We could workaround this by
 * freeing the ASID from the context of the dying mm (e.g. in arch_exit_mmap),
 * but we'd then need to make sure that we didn't dirty any TLBs afterwards.
 * Setting a reserved TTBR0 or EPD0 would work, but it all gets ugly when you
 * take CPU migration into account.
 */
#define destroy_context(mm)		do { } while(0)
void check_and_switch_context(struct mm_struct *mm);

static inline int
init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
	atomic64_set(&mm->context.id, 0);
	refcount_set(&mm->context.pinned, 0);
	return 0;
}

#ifdef CONFIG_ARM64_SW_TTBR0_PAN
static inline void update_saved_ttbr0(struct task_struct *tsk,
				      struct mm_struct *mm)
{
	u64 ttbr;

	if (!system_uses_ttbr0_pan())
		return;

	if (mm == &init_mm)
		ttbr = phys_to_ttbr(__pa_symbol(reserved_pg_dir));
	else
		ttbr = phys_to_ttbr(virt_to_phys(mm->pgd)) | ASID(mm) << 48;

	WRITE_ONCE(task_thread_info(tsk)->ttbr0, ttbr);
}
#else
static inline void update_saved_ttbr0(struct task_struct *tsk,
				      struct mm_struct *mm)
{
}
#endif

static inline void
enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
	/*
	 * We don't actually care about the ttbr0 mapping, so point it at the
	 * zero page.
	 */
	update_saved_ttbr0(tsk, &init_mm);
}

static inline void __switch_mm(struct mm_struct *next)
{
	/*
	 * init_mm.pgd does not contain any user mappings and it is always
	 * active for kernel addresses in TTBR1. Just set the reserved TTBR0.
	 */
	if (next == &init_mm) {
		cpu_set_reserved_ttbr0();
		return;
	}

	check_and_switch_context(next);
}

static inline void
switch_mm(struct mm_struct *prev, struct mm_struct *next,
	  struct task_struct *tsk)
{
	if (prev != next)
		__switch_mm(next);

	/*
	 * Update the saved TTBR0_EL1 of the scheduled-in task as the previous
	 * value may have not been initialised yet (activate_mm caller) or the
	 * ASID has changed since the last run (following the context switch
	 * of another thread of the same process).
	 */
	update_saved_ttbr0(tsk, next);
}

#define deactivate_mm(tsk,mm)	do { } while (0)
#define activate_mm(prev,next)	switch_mm(prev, next, current)

static inline const struct cpumask *
task_cpu_possible_mask(struct task_struct *p)
{
	if (!static_branch_unlikely(&arm64_mismatched_32bit_el0))
		return cpu_possible_mask;

	if (!is_compat_thread(task_thread_info(p)))
		return cpu_possible_mask;

	return system_32bit_el0_cpumask();
}
#define task_cpu_possible_mask	task_cpu_possible_mask

void verify_cpu_asid_bits(void);
void post_ttbr_update_workaround(void);

unsigned long arm64_mm_context_get(struct mm_struct *mm);
void arm64_mm_context_put(struct mm_struct *mm);

#endif /* !__ASSEMBLY__ */

#endif /* !__ASM_MMU_CONTEXT_H */

```




```bash
$matches = Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source" -Recurse -File -Include .config,defconfig,*.config | Select-String -Pattern "CONFIG_ARM64_4K_PAGES|CONFIG_ARM64_16K_PAGES|CONFIG_ARM64_64K_PAGES|CONFIG_ARM64_VA_BITS="; if ($matches) { $matches | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() } } else { "NO_MATCH" }
```

**Current Output:**
```

<|cursor|>
```

Subagent: ea74cfbd-9215-442a-88aa-1df737162908


**Command Executed:**
```bash
$m = Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\include\linux" -Recurse -File | Select-String -Pattern "struct page \{"; $m | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() }
```

**Output:**
```
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\include\linux\mm_types.h:71:struct page {
```



**Files Read:**

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\include\linux\mm_types.h**
```
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#include <linux/mm_types_task.h>

#include <linux/auxvec.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rbtree.h>
#include <linux/rwsem.h>
#include <linux/completion.h>
#include <linux/cpumask.h>
#include <linux/uprobes.h>
#include <linux/page-flags-layout.h>
#include <linux/workqueue.h>
#include <linux/seqlock.h>
#include <linux/android_kabi.h>

#include <asm/mmu.h>

#ifndef AT_VECTOR_SIZE_ARCH
#define AT_VECTOR_SIZE_ARCH 0
#endif
#define AT_VECTOR_SIZE (2*(AT_VECTOR_SIZE_ARCH + AT_VECTOR_SIZE_BASE + 1))

#define INIT_PASID	0

struct address_space;
struct mem_cgroup;

/*
 * Each physical page in the system has a struct page associated with
 * it to keep track of whatever it is we are using the page for at the
 * moment. Note that we have no way to track which tasks are using
 * a page, though if it is a pagecache page, rmap structures can tell us
 * who is mapping it.
 *
 * If you allocate the page using alloc_pages(), you can use some of the
 * space in struct page for your own purposes.  The five words in the main
 * union are available, except for bit 0 of the first word which must be
 * kept clear.  Many users use this word to store a pointer to an object
 * which is guaranteed to be aligned.  If you use the same storage as
 * page->mapping, you must restore it to NULL before freeing the page.
 *
 * If your page will not be mapped to userspace, you can also use the four
 * bytes in the mapcount union, but you must call page_mapcount_reset()
 * before freeing it.
 *
 * If you want to use the refcount field, it must be used in such a way
 * that other CPUs temporarily incrementing and then decrementing the
 * refcount does not cause problems.  On receiving the page from
 * alloc_pages(), the refcount will be positive.
 *
 * If you allocate pages of order > 0, you can use some of the fields
 * in each subpage, but you may need to restore some of their values
 * afterwards.
 *
 * SLUB uses cmpxchg_double() to atomically update its freelist and
 * counters.  That requires that freelist & counters be adjacent and
 * double-word aligned.  We align all struct pages to double-word
 * boundaries, and ensure that 'freelist' is aligned within the
 * struct.
 */
#ifdef CONFIG_HAVE_ALIGNED_STRUCT_PAGE
#define _struct_page_alignment	__aligned(2 * sizeof(unsigned long))
#else
#define _struct_page_alignment
#endif

struct page {
	unsigned long flags;		/* Atomic flags, some possibly
					 * updated asynchronously */
	/*
	 * Five words (20/40 bytes) are available in this union.
	 * WARNING: bit 0 of the first word is used for PageTail(). That
	 * means the other users of this union MUST NOT use the bit to
	 * avoid collision and false-positive PageTail().
	 */
	union {
		struct {	/* Page cache and anonymous pages */
			/**
			 * @lru: Pageout list, eg. active_list protected by
			 * pgdat->lru_lock.  Sometimes used as a generic list
			 * by the page owner.
			 */
			struct list_head lru;
			/* See page-flags.h for PAGE_MAPPING_FLAGS */
			struct address_space *mapping;
			pgoff_t index;		/* Our offset within mapping. */
			/**
			 * @private: Mapping-private opaque data.
			 * Usually used for buffer_heads if PagePrivate.
			 * Used for swp_entry_t if PageSwapCache.
			 * Indicates order in the buddy system if PageBuddy.
			 */
			unsigned long private;
		};
		struct {	/* page_pool used by netstack */
			/**
			 * @dma_addr: might require a 64-bit value on
			 * 32-bit architectures.
			 */
			unsigned long dma_addr[2];
		};
		struct {	/* slab, slob and slub */
			union {
				struct list_head slab_list;
				struct {	/* Partial pages */
					struct page *next;
#ifdef CONFIG_64BIT
					int pages;	/* Nr of pages left */
					int pobjects;	/* Approximate count */
#else
					short int pages;
					short int pobjects;
#endif
				};
			};
			struct kmem_cache *slab_cache; /* not slob */
			/* Double-word boundary */
			void *freelist;		/* first free object */
			union {
				void *s_mem;	/* slab: first object */
				unsigned long counters;		/* SLUB */
				struct {			/* SLUB */
					unsigned inuse:16;
					unsigned objects:15;
					unsigned frozen:1;
				};
			};
		};
		struct {	/* Tail pages of compound page */
			unsigned long compound_head;	/* Bit zero is set */

			/* First tail page only */
			unsigned char compound_dtor;
			unsigned char compound_order;
			atomic_t compound_mapcount;
			unsigned int compound_nr; /* 1 << compound_order */
		};
		struct {	/* Second tail page of compound page */
			unsigned long _compound_pad_1;	/* compound_head */
			atomic_t hpage_pinned_refcount;
			/* For both global and memcg */
			struct list_head deferred_list;
		};
		struct {	/* Page table pages */
			unsigned long _pt_pad_1;	/* compound_head */
			pgtable_t pmd_huge_pte; /* protected by page->ptl */
			unsigned long _pt_pad_2;	/* mapping */
			union {
				struct mm_struct *pt_mm; /* x86 pgds only */
				atomic_t pt_frag_refcount; /* powerpc */
			};
#if ALLOC_SPLIT_PTLOCKS
			spinlock_t *ptl;
#else
			spinlock_t ptl;
#endif
		};
		struct {	/* ZONE_DEVICE pages */
			/** @pgmap: Points to the hosting device page map. */
			struct dev_pagemap *pgmap;
			void *zone_device_data;
			/*
			 * ZONE_DEVICE private pages are counted as being
			 * mapped so the next 3 words hold the mapping, index,
			 * and private fields from the source anonymous or
			 * page cache page while the page is migrated to device
			 * private memory.
			 * ZONE_DEVICE MEMORY_DEVICE_FS_DAX pages also
			 * use the mapping, index, and private fields when
			 * pmem backed DAX files are mapped.
			 */
		};

		/** @rcu_head: You can use this to free a page by RCU. */
		struct rcu_head rcu_head;
	};

	union {		/* This union is 4 bytes in size. */
		/*
		 * If the page can be mapped to userspace, encodes the number
		 * of times this page is referenced by a page table.
		 */
		atomic_t _mapcount;

		/*
		 * If the page is neither PageSlab nor mappable to userspace,
		 * the value stored here may help determine what this page
		 * is used for.  See page-flags.h for a list of page types
		 * which are currently stored here.
		 */
		unsigned int page_type;

		unsigned int active;		/* SLAB */
		int units;			/* SLOB */
	};

	/* Usage count. *DO NOT USE DIRECTLY*. See page_ref.h */
	atomic_t _refcount;

#ifdef CONFIG_MEMCG
	union {
		struct mem_cgroup *mem_cgroup;
		struct obj_cgroup **obj_cgroups;
	};
#endif

	/*
	 * On machines where all RAM is mapped into kernel address space,
	 * we can simply calculate the virtual address. On machines with
	 * highmem some memory is mapped into kernel virtual memory
	 * dynamically, so we need a place to store that address.
	 * Note that this field could be 16 bits on x86 ... ;)
	 *
	 * Architectures with slow multiplication can define
	 * WANT_PAGE_VIRTUAL in asm/page.h
	 */
#if defined(WANT_PAGE_VIRTUAL)
	void *virtual;			/* Kernel virtual address (NULL if
					   not kmapped, ie. highmem) */
#endif /* WANT_PAGE_VIRTUAL */

#ifdef LAST_CPUPID_NOT_IN_PAGE_FLAGS
	int _last_cpupid;
#endif
} _struct_page_alignment;

static inline atomic_t *compound_mapcount_ptr(struct page *page)
{
	return &page[1].compound_mapcount;
}

static inline atomic_t *compound_pincount_ptr(struct page *page)
{
	return &page[2].hpage_pinned_refcount;
}

/*
 * Used for sizing the vmemmap region on some architectures
 */
#define STRUCT_PAGE_MAX_SHIFT	(order_base_2(sizeof(struct page)))

#define PAGE_FRAG_CACHE_MAX_SIZE	__ALIGN_MASK(32768, ~PAGE_MASK)
#define PAGE_FRAG_CACHE_MAX_ORDER	get_order(PAGE_FRAG_CACHE_MAX_SIZE)

#define page_private(page)		((page)->private)

static inline void set_page_private(struct page *page, unsigned long private)
{
	page->private = private;
}

struct page_frag_cache {
	void * va;
#if (PAGE_SIZE < PAGE_FRAG_CACHE_MAX_SIZE)
	__u16 offset;
	__u16 size;
#else
	__u32 offset;
#endif
	/* we maintain a pagecount bias, so that we dont dirty cache line
	 * containing page->_refcount every time we allocate a fragment.
	 */
	unsigned int		pagecnt_bias;
	bool pfmemalloc;
};

typedef unsigned long vm_flags_t;

/*
 * A region containing a mapping of a non-memory backed file under NOMMU
 * conditions.  These are held in a global tree and are pinned by the VMAs that
 * map parts of them.
 */
struct vm_region {
	struct rb_node	vm_rb;		/* link in global region tree */
	vm_flags_t	vm_flags;	/* VMA vm_flags */
	unsigned long	vm_start;	/* start address of region */
	unsigned long	vm_end;		/* region initialised to here */
	unsigned long	vm_top;		/* region allocated to here */
	unsigned long	vm_pgoff;	/* the offset in vm_file corresponding to vm_start */
	struct file	*vm_file;	/* the backing file or NULL */

	int		vm_usage;	/* region usage count (access under nommu_region_sem) */
	bool		vm_icache_flushed : 1; /* true if the icache has been flushed for
						* this region */
};

#ifdef CONFIG_USERFAULTFD
#define NULL_VM_UFFD_CTX ((struct vm_userfaultfd_ctx) { NULL, })
struct vm_userfaultfd_ctx {
	struct userfaultfd_ctx *ctx;
};
#else /* CONFIG_USERFAULTFD */
#define NULL_VM_UFFD_CTX ((struct vm_userfaultfd_ctx) {})
struct vm_userfaultfd_ctx {};
#endif /* CONFIG_USERFAULTFD */

/*
 * This struct describes a virtual memory area. There is one of these
 * per VM-area/task. A VM area is any part of the process virtual memory
 * space that has a special rule for the page-fault handlers (ie a shared
 * library, the executable area etc).
 */
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */

	unsigned long vm_start;		/* Our start address within vm_mm. */
	unsigned long vm_end;		/* The first byte after our end address
					   within vm_mm. */

	/* linked list of VM areas per task, sorted by address */
	struct vm_area_struct *vm_next, *vm_prev;

	struct rb_node vm_rb;

	/*
	 * Largest free memory gap in bytes to the left of this VMA.
	 * Either between this VMA and vma->vm_prev, or between one of the
	 * VMAs below us in the VMA rbtree and its ->vm_prev. This helps
	 * get_unmapped_area find a free area of the right size.
	 */
	unsigned long rb_subtree_gap;

	/* Second cache line starts here. */

	struct mm_struct *vm_mm;	/* The address space we belong to. */

	/*
	 * Access permissions of this VMA.
	 * See vmf_insert_mixed_prot() for discussion.
	 */
	pgprot_t vm_page_prot;
	unsigned long vm_flags;		/* Flags, see mm.h. */

	/*
	 * For areas with an address space and backing store,
	 * linkage into the address_space->i_mmap interval tree.
	 *
	 * For private anonymous mappings, a pointer to a null terminated string
	 * in the user process containing the name given to the vma, or NULL
	 * if unnamed.
	 */
	union {
		struct {
			struct rb_node rb;
			unsigned long rb_subtree_last;
		} shared;
		const char __user *anon_name;
	};

	/*
	 * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
	 * list, after a COW of one of the file pages.	A MAP_SHARED vma
	 * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
	 * or brk vma (with NULL file) can only be in an anon_vma list.
	 */
	struct list_head anon_vma_chain; /* Serialized by mmap_lock &
					  * page_table_lock */
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */

	/* Function pointers to deal with this struct. */
	const struct vm_operations_struct *vm_ops;

	/* Information about our backing store: */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units */
	struct file * vm_file;		/* File we map to (can be NULL). */
	void * vm_private_data;		/* was vm_pte (shared mem) */

#ifdef CONFIG_SWAP
	atomic_long_t swap_readahead_info;
#endif
#ifndef CONFIG_MMU
	struct vm_region *vm_region;	/* NOMMU mapping region */
#endif
#ifdef CONFIG_NUMA
	struct mempolicy *vm_policy;	/* NUMA policy for the VMA */
#endif
	struct vm_userfaultfd_ctx vm_userfaultfd_ctx;
#ifdef CONFIG_SPECULATIVE_PAGE_FAULT
	seqcount_t vm_sequence;
	atomic_t vm_ref_count;		/* see vma_get(), vma_put() */
#endif

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
} __randomize_layout;

struct core_thread {
	struct task_struct *task;
	struct core_thread *next;
};

struct core_state {
	atomic_t nr_threads;
	struct core_thread dumper;
	struct completion startup;
};

struct kioctx_table;
struct mm_struct {
	struct {
		struct vm_area_struct *mmap;		/* list of VMAs */
		struct rb_root mm_rb;
		u64 vmacache_seqnum;                   /* per-thread vmacache */
#ifdef CONFIG_SPECULATIVE_PAGE_FAULT
		rwlock_t mm_rb_lock;
#endif
#ifdef CONFIG_MMU
		unsigned long (*get_unmapped_area) (struct file *filp,
				unsigned long addr, unsigned long len,
				unsigned long pgoff, unsigned long flags);
#endif
		unsigned long mmap_base;	/* base of mmap area */
		unsigned long mmap_legacy_base;	/* base of mmap area in bottom-up allocations */
#ifdef CONFIG_HAVE_ARCH_COMPAT_MMAP_BASES
		/* Base adresses for compatible mmap() */
		unsigned long mmap_compat_base;
		unsigned long mmap_compat_legacy_base;
#endif
		unsigned long task_size;	/* size of task vm space */
		unsigned long highest_vm_end;	/* highest vma end address */
		pgd_t * pgd;

#ifdef CONFIG_MEMBARRIER
		/**
		 * @membarrier_state: Flags controlling membarrier behavior.
		 *
		 * This field is close to @pgd to hopefully fit in the same
		 * cache-line, which needs to be touched by switch_mm().
		 */
		atomic_t membarrier_state;
#endif

		/**
		 * @mm_users: The number of users including userspace.
		 *
		 * Use mmget()/mmget_not_zero()/mmput() to modify. When this
		 * drops to 0 (i.e. when the task exits and there are no other
		 * temporary reference holders), we also release a reference on
		 * @mm_count (which may then free the &struct mm_struct if
		 * @mm_count also drops to 0).
		 */
		atomic_t mm_users;

		/**
		 * @mm_count: The number of references to &struct mm_struct
		 * (@mm_users count as 1).
		 *
		 * Use mmgrab()/mmdrop() to modify. When this drops to 0, the
		 * &struct mm_struct is freed.
		 */
		atomic_t mm_count;

		/**
		 * @has_pinned: Whether this mm has pinned any pages.  This can
		 * be either replaced in the future by @pinned_vm when it
		 * becomes stable, or grow into a counter on its own. We're
		 * aggresive on this bit now - even if the pinned pages were
		 * unpinned later on, we'll still keep this bit set for the
		 * lifecycle of this mm just for simplicity.
		 */
		atomic_t has_pinned;

#ifdef CONFIG_MMU
		atomic_long_t pgtables_bytes;	/* PTE page table pages */
#endif
		int map_count;			/* number of VMAs */

		spinlock_t page_table_lock; /* Protects page tables and some
					     * counters
					     */
		/*
		 * With some kernel config, the current mmap_lock's offset
		 * inside 'mm_struct' is at 0x120, which is very optimal, as
		 * its two hot fields 'count' and 'owner' sit in 2 different
		 * cachelines,  and when mmap_lock is highly contended, both
		 * of the 2 fields will be accessed frequently, current layout
		 * will help to reduce cache bouncing.
		 *
		 * So please be careful with adding new fields before
		 * mmap_lock, which can easily push the 2 fields into one
		 * cacheline.
		 */
		struct rw_semaphore mmap_lock;

		struct list_head mmlist; /* List of maybe swapped mm's.	These
					  * are globally strung together off
					  * init_mm.mmlist, and are protected
					  * by mmlist_lock
					  */


		unsigned long hiwater_rss; /* High-watermark of RSS usage */
		unsigned long hiwater_vm;  /* High-water virtual memory usage */

		unsigned long total_vm;	   /* Total pages mapped */
		unsigned long locked_vm;   /* Pages that have PG_mlocked set */
		atomic64_t    pinned_vm;   /* Refcount permanently increased */
		unsigned long data_vm;	   /* VM_WRITE & ~VM_SHARED & ~VM_STACK */
		unsigned long exec_vm;	   /* VM_EXEC & ~VM_WRITE & ~VM_STACK */
		unsigned long stack_vm;	   /* VM_STACK */
		unsigned long def_flags;

		/**
		 * @write_protect_seq: Locked when any thread is write
		 * protecting pages mapped by this mm to enforce a later COW,
		 * for instance during page table copying for fork().
		 */
		seqcount_t write_protect_seq;

		spinlock_t arg_lock; /* protect the below fields */

		unsigned long start_code, end_code, start_data, end_data;
		unsigned long start_brk, brk, start_stack;
		unsigned long arg_start, arg_end, env_start, env_end;

		unsigned long saved_auxv[AT_VECTOR_SIZE]; /* for /proc/PID/auxv */

		/*
		 * Special counters, in some configurations protected by the
		 * page_table_lock, in other configurations by being atomic.
		 */
		struct mm_rss_stat rss_stat;

		struct linux_binfmt *binfmt;

		/* Architecture-specific MM context */
		mm_context_t context;

		unsigned long flags; /* Must use atomic bitops to access */

		struct core_state *core_state; /* coredumping support */

#ifdef CONFIG_AIO
		spinlock_t			ioctx_lock;
		struct kioctx_table __rcu	*ioctx_table;
#endif
#ifdef CONFIG_MEMCG
		/*
		 * "owner" points to a task that is regarded as the canonical
		 * user/owner of this mm. All of the following must be true in
		 * order for it to be changed:
		 *
		 * current == mm->owner
		 * current->mm != mm
		 * new_owner->mm == mm
		 * new_owner->alloc_lock is held
		 */
		struct task_struct __rcu *owner;
#endif
		struct user_namespace *user_ns;

		/* store ref to file /proc/<pid>/exe symlink points to */
		struct file __rcu *exe_file;
#ifdef CONFIG_MMU_NOTIFIER
		struct mmu_notifier_subscriptions *notifier_subscriptions;
#endif
#if defined(CONFIG_TRANSPARENT_HUGEPAGE) && !USE_SPLIT_PMD_PTLOCKS
		pgtable_t pmd_huge_pte; /* protected by page_table_lock */
#endif
#ifdef CONFIG_NUMA_BALANCING
		/*
		 * numa_next_scan is the next time that the PTEs will be marked
		 * pte_numa. NUMA hinting faults will gather statistics and
		 * migrate pages to new nodes if necessary.
		 */
		unsigned long numa_next_scan;

		/* Restart point for scanning and setting pte_numa */
		unsigned long numa_scan_offset;

		/* numa_scan_seq prevents two threads setting pte_numa */
		int numa_scan_seq;
#endif
		/*
		 * An operation with batched TLB flushing is going on. Anything
		 * that can move process memory needs to flush the TLB when
		 * moving a PROT_NONE or PROT_NUMA mapped page.
		 */
		atomic_t tlb_flush_pending;
#ifdef CONFIG_ARCH_WANT_BATCHED_UNMAP_TLB_FLUSH
		/* See flush_tlb_batched_pending() */
		bool tlb_flush_batched;
#endif
		struct uprobes_state uprobes_state;
#ifdef CONFIG_HUGETLB_PAGE
		atomic_long_t hugetlb_usage;
#endif
		struct work_struct async_put_work;

#ifdef CONFIG_IOMMU_SUPPORT
		u32 pasid;
#endif

		ANDROID_KABI_RESERVE(1);
	} __randomize_layout;

	/*
	 * The mm_cpumask needs to be at the end of mm_struct, because it
	 * is dynamically sized based on nr_cpu_ids.
	 */
	unsigned long cpu_bitmap[];
};

extern struct mm_struct init_mm;

/* Pointer magic because the dynamic array size confuses some compilers. */
static inline void mm_init_cpumask(struct mm_struct *mm)
{
	unsigned long cpu_bitmap = (unsigned long)mm;

	cpu_bitmap += offsetof(struct mm_struct, cpu_bitmap);
	cpumask_clear((struct cpumask *)cpu_bitmap);
}

/* Future-safe accessor for struct mm_struct's cpu_vm_mask. */
static inline cpumask_t *mm_cpumask(struct mm_struct *mm)
{
	return (struct cpumask *)&mm->cpu_bitmap;
}

struct mmu_gather;
extern void tlb_gather_mmu(struct mmu_gather *tlb, struct mm_struct *mm,
				unsigned long start, unsigned long end);
extern void tlb_finish_mmu(struct mmu_gather *tlb,
				unsigned long start, unsigned long end);

static inline void init_tlb_flush_pending(struct mm_struct *mm)
{
	atomic_set(&mm->tlb_flush_pending, 0);
}

static inline void inc_tlb_flush_pending(struct mm_struct *mm)
{
	atomic_inc(&mm->tlb_flush_pending);
	/*
	 * The only time this value is relevant is when there are indeed pages
	 * to flush. And we'll only flush pages after changing them, which
	 * requires the PTL.
	 *
	 * So the ordering here is:
	 *
	 *	atomic_inc(&mm->tlb_flush_pending);
	 *	spin_lock(&ptl);
	 *	...
	 *	set_pte_at();
	 *	spin_unlock(&ptl);
	 *
	 *				spin_lock(&ptl)
	 *				mm_tlb_flush_pending();
	 *				....
	 *				spin_unlock(&ptl);
	 *
	 *	flush_tlb_range();
	 *	atomic_dec(&mm->tlb_flush_pending);
	 *
	 * Where the increment if constrained by the PTL unlock, it thus
	 * ensures that the increment is visible if the PTE modification is
	 * visible. After all, if there is no PTE modification, nobody cares
	 * about TLB flushes either.
	 *
	 * This very much relies on users (mm_tlb_flush_pending() and
	 * mm_tlb_flush_nested()) only caring about _specific_ PTEs (and
	 * therefore specific PTLs), because with SPLIT_PTE_PTLOCKS and RCpc
	 * locks (PPC) the unlock of one doesn't order against the lock of
	 * another PTL.
	 *
	 * The decrement is ordered by the flush_tlb_range(), such that
	 * mm_tlb_flush_pending() will not return false unless all flushes have
	 * completed.
	 */
}

static inline void dec_tlb_flush_pending(struct mm_struct *mm)
{
	/*
	 * See inc_tlb_flush_pending().
	 *
	 * This cannot be smp_mb__before_atomic() because smp_mb() simply does
	 * not order against TLB invalidate completion, which is what we need.
	 *
	 * Therefore we must rely on tlb_flush_*() to guarantee order.
	 */
	atomic_dec(&mm->tlb_flush_pending);
}

static inline bool mm_tlb_flush_pending(struct mm_struct *mm)
{
	/*
	 * Must be called after having acquired the PTL; orders against that
	 * PTLs release and therefore ensures that if we observe the modified
	 * PTE we must also observe the increment from inc_tlb_flush_pending().
	 *
	 * That is, it only guarantees to return true if there is a flush
	 * pending for _this_ PTL.
	 */
	return atomic_read(&mm->tlb_flush_pending);
}

static inline bool mm_tlb_flush_nested(struct mm_struct *mm)
{
	/*
	 * Similar to mm_tlb_flush_pending(), we must have acquired the PTL
	 * for which there is a TLB flush pending in order to guarantee
	 * we've seen both that PTE modification and the increment.
	 *
	 * (no requirement on actually still holding the PTL, that is irrelevant)
	 */
	return atomic_read(&mm->tlb_flush_pending) > 1;
}

struct vm_fault;

/**
 * typedef vm_fault_t - Return type for page fault handlers.
 *
 * Page fault handlers return a bitmask of %VM_FAULT values.
 */
typedef __bitwise unsigned int vm_fault_t;

/**
 * enum vm_fault_reason - Page fault handlers return a bitmask of
 * these values to tell the core VM what happened when handling the
 * fault. Used to decide whether a process gets delivered SIGBUS or
 * just gets major/minor fault counters bumped up.
 *
 * @VM_FAULT_OOM:		Out Of Memory
 * @VM_FAULT_SIGBUS:		Bad access
 * @VM_FAULT_MAJOR:		Page read from storage
 * @VM_FAULT_WRITE:		Special case for get_user_pages
 * @VM_FAULT_HWPOISON:		Hit poisoned small page
 * @VM_FAULT_HWPOISON_LARGE:	Hit poisoned large page. Index encoded
 *				in upper bits
 * @VM_FAULT_SIGSEGV:		segmentation fault
 * @VM_FAULT_NOPAGE:		->fault installed the pte, not return page
 * @VM_FAULT_LOCKED:		->fault locked the returned page
 * @VM_FAULT_RETRY:		->fault blocked, must retry
 * @VM_FAULT_FALLBACK:		huge page fault failed, fall back to small
 * @VM_FAULT_DONE_COW:		->fault has fully handled COW
 * @VM_FAULT_NEEDDSYNC:		->fault did not modify page tables and needs
 *				fsync() to complete (for synchronous page faults
 *				in DAX)
 * @VM_FAULT_HINDEX_MASK:	mask HINDEX value
 *
 */
enum vm_fault_reason {
	VM_FAULT_OOM            = (__force vm_fault_t)0x000001,
	VM_FAULT_SIGBUS         = (__force vm_fault_t)0x000002,
	VM_FAULT_MAJOR          = (__force vm_fault_t)0x000004,
	VM_FAULT_WRITE          = (__force vm_fault_t)0x000008,
	VM_FAULT_HWPOISON       = (__force vm_fault_t)0x000010,
	VM_FAULT_HWPOISON_LARGE = (__force vm_fault_t)0x000020,
	VM_FAULT_SIGSEGV        = (__force vm_fault_t)0x000040,
	VM_FAULT_NOPAGE         = (__force vm_fault_t)0x000100,
	VM_FAULT_LOCKED         = (__force vm_fault_t)0x000200,
	VM_FAULT_RETRY          = (__force vm_fault_t)0x000400,
	VM_FAULT_FALLBACK       = (__force vm_fault_t)0x000800,
	VM_FAULT_DONE_COW       = (__force vm_fault_t)0x001000,
	VM_FAULT_NEEDDSYNC      = (__force vm_fault_t)0x002000,
	VM_FAULT_PTNOTSAME      = (__force vm_fault_t)0x004000,
	VM_FAULT_HINDEX_MASK    = (__force vm_fault_t)0x0f0000,
};

/* Encode hstate index for a hwpoisoned large page */
#define VM_FAULT_SET_HINDEX(x) ((__force vm_fault_t)((x) << 16))
#define VM_FAULT_GET_HINDEX(x) (((__force unsigned int)(x) >> 16) & 0xf)

#define VM_FAULT_ERROR (VM_FAULT_OOM | VM_FAULT_SIGBUS |	\
			VM_FAULT_SIGSEGV | VM_FAULT_HWPOISON |	\
			VM_FAULT_HWPOISON_LARGE | VM_FAULT_FALLBACK)

#define VM_FAULT_RESULT_TRACE \
	{ VM_FAULT_OOM,                 "OOM" },	\
	{ VM_FAULT_SIGBUS,              "SIGBUS" },	\
	{ VM_FAULT_MAJOR,               "MAJOR" },	\
	{ VM_FAULT_WRITE,               "WRITE" },	\
	{ VM_FAULT_HWPOISON,            "HWPOISON" },	\
	{ VM_FAULT_HWPOISON_LARGE,      "HWPOISON_LARGE" },	\
	{ VM_FAULT_SIGSEGV,             "SIGSEGV" },	\
	{ VM_FAULT_NOPAGE,              "NOPAGE" },	\
	{ VM_FAULT_LOCKED,              "LOCKED" },	\
	{ VM_FAULT_RETRY,               "RETRY" },	\
	{ VM_FAULT_FALLBACK,            "FALLBACK" },	\
	{ VM_FAULT_DONE_COW,            "DONE_COW" },	\
	{ VM_FAULT_NEEDDSYNC,           "NEEDDSYNC" }

struct vm_special_mapping {
	const char *name;	/* The name, e.g. "[vdso]". */

	/*
	 * If .fault is not provided, this points to a
	 * NULL-terminated array of pages that back the special mapping.
	 *
	 * This must not be NULL unless .fault is provided.
	 */
	struct page **pages;

	/*
	 * If non-NULL, then this is called to resolve page faults
	 * on the special mapping.  If used, .pages is not checked.
	 */
	vm_fault_t (*fault)(const struct vm_special_mapping *sm,
				struct vm_area_struct *vma,
				struct vm_fault *vmf);

	int (*mremap)(const struct vm_special_mapping *sm,
		     struct vm_area_struct *new_vma);
};

enum tlb_flush_reason {
	TLB_FLUSH_ON_TASK_SWITCH,
	TLB_REMOTE_SHOOTDOWN,
	TLB_LOCAL_SHOOTDOWN,
	TLB_LOCAL_MM_SHOOTDOWN,
	TLB_REMOTE_SEND_IPI,
	NR_TLB_FLUSH_REASONS,
};

 /*
  * A swap entry has to fit into a "unsigned long", as the entry is hidden
  * in the "index" field of the swapper address space.
  */
typedef struct {
	unsigned long val;
} swp_entry_t;

/* Return the name for an anonymous mapping or NULL for a file-backed mapping */
static inline const char __user *vma_get_anon_name(struct vm_area_struct *vma)
{
	if (vma->vm_file)
		return NULL;

	return vma->anon_name;
}

#endif /* _LINUX_MM_TYPES_H */

```



MarkAsCompleted: 1 items
主要问题已经定位清楚了：报告的大方向多数是对的，但有几处修复建议把宏当成符号了，另外对 `wx_region` 残留问题的严重性明显低估了。
MarkAsCompleted: 1 items

```bash
Get-ChildItem -Path "C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64","C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\include" -Recurse -File | Select-String -Pattern "#define virt_to_page|static inline .*virt_to_page|extern .*virt_to_page" | ForEach-Object { "{0}:{1}:{2}" -f $_.Path, $_.LineNumber, $_.Line.Trim() }
```

**Current Output:**
```
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\memory.h:344:#define virt_to_page(x)		pfn_to_page(virt_to_pfn(x))
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\memory.h:353:#define virt_to_page(x)	({		
C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\include\asm-generic\page.h:84:#define virt_to_page(addr)	pfn_to_page(virt_to_pfn(addr))
<|cursor|>
```

Subagent: 153fd478-c3e0-40ea-af15-a7a1e26b29ca



**Files Read:**

**C:\Users\24151\Documents\GitHub\vmware-shared-gki-source\common\arch\arm64\include\asm\memory.h**
```
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/memory.h
 *
 * Copyright (C) 2000-2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * Note: this file should not be included by non-asm/.h files
 */
#ifndef __ASM_MEMORY_H
#define __ASM_MEMORY_H

#include <linux/const.h>
#include <linux/sizes.h>
#include <asm/page-def.h>

/*
 * Size of the PCI I/O space. This must remain a power of two so that
 * IO_SPACE_LIMIT acts as a mask for the low bits of I/O addresses.
 */
#define PCI_IO_SIZE		SZ_16M

/*
 * VMEMMAP_SIZE - allows the whole linear region to be covered by
 *                a struct page array
 *
 * If we are configured with a 52-bit kernel VA then our VMEMMAP_SIZE
 * needs to cover the memory region from the beginning of the 52-bit
 * PAGE_OFFSET all the way to PAGE_END for 48-bit. This allows us to
 * keep a constant PAGE_OFFSET and "fallback" to using the higher end
 * of the VMEMMAP where 52-bit support is not available in hardware.
 */
#define VMEMMAP_SIZE ((_PAGE_END(VA_BITS_MIN) - PAGE_OFFSET) \
			>> (PAGE_SHIFT - STRUCT_PAGE_MAX_SHIFT))

/*
 * PAGE_OFFSET - the virtual address of the start of the linear map, at the
 *               start of the TTBR1 address space.
 * PAGE_END - the end of the linear map, where all other kernel mappings begin.
 * KIMAGE_VADDR - the virtual address of the start of the kernel image.
 * VA_BITS - the maximum number of bits for virtual addresses.
 */
#define VA_BITS			(CONFIG_ARM64_VA_BITS)
#define _PAGE_OFFSET(va)	(-(UL(1) << (va)))
#define PAGE_OFFSET		(_PAGE_OFFSET(VA_BITS))
#define KIMAGE_VADDR		(MODULES_END)
#define BPF_JIT_REGION_START	(KASAN_SHADOW_END)
#define BPF_JIT_REGION_SIZE	(SZ_128M)
#define BPF_JIT_REGION_END	(BPF_JIT_REGION_START + BPF_JIT_REGION_SIZE)
#define MODULES_END		(MODULES_VADDR + MODULES_VSIZE)
#define MODULES_VADDR		(BPF_JIT_REGION_END)
#define MODULES_VSIZE		(SZ_128M)
#define VMEMMAP_START		(-VMEMMAP_SIZE - SZ_2M)
#define VMEMMAP_END		(VMEMMAP_START + VMEMMAP_SIZE)
#define PCI_IO_END		(VMEMMAP_START - SZ_2M)
#define PCI_IO_START		(PCI_IO_END - PCI_IO_SIZE)
#define FIXADDR_TOP		(PCI_IO_START - SZ_2M)

#if VA_BITS > 48
#define VA_BITS_MIN		(48)
#else
#define VA_BITS_MIN		(VA_BITS)
#endif

#define _PAGE_END(va)		(-(UL(1) << ((va) - 1)))

#define KERNEL_START		_text
#define KERNEL_END		_end

/*
 * Generic and tag-based KASAN require 1/8th and 1/16th of the kernel virtual
 * address space for the shadow region respectively. They can bloat the stack
 * significantly, so double the (minimum) stack size when they are in use.
 */
#if defined(CONFIG_KASAN_GENERIC) || defined(CONFIG_KASAN_SW_TAGS)
#define KASAN_SHADOW_OFFSET	_AC(CONFIG_KASAN_SHADOW_OFFSET, UL)
#define KASAN_SHADOW_END	((UL(1) << (64 - KASAN_SHADOW_SCALE_SHIFT)) \
					+ KASAN_SHADOW_OFFSET)
#define KASAN_THREAD_SHIFT	1
#else
#define KASAN_THREAD_SHIFT	0
#define KASAN_SHADOW_END	(_PAGE_END(VA_BITS_MIN))
#endif /* CONFIG_KASAN */

#define MIN_THREAD_SHIFT	(14 + KASAN_THREAD_SHIFT)

/*
 * VMAP'd stacks are allocated at page granularity, so we must ensure that such
 * stacks are a multiple of page size.
 */
#if defined(CONFIG_VMAP_STACK) && (MIN_THREAD_SHIFT < PAGE_SHIFT)
#define THREAD_SHIFT		PAGE_SHIFT
#else
#define THREAD_SHIFT		MIN_THREAD_SHIFT
#endif

#if THREAD_SHIFT >= PAGE_SHIFT
#define THREAD_SIZE_ORDER	(THREAD_SHIFT - PAGE_SHIFT)
#endif

#define THREAD_SIZE		(UL(1) << THREAD_SHIFT)

/*
 * By aligning VMAP'd stacks to 2 * THREAD_SIZE, we can detect overflow by
 * checking sp & (1 << THREAD_SHIFT), which we can do cheaply in the entry
 * assembly.
 */
#ifdef CONFIG_VMAP_STACK
#define THREAD_ALIGN		(2 * THREAD_SIZE)
#else
#define THREAD_ALIGN		THREAD_SIZE
#endif

#define IRQ_STACK_SIZE		THREAD_SIZE

#define OVERFLOW_STACK_SIZE	SZ_4K

/*
 * Alignment of kernel segments (e.g. .text, .data).
 *
 *  4 KB granule:  16 level 3 entries, with contiguous bit
 * 16 KB granule:   4 level 3 entries, without contiguous bit
 * 64 KB granule:   1 level 3 entry
 */
#define SEGMENT_ALIGN		SZ_64K

/*
 * Memory types available.
 *
 * IMPORTANT: MT_NORMAL must be index 0 since vm_get_page_prot() may 'or' in
 *	      the MT_NORMAL_TAGGED memory type for PROT_MTE mappings. Note
 *	      that protection_map[] only contains MT_NORMAL attributes.
 */
#define MT_NORMAL		0
#define MT_NORMAL_TAGGED	1
#define MT_NORMAL_NC		2
#define MT_NORMAL_WT		3
#define MT_DEVICE_nGnRnE	4
#define MT_DEVICE_nGnRE		5
#define MT_DEVICE_GRE		6
#define MT_NORMAL_iNC_oWB	7

/*
 * Memory types for Stage-2 translation
 */
#define MT_S2_NORMAL		0xf
#define MT_S2_DEVICE_nGnRE	0x1

/*
 * Memory types for Stage-2 translation when ID_AA64MMFR2_EL1.FWB is 0001
 * Stage-2 enforces Normal-WB and Device-nGnRE
 */
#define MT_S2_FWB_NORMAL	6
#define MT_S2_FWB_DEVICE_nGnRE	1

#ifdef CONFIG_ARM64_4K_PAGES
#define IOREMAP_MAX_ORDER	(PUD_SHIFT)
#else
#define IOREMAP_MAX_ORDER	(PMD_SHIFT)
#endif

#ifndef __ASSEMBLY__

#include <linux/bitops.h>
#include <linux/compiler.h>
#include <linux/mmdebug.h>
#include <linux/types.h>
#include <asm/bug.h>

extern u64			vabits_actual;
#define PAGE_END		(_PAGE_END(vabits_actual))

extern s64			memstart_addr;
/* PHYS_OFFSET - the physical address of the start of memory. */
#define PHYS_OFFSET		({ VM_BUG_ON(memstart_addr & 1); memstart_addr; })

/* the virtual base of the kernel image */
extern u64			kimage_vaddr;

/* the offset between the kernel virtual and physical mappings */
extern u64			kimage_voffset;

static inline unsigned long kaslr_offset(void)
{
	return kimage_vaddr - KIMAGE_VADDR;
}

/*
 * Allow all memory at the discovery stage. We will clip it later.
 */
#define MIN_MEMBLOCK_ADDR	0
#define MAX_MEMBLOCK_ADDR	U64_MAX

/*
 * PFNs are used to describe any physical page; this means
 * PFN 0 == physical address 0.
 *
 * This is the PFN of the first RAM page in the kernel
 * direct-mapped view.  We assume this is the first page
 * of RAM in the mem_map as well.
 */
#define PHYS_PFN_OFFSET	(PHYS_OFFSET >> PAGE_SHIFT)

/*
 * When dealing with data aborts, watchpoints, or instruction traps we may end
 * up with a tagged userland pointer. Clear the tag to get a sane pointer to
 * pass on to access_ok(), for instance.
 */
#define __untagged_addr(addr)	\
	((__force __typeof__(addr))sign_extend64((__force u64)(addr), 55))

#define untagged_addr(addr)	({					\
	u64 __addr = (__force u64)(addr);					\
	__addr &= __untagged_addr(__addr);				\
	(__force __typeof__(addr))__addr;				\
})

#if defined(CONFIG_KASAN_SW_TAGS) || defined(CONFIG_KASAN_HW_TAGS)
#define __tag_shifted(tag)	((u64)(tag) << 56)
#define __tag_reset(addr)	__untagged_addr(addr)
#define __tag_get(addr)		(__u8)((u64)(addr) >> 56)
#else
#define __tag_shifted(tag)	0UL
#define __tag_reset(addr)	(addr)
#define __tag_get(addr)		0
#endif /* CONFIG_KASAN_SW_TAGS || CONFIG_KASAN_HW_TAGS */

static inline const void *__tag_set(const void *addr, u8 tag)
{
	u64 __addr = (u64)addr & ~__tag_shifted(0xff);
	return (const void *)(__addr | __tag_shifted(tag));
}

#ifdef CONFIG_KASAN_HW_TAGS
#define arch_enable_tagging_sync()		mte_enable_kernel_sync()
#define arch_enable_tagging_async()		mte_enable_kernel_async()
#define arch_set_tagging_report_once(state)	mte_set_report_once(state)
#define arch_force_async_tag_fault()		mte_check_tfsr_exit()
#define arch_get_random_tag()			mte_get_random_tag()
#define arch_get_mem_tag(addr)			mte_get_mem_tag(addr)
#define arch_set_mem_tag_range(addr, size, tag, init)	\
			mte_set_mem_tag_range((addr), (size), (tag), (init))
#endif /* CONFIG_KASAN_HW_TAGS */

/*
 * Physical vs virtual RAM address space conversion.  These are
 * private definitions which should NOT be used outside memory.h
 * files.  Use virt_to_phys/phys_to_virt/__pa/__va instead.
 */


/*
 * Check whether an arbitrary address is within the linear map, which
 * lives in the [PAGE_OFFSET, PAGE_END) interval at the bottom of the
 * kernel's TTBR1 address range.
 */
#define __is_lm_address(addr)	(((u64)(addr) ^ PAGE_OFFSET) < (PAGE_END - PAGE_OFFSET))

#define __lm_to_phys(addr)	(((addr) & ~PAGE_OFFSET) + PHYS_OFFSET)
#define __kimg_to_phys(addr)	((addr) - kimage_voffset)

#define __virt_to_phys_nodebug(x) ({					\
	phys_addr_t __x = (phys_addr_t)(__tag_reset(x));		\
	__is_lm_address(__x) ? __lm_to_phys(__x) : __kimg_to_phys(__x);	\
})

#define __pa_symbol_nodebug(x)	__kimg_to_phys((phys_addr_t)(x))

#ifdef CONFIG_DEBUG_VIRTUAL
extern phys_addr_t __virt_to_phys(unsigned long x);
extern phys_addr_t __phys_addr_symbol(unsigned long x);
#else
#define __virt_to_phys(x)	__virt_to_phys_nodebug(x)
#define __phys_addr_symbol(x)	__pa_symbol_nodebug(x)
#endif /* CONFIG_DEBUG_VIRTUAL */

#define __phys_to_virt(x)	((unsigned long)((x) - PHYS_OFFSET) | PAGE_OFFSET)
#define __phys_to_kimg(x)	((unsigned long)((x) + kimage_voffset))

/*
 * Convert a page to/from a physical address
 */
#define page_to_phys(page)	(__pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys)	(pfn_to_page(__phys_to_pfn(phys)))

/*
 * Note: Drivers should NOT use these.  They are the wrong
 * translation for translating DMA addresses.  Use the driver
 * DMA support - see dma-mapping.h.
 */
#define virt_to_phys virt_to_phys
static inline phys_addr_t virt_to_phys(const volatile void *x)
{
	return __virt_to_phys((unsigned long)(x));
}

#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(phys_addr_t x)
{
	return (void *)(__phys_to_virt(x));
}

/*
 * Drivers should NOT use these either.
 */
#define __pa(x)			__virt_to_phys((unsigned long)(x))
#define __pa_symbol(x)		__phys_addr_symbol(RELOC_HIDE((unsigned long)(x), 0))
#define __pa_nodebug(x)		__virt_to_phys_nodebug((unsigned long)(x))
#define __va(x)			((void *)__phys_to_virt((phys_addr_t)(x)))
#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)
#define virt_to_pfn(x)		__phys_to_pfn(__virt_to_phys((unsigned long)(x)))
#define sym_to_pfn(x)		__phys_to_pfn(__pa_symbol(x))

/*
 * With non-canonical CFI jump tables, the compiler replaces function
 * address references with the address of the function's CFI jump
 * table entry. This results in __pa_symbol(function) returning the
 * physical address of the jump table entry, which can lead to address
 * space confusion since the jump table points to the function's
 * virtual address. Therefore, use inline assembly to ensure we are
 * always taking the address of the actual function.
 */
#define __va_function(x) ({						\
	void *addr;							\
	asm("adrp %0, " __stringify(x) "\n\t"				\
	    "add  %0, %0, :lo12:" __stringify(x) : "=r" (addr));	\
	addr;								\
})

#define __pa_function(x) 	__pa_symbol(__va_function(x))

/*
 *  virt_to_page(x)	convert a _valid_ virtual address to struct page *
 *  virt_addr_valid(x)	indicates whether a virtual address is valid
 */
#define ARCH_PFN_OFFSET		((unsigned long)PHYS_PFN_OFFSET)

#if !defined(CONFIG_SPARSEMEM_VMEMMAP) || defined(CONFIG_DEBUG_VIRTUAL)
#define page_to_virt(x)	({						\
	__typeof__(x) __page = x;					\
	void *__addr = __va(page_to_phys(__page));			\
	(void *)__tag_set((const void *)__addr, page_kasan_tag(__page));\
})
#define virt_to_page(x)		pfn_to_page(virt_to_pfn(x))
#else
#define page_to_virt(x)	({						\
	__typeof__(x) __page = x;					\
	u64 __idx = ((u64)__page - VMEMMAP_START) / sizeof(struct page);\
	u64 __addr = PAGE_OFFSET + (__idx * PAGE_SIZE);			\
	(void *)__tag_set((const void *)__addr, page_kasan_tag(__page));\
})

#define virt_to_page(x)	({						\
	u64 __idx = (__tag_reset((u64)x) - PAGE_OFFSET) / PAGE_SIZE;	\
	u64 __addr = VMEMMAP_START + (__idx * sizeof(struct page));	\
	(struct page *)__addr;						\
})
#endif /* !CONFIG_SPARSEMEM_VMEMMAP || CONFIG_DEBUG_VIRTUAL */

#define virt_addr_valid(addr)	({					\
	__typeof__(addr) __addr = __tag_reset(addr);			\
	__is_lm_address(__addr) && pfn_valid(virt_to_pfn(__addr));	\
})

void dump_mem_limit(void);
#endif /* !ASSEMBLY */

/*
 * Given that the GIC architecture permits ITS implementations that can only be
 * configured with a LPI table address once, GICv3 systems with many CPUs may
 * end up reserving a lot of different regions after a kexec for their LPI
 * tables (one per CPU), as we are forced to reuse the same memory after kexec
 * (and thus reserve it persistently with EFI beforehand)
 */
#if defined(CONFIG_EFI) && defined(CONFIG_ARM_GIC_V3_ITS)
# define INIT_MEMBLOCK_RESERVED_REGIONS	(INIT_MEMBLOCK_REGIONS + NR_CPUS + 1)
#endif

#include <asm-generic/memory_model.h>

#endif /* __ASM_MEMORY_H */

```



我对照了 `Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/bug-audit.md`、`Documents/GitHub/无痕hook/KernelPatch/kpms/wxjump/wxjump.c` 和 `Documents/GitHub/vmware-shared-gki-source`。结论是：

**这份报告主体判断基本对，但确实有几处说错/说得不严谨。**

## 明确说对了的部分
- **Bug 1 对。** `wxjump.c:59` 的 `0xF13` 确实把 `AttrIndx` 设成了 4；而内核 `memory.h:134-141` 明确是 `MT_NORMAL=0`、`MT_DEVICE_nGnRnE=4`。
- **Bug 2 对。** `flush_dcache_page` 的签名确实是 `struct page *`，见 `flush.c:70-74`；`wxjump.c:559-560` 传进去的是 `shadow_page_va`。`struct page.flags` 也确实在首字段，见 `mm_types.h:71-73`。
- **Bug 3 对。** `PAGE_OFFSET` 的定义确实是 `-(1UL << VA_BITS)`，见 `memory.h:43-45`；`wxjump.c:1055-1056` 少移了一位。
- **Bug 4 对。** `wxjump.c:1055,1076` 取到的确实是 `T1SZ/TG1`，不是 `T0SZ/TG0`。而内核在 `proc.S:23-29,476-478` 里又确实把两边一起配置，所以“通常不炸”这个判断基本成立。
- **Bug 5 对。** 正常用户只读可执行页包含 `PTE_PXN`，见 `pgtable-prot.h:88-89`；但 `wxjump.c:883` 恢复时没带。

## 报告里不准确的地方
1. **Bug 1 的修复建议有一条是错的。**  
   报告里说可以 `kallsyms_lookup_name("_PAGE_DEFAULT")`。这不成立，因为 `_PAGE_DEFAULT` 在 `pgtable-prot.h:66` 里是**宏**，不是可解析的内核符号。

2. **Bug 2 的修复建议也有一条不对。**  
   报告里写“用 `virt_to_page` 转换，需额外解析符号”。但这套源码里 `virt_to_page` 在 `memory.h:333-358` 是**宏**，不是一个能用 `kallsyms` 找到的函数符号。

3. **Bug 7 里“`0xFC3` 在你内核上恰好正确”这个说法太绝对。**  
   更准确地说：  
   - `0xFC3` 的 **AttrIndx=0**，在这套内核上确实对应 `MT_NORMAL`；  
   - 但它仍然**缺 `PTE_PXN`**，所以不能说“这个 PTE 完全正确”，只能说“内存类型这部分是对的”。

4. **Bug 3 里“当前不会走到此路径”证据不足。**  
   我在源码树里没找到 `physvirt_offset`，所以是否绕开 fallback，实际上取决于 `wxjump.c:1058-1073` 那段 `AT S1E1R` 的**运行时探测**是否成功。  
   只看源码，不能直接证明“当前一定不会走到该路径”。

## 报告明显低估/漏掉的地方
1. **Bug 8 不是“轻微内存泄漏”这么简单。**  
   这个问题更严重，会影响后续 patch 逻辑：
   - `wxjump_do_patch` 命中已有 region 后，若目标页还没 `shadow_page_va`，会先走 `wxjump.c:701-724`；
   - 然后并发检查又会在 `wxjump.c:733-741` 看到这个 region 还在链表里，于是递归调用 `wxjump_do_patch`。
   
   也就是说，这不只是“region 没回收”，而是可能导致：
   - 同一 VMA 再 patch 新页时递归打转；
   - release 后重 patch 时递归打转；
   - error path 留下 stale region 后，后续 patch 继续出问题。

2. **Bug 6 也不只是报告写的两个失败点。**  
   报告提到了 `wxjump.c:783-798` 的失败泄漏，但还漏了 `wxjump.c:814-820`：  
   `wxjump_switch_mapping` 失败时，shadow page 虽然释放了，**region 还是留在链表里**，后果和上面一样。

3. **Bug 5 的 PXN 问题不只出现在 RELEASE。**  
   `wxjump_exit` 的恢复路径在 `wxjump.c:1414-1416` 也用了同样的 `PTE_USER_RDONLY` 模式，所以同类问题不只一处。

## 一句话结论
- **核心技术判断大多是对的。**
- **真正“说错”的，主要是两条修复建议把宏当成了符号。**
- **最需要纠正的是对 `wx_region` 残留问题的定性**：它不只是“轻微泄漏”，而是会直接影响后续 patch 的正确性。

如果你要，我可以下一步直接帮你把 `bug-audit.md` 改成一版更严谨的修订稿。