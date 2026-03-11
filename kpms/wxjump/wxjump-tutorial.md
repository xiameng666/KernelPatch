# wxjump 教学文档 — W^X 影子页跳转 Hook

## 目录

1. 前置知识：ARM64 虚拟内存体系
2. 前置知识：ARM64 异常模型
3. 前置知识：Linux 内核内存管理 API
4. wxjump 核心原理
5. 数据结构详解
6. 完整流程分析
7. 关键代码逐行解读
8. 参考资料与验证来源

---

## 1. 前置知识：ARM64 虚拟内存体系

### 1.1 页表层级结构

ARM64 使用**多级页表**将虚拟地址 (VA) 翻译为物理地址 (PA)。翻译过程由 MMU 硬件自动执行。

以最常见的 **4KB 页粒度 (granule)** 为例，使用 4 级页表：

```
虚拟地址 (48-bit 有效):
┌──────┬──────┬──────┬──────┬──────────┐
│ 未用 │ L0   │ L1   │ L2   │ L3       │ 页内偏移
│      │ 9bit │ 9bit │ 9bit │ 9bit     │ 12bit
└──────┴──────┴──────┴──────┴──────────┘

L0 (PGD) → L1 (PUD) → L2 (PMD) → L3 (PTE) → 物理页帧
```

每级页表有 512 个条目 (2^9)，每个条目 8 字节 (64-bit)。

**TCR_EL1 寄存器**控制页表配置：
- **T0SZ** (bits[5:0]): 控制 TTBR0 (用户空间) 地址空间大小，有效 VA 位数 = 64 - T0SZ
- **T1SZ** (bits[21:16]): 控制 TTBR1 (内核空间) 地址空间大小，有效 VA 位数 = 64 - T1SZ
- **TG0** (bits[15:14]): TTBR0 页粒度 — 0b00=4KB, 0b01=64KB, 0b10=16KB
- **TG1** (bits[31:30]): TTBR1 页粒度 — 0b01=16KB, 0b10=4KB, 0b11=64KB (注意编码与 TG0 不同!)

wxjump 在初始化时读取 TCR_EL1 来确定页表配置：

```c
asm volatile("mrs %0, tcr_el1" : "=r"(tcr));
// ⚠️ 代码变量名为 t0sz/tg0，但实际提取的是 T1SZ/TG1 (内核空间配置)
// 在 Android 上 T0SZ=T1SZ 且 TG0=TG1，所以实际无影响
int t0sz = (tcr >> 16) & 0x3F;           // bits[21:16] = T1SZ (非 T0SZ!)
uint64_t tg0 = (tcr >> 30) & 3;          // bits[31:30] = TG1 (非 TG0!)
// 根据 tg0(实为TG1) 确定 page_shift (12/14/16)
// 根据 t0sz(实为T1SZ) 计算页表级数和 PAGE_OFFSET
```

> **验证来源**: ARM Architecture Reference Manual (ARMv8-A), D5.2 "The VMSAv8-64 translation table format";
> Linux 内核 `arch/arm64/include/asm/sysreg.h`: `#define TCR_T0SZ_OFFSET 0`, `#define TCR_T1SZ_OFFSET 16`;
> Arm Developer 文档 https://developer.arm.com/documentation/102376/latest/Describing-memory-in-AArch64
>
> **⚠️ 注意**: 代码中变量名 `t0sz`/`tg0` 与实际提取的 TCR 字段不匹配（提取的是 T1SZ/TG1），
> 但因为 Android 内核配置中 T0SZ=T1SZ、TG0=TG1，所以不影响正确性。

### 1.2 页表项 (PTE) 格式

ARM64 Level 3 页描述符 (4KB 页) 的关键位域：

```
63  55 54 53 52 51   12 11 10  9  8  7  6  5  4  2  1  0
┌───┬───┬───┬───┬──────┬───┬───┬──┬──┬──┬──┬──┬────┬──┬──┐
│res│UXN│PXN│CON│OA    │nG │AF │SH│AP│NS│AI│ res  │ 1│ 1│
│   │[54│[53│[52│[47:12│[11│[10│[9:│[7:│[5│[4:│    │  │  │
│   │]  │]  │]  │]     │]  │]  │8] │6] │] │2] │    │  │  │
└───┴───┴───┴───┴──────┴───┴───┴──┴──┴──┴──┴──┴────┴──┴──┘
```

