# wxjump + frida-rust 无痕断点 全栈审计报告

**审计者**: Oz (claude 4.6 opus)
**日期**: 2026-03-11
**审计目标**: 定位"长时间运行导致安卓手机重启"的根因
**审计范围**: `wxjump.c` (内核侧) + `frida-rust/quickjs-hook` (用户态侧)
**基准 commit**: `0709dd5` (KernelPatch), frida-rust 当前 HEAD

---

## 结论摘要

**手机重启的第一嫌疑在 `wxjump` 内核侧。** 发现 3 个可直接导致内核 panic 的高危并发 bug (P0)，均与 region/page_info 生命周期管理有关。`frida-rust` 用户态也有 4 个确认成立的 bug，更像"放大器"——让关键进程 crash/卡死，间接表现为系统重启。

严重度分布：

- **P0 (内核 panic / 整机重启)**: 3 个，全部在 `wxjump`
- **P1 (关键进程 crash / 死锁)**: 4 个，全部在 `frida-rust`
- **P2 (可移植性 / 正确性隐患)**: 2 个，跨两侧

---

## P0-1: `exit_mmap` / 模块卸载绕过 refcount，导致内核 UAF

**严重度**: P0 — 可直接内核 panic
**位置**: `wxjump.c (1051-1113)` `exit_mmap_before()`，`wxjump.c (1479-1576)` `wxjump_exit()`

### 问题描述

`wxjump_find_region()` / `wxjump_put_region()` 实现了 refcount 机制保护 region 生命周期：

```c
// wxjump.c:325-338 — find_region 增加 refcount
static struct wx_region *wxjump_find_region(void *mm, unsigned long addr)
{
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

// wxjump.c:341-365 — put_region: refcount 降到 0 才真正释放
static void wxjump_put_region(struct wx_region *r)
{
    wx_spin_lock();
    r->refcount--;
    if (r->refcount > 0) {
        wx_spin_unlock();
        return;
    }
    list_del_init(&r->list);
    wx_spin_unlock();
    // ... free pages, free region
}
```

但 `exit_mmap_before()` 和 `wxjump_exit()` **完全无视 refcount**，直接从链表摘除并释放：

```c
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

**严重度**: P1 — 关键进程死锁 / 无响应
**位置**: `frida-rust/quickjs-hook/src/jsapi/interceptor.rs (65-114, 118-157)`，`frida-rust/quickjs-hook/src/jsapi/hook_api.rs (62-146, 150-210)`

### 问题描述

所有回调 wrapper 的模式都是：

```rust
// interceptor.rs:74-98 — on_enter_wrapper
let guard = INTERCEPTOR_REGISTRY.lock().unwrap();  // ← 加锁
// ... 取出 hook ...
let result = ffi::JS_Call(ctx, on_enter, global, 1, ...);  // ← 持锁调 JS!
// guard 直到函数结束才 drop → 整个 JS_Call 执行期间都持锁
```

```rust
// hook_api.rs:73-146 — hook_callback_wrapper
let guard = HOOK_REGISTRY.lock().unwrap();  // ← 加锁
// ... 取出 hook_data ...
let result = ffi::JS_Call(ctx, callback, global, 1, ...);  // ← 持锁调 JS!
```

如果 JS 回调内部调用了任何需要再次获取同一 mutex 的操作：

- `Interceptor.detachAll()` → 调 `cleanup_interceptor_hooks()` → 锁 `INTERCEPTOR_REGISTRY`
- `unhook()` → 调 `HOOK_REGISTRY.lock()`
- 甚至嵌套触发同一地址的 hook callback

结果就是 **同一线程重入同一 `Mutex`**，Rust 的 `Mutex` 不是递归锁 → **死锁**。

### 修复方向

在持锁期间只做查找和复制，把 callback 信息复制到局部变量，**解锁后** 再调用 `JS_Call`：

```rust
let (ctx, callback_bytes) = {
    let guard = INTERCEPTOR_REGISTRY.lock().unwrap();
    let registry = guard.as_ref()?;
    let hook = registry.get(&target_addr)?;
    (hook.ctx, hook.on_enter_bytes)
};
// guard 已 drop，锁已释放
let callback: ffi::JSValue = unsafe { std::ptr::read(...) };
let result = ffi::JS_Call(ctx, callback, ...);  // ← 无锁调用
```

---

## P1-2: `HOOK_REGISTRY` 与 `INTERCEPTOR_REGISTRY` 是两把不同锁，无法串行化同一 JSContext

**严重度**: P1 — 并发访问 QuickJS 导致内部状态损坏
**位置**: `frida-rust/quickjs-hook/src/jsapi/interceptor.rs (55)`，`frida-rust/quickjs-hook/src/jsapi/hook_api.rs (51)`，`frida-rust/quickjs-hook/src/lib.rs (44-45)`

### 问题描述

```rust
// interceptor.rs:55
static INTERCEPTOR_REGISTRY: Mutex<Option<HashMap<u64, InterceptorHook>>> = Mutex::new(None);

