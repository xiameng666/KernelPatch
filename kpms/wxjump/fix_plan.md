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
