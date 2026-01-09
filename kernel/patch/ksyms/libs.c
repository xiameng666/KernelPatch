/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

/**
 * @file libs.c
 * @brief 内核符号库导入模块 - 为KernelPatch导入内核常用库函数符号
 * @details 本模块负责导入和导出内核中的各种库函数符号，包括调试、用户空间访问、
 * 字符串处理、内存操作、格式化输出等功能，为KernelPatch模块提供完整的库函数支持
 */

#include <ksyms.h>
#include <ktypes.h>
#include <symbol.h>
#include <common.h>
#include <stdarg.h>

//=============================================================================
// 调试相关函数（来自lib/dump_stack.c）
//=============================================================================

/**
 * @brief 按指定日志级别转储当前调用堆栈
 * @param log_lvl 日志级别字符串（如KERN_ERR、KERN_WARNING等）
 * @details 用于调试时打印当前函数调用链，可指定不同的内核日志级别
 */
void kfunc_def(dump_stack_lvl)(const char *log_lvl) = 0;

/**
 * @brief 转储当前调用堆栈（使用默认日志级别）
 * @details 调试时打印当前函数调用链的简化版本，等同于dump_stack_lvl(NULL)
 */
void kfunc_def(dump_stack)(void) = 0;

/**
 * @brief 调试相关函数符号匹配
 * @param name 符号名称
 * @param addr 符号地址
 * @details 在内核符号表中查找并匹配调试相关的函数地址
 */
static void _linux_lib_misc(const char *name, unsigned long addr)
{
    kfunc_match(dump_stack_lvl, name, addr);
    kfunc_match(dump_stack, name, addr);
}

#include <linux/uaccess.h>

//=============================================================================
// 用户空间访问函数（来自lib/strncpy_from_user.c和相关文件）
//=============================================================================

/**
 * @brief 从用户空间安全复制字符串（无故障版本）
 * @param dst 目标内核缓冲区
 * @param unsafe_addr 用户空间源地址
 * @param count 最大复制字节数
 * @return 成功返回复制的字节数，失败返回负错误码
 * @details 即使用户地址无效也不会触发页面错误，适用于不确定用户地址有效性的场景
 */
long kfunc_def(strncpy_from_user_nofault)(char *dst, const void __user *unsafe_addr, long count) = 0;

/**
 * @brief 从不安全用户空间复制字符串
 * @param dst 目标内核缓冲区
 * @param unsafe_addr 用户空间源地址
 * @param count 最大复制字节数
 * @return 成功返回复制的字节数，失败返回负错误码
 * @details 用于复制可能无效的用户空间字符串，有额外的安全检查
 */
long kfunc_def(strncpy_from_unsafe_user)(char *dst, const void __user *unsafe_addr, long count) = 0;

/**
 * @brief 从用户空间复制字符串（标准版本）
 * @param dest 目标内核缓冲区
 * @param src 用户空间源字符串
 * @param count 最大复制字节数
 * @return 成功返回复制的字节数，失败返回负错误码
 * @details 标准的用户空间字符串复制函数，包含访问检查
 */
long kfunc_def(strncpy_from_user)(char *dest, const char __user *src, long count) = 0;

/**
 * @brief 测量用户空间字符串长度（无故障版本）
 * @param unsafe_addr 用户空间字符串地址
 * @param count 最大检查长度
 * @return 字符串长度或负错误码
 * @details 安全地测量用户空间字符串长度，不会因无效地址而出错
 */
long kfunc_def(strnlen_user_nofault)(const void __user *unsafe_addr, long count) = 0;

/**
 * @brief 测量不安全用户空间字符串长度
 * @param unsafe_addr 用户空间字符串地址
 * @param count 最大检查长度
 * @return 字符串长度或负错误码
 */
long kfunc_def(strnlen_unsafe_user)(const void __user *unsafe_addr, long count) = 0;

/**
 * @brief 测量用户空间字符串长度（标准版本）
 * @param str 用户空间字符串
 * @param n 最大检查长度
 * @return 字符串长度
 */
