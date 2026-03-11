# `demo-wxshadow-lite`

这是一个教学版 KPM，用来把“反编译出来的 `wxshadow` 思路”拆成最小可学单元。

## 学习顺序

1. 先看模块骨架：`wxshadow_lite.c`
2. 再看控制入口：`wxshadow_lite_ctl0`
3. 再看 hook 入口：`demo_add_before` / `demo_add_after`
4. 最后再对照原始反编译包里的 `init/exit/prctl/fault`

## 它演示了什么

- `KPM_NAME` / `KPM_VERSION` / `KPM_LICENSE` / `KPM_AUTHOR` / `KPM_DESCRIPTION`
- `KPM_INIT` / `KPM_CTL0` / `KPM_CTL1` / `KPM_EXIT`
- `compat_copy_to_user`
- `hook_wrap2` / `hook_unwrap`
- `hook_fargs2_t` 和 `hook_local_t`
- `u64` / `pid_t` / `uid_t` / `umode_t` 这类 KPM 常见类型从哪里来

## 模块结构

- `wxshadow_lite_init`
  - 记录 `event` 和 `args`
  - 打印 `kpver` / `kver`
  - 如果参数是 `autohook`，启动本地函数 hook
- `wxshadow_lite_ctl0`
  - 字符串命令入口
  - 支持 `ping` / `state` / `hook` / `unhook` / `test <a> <b>` / `bias <n>`
- `wxshadow_lite_ctl1`
  - 原始三参数入口
  - 这里用来直接修改 `g_add_bias`
- `wxshadow_lite_exit`
  - 统一清理 hook

## 和 `wxshadow` 的对应关系

- 这个教学版的 `ctl0/ctl1`，对应原版的“控制面”
- 这个教学版的 `hook_wrap2(demo_add)`，对应原版的 `hook_wrap(brk_handler)`、`hook_wrap(single_step_handler)` 等
- 这个教学版故意不碰页表、TLB、`mm_struct`、`vm_area_struct`，这样更适合先学 KPM 框架本身

## 常用头文件速查

- `kernel/include/kpmodule.h`
  - KPM 元信息宏和 `init/ctl/exit` 签名
- `kernel/include/compiler.h`
  - `__user`、`__noinline`、`__aligned` 等属性宏
- `kernel/include/ktypes.h`
  - `u64`、`s32`、`pid_t`、`uid_t`、`umode_t` 等基础类型
- `kernel/patch/include/kputils.h`
  - `compat_copy_to_user`、`compat_strncpy_from_user`
- `kernel/include/hook.h`
  - `hook_wrapN`、`hook_fargsN_t`、`hook_local_t`
- `kernel/linux/include/linux/kernel.h`
  - `snprintf`、`scnprintf`、`sscanf`
- `kernel/linux/include/linux/printk.h`
  - `pr_info`、`pr_warn`、`pr_err`
- `kernel/linux/include/linux/string.h`
  - `strcmp`、`strscpy`、`memset`

## 编译

```sh
cd kpms/demo-wxshadow-lite
make
```

## 学习建议

- 先把 `test 20 10` 跑通
- 再执行 `hook`
- 再执行 `test 20 10`
- 再执行 `bias 7`
- 再执行一次 `test 20 10`
- 最后执行 `state`

这样你就能直观看到：

- 没 hook 时，函数结果是原始值
- hook 后，`after` 回调会改 `ret`
- `ctl0` 和 `ctl1` 都能修改模块状态