**与 wxjump 直接相关的关键位**：

| 位 | 名称 | 说明 |
|---|---|---|
| [1:0] | Type | `0b11` = 有效的页描述符 |
| [4:2] | AttrIndx | 内存属性索引 (引用 MAIR_EL1 寄存器) |
| [7:6] | AP | 访问权限：`0b01` = EL0/EL1 读写, `0b11` = EL0/EL1 只读 |
| [9:8] | SH | 共享性：`0b11` = Inner Shareable |
| [10] | AF | 访问标志 (Access Flag)，必须为 1 否则触发 Access Flag Fault |
| [47:12] | OA | 输出物理地址的高位 (页帧号 PFN = OA >> 12) |
| [53] | PXN | 特权执行禁止 (Privileged Execute-Never) |
| [54] | UXN | 用户执行禁止 (User Execute-Never) |

> **验证来源**: Linux 内核 `arch/arm64/include/asm/pgtable-hwdef.h` 定义:
> `PTE_PXN = 1 << 53`, `PTE_UXN = 1 << 54`, `PTE_AF = 1 << 10`；
> LWN.net 文章 https://lwn.net/Articles/657423/ 中 Linux 内核 PTE 位定义

### 1.3 Execute-Only 页面权限

ARM64 支持一种特殊权限组合：**execute-only (--x)**。

| PTE_USER (AP[1]) | PTE_UXN (bit 54) | 用户态权限 |
|---|---|---|
| 1 | 0 | 可读 + 可执行 (r-x) |
| 1 | 1 | 可读 + 不可执行 (r--) |
| 0 | 0 | **execute-only (--x)** |
| 0 | 1 | 不可访问 (---) |

当 **PTE_USER=0** 且 **PTE_UXN=0** 时：
- 用户态 (EL0) **可以执行**代码 (取指成功)
- 用户态 (EL0) **不能读取**数据 (数据访问触发 Permission Fault)
- 内核 (EL1) 仍然可以访问

**这正是 wxjump 的核心利用点**: 将 shadow page 设为 execute-only，CPU 可以从中取指执行，但如果用户态代码尝试**读取**该页（比如做 CRC 完整性校验），就会触发 Data Abort，被我们的 fault handler 拦截后切换到原始页。

> **验证来源**: Linux 内核 commit bc07c2c6e9ed (Catalin Marinas, ARM 工程师):
> "The ARMv8 architecture allows execute-only user permissions by clearing the PTE_UXN and PTE_USER bits."
> 定义: `PAGE_EXECONLY = _PAGE_DEFAULT | PTE_NG | PTE_PXN` (无 PTE_USER，无 PTE_UXN)

### 1.4 物理地址 ↔ 虚拟地址转换

Linux 内核将所有物理内存线性映射到一段内核虚拟地址空间 (称为 **linear map** 或 **direct map**)。转换关系：

```
VA = PA + physvirt_offset
PA = VA - physvirt_offset
```

其中 `physvirt_offset` 是编译/启动时确定的偏移量。wxjump 通过以下方式获取：

1. 尝试从 kallsyms 查找 `physvirt_offset` 变量
2. 如果不存在，使用 `AT S1E1R` + `PAR_EL1` 指令进行地址翻译来推算
3. 回退使用 `PAGE_OFFSET - memstart_addr` 计算

```c
// AT S1E1R: 对 EL1 读权限做地址翻译
asm volatile("at s1e1r, %0" :: "r"(test_page));
isb();
asm volatile("mrs %0, par_el1" : "=r"(par));
// PAR_EL1 bit[0]=0 表示翻译成功, bits[47:12] 为物理地址
if (!(par & 1)) {
    uint64_t pa = (par & PA_MASK) | (test_page & 0xFFF);
    detected_physvirt_offset = test_page - pa;
}
```

> **验证来源**: ARM Architecture Reference Manual, C5.2 "Address translation instructions";
> PAR_EL1 格式: bit[0] 为成功标志, bits[47:12] 为物理地址

---

## 2. 前置知识：ARM64 异常模型

### 2.1 异常等级 (Exception Level)

ARM64 定义了 4 个异常等级：

```
EL0 — 用户态应用程序
EL1 — OS 内核 (Linux)
EL2 — 虚拟机管理器 (Hypervisor)
EL3 — 安全监控器 (Secure Monitor)
```