long kfunc_def(strnlen_user)(const char __user *str, long n);

/**
 * @brief 用户空间访问函数符号匹配
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配用户空间字符串操作相关的内核函数地址
 */
static void _linux_lib_strncpy_from_user_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(strncpy_from_user_nofault, name, addr);
    kfunc_match(strncpy_from_unsafe_user, name, addr);
    kfunc_match(strncpy_from_user, name, addr);

    // 字符串长度函数暂时注释掉，可能在某些内核版本中不可用
    // kfunc_match(strnlen_user_nofault, name, addr);
    // kfunc_match(strnlen_unsafe_user, name, addr);
    // kfunc_match(strnlen_user, name, addr);
}

//=============================================================================
// 字符串处理函数（来自lib/string.c）
//=============================================================================

#include <linux/string.h>

/**
 * @brief 不区分大小写比较字符串（限定长度）
 * @param s1 第一个字符串
 * @param s2 第二个字符串  
 * @param len 比较的最大字符数
 * @return 0相等，<0 s1<s2，>0 s1>s2
 */
int kfunc_def(strncasecmp)(const char *s1, const char *s2, size_t len) = 0;
KP_EXPORT_SYMBOL(kfunc(strncasecmp));

/**
 * @brief 不区分大小写比较字符串（无长度限制）
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @return 0相等，<0 s1<s2，>0 s1>s2
 */
int kfunc_def(strcasecmp)(const char *s1, const char *s2) = 0;
KP_EXPORT_SYMBOL(kfunc(strcasecmp));

/**
 * @brief 复制字符串
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @return 目标字符串指针
 * @details 复制整个源字符串到目标缓冲区，包括结尾的'\0'
 */
char *kfunc_def(strcpy)(char *dest, const char *src) = 0;
KP_EXPORT_SYMBOL(kfunc(strcpy));

/**
 * @brief 复制字符串（限定长度）
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param count 最大复制字符数
 * @return 目标字符串指针
 */
char *kfunc_def(strncpy)(char *dest, const char *src, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(strncpy));

/**
 * @brief 安全复制字符串（保证null结尾）
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param size 目标缓冲区大小
 * @return 源字符串长度
 */
size_t kfunc_def(strlcpy)(char *dest, const char *src, size_t size) = 0;
KP_EXPORT_SYMBOL(kfunc(strlcpy));

/**
 * @brief 安全复制字符串（返回复制的字符数）
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param count 目标缓冲区大小
 * @return 复制的字符数，负值表示截断
 */
ssize_t kfunc_def(strscpy)(char *dest, const char *src, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(strscpy));

/**
 * @brief 安全复制字符串并用null填充剩余空间
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param count 目标缓冲区大小
 * @return 复制的字符数，负值表示截断
 */
ssize_t kfunc_def(strscpy_pad)(char *dest, const char *src, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(strscpy_pad));

/**
 * @brief 复制字符串并返回结尾指针
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @return 指向目标字符串结尾'\0'的指针
 */
char *kfunc_def(stpcpy)(char *__restrict__ dest, const char *__restrict__ src) = 0;
KP_EXPORT_SYMBOL(kfunc(stpcpy));

/**
 * @brief 连接字符串
 * @param dest 目标字符串（会被修改）
 * @param src 要追加的源字符串
 * @return 目标字符串指针
 */
char *kfunc_def(strcat)(char *dest, const char *src) = 0;
KP_EXPORT_SYMBOL(kfunc(strcat));

/**
 * @brief 连接字符串（限定长度）
 * @param dest 目标字符串（会被修改）
 * @param src 要追加的源字符串
 * @param count 最大追加字符数
 * @return 目标字符串指针
 */
char *kfunc_def(strncat)(char *dest, const char *src, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(strncat));

/**
 * @brief 安全连接字符串（保证结果不超过缓冲区）
 * @param dest 目标字符串
 * @param src 要追加的源字符串
 * @param count 目标缓冲区总大小
 * @return 尝试创建的字符串总长度
 */
