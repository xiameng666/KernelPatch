# wxjump.c 二次审计报告 (Post-fix Re-audit)

## 前9个修复验证

所有 9 个修复均已正确应用：

- **Fix 1** `PTE_BASE_FLAGS = 0xF03` ✅ — 内核源码确认 `AttrIndx=0 → MT_NORMAL`
- **Fix 2** `orig_pte` 字段 ✅
- **Fix 3** `do_patch` 保存 `orig_pte` ✅
- **Fix 4** `dc cvau` 替代 `flush_dcache_page` ✅ — 内核源码确认 `flush_dcache_page()` 仅清除 `PG_dcache_clean` 标志，不实际刷缓存，新版本反而更正确
- **Fix 5** T1SZ/TG1 命名 ✅ — 内核源码确认 `TCR_T1SZ_OFFSET=16`、`TCR_TG1_SHIFT=30`；`(60-t1sz)/bits_per_level` 与内核宏 `ARM64_HW_PGTABLE_LEVELS(va_bits) = (va_bits-4)/(PAGE_SHIFT-3)` 完全一致
- **Fix 6/7/8** `orig_pte` 直写恢复 ✅
- **Fix 9** `goto` 替代递归 ✅

## 新发现的 BUG

### 严重 (HIGH) — 修复引入的回归

**BUG-A: `kfn___flush_tlb_range` 函数指针声明残缺 (line 107-108)**

```c path="C:\\Users\\24151\\Documents\\GitHub\\无痕hook\\KernelPatch\\kpms\\wxjump\\wxjump.c" start=107 end=108
static void  (*kfn___flush_tlb_range)
                                       unsigned long stride, int last_level, int tlb_level);
```

缺少 `(void *vma, unsigned long start, unsigned long end,`——参数列表前三个参数及左括号被截断。**编译错误，模块无法构建**。 推测是 Fix 4 删除 `kfn_flush_dcache_page` 声明时，误删了相邻多行声明的上半段。

应为：

```c
static void  (*kfn___flush_tlb_range)(void *vma, unsigned long start, unsigned long end,
                                       unsigned long stride, int last_level, int tlb_level);
```

---

**BUG-B: stale region goto 路径 `vma` 未初始化 (line 690 → 734 → 800)**

```c path="C:\\Users\\24151\\Documents\\GitHub\\无痕hook\\KernelPatch\\kpms\\wxjump\\wxjump.c" start=690 end=690
    void *vma;
```

声明时未赋值 `NULL`。当已有 region 但 `shadow_page_va == 0`（stale page）时，line 734 的 `goto setup_shadow_on_existing` 跳转后，line 800 的 `if (!vma)` 检查的是栈上的垃圾值（几乎不会为 NULL），导致跳过 `kfn_find_vma`，用垃圾指针操作 PTE。**必然内核崩溃**。 Fix 9 用 goto 替代递归时引入。

修复：`void *vma = NULL;`

### 中等 (MEDIUM) — 预存问题

**BUG-C: `wxjump_exit` 自旋锁内调用可睡眠分配 (line 1410-1422)**

`wx_spin_lock()` 在 line 1410 获取后，line 1421 调用 `kfn_kzalloc(..., wx_gfp_kernel)` (GFP_KERNEL = 3264，可睡眠)。**自旋锁内睡眠 → 死锁风险**。

修复方案：先 unlock，做分配，再 lock 遍历删除；或改用 GFP_ATOMIC / 栈上固定数组。

**BUG-D: `cleanup_list` 分配失败无 NULL 检查 (line 1422 → 1428)**

若 `kfn_kzalloc` 返回 NULL，line 1428 `cleanup_list[i++] = r` 直接 **NULL 解引用**。

**BUG-E: PTE 写入与 TLBI 之间缺少 `dsb(ishst)` (wxjump_switch_mapping line 311-312 及所有直写路径)**

ARM ARM 要求 PTE store 之后、TLBI 之前插入 DSB 确保写入对 TLB walker 可见。当前直接 `*pte = ...; TLBI` 可能导致旧 TLB 条目被重新缓存。影响所有 PTE 修改路径（switch_mapping、RELEASE、exit_mmap、wxjump_exit）。

### 低 (LOW) — 潜在 / 非关键

- `get_user_pte` line 283 硬编码 `(addr >> 12) & 0x1FF`，仅适用 4KB 页（GKI 下无影响）
- Shadow page PTE 未设 PXN (bit 53)，EL1 可执行用户影子页内容
- `global_lock` 类型为 `uint64_t` 而非正式 `spinlock_t`

---

**总结：9 个原始修复全部正确生效，但引入了 2 个新的高严重度回归 (A: 编译错误, B: vma 未初始化)，以及暴露了 3 个预存的中等问题。建议优先修复 BUG-A 和 BUG-B。**