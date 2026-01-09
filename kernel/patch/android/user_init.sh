#!/bin/sh

# Android系统初始化脚本 - 在不同启动阶段加载KernelPatch模块和服务

KPMS_DIR="/data/adb/ap/kpms/"          # KernelPatch模块目录
MAGISK_POLICY_PATH="/data/adb/ap/bin/magiskpolicy"  # Magisk策略工具路径
SUPERCMD="truncate"                    # 超级命令名称
MAGISK_SCTX="u:r:magisk:s0"          # Magisk SELinux上下文
APD_PATH="/data/adb/apd"              # APD守护进程路径
DEV_LOG_DIR="/dev/user_init_log/"     # 临时日志目录

skey="$1"    # 超级密钥
event="$2"   # 启动事件类型

mkdir -p "$DEV_LOG_DIR"

LOG_FILE="$DEV_LOG_DIR""$event"

# 重定向输出到日志文件
exec >>$LOG_FILE 2>&1

set -x

# 加载KernelPatch模块的函数
load_modules() {
    for dir in "$KPMS_DIR/*"; do
        if [ ! -d "$dir" ]; then continue; fi        # 跳过非目录项
        if [ -e "$dir/disable" ]; then continue; fi  # 跳过已禁用的模块
        main_sh="$dir/main.sh"
        if [ -e "$main_sh" ]; then
            touch "$dir/disable"                      # 创建临时禁用标记防止重复加载
            echo "loading $dir/main.sh ..."
            . "$main_sh"                             # 执行模块的初始化脚本
            rm -f "$dir/disable"                     # 移除临时禁用标记
        else
            echo "Error: $main_sh not found in $dir"
        fi
    done
}

# 主处理函数 - 根据不同的启动事件执行相应操作
handle() {
    $SUPERCMD $skey event $event "before"           # 触发事件前置处理
    case "$event" in
    "early-init" | "init" | "late-init") ;;        # 早期初始化阶段，无特殊操作
    "post-fs-data")
        $MAGISK_POLICY_PATH --magisk --live         # 应用Magisk策略
        load_modules $skey $event                   # 加载模块
        $SUPERCMD $skey -Z $MAGISK_SCTX exec $APD_PATH -s $skey $event  # 启动APD服务
        ;;
    "services")
        $SUPERCMD $skey -Z $MAGISK_SCTX exec $APD_PATH -s $skey $event  # 启动服务阶段的APD
        ;;
    "boot-completed")
        $SUPERCMD $skey -Z $MAGISK_SCTX exec $APD_PATH -s $skey $event  # 启动完成阶段的APD
        $SUPERCMD su -Z $MAGISK_SCTX exec $APD_PATH uid-listener &      # 后台启动UID监听器
        ;;
    *)
        echo "unknown user_init event: $event"      # 未知事件类型
        ;;
    esac
    $SUPERCMD $skey event $event "after"            # 触发事件后置处理
}

handle