size_t kfunc_def(strlcat)(char *dest, const char *src, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(strlcat));
/**
 * @brief 比较两个字符串
 * @param cs 第一个字符串
 * @param ct 第二个字符串
 * @return 0相等，<0 cs<ct，>0 cs>ct
 */
int kfunc_def(strcmp)(const char *cs, const char *ct) = 0;
KP_EXPORT_SYMBOL(kfunc(strcmp));

/**
 * @brief 比较两个字符串（限定长度）
 * @param cs 第一个字符串
 * @param ct 第二个字符串
 * @param count 比较的最大字符数
 * @return 0相等，<0 cs<ct，>0 cs>ct
 */
int kfunc_def(strncmp)(const char *cs, const char *ct, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(strncmp));

/**
 * @brief 在字符串中查找字符
 * @param s 源字符串
 * @param c 要查找的字符
 * @return 找到字符的指针，未找到返回NULL
 */
char *kfunc_def(strchr)(const char *s, int c) = 0;
KP_EXPORT_SYMBOL(kfunc(strchr));

/**
 * @brief 在字符串中查找字符（未找到返回字符串末尾）
 * @param s 源字符串
 * @param c 要查找的字符
 * @return 找到字符的指针，未找到返回指向'\0'的指针
 */
char *kfunc_def(strchrnul)(const char *s, int c) = 0;
KP_EXPORT_SYMBOL(kfunc(strchrnul));

/**
 * @brief 在限定长度内查找字符（未找到返回末尾）
 * @param s 源字符串
 * @param count 搜索的最大字符数
 * @param c 要查找的字符
 * @return 找到字符的指针或搜索区域末尾指针
 */
char *kfunc_def(strnchrnul)(const char *s, size_t count, int c) = 0;
KP_EXPORT_SYMBOL(kfunc(strnchrnul));

/**
 * @brief 从字符串末尾向前查找字符
 * @param s 源字符串
 * @param c 要查找的字符
 * @return 找到字符的指针，未找到返回NULL
 */
char *kfunc_def(strrchr)(const char *s, int c) = 0;
KP_EXPORT_SYMBOL(kfunc(strrchr));

/**
 * @brief 在限定长度内查找字符
 * @param s 源字符串
 * @param count 搜索的最大字符数
 * @param c 要查找的字符
 * @return 找到字符的指针，未找到返回NULL
 */
char *kfunc_def(strnchr)(const char *s, size_t count, int c) = 0;
KP_EXPORT_SYMBOL(kfunc(strnchr));

/**
 * @brief 跳过字符串开头的空白字符
 * @param str 源字符串
 * @return 指向第一个非空白字符的指针
 */
char *kfunc_def(skip_spaces)(const char *str) = 0;
KP_EXPORT_SYMBOL(kfunc(skip_spaces));

/**
 * @brief 去除字符串两端的空白字符
 * @param s 要处理的字符串（会被修改）
 * @return 处理后的字符串指针
 */
char *kfunc_def(strim)(char *s) = 0;
KP_EXPORT_SYMBOL(kfunc(strim));

/**
 * @brief 计算字符串长度
 * @param s 源字符串
 * @return 字符串长度（不包括结尾的'\0'）
 */
size_t kfunc_def(strlen)(const char *s) = 0;
KP_EXPORT_SYMBOL(kfunc(strlen));

/**
 * @brief 计算字符串长度（限定最大长度）
 * @param s 源字符串
 * @param count 最大检查长度
 * @return 字符串长度，不超过count
 */
size_t kfunc_def(strnlen)(const char *s, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(strnlen));

/**
 * @brief 计算字符串前缀中包含指定字符集的长度
 * @param s 源字符串
 * @param accept 接受的字符集
 * @return 前缀长度
 */
size_t kfunc_def(strspn)(const char *s, const char *accept) = 0;
KP_EXPORT_SYMBOL(kfunc(strspn));

