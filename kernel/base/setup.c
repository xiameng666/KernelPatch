/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 内核补丁设置和配置数据结构定义

#include "setup.h"
#include "../version"

// 设置头部信息 - 存储在.setup.header段中
setup_header_t header __section(.setup.header) = { .magic = KP_MAGIC,  // 魔数标识
                                                   .kp_version.major = MAJOR,  // 主版本号
                                                   .kp_version.minor = MINOR,  // 次版本号
                                                   .kp_version.patch = PATCH,  // 补丁版本号
                                                   .config_flags = 0  // 配置标志
#ifdef ANDROID
                                                                   | CONFIG_ANDROID  // Android配置
#endif
#ifdef DEBUG
                                                                   | CONFIG_DEBUG    // 调试配置
#endif
                                                   ,
                                                   .compile_time = __TIME__ " " __DATE__ };  // 编译时间

// 预设配置数据 - 存储在.setup.preset段中
setup_preset_t setup_preset __section(.setup.preset) = { 0 };

// 栈空间定义 - 存储在.setup.data段中，16字节对齐
struct
{
    uint8_t fp[STACK_SIZE];  // 栈帧指针区域
    uint8_t sp[0];           // 栈指针标记（零长度数组）
} stack __section(.setup.data) __aligned(16);
