/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 内核补丁启动日志系统 - 提供早期启动阶段的日志记录功能

#include <stdint.h>

#define BOOT_LOG_SIZE 1024  // 启动日志缓冲区大小

static char boot_log[BOOT_LOG_SIZE] = { 0 };  // 启动日志缓冲区
static int boot_log_len = 0;                  // 当前日志长度
static int boot_log_fin = 0;                  // 日志是否已完成标志