/**
 * @brief 计算字符串前缀中不包含指定字符集的长度
 * @param s 源字符串
 * @param reject 拒绝的字符集
 * @return 前缀长度
 */
size_t kfunc_def(strcspn)(const char *s, const char *reject) = 0;
KP_EXPORT_SYMBOL(kfunc(strcspn));

/**
 * @brief 在字符串中查找任一指定字符
 * @param cs 源字符串
 * @param ct 包含要查找字符的字符串
 * @return 找到字符的指针，未找到返回NULL
 */
char *kfunc_def(strpbrk)(const char *cs, const char *ct) = 0;
KP_EXPORT_SYMBOL(kfunc(strpbrk));

/**
 * @brief 字符串分隔函数
 * @param s 指向字符串指针的指针（会被修改）
 * @param ct 分隔符字符串
 * @return 下一个标记的指针，无更多标记返回NULL
 */
char *kfunc_def(strsep)(char **s, const char *ct) = 0;
KP_EXPORT_SYMBOL(kfunc(strsep));

/**
 * @brief sysfs风格字符串比较（忽略尾随换行符）
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @return true表示相等，false表示不等
 */
bool kfunc_def(sysfs_streq)(const char *s1, const char *s2) = 0;
KP_EXPORT_SYMBOL(kfunc(sysfs_streq));

/**
 * @brief 在字符串数组中查找匹配项
 * @param array 字符串数组
 * @param n 数组元素数量
 * @param string 要查找的字符串
 * @return 匹配的索引，未找到返回负值
 */
int kfunc_def(match_string)(const char *const *array, size_t n, const char *string) = 0;
KP_EXPORT_SYMBOL(kfunc(match_string));

/**
 * @brief sysfs风格字符串数组匹配
 * @param array 字符串数组
 * @param n 数组元素数量
 * @param str 要查找的字符串
 * @return 匹配的索引，未找到返回负值
 */
int kfunc_def(__sysfs_match_string)(const char *const *array, size_t n, const char *str) = 0;
KP_EXPORT_SYMBOL(kfunc(__sysfs_match_string));

//=============================================================================
// 内存操作函数
//=============================================================================

/**
 * @brief 用指定值填充内存区域
 * @param s 目标内存指针
 * @param c 填充值
 * @param count 填充字节数
 * @return 目标内存指针
 */
void *kfunc_def(memset)(void *s, int c, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(memset));

/**
 * @brief 用16位值填充内存区域
 * @param s 目标16位整数数组
 * @param v 填充的16位值
 * @param count 填充的16位值个数
 * @return 目标内存指针
 */
void *kfunc_def(memset16)(uint16_t *s, uint16_t v, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(memset16));

/**
 * @brief 用32位值填充内存区域
 * @param s 目标32位整数数组
 * @param v 填充的32位值
 * @param count 填充的32位值个数
 * @return 目标内存指针
 */
void *kfunc_def(memset32)(uint32_t *s, uint32_t v, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(memset32));

/**
 * @brief 用64位值填充内存区域
 * @param s 目标64位整数数组
 * @param v 填充的64位值
 * @param count 填充的64位值个数
 * @return 目标内存指针
 */
void *kfunc_def(memset64)(uint64_t *s, uint64_t v, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(memset64));

/**
 * @brief 复制内存区域
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 复制字节数
 * @return 目标内存指针
 * @details 适用于源和目标区域不重叠的情况
 */
void *kfunc_def(memcpy)(void *dest, const void *src, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(memcpy));

/**
 * @brief 复制内存区域（处理重叠）
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 复制字节数
 * @return 目标内存指针
 * @details 可以安全处理源和目标区域重叠的情况
 */
void *kfunc_def(memmove)(void *dest, const void *src, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(memmove));

/**
 * @brief 比较内存区域
 * @param cs 第一个内存区域
 * @param ct 第二个内存区域
 * @param count 比较字节数
 * @return 0相等，<0 cs<ct，>0 cs>ct
 */
