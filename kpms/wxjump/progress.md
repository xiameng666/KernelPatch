# Progress

## 2026-03-14
- **原因**: 配合 Frida gum stealth hook 集成，需要让用户态检测 wxjump KPM 是否已加载
- **修改**: wxjump.c 新增 `WXJUMP_PRCTL_QUERY (0x57585806)` prctl 命令，在 `prctl_before` 中拦截并返回 0 表示 KPM 可用。Frida gum 的 `gum_stealth_is_available()` 通过此命令探测 wxjump 状态
- **时间**: 2026-03-14 16:30
