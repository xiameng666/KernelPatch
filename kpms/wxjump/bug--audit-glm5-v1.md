● 基于对代码的深入分析,我可以指出几个可能导致长时间运行后黑屏的原因:

  黑屏原因分析

  1. 多核 TLB 一致性问题 (最可能)

  static void wxjump_flush_tlb_page(void *vma, unsigned long addr)
  {
      dsb(ishst);
      // ...
      asm volatile("tlbi vale1is, %0" :: "r"(val));  // 只刷新当前CPU的TLB
      // ...
  }

  问题: tlbi vale1is 只刷新当前 CPU 的 TLB 条目。当 CPU A 切换 PTE 后,CPU B 的 TLB 中仍缓存着旧的映射。长时间运行后,被
  hook 的代码可能在多个核心间调度执行,导致:
  - CPU B 执行到旧的 shadow page 地址 → 访问已释放的内存 → 黑屏
  - 或者 CPU B 看到 STATE_ORIG_R 但 TLB 仍指向 shadow → 执行失败

  2. 状态机竞态条件

  static int wxjump_switch_mapping(...)
  {
      // ...
      *pte = make_pte(pfn, extra_flags);  // 修改 PTE
      wxjump_flush_tlb_page(vma, addr);   // 刷新 TLB
      return 0;
  }

  // 在 wxjump_handle_read_fault 中
  ret = wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_UXN_USER_RO);
  // ...
  pi->state = STATE_ORIG_R;  // 状态更新在 PTE 修改之后

  问题: 在 wxjump_switch_mapping 返回后、pi->state 更新前存在窗口。另一个 CPU 的 fault handler 可能:
  1. 看到新的 PTE 但旧的状态 → 状态检查失败 → 返回 -1 → fault 未处理 → 崩溃

  3. 自旋锁死锁风险

  static void do_page_fault_before(hook_fargs3_t *fargs, void *udata)
  {
      // ...
      struct wx_region *region = wxjump_find_region(mm, far);  // 内部获取自旋锁
      // ...
      if (ec == 0x20) {
          if (!wxjump_handle_exec_fault(mm, far)) { ... }  // 内部也获取自旋锁
      }
  }

  问题: 在 page fault 路径中持有自旋锁,如果期间发生:
  - 另一个 page fault (访问 region_list 链表节点时)
  - 调度器抢占
  - NMI/硬中断

  可能导致死锁。长时间运行增加了触发概率。

  4. refcount 非原子操作

  static struct wx_region *wxjump_find_region(void *mm, unsigned long addr)
  {
      wx_spin_lock();
      list_for_each_entry(r, &region_list, list) {
          if (r->mm == mm && ...) {
              r->refcount++;  // 在锁内,OK
              wx_spin_unlock();
              return r;
          }
      }
      // ...
  }

  static void wxjump_put_region(struct wx_region *r)
  {
      wx_spin_lock();
      r->refcount--;  // 在锁内,OK
      if (r->refcount > 0) {
          wx_spin_unlock();
          return;
      }
      list_del_init(&r->list);  // 从链表删除
      wx_spin_unlock();
      // 释放内存...
  }

  问题: 虽然在锁内操作,但 wxjump_put_region 在释放内存前解锁,另一个线程可能在锁释放后、内存释放前通过 find_region
  找到这个即将被释放的 region → use-after-free。

  5. I-Cache 与 D-Cache 不一致

  static inline void wx_flush_dcache_va(uint64_t va, unsigned long size)
  {
      for (addr = va; addr < va + size; addr += 64)
          asm volatile("dc cvau, %0" :: "r"(addr) : "memory");  // 按缓存行刷新
      dsb(ish);
  }

  static inline void wx_flush_icache(unsigned long addr)
  {
      if (kfn___flush_icache_range) {
          kfn___flush_icache_range(page_start, page_start + WX_PAGE_SIZE);
      } else {
          asm volatile("ic ialluis" ::: "memory");  // 刷新所有 icache
      }
  }

  问题: dc cvau 只刷新当前 CPU 的 dcache。在多核系统上,修改 shadow page 后,CPU A 的 dcache 已刷新,但 CPU B 的 icache
  可能仍缓存旧指令。

  ---
  建议的修复方向

  1. 使用 flush_tlb_mm 或 flush_tlb_kernel_range 来刷新所有 CPU 的 TLB,或者使用 IPI 通知其他核心。
  2. 将状态更新移到 PTE 修改之前,并使用写屏障:
  pi->state = STATE_ORIG_R;
  smp_wmb();  // 写屏障
  *pte = make_pte(...);
  wxjump_flush_tlb_page(...);
  3. 在 page fault handler 中使用 spin_lock_irqsave 防止中断重入。
  4. 使用 atomic_t 管理 refcount,并在 put_region 中先从链表移除再释放。
  5. 使用 __flush_dcache_area 或 clean_dcache_area 确保多核 dcache 一致性。

  ---
  需要我帮你生成具体的修复代码吗?