int kfunc_def(memcmp)(const void *cs, const void *ct, size_t count) = 0;
KP_EXPORT_SYMBOL(kfunc(memcmp));

/**
 * @brief 比较内存区域（bcmp版本）
 * @param a 第一个内存区域
 * @param b 第二个内存区域
 * @param len 比较字节数
 * @return 0相等，非0不等
 */
int kfunc_def(bcmp)(const void *a, const void *b, size_t len) = 0;
KP_EXPORT_SYMBOL(kfunc(bcmp));
/**
 * @brief 在内存中扫描指定字节值
 * @param addr 起始内存地址
 * @param c 要扫描的字节值
 * @param size 扫描的内存大小
 * @return 找到字节的指针，未找到返回末尾指针
 */
void *kfunc_def(memscan)(void *addr, int c, size_t size) = 0;
KP_EXPORT_SYMBOL(kfunc(memscan));

/**
 * @brief 在字符串中查找子字符串
 * @param s1 源字符串
 * @param s2 要查找的子字符串
 * @return 找到子字符串的指针，未找到返回NULL
 */
char *kfunc_def(strstr)(const char *s1, const char *s2) = 0;
KP_EXPORT_SYMBOL(kfunc(strstr));

/**
 * @brief 在限定长度内查找子字符串
 * @param s1 源字符串
 * @param s2 要查找的子字符串
 * @param len 搜索的最大长度
 * @return 找到子字符串的指针，未找到返回NULL
 */
char *kfunc_def(strnstr)(const char *s1, const char *s2, size_t len) = 0;
KP_EXPORT_SYMBOL(kfunc(strnstr));

/**
 * @brief 在内存中查找指定字节
 * @param s 内存区域
 * @param c 要查找的字节值
 * @param n 搜索的字节数
 * @return 找到字节的指针，未找到返回NULL
 */
void *kfunc_def(memchr)(const void *s, int c, size_t n) = 0;
KP_EXPORT_SYMBOL(kfunc(memchr));

/**
 * @brief 在内存中查找不等于指定值的字节
 * @param start 起始内存地址
 * @param c 不等于的字节值
 * @param bytes 搜索的字节数
 * @return 找到不等字节的指针，未找到返回NULL
 */
void *kfunc_def(memchr_inv)(const void *start, int c, size_t bytes) = 0;
KP_EXPORT_SYMBOL(kfunc(memchr_inv));

/**
 * @brief 替换字符串中的字符
 * @param s 要处理的字符串
 * @param old 要替换的字符
 * @param new 替换成的字符
 * @return 处理后的字符串指针
 */
char *kfunc_def(strreplace)(char *s, char old, char new) = 0;
KP_EXPORT_SYMBOL(kfunc(strreplace));

/**
 * @brief 触发fortify错误panic
 * @param name 函数名称
 * @details 用于缓冲区溢出检测时触发panic
 */
void kfunc_def(fortify_panic)(const char *name) = 0;
KP_EXPORT_SYMBOL(kfunc(fortify_panic));

/**
 * @brief 将字符串转换为无符号长长整型
 * @param s 要转换的字符串
 * @param base 进制基数
 * @param res 存储结果的指针
 * @return 0成功，负值表示错误
 */
int __must_check kfunc_def(kstrtoull)(const char *s, unsigned int base, unsigned long long *res) = 0;
KP_EXPORT_SYMBOL(kfunc(kstrtoull));

/**
 * @brief 将字符串转换为有符号长长整型
 * @param s 要转换的字符串
 * @param base 进制基数
 * @param res 存储结果的指针
 * @return 0成功，负值表示错误
 */
int __must_check kfunc_def(kstrtoll)(const char *s, unsigned int base, long long *res) = 0;
KP_EXPORT_SYMBOL(kfunc(kstrtoll));

/**
 * @brief 字符串处理函数符号匹配
 * @param name 符号名称
 * @param addr 符号地址
 * @details 在内核符号表中查找并匹配字符串和内存操作相关的函数地址
 */
