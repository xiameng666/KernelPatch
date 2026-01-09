/* 字节序转换工具函数集合 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include "order.h"

/**
 * @brief 16位无符号整数字节交换
 * @details 将高低字节互换位置，用于大小端转换
 * @param val 待交换的16位无符号整数
 * @return 字节交换后的结果
 */
inline uint16_t u16swp(uint16_t val)
{
    // 将高8位移到低8位，低8位移到高8位
    return (val << 8) | (val >> 8);
}

/**
 * @brief 16位有符号整数字节交换
 * @details 执行字节交换并保持符号位正确性
 * @param val 待交换的16位有符号整数
 * @return 字节交换后的结果
 */
inline int16_t i16swp(int16_t val)
{
    // 高8位移到低8位，低8位移到高8位，掩码确保符号扩展正确
    return (val << 8) | ((val >> 8) & 0xFF);
}

/**
 * @brief 转换为小端格式的16位无符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始16位无符号整数
 * @return 小端格式的结果
 */
uint16_t u16le(uint16_t val)
{
    // 如果当前是大端系统则交换字节，否则保持原值
    return is_be() ? u16swp(val) : val;
}

/**
 * @brief 转换为大端格式的16位无符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始16位无符号整数
 * @return 大端格式的结果
 */
uint16_t u16be(uint16_t val)
{
    // 如果当前是大端系统则保持原值，否则交换字节
    return is_be() ? val : u16swp(val);
}

/**
 * @brief 转换为小端格式的16位有符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始16位有符号整数
 * @return 小端格式的结果
 */
int16_t i16le(int16_t val)
{
    // 如果当前是大端系统则交换字节，否则保持原值
    return is_be() ? i16swp(val) : val;
}

/**
 * @brief 转换为大端格式的16位有符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始16位有符号整数
 * @return 大端格式的结果
 */
int16_t i16be(int16_t val)
{
    // 如果当前是大端系统则保持原值，否则交换字节
    return is_be() ? val : i16swp(val);
}

/**
 * @brief 32位无符号整数字节交换
 * @details 将32位整数的4个字节顺序完全颠倒
 * @param val 待交换的32位无符号整数
 * @return 字节交换后的结果
 */
uint32_t u32swp(uint32_t val)
{
    // 第一步：交换相邻字节对，0x12345678 -> 0x21436587
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    // 第二步：交换16位字，0x21436587 -> 0x87654321
    return (val << 16) | (val >> 16);
}

/**
 * @brief 32位有符号整数字节交换
 * @details 执行字节交换并确保符号位处理正确
 * @param val 待交换的32位有符号整数
 * @return 字节交换后的结果
 */
int32_t i32swp(int32_t val)
{
    // 第一步：交换相邻字节对
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    // 第二步：交换16位字，掩码防止符号扩展问题
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

/**
 * @brief 转换为小端格式的32位无符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始32位无符号整数
 * @return 小端格式的结果
 */
uint32_t u32le(uint32_t val)
{
    // 如果当前是大端系统则交换字节，否则保持原值
    return is_be() ? u32swp(val) : val;
}

/**
 * @brief 转换为大端格式的32位无符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始32位无符号整数
 * @return 大端格式的结果
 */
uint32_t u32be(uint32_t val)
{
    // 如果当前是大端系统则保持原值，否则交换字节
    return is_be() ? val : u32swp(val);
}

/**
 * @brief 转换为小端格式的32位有符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始32位有符号整数
 * @return 小端格式的结果
 */
int32_t i32le(int32_t val)
{
    // 如果当前是大端系统则交换字节，否则保持原值
    return is_be() ? i32swp(val) : val;
}

/**
 * @brief 转换为大端格式的32位有符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始32位有符号整数
 * @return 大端格式的结果
 */
int32_t i32be(int32_t val)
{
    // 如果当前是大端系统则保持原值，否则交换字节
    return is_be() ? val : i32swp(val);
}

/**
 * @brief 64位有符号整数字节交换
 * @details 将64位整数的8个字节顺序完全颠倒
 * @param val 待交换的64位有符号整数
 * @return 字节交换后的结果
 */
int64_t i64swp(int64_t val)
{
    // 第一步：交换相邻字节对
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    // 第二步：交换相邻16位字对
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    // 第三步：交换32位字，掩码防止符号扩展问题
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}

/**
 * @brief 64位无符号整数字节交换
 * @details 将64位整数的8个字节顺序完全颠倒
 * @param val 待交换的64位无符号整数
 * @return 字节交换后的结果
 */
uint64_t u64swp(uint64_t val)
{
    // 第一步：交换相邻字节对
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    // 第二步：交换相邻16位字对
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    // 第三步：交换32位字
    return (val << 32) | (val >> 32);
}

/**
 * @brief 转换为小端格式的64位有符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始64位有符号整数
 * @return 小端格式的结果
 */
int64_t i64le(int64_t val)
{
    // 如果当前是大端系统则交换字节，否则保持原值
    return is_be() ? i64swp(val) : val;
}

/**
 * @brief 转换为大端格式的64位有符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始64位有符号整数
 * @return 大端格式的结果
 */
int64_t i64be(int64_t val)
{
    // 如果当前是大端系统则保持原值，否则交换字节
    return is_be() ? val : i64swp(val);
}

/**
 * @brief 转换为小端格式的64位无符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始64位无符号整数
 * @return 小端格式的结果
 */
uint64_t u64le(uint64_t val)
{
    // 如果当前是大端系统则交换字节，否则保持原值
    return is_be() ? u64swp(val) : val;
}

/**
 * @brief 转换为大端格式的64位无符号整数
 * @details 根据当前系统字节序决定是否执行转换
 * @param val 原始64位无符号整数
 * @return 大端格式的结果
 */
uint64_t u64be(uint64_t val)
{
    // 如果当前是大端系统则保持原值，否则交换字节
    return is_be() ? val : u64swp(val);
}