// hook_api.rs:51
static HOOK_REGISTRY: Mutex<Option<HashMap<u64, HookData>>> = Mutex::new(None);
```

两套 registry 保存的 `ctx` 都指向同一个全局 `JSContext`（来自 `JS_ENGINE: Mutex<Option<JSEngine>>`），但用的是**不同的锁**。

当两个线程分别从 `hook()` 和 `Interceptor.attach()` 路径进入回调时：

- 线程 A 锁 `HOOK_REGISTRY`，调 `JS_Call`
- 线程 B 锁 `INTERCEPTOR_REGISTRY`，也调 `JS_Call`
- 两者都觉得自己已经"加锁"了，但实际上**同时进入了同一个 QuickJS JSContext**

QuickJS 不是线程安全的，并发访问会导致内部数据结构损坏。

### 修复方向

引入**单一全局 JSContext 执行锁**，所有需要调用 QuickJS API 的路径都必须通过这一把锁串行化。

---

## P1-3: attach thunk 未保存 SIMD/FP 寄存器，`nzcv` 声明但未实际保存

**严重度**: P1 — 被 hook 函数行为异常
**位置**: `frida-rust/quickjs-hook/src/hook_engine.c (600-699)` `generate_attach_thunk()`，`frida-rust/quickjs-hook/src/hook_engine.h (31-36)` `HookContext`

### 问题描述

`HookContext` 声明了 `nzcv` 字段：

```c
// hook_engine.h:31-36
typedef struct {
    uint64_t x[31];     /* x0-x30 */
    uint64_t sp;
    uint64_t pc;
    uint64_t nzcv;      /* Condition flags */
} HookContext;
```

但 `generate_attach_thunk()` 中：

1. **`nzcv` 从未被保存或恢复** — thunk 里没有 `mrs/msr nzcv` 指令
2. **SIMD/FP 寄存器 (q0-q31) 完全不保存** — 任何使用浮点/向量的函数被 hook 后行为不可预测
3. **调原函数前只恢复 x0-x7**：

```c
// hook_engine.c:653-657
/* Restore x0-x7 (arguments) - they may have been modified by callback */
for (int i = 0; i < 8; i += 2) {
    arm64_writer_put_ldp_reg_reg_reg_offset(&w, ARM64_REG_X0 + i, ARM64_REG_X0 + i + 1,
                                             ARM64_REG_SP, i * 8, ARM64_INDEX_SIGNED_OFFSET);
}
```

`x8` (indirect result location register) 未恢复，对返回大结构体的函数是致命的。`x9-x15` (caller-saved temporaries) 在 on_enter callback 执行期间可能被破坏，虽然按 AAPCS 是 caller-saved，但某些编译器优化后的代码可能依赖其值。

### 影响

- hook 使用浮点参数/返回值的函数 → 返回垃圾值
- hook 返回大结构体的函数 → x8 被 callback 的 C 代码覆盖 → 写到错误地址
- 条件分支依赖 nzcv → callback 修改了 flags → 被 hook 函数分支走错

### 修复方向

1. 在 thunk 入口保存 `nzcv`：`mrs x16, nzcv` + `str x16, [sp, #nzcv_offset]`
2. 在调原函数前恢复 `nzcv`：`ldr x16, [sp, #nzcv_offset]` + `msr nzcv, x16`
3. 至少保存/恢复 `q0-q7` (AAPCS 参数/返回值)，理想情况下保存 `q0-q31`
4. 恢复 `x8` (line 653 的循环改为 `i < 9`)

---

## P1-4: `hook_remove()` / `hook_engine_cleanup()` 无 in-flight quiesce

**严重度**: P1 — 可导致用户态 crash
**位置**: `frida-rust/quickjs-hook/src/hook_engine.c (847-897)` `hook_remove()`，`(910-940)` `hook_engine_cleanup()`

### 问题描述

```c
// hook_engine.c:847-897
int hook_remove(void* target) {
    pthread_mutex_lock(&g_engine.lock);
    // ... 找到 entry ...
    if (entry->stealth) {
        wxshadow_release(target, entry->original_size);
    } else {
        target_write(target, entry->original_bytes, entry->original_size);
    }
    // 从链表移除
    free_entry(entry);                    // ← thunk/trampoline 内存回到 free list
    pthread_mutex_unlock(&g_engine.lock);
    return HOOK_OK;
}
```

`hook_remove()` 恢复原字节后立即把 entry（包括 thunk 和 trampoline）放回 free list。但此时可能有其他线程**正在执行这个 thunk 或 trampoline 的代码**。

- 如果 free list 上的 thunk 内存被后续 `hook_alloc()` 重用并覆写 → 正在执行的线程跳飞
- `hook_engine_cleanup()` 更激进：直接 `free(entry)` → thunk/trampoline 可能被系统 malloc 回收

### 修复方向

引入 in-flight 计数器或 epoch-based 回收：

- 每个 entry 加 `in_flight_count`
- thunk 入口 `atomic_inc(in_flight_count)`，出口 `atomic_dec(in_flight_count)`
- `hook_remove()` 恢复原字节后等待 `in_flight_count == 0` 再回收

---

## P2-1: 64-byte cache line 硬编码

**严重度**: P2 — 可移植性
**位置**: `wxjump.c (180-185)` `wx_flush_dcache_va()`，`hook_engine.c (275-285)` `hook_flush_cache()`

两处都硬编码 `addr += 64` 作为 cache line 步进。Cortex-A510 等小核 cache line 可能为 32 或 128 字节。建议运行时读取 `CTR_EL0.DminLine` / `CTR_EL0.IminLine`。

在 GKI 常见 SoC (Cortex-A5x/A7x/X1-X4) 上 cache line 确实是 64 字节，所以当前不影响主流设备，但不是"正确"的做法。

---

## P2-2: 4KB 页面假设

**严重度**: P2 — 可移植性
**位置**: `wxjump.c (51, 285, 322, 687-689, 909, ...)`，`hook_engine.c (297-352)`

多处硬编码 `4096` / `0xFFF` / `>> 12`。Android GKI 5.10+ 默认 4KB 页，但 `get_user_pte()` line 285 的 `(addr >> 12) & 0x1FF` 也是 4KB 特定的。如果未来内核启用 16KB 页，这些代码全部需要修改。

当前 GKI 下无影响，但作为技术债务记录。

---

## 与前序审计报告的关系

- **GLM5 报告 (`bug--audit-glm5-v1.md`)**: 第 2 条"状态机竞态窗口"成立，对应本报告 P0-2；其余 4 条不成立或证据不足（已在之前的交互中确认）
- **Claude v2 报告 (`bug-audit-claude-v2.md`)**: BUG-A/B/C/D/E 是该 commit 引入的回归，与本报告发现的问题正交（本报告基于已修复后的 `0709dd5`）
- **本报告是首次覆盖 `frida-rust` 用户态侧的审计**

---

## 重启根因排序

如果是**真正的内核 panic / 整机重启 (有 pstore)**：

1. P0-1: `exit_mmap` / 模块卸载绕过 refcount → UAF
2. P0-2: fault 路径解锁后裸用 `page_info` → UAF
3. P0-3: 对 live shadow page 直接 memcpy → 指令流损坏

如果是**系统界面重启 / zygote 重启 / "看起来像重启"**：

1. P1-1: registry mutex + JS_Call 自锁 → 死锁
2. P1-2: 两套 registry 导致 JSContext 并发 → crash
3. P1-3: thunk ABI 不完整 → 关键进程异常
4. P1-4: remove 无 quiesce → 代码执行中被覆写

**建议修复顺序**: P0-1 → P0-2 → P0-3 → P1-1 → P1-2 → P1-3 → P1-4 → P2