static void _linux_lib_string_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(strncasecmp, name, addr);
    kfunc_match(strcasecmp, name, addr);
    kfunc_match(strcpy, name, addr);
    kfunc_match(strncpy, name, addr);
    kfunc_match(strlcpy, name, addr);
    kfunc_match(strscpy, name, addr);
    kfunc_match(strscpy_pad, name, addr);
    kfunc_match(stpcpy, name, addr);
    kfunc_match(strcat, name, addr);
    kfunc_match(strncat, name, addr);
    kfunc_match(strlcat, name, addr);
    kfunc_match(strcmp, name, addr);
    kfunc_match(strncmp, name, addr);
    kfunc_match(strchr, name, addr);
    kfunc_match(strchrnul, name, addr);
    kfunc_match(strnchrnul, name, addr);
    kfunc_match(strrchr, name, addr);
    kfunc_match(strnchr, name, addr);
    kfunc_match(skip_spaces, name, addr);
    kfunc_match(strim, name, addr);
    kfunc_match(strlen, name, addr);
    kfunc_match(strnlen, name, addr);
    kfunc_match(strspn, name, addr);
    kfunc_match(strcspn, name, addr);
    kfunc_match(strpbrk, name, addr);
    kfunc_match(strsep, name, addr);
    // kfunc_match(sysfs_streq, name, addr);        // 某些版本可能不可用
    kfunc_match(match_string, name, addr);
    // kfunc_match(__sysfs_match_string, name, addr); // 某些版本可能不可用
    kfunc_match(memset, name, addr);
    // kfunc_match(memset16, name, addr);           // 某些版本可能不可用
    // kfunc_match(memset32, name, addr);           // 某些版本可能不可用
    // kfunc_match(memset64, name, addr);           // 某些版本可能不可用
    kfunc_match(memcpy, name, addr);
    kfunc_match(memmove, name, addr);
    kfunc_match(memcmp, name, addr);
    kfunc_match(bcmp, name, addr);
    kfunc_match(memscan, name, addr);
    kfunc_match(strstr, name, addr);
    kfunc_match(strnstr, name, addr);
    kfunc_match(memchr, name, addr);
    kfunc_match(memchr_inv, name, addr);
    kfunc_match(strreplace, name, addr);
    // kfunc_match(fortify_panic, name, addr);      // 某些版本可能不可用
    kfunc_match(kstrtoull, name, addr);
    kfunc_match(kstrtoll, name, addr);
}

//=============================================================================
// 参数解析函数（来自lib/argv_split.c）
//=============================================================================

/**
 * @brief 释放argv数组内存
 * @param argv 要释放的字符串指针数组
 * @details 释放由argv_split分配的内存
 */
void kfunc_def(argv_free)(char **argv) = 0;
KP_EXPORT_SYMBOL(kfunc(argv_free));

/**
 * @brief 分割字符串为参数数组
 * @param gfp 内存分配标志
 * @param str 要分割的字符串
 * @param argcp 存储参数个数的指针
 * @return 参数字符串数组，失败返回NULL
 * @details 将命令行字符串分割成独立的参数，模拟shell行为
 */
char **kfunc_def(argv_split)(gfp_t gfp, const char *str, int *argcp) = 0;
KP_EXPORT_SYMBOL(kfunc(argv_split));

/**
 * @brief 参数解析函数符号匹配
 * @param name 符号名称
 * @param addr 符号地址
 */
static void _linux_lib_argv_split_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(argv_free, name, addr);
    kfunc_match(argv_split, name, addr);
}

//=============================================================================
// 序列缓冲区函数
//=============================================================================

#include <linux/seq_buf.h>
#include <linux/trace_seq.h>

/**
 * @brief 将序列缓冲区内容复制到用户空间
 * @param s 序列缓冲区
 * @param ubuf 用户空间缓冲区
 * @param cnt 复制的字节数
 * @return 成功复制的字节数或负错误码
 */
int kfunc_def(seq_buf_to_user)(struct seq_buf *s, char __user *ubuf, int cnt) = 0;