页错误发生时，如果故障来自 EL0，异常被路由到 EL1 (内核)。

### 2.2 ESR_EL1 — 异常综合征寄存器

当异常被路由到 EL1 时，**ESR_EL1** (Exception Syndrome Register) 记录异常的原因：

```
63    32 31    26 25  24          0
┌───────┬────────┬───┬───────────┐
│ ISS2  │   EC   │IL │    ISS    │
│[63:32]│[31:26] │[25│  [24:0]   │
└───────┴────────┴───┴───────────┘
```

**EC (Exception Class)** — 标识异常类型：

| EC 值 | 含义 |
|---|---|
| 0x20 | **Instruction Abort from EL0** (用户态取指失败) |
| 0x21 | Instruction Abort from EL1 (内核态取指失败) |
| 0x24 | **Data Abort from EL0** (用户态数据访问失败) |
| 0x25 | Data Abort from EL1 (内核态数据访问失败) |

wxjump 只处理 **EL0 的异常** (EC=0x20 和 EC=0x24)，忽略 EL1 异常 (EC=0x21/0x25)：

```c
unsigned long ec = (esr >> 26) & 0x3FUL;
if (ec != 0x24 && ec != 0x20) {
    // 不是 EL0 的 fault，跳过
    wx_fault_el1_skip_count++;
    return;
}
```

> **验证来源**: ARM 官方文档 https://developer.arm.com/documentation/ddi0601/latest/AArch64-Registers/ESR-EL1;
> EDK2 (UEFI 固件) 源码和 RTEMS 内核中的 EC 解码表均确认这些值

### 2.3 DFSC/IFSC — 错误状态码

对于 Data Abort (EC=0x24/0x25)，ISS 的低 6 位是 **DFSC** (Data Fault Status Code)：

| DFSC 值 | 含义 |
|---|---|
| 0b0001xx | Translation Fault (页表项不存在) |
| 0b0010xx | Access Flag Fault (AF=0) |
| **0b0011xx** | **Permission Fault (权限不足)** |
| 0b010000 | Synchronous External Abort |

其中低 2 位表示故障发生的页表级别 (0-3)。

wxjump 只关心 **Permission Fault**，通过掩码检测：

```c
// DFSC 的 bits[5:2] 为 0b0011 时是 Permission Fault
// 0x3C = 0b00111100，提取 bits[5:2]
if ((esr & 0x3C) != 0x0C)  // 0x0C = 0b001100
    return;  // 不是 Permission Fault，跳过
```

Permission Fault 正是 wxjump 需要拦截的——当用户态代码访问 execute-only 页面时触发。

### 2.4 WnR 位

ESR_EL1 的 bit[6] 是 **WnR** (Write not Read) 标志：

| WnR | 含义 |
|---|---|
| 0 | **读操作**引起的 abort |
| 1 | **写操作**引起的 abort |

wxjump 利用此位区分三种情况：

```c
if (ec == 0x20) {
    // Instruction Abort → 执行错误（从 ORIG_R 页取指失败）
    wxjump_handle_exec_fault(...);
} else if (ec == 0x24 && !(esr & 0x40)) {
    // Data Abort + WnR=0 → 读错误（CRC 检测尝试读 shadow page）
    wxjump_handle_read_fault(...);
} else {
    // Data Abort + WnR=1 → 写错误（页面内容变更）
    wxjump_handle_write_fault(...);
}
```

