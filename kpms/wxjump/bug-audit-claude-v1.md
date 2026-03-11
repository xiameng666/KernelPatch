# wxjump.c Bug 审计报告

审计时间: 2026-03-11
验证内核: Linux 5.10.101-android12-9 (GKI)
内核源码: `vmware-shared-gki-source/common/`

---

## 🔴 致命 Bug: PTE_BASE_FLAGS 内存类型错误

**文件**: wxjump.c 第 59 行
**代码**:
```c
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

| T1SZ | VA_BITS | 代码结果 | 正确 PAGE_OFFSET |
|------|---------|----------|------------------|
| 25 | 39 | `-1ULL << 38` = `0xFFFFFFC000000000` | `-1ULL << 39` = `0xFFFFFF8000000000` |
| 16 | 48 | `-1ULL << 47` = `0xFFFF800000000000` | `-1ULL << 48` = `0xFFFF000000000000` |

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

| # | 严重度 | Bug | 你的 5.10 GKI 上触发? | 行号 |
|---|--------|-----|----------------------|------|
| 1 | 🔴致命 | PTE_BASE_FLAGS AttrIndx=4 映射为设备内存 | **是! 所有 PTE 操作受影响** | 59 |
| 2 | 🔴严重 | flush_dcache_page 传入 VA 而非 struct page* | 是，可能静默损坏代码 | 559 |
| 3 | 🔴中等 | PAGE_OFFSET 公式差一位 | 否 (AT S1E1R 正常) | 1056 |
| 4 | 🟡中等 | TCR 提取 T1SZ/TG1 而非 T0SZ/TG0 | 否 (Android 配置相同) | 1055,1076 |
| 5 | 🟡中等 | RELEASE 后缺失 PXN 保护 | 是，安全性降低 | 883 |
| 6 | 🟡低 | do_patch 错误路径 region 泄漏 | 仅在分配失败时 | 784,797 |
| 7 | 🟢信息 | exit_mmap 0xFC3 与 PTE_BASE_FLAGS 不一致 | 0xFC3 在你内核上恰好正确 | 1000 |
| 8 | 🟢信息 | wx_region 释放后不回收 | 是，轻微内存泄漏 | do_release |

**最优先修复**: Bug 1 (PTE_BASE_FLAGS)。在你的内核上，这会导致 wxjump **完全无法工作** — 所有页面被映射为不可缓存的设备内存，代码执行行为未定义。