/**
 * @brief 将跟踪序列内容复制到用户空间
 * @param s 跟踪序列
 * @param ubuf 用户空间缓冲区
 * @param cnt 复制的字节数
 * @return 成功复制的字节数或负错误码
 */
int kfunc_def(trace_seq_to_user)(struct trace_seq *s, char __user *ubuf, int cnt) = 0;

/**
 * @brief 将xt数据复制到用户空间
 * @param dst 用户空间目标
 * @param src 内核空间源数据
 * @param usersize 用户大小
 * @param size 实际大小
 * @param aligned_size 对齐大小
 * @return 成功返回0，失败返回错误码
 */
int kfunc_def(xt_data_to_user)(void __user *dst, const void *src, int usersize, int size, int aligned_size) = 0;

/**
 * @brief 将位图复制到用户空间
 * @param bits 位图
 * @param maxbit 最大位数
 * @param maxlen 最大长度
 * @param p 用户空间指针
 * @param compat 兼容模式标志
 * @return 成功返回0，失败返回错误码
 */
int kfunc_def(bits_to_user)(unsigned long *bits, unsigned int maxbit, unsigned int maxlen, void __user *p,
                            int compat) = 0;

/**
 * @brief 序列缓冲区函数符号匹配
 * @param name 符号名称
 * @param addr 符号地址
 */
static void _linux_lib_seq_buf_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(seq_buf_to_user, name, addr);
    kfunc_match(trace_seq_to_user, name, addr);
    kfunc_match(xt_data_to_user, name, addr);
    // TODO: 静态函数可能无法直接匹配
    kfunc_match(bits_to_user, name, addr);
}

//=============================================================================
// 格式化输出函数（来自linux/include/kernel.h）
//=============================================================================

/**
 * @brief 格式化字符串输出到缓冲区
 * @param buf 输出缓冲区
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 输出的字符数
 */
int kfunc_def(sprintf)(char *buf, const char *fmt, ...) = 0;
KP_EXPORT_SYMBOL(kfunc(sprintf));

/**
 * @brief 格式化字符串输出到缓冲区（va_list版本）
 * @param buf 输出缓冲区
 * @param fmt 格式字符串
 * @param args 参数列表
 * @return 输出的字符数
 */
int kfunc_def(vsprintf)(char *buf, const char *fmt, va_list args) = 0;
KP_EXPORT_SYMBOL(kfunc(vsprintf));

/**
 * @brief 安全格式化字符串输出（限制长度）
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 输出的字符数（不包括结尾null）
 */
int kfunc_def(snprintf)(char *buf, size_t size, const char *fmt, ...) = 0;
KP_EXPORT_SYMBOL(kfunc(snprintf));

/**
 * @brief 安全格式化字符串输出（va_list版本）
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 * @param fmt 格式字符串
 * @param args 参数列表
 * @return 输出的字符数（不包括结尾null）
 */
int kfunc_def(vsnprintf)(char *buf, size_t size, const char *fmt, va_list args) = 0;
KP_EXPORT_SYMBOL(kfunc(vsnprintf));

/**
 * @brief 安全格式化字符串输出（确保不超出缓冲区）
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 实际输出的字符数
 */
int kfunc_def(scnprintf)(char *buf, size_t size, const char *fmt, ...) = 0;
KP_EXPORT_SYMBOL(kfunc(scnprintf));

/**
 * @brief 安全格式化字符串输出（va_list版本，确保不超出缓冲区）
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 * @param fmt 格式字符串
 * @param args 参数列表
 * @return 实际输出的字符数
 */
int kfunc_def(vscnprintf)(char *buf, size_t size, const char *fmt, va_list args) = 0;
KP_EXPORT_SYMBOL(kfunc(vscnprintf));

/**
 * @brief 分配内存并格式化字符串
 * @param gfp 内存分配标志
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 分配的字符串指针，失败返回NULL
 * @details 自动分配足够的内存来存储格式化后的字符串
 */
