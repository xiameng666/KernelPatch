/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2024 bmax121. All Rights Reserved.
 * 用户事件报告模块 - 提供用户事件记录功能
 */

#include <user_event.h>

#include <log.h>

// 报告用户事件到内核日志
// event: 事件名称
// args: 事件参数
// 返回值: 0=成功
int report_user_event(const char *event, const char *args)
{
    // 记录用户报告的事件和参数到内核信息日志
    logki("user report event: %s, args: %s\n", event, args);
    return 0;
}