> **验证来源**: Google 的 aarch64-esr-decoder 工具 (https://github.com/google/aarch64-esr-decoder) 确认了 WnR 在 bit 6 的位置

### 2.5 FAR_EL1 — 故障地址寄存器

当 Data Abort 或 Instruction Abort 发生时，**FAR_EL1** 保存引起异常的虚拟地址。在 Linux 内核的 `do_page_fault` 中，FAR_EL1 的值作为第一个参数传入。

wxjump 的 hook 函数签名：
```c
static void do_page_fault_before(hook_fargs3_t *fargs, void *udata)
{
    unsigned long far = fargs->arg0;    // FAR_EL1 (故障虚拟地址)
    unsigned long esr = fargs->arg1;    // ESR_EL1 (异常综合征)
    ...
}
```

---

## 3. 前置知识：Linux 内核内存管理 API

wxjump 不能直接 `#include` 内核头文件（因为它是 KPM 模块），所以通过 `kallsyms_lookup_name()` 在运行时动态解析内核符号。以下是使用到的关键 API：

### 3.1 进程地址空间

| 函数 | 原型 | 说明 |
|---|---|---|
| `get_task_mm` | `struct mm_struct *get_task_mm(struct task_struct *task)` | 获取任务的 mm_struct 并增加引用计数 |
| `mmput` | `void mmput(struct mm_struct *mm)` | 减少 mm_struct 引用计数，计数归零时释放 |
| `find_vma` | `struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)` | 查找**第一个 vm_end > addr** 的 VMA |

**mm_struct** 是进程地址空间的核心描述符，包含 pgd (页全局目录) 指针和所有 VMA 链表。

**vm_area_struct** 描述一段连续的虚拟内存区域：
```c
struct vm_area_struct {
    unsigned long vm_start;    // 起始地址 (含)
    unsigned long vm_end;      // 结束地址 (不含)
    struct vm_area_struct *vm_next;
    struct mm_struct *vm_mm;   // 所属 mm
    pgprot_t vm_page_prot;     // 页保护属性
    unsigned long vm_flags;    // VM_READ | VM_WRITE | VM_EXEC 等
    ...
};
```

**注意**: `find_vma` 返回的 VMA 的 `vm_start` 可能大于传入的 addr，需要额外检查 `addr >= vma->vm_start`。

> **验证来源**: Linux 内核文档 https://www.kernel.org/doc/html/v6.7/core-api/mm-api.html;
> `mm_types.h` 和 `mm.h` 中的定义

### 3.2 页面分配

| 函数 | 说明 |
|---|---|
| `__get_free_pages(gfp, order)` | 分配 2^order 个连续物理页，返回内核虚拟地址 |
| `free_pages(addr, order)` | 释放由 __get_free_pages 分配的页面 |

wxjump 使用 order=0 (单页, 4KB) 来分配影子页。

### 3.3 内核内存分配

| 函数 | 说明 |
|---|---|
| `kzalloc(size, flags)` | 分配并清零内核内存 |
| `kcalloc(n, size, flags)` | 分配 n 个元素的数组并清零 |
| `kfree(ptr)` | 释放 kzalloc/kcalloc 分配的内存 |

### 3.4 缓存/TLB 维护

ARM64 使用分离的指令缓存 (I-cache) 和数据缓存 (D-cache)，修改代码后必须手动同步：

| 操作 | 说明 |
|---|---|
| `flush_dcache_page` | 将数据缓存中的脏页写回主存 |
| `__flush_icache_range` | 使指令缓存中指定范围的条目失效 |
| `flush_tlb_page` / `__flush_tlb_range` | 使 TLB 中指定地址的条目失效 |
| `dsb(ish)` | Data Synchronization Barrier (Inner Shareable) — 确保之前的内存操作完成 |
| `isb()` | Instruction Synchronization Barrier — 刷新流水线 |

wxjump 在切换 PTE 后必须执行 TLB 刷新，否则 CPU 可能仍使用旧的 TLB 条目。修改代码页后还需刷新 I-cache。

TLBI (TLB Invalidate) 指令回退实现：
```c
// tlbi vale1is: 按虚拟地址+ASID 失效 TLB 条目 (Inner Shareable)
asm volatile("tlbi vale1is, %0" :: "r"(val));
// tlbi vaale1is: 按虚拟地址失效所有 ASID 的 TLB 条目
asm volatile("tlbi vaale1is, %0" :: "r"(tlbi_val));
```

> **验证来源**: ARM Architecture Reference Manual, D5.10 "TLB maintenance instructions";
> Linux 内核 `arch/arm64/include/asm/tlbflush.h`

---

## 4. wxjump 核心原理

### 4.1 问题背景

传统 inline hook 直接修改目标函数的代码字节（例如覆盖前 N 条指令为跳转指令）。这种方法的缺点是：**反 hook 检测可以通过 CRC/hash 校验发现代码被篡改**。

例如，某些安全 SDK 会周期性地读取 `.text` 段的内容，计算 CRC32 并与预期值比较。如果不一致，就判定存在 hook。

### 4.2 W^X 影子页方案

wxjump 的解决方案是利用 ARM64 的 execute-only 页面权限，创建**两份物理页**：

```
原始页 (orig page)          影子页 (shadow page)
┌─────────────────┐        ┌─────────────────┐
│ 原始代码         │        │ 原始代码         │
│ (未修改)          │        │ + 跳转指令 patch │
│                   │        │                   │
└─────────────────┘        └─────────────────┘
     PFN = A                    PFN = B

CPU 执行时 → PTE 指向 shadow page (--x, execute-only)
CRC 读取时 → PTE 切到 orig page (r--, 只读不可执行)
```

**关键洞察**：CPU 的取指操作和数据读取走不同的路径。execute-only 页面允许取指但禁止数据读取。当 CRC 检测代码尝试读取时，触发 Data Abort，wxjump 拦截后切换到原始页——CRC 读到的是未修改的原始代码。

### 4.3 三态状态机

```
         PATCH 命令
  NONE ──────────────► SHADOW_X (影子页可执行)
                           │          ▲
                 用户态读取  │          │ 用户态执行
               (Data Abort) │          │ (Insn Abort)
                           ▼          │
                        ORIG_R (原始页只读)
```

| 状态 | PTE 指向 | 权限 | 触发条件 |
|---|---|---|---|
| NONE | 原始页 | 正常 (r-x) | 初始状态 / 释放后 |
| SHADOW_X | 影子页 | --x (execute-only) | PATCH 命令 / 执行恢复 |
| ORIG_R | 原始页 | r-- (只读不可执行) | CRC 读取触发 |

### 4.4 与 wxshadow 的区别

| 特性 | wxshadow | wxjump |
|---|---|---|
| hook 方式 | BRK #7 断点 | 用户提供的跳转指令 |
| 内核开销 | 每次命中触发 BRK 异常 + 单步 | 零异常 (直接跳转) |
| 状态机 | 4 态 (含 STEPPING) | 3 态 (无 STEPPING) |
| 用途 | 调试/监控 | 高性能 inline hook |

---

## 5. 数据结构详解

### 5.1 page_info — 单页面管理

```c
struct page_info {
    uint64_t orig_pfn;        // 原始物理页帧号
    uint64_t shadow_pfn;      // 影子物理页帧号
    uint64_t shadow_page_va;  // 影子页的内核虚拟地址 (用于读写影子页内容)
    uint32_t state;           // STATE_NONE / STATE_ORIG_R / STATE_SHADOW_X
    uint32_t patch_count;     // 本页上的活跃 patch 数量
};
```

- **orig_pfn**: 从当前 PTE 读取的原始页帧号，用于恢复映射
- **shadow_pfn**: 通过 `__get_free_pages` 分配的影子页的页帧号
- **shadow_page_va**: 影子页的内核虚拟地址，用于 `memcpy` 写入 patch 内容
- **patch_count**: 引用计数，同一个页面可以有多个 patch；归零时释放影子页

### 5.2 wx_region — VMA 区域管理

```c
struct wx_region {
    struct list_head list;       // 全局链表节点
    void            *mm;         // 所属进程的 mm_struct
    unsigned long    vm_start;   // VMA 起始地址
    unsigned long    vm_end;     // VMA 结束地址
    struct page_info *pages;     // page_info 数组 (nr_pages 个元素)
    int              nr_pages;   // VMA 包含的页面数
    int              refcount;   // 引用计数
};
```

**为什么按 VMA 粒度管理**:
- 一个 VMA 对应一段连续的虚拟地址空间 (如 `.text` 段)
- 同一 VMA 内的多个页面可以共享同一个 `wx_region`
- `page_info` 数组通过 `(addr - vm_start) >> 12` 直接索引

### 5.3 全局状态

```c
static LIST_HEAD(region_list);    // 所有 wx_region 的全局链表
static uint64_t global_lock;      // 自旋锁 (保护 region_list)
```

查找 region 的过程：遍历 `region_list`，匹配 `mm` 和地址范围。

---

## 6. 完整流程分析

### 6.1 模块初始化 (wxjump_init)

```
wxjump_init()
  │
  ├─ 1. resolve_symbols()
  │     ├─ kallsyms_lookup_name("find_vma") → kfn_find_vma
  │     ├─ kallsyms_lookup_name("get_task_mm") → kfn_get_task_mm
  │     ├─ kallsyms_lookup_name("mmput") → kfn_mmput
  │     ├─ kallsyms_lookup_name("__get_free_pages") → kfn___get_free_pages
  │     ├─ kallsyms_lookup_name("free_pages") → kfn_free_pages
  │     ├─ kallsyms_lookup_name("memstart_addr") → kvar_memstart_addr
  │     ├─ 读取 TCR_EL1 → 计算 page_shift, page_level
  │     ├─ AT S1E1R + PAR_EL1 → 推算 physvirt_offset
  │     ├─ kallsyms_lookup_name("_raw_spin_lock") → 自旋锁
  │     ├─ kallsyms_lookup_name("flush_*") → 缓存/TLB 刷新函数
  │     ├─ kallsyms_lookup_name("kzalloc"/"kfree") → 内存分配
  │     └─ kallsyms_lookup_name("do_page_fault") → 页错误处理器
  │
  ├─ 2. scan_vma_offsets()
  │     └─ 扫描 vm_area_struct 中 vm_mm 字段的偏移量
  │        (不同内核版本/配置下 vm_mm 偏移不同)
  │
  ├─ 3. detect_mm_offset()
  │     └─ 推断 task_struct 中 mm 字段的偏移量
  │
  ├─ 4. try_scan_context_id()
  │     └─ 扫描 mm_struct 中 context.id (ASID) 的偏移量
  │        (用于精确 TLB 刷新)
  │
  ├─ 5. hook_wrap(do_page_fault, ..., do_page_fault_before)
  │     └─ KernelPatch 框架的 inline hook，在 do_page_fault 执行前插入回调
  │
  ├─ 6. hook_wrap(exit_mmap, ..., exit_mmap_before)
  │     └─ 进程退出时清理影子页
  │
  └─ 7. hook_syscalln(167, ..., prctl_before)
        └─ hook prctl 系统调用 (ARM64 syscall号=167)，
           拦截 PATCH/RELEASE 命令
```

### 6.2 PATCH 操作 (用户态→内核)

用户态通过 prctl 发起 PATCH 请求：

```c
// 用户态调用:
prctl(0x57585804,  // WXJUMP_PRCTL_PATCH
      page_addr,   // 目标页面地址 (4KB 对齐)
      buf_ptr,     // 用户态缓冲区 (包含跳转指令)
      len,         // 字节数 (通常 20 字节: 4条 MOVZ/MOVK + 1条 BR)
      offset);     // 页内偏移
```

内核中的完整处理流程：

```
prctl_before() [syscall hook]
  │
  ├─ 检查 option == WXJUMP_PRCTL_PATCH
  ├─ get_task_mm(current) → mm
  │
  └─ wxjump_do_patch(mm, page_addr, user_buf, len, offset)
       │
       ├─ 1. 参数校验: offset + len <= 4096, len <= 64
       │
       ├─ 2. wxjump_copy_from_user(mm, user_buf, kbuf, len)
       │     └─ 遍历用户态 buf 的页表 → 找到物理页 → 通过线性映射读取
       │        (避免依赖 copy_from_user 内核符号)
       │
       ├─ 3. wxjump_find_region(mm, page_addr)
       │     ├─ [找到且已有 shadow page] → 直接写入 + 刷新 icache → 返回
       │     └─ [未找到] → 继续创建
       │
       ├─ 4. find_vma(mm, page_addr) → vma
       │
       ├─ 5. 创建 wx_region
       │     ├─ kzalloc(sizeof(wx_region))
       │     ├─ kcalloc(nr_pages, sizeof(page_info))
       │     ├─ 填充 mm, vm_start, vm_end, nr_pages
       │     └─ list_add(&new_region->list, &region_list)
       │
       ├─ 6. 获取原始页信息
       │     ├─ get_user_pte(mm, page_addr) → pte
       │     └─ orig_pfn = (pte_val >> 12) & PFN_MASK
       │
       ├─ 7. 分配影子页
       │     ├─ shadow_va = __get_free_pages(GFP_KERNEL, 0)
       │     └─ shadow_pfn = va_to_pa(shadow_va) >> 12
       │
       ├─ 8. 复制内容 + 应用 patch
       │     ├─ memcpy(shadow_va, orig_va, 4096)  // 完整复制原始页
       │     └─ memcpy(shadow_va + offset, kbuf, len)  // 覆写 patch 区域
       │
       ├─ 9. 切换 PTE → shadow page (execute-only)
       │     ├─ *pte = make_pte(shadow_pfn, 0)
       │     │   // 0 = 无额外 AP/UXN 标志 → execute-only
       │     │   // PTE_BASE_FLAGS (0xF13) 包含 Valid+AF+SH
       │     └─ flush_tlb_page(vma, page_addr)
       │
       ├─ 10. 刷新 I-cache
       │      └─ __flush_icache_range(page_start, page_start + 4096)
       │
       └─ 11. pi->state = STATE_SHADOW_X, pi->patch_count = 1
```

### 6.3 运行时状态切换 (页错误驱动)

#### 场景 A: 正常执行 (无异常)

```
CPU 取指 → PTE 指向 shadow page (--x) → 执行跳转指令 → 跳到用户 thunk
                                         ↑
                                    零内核异常！
```

#### 场景 B: CRC 读取检测 (Data Abort)

```
CRC 代码读取 .text 段
  → PTE 指向 shadow page (--x, execute-only)
  → 数据访问被拒绝
  → Data Abort (EC=0x24, WnR=0, Permission Fault)
  → do_page_fault_before() 拦截

wxjump_handle_read_fault():
  1. 找到 region 和 page_info
  2. 验证映射仍然有效
  3. *pte = make_pte(orig_pfn, PTE_UXN_USER_RO)
     // PTE_UXN_USER_RO = UXN + 只读 → 可读不可执行 (r--)
  4. flush_tlb_page()
  5. state = STATE_ORIG_R
  6. fargs->skip_origin = 1  // 跳过原始 do_page_fault
  7. 返回 → CPU 重试读取 → 成功读到原始未修改的代码
     → CRC 校验通过！✓
```

#### 场景 C: CRC 之后恢复执行 (Instruction Abort)

```
CPU 继续执行目标函数
  → PTE 指向 orig page (r--, 不可执行)
  → 取指被拒绝
  → Instruction Abort (EC=0x20, Permission Fault)
  → do_page_fault_before() 拦截

wxjump_handle_exec_fault():
  1. 找到 region 和 page_info
  2. 验证映射有效
  3. flush_dcache_page(shadow_page)  // 确保 D-cache 写回
  4. *pte = make_pte(shadow_pfn, 0)  // 切回 execute-only
  5. flush_tlb_page() + flush_icache()
  6. state = STATE_SHADOW_X
  7. fargs->skip_origin = 1
  8. 返回 → CPU 重试取指 → 执行 shadow page 的跳转指令
```

#### 场景 D: 写操作 (清理)

如果有代码试图写入被 hook 的页面，说明映射已经改变，wxjump 清理该页面的状态：

```
写操作 → Data Abort (WnR=1) → wxjump_handle_write_fault()
  → wxjump_auto_cleanup(): 释放 shadow page，重置 state = NONE
  → 返回 -1 让原始 do_page_fault 处理
```

### 6.4 RELEASE 操作

```c
prctl(0x57585805,  // WXJUMP_PRCTL_RELEASE
      page_addr,   // 目标页面地址
      len,         // patch 长度
      offset,      // 页内偏移
      0);
```

```
wxjump_do_release():
  1. 从 orig page 恢复 offset 处的原始字节到 shadow page
  2. patch_count--
  3. 如果 patch_count == 0:
     ├─ *pte = make_pte(orig_pfn, PTE_USER_RDONLY)  // 恢复正常映射
     ├─ flush_icache()  // 清除 shadow 指令的 I-cache 残留
     ├─ free_pages(shadow_va)  // 释放影子页
     └─ state = STATE_NONE
  4. 如果 patch_count > 0:
     └─ flush_icache()  // 让恢复的字节生效
```

### 6.5 进程退出清理

```
exit_mmap_before(mm):
  1. 遍历 region_list，收集所有 mm 匹配的 region
  2. 对每个 region 的每个活跃 page:
     ├─ 恢复 PTE 指向 orig_pfn
     ├─ flush_tlb_page()
     └─ free_pages(shadow_va)
  3. kfree(pages), kfree(region)
```

### 6.6 模块卸载 (wxjump_exit)

```
wxjump_exit():
  1. unhook_syscalln(prctl)
  2. hook_unwrap_remove(exit_mmap)
  3. hook_unwrap_remove(do_page_fault)
  4. 遍历所有 region:
     ├─ 恢复所有页面的 PTE
     ├─ 刷新 TLB 和 I-cache
     ├─ 释放所有 shadow page
     └─ 释放 region 结构体
```

---

## 7. 关键代码逐行解读

### 7.1 PTE 构造

```c
static inline uint64_t make_pte(uint64_t pfn, uint64_t flags)
{
    return (pfn << 12) | flags | PTE_BASE_FLAGS;
}
```

- `pfn << 12`: 将页帧号放到 bits[47:12] (物理地址高位)
- `PTE_BASE_FLAGS` = 0xF13:
  - bit[0] = 1: 有效 (Valid)
  - bit[1] = 1: Page descriptor (bits[1:0]=0b11)
  - bit[4] = 1: AttrIndx (Normal memory)
  - bit[8:9] = 0b11: Inner Shareable
  - bit[10] = 1: AF (Access Flag)
  - bit[11] = 1: nG (non-Global, 使用 ASID)
- `flags`: 额外权限位
  - 0: execute-only (无 AP/UXN 额外位)
  - `PTE_UXN_USER_RO` = 0x400000000000C0: UXN(bit54) + AP[1](bit7) + AP[0](bit6) → 只读不可执行
  - `PTE_USER_RDONLY` = 0xC0: AP[7:6]=0b11 → 用户只读+可执行 (正常权限)

### 7.2 页表遍历

```c
static uint64_t *get_user_pte(void *mm, unsigned long addr, uint64_t *ptl_out)
```

这个函数手动遍历用户态页表（4级），找到指定虚拟地址对应的 PTE 指针：

1. 从 mm->pgd 读取 PGD (Page Global Directory) 基址
2. 根据虚拟地址的不同位段，逐级索引到对应的表项
3. 每级检查表项是否有效（非零且 bits[1:0] 正确）
4. 如果 PMD 级遇到 block mapping (2MB)，报错退出（不支持）
5. 最终返回 PTE 指针，可以直接修改

### 7.3 偏移量扫描

由于 KPM 不能依赖内核结构体定义（不同内核版本偏移不同），wxjump 在运行时动态扫描关键偏移：

**scan_vma_offsets()**: 获取当前进程的 mm，然后取第一个 VMA，在 VMA 结构体中扫描哪个 offset 存放了指向 mm 的指针 → 即 `vm_mm` 字段偏移。

**try_scan_context_id()**: 读取 TTBR0_EL1 的 ASID，然后在 mm_struct 中扫描匹配的值 → 即 `mm_struct.context.id` 字段偏移。

---

## 8. 参考资料与验证来源

### ARM 架构官方文档
- **ARM Architecture Reference Manual (ARMv8-A)**
  - D5: "The AArch64 Virtual Memory System Architecture"
  - D5.2: PTE 格式、权限位
  - C5.2: AT (地址翻译) 指令
  - D5.10: TLBI 指令
- **ARM Developer 文档**: https://developer.arm.com/documentation/102376/latest/ (地址翻译)
- **ESR_EL1 寄存器**: https://developer.arm.com/documentation/ddi0601/latest/AArch64-Registers/ESR-EL1

### Linux 内核源码
- `arch/arm64/include/asm/pgtable-hwdef.h` — PTE 位定义
- `arch/arm64/include/asm/pgtable-prot.h` — PAGE_EXECONLY 等权限模板
- `arch/arm64/mm/fault.c` — do_page_fault 处理流程
- `arch/arm64/include/asm/tlbflush.h` — TLB 刷新
- `include/linux/mm_types.h` — mm_struct, vm_area_struct 定义
- Linux kernel commit `bc07c2c6e9ed` — execute-only 页面支持

### 在线工具与文档
- **Linux 内核 API 文档**: https://docs.kernel.org/core-api/mm-api.html
- **ESR 解码器**: https://github.com/google/aarch64-esr-decoder
- **Linux 内核交叉引用**: https://elixir.bootlin.com/linux/latest/source
- **ARM 系统寄存器解析**: https://arm.jonpalmisc.com/latest_sysreg/
- **OSDev Wiki ARM Paging**: https://wiki.osdev.org/ARM_Paging