char *kfunc_def(kasprintf)(gfp_t gfp, const char *fmt, ...) = 0;
KP_EXPORT_SYMBOL(kfunc(kasprintf));

/**
 * @brief 分配内存并格式化字符串（va_list版本）
 * @param gfp 内存分配标志
 * @param fmt 格式字符串
 * @param args 参数列表
 * @return 分配的字符串指针，失败返回NULL
 */
char *kfunc_def(kvasprintf)(gfp_t gfp, const char *fmt, va_list args) = 0;
KP_EXPORT_SYMBOL(kfunc(kvasprintf));

/**
 * @brief 从字符串解析格式化输入
 * @param buf 输入字符串
 * @param fmt 格式字符串
 * @param ... 输出变量指针
 * @return 成功解析的项目数
 */
int kfunc_def(sscanf)(const char *buf, const char *fmt, ...) = 0;
KP_EXPORT_SYMBOL(kfunc(sscanf));

/**
 * @brief 从字符串解析格式化输入（va_list版本）
 * @param buf 输入字符串
 * @param fmt 格式字符串
 * @param args 参数列表
 * @return 成功解析的项目数
 */
int kfunc_def(vsscanf)(const char *buf, const char *fmt, va_list args) = 0;
KP_EXPORT_SYMBOL(kfunc(vsscanf));

/**
 * @brief 格式化输出函数符号匹配
 * @param name 符号名称
 * @param addr 符号地址
 * @details 匹配内核中的格式化输出和输入相关函数
 */
static void _linux_include_kernel_sym_match(const char *name, unsigned long addr)
{
    kfunc_match(sprintf, name, addr);
    kfunc_match(vsprintf, name, addr);
    kfunc_match(snprintf, name, addr);
    kfunc_match(vsnprintf, name, addr);
    kfunc_match(scnprintf, name, addr);
    kfunc_match(vscnprintf, name, addr);
    kfunc_match(kasprintf, name, addr);
    kfunc_match(kvasprintf, name, addr);
    kfunc_match(sscanf, name, addr);
    kfunc_match(vsscanf, name, addr);
}

/**
 * @brief 内核库函数符号初始化的内部函数
 * @param data 用户数据（未使用）
 * @param name 符号名称
 * @param m 模块指针（未使用）
 * @param addr 符号地址
 * @details 遍历内核符号表时的回调函数，用于匹配各类库函数符号
 */
static void _linux_libs_symbol_init(void *data, const char *name, struct module *m, unsigned long addr)
{
    _linux_lib_misc(name, addr);                    // 匹配调试相关函数
    _linux_lib_strncpy_from_user_sym_match(name, addr);  // 匹配用户空间访问函数
    _linux_lib_string_sym_match(name, addr);        // 匹配字符串和内存操作函数
    _linux_lib_argv_split_sym_match(name, addr);    // 匹配参数解析函数
    _linux_lib_seq_buf_sym_match(name, addr);       // 匹配序列缓冲区函数
    _linux_include_kernel_sym_match(name, addr);    // 匹配格式化输出函数
}

/**
 * @brief 初始化Linux内核库函数符号
 * @param name 符号名称（当使用kallsyms_lookup_name时使用）
 * @param addr 符号地址（当使用kallsyms_lookup_name时使用）
 * @details 根据编译配置选择不同的符号查找方式：
 * - 如果定义了INIT_USE_KALLSYMS_LOOKUP_NAME，则使用kallsyms_lookup_name查找
 * - 否则遍历所有内核符号进行匹配
 */
void linux_libs_symbol_init(const char *name, unsigned long addr)
{
#ifdef INIT_USE_KALLSYMS_LOOKUP_NAME
    // 使用kallsyms_lookup_name方式初始化（需要预知符号名称）
    _linux_libs_symbol_init(0, 0, 0, 0);
#else
    // 遍历所有内核符号进行匹配（更通用但较慢）
    kallsyms_on_each_symbol(_linux_libs_symbol_init, 0);
#endif
}
