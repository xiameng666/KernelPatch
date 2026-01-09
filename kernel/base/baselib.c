/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// 基础C库函数实现 - 提供内核环境下的标准库函数

#include <baselib.h>

// 复制内存直到遇到指定字符或达到指定长度
void *lib_memccpy(void *dst, const void *src, int c, size_t n)
{
    char *q = (char *)dst;
    const char *p = (const char *)src;
    char ch;
    while (n--) {
        *q++ = ch = *p++;  // 复制字符
        if (ch == (char)c) return q;  // 遇到指定字符则返回下一位置
    }
    return 0;  // 未找到指定字符
}

// 在内存区域中查找指定字符的首次出现
void *lib_memchr(const void *s, int c, size_t n)
{
    const unsigned char *sp = (const unsigned char *)s;
    while (n--) {
        if (*sp == (unsigned char)c) return (void *)sp;  // 找到字符，返回位置
        sp++;
    }
    return 0;  // 未找到字符
}

// 比较两个内存区域的内容
int lib_memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *c1 = (const unsigned char *)s1;
    const unsigned char *c2 = (const unsigned char *)s2;
    int d = 0;
    while (n--) {
        d = (int)*c1++ - (int)*c2++;  // 逐字节比较
        if (d) break;  // 发现不同立即返回差值
    }
    return d;  // 返回差值，0表示相等
}

// 复制指定长度的内存内容
void *lib_memcpy(void *dst, const void *src, size_t n)
{
    const char *p = (const char *)src;
    char *q = (char *)dst;
    while (n--) {
        *q++ = *p++;  // 逐字节复制
    }
    return dst;  // 返回目标地址
}

// 安全的内存移动函数 - 处理内存重叠情况
void *lib_memmove(void *dst, const void *src, size_t n)
{
    const char *p = (const char *)src;
    char *q = (char *)dst;

    if (q < p) {
        // 目标在源之前，正向复制
        while (n--) {
            *q++ = *p++;
        }
    } else {
        // 目标在源之后，反向复制以避免覆盖
        p += n;
        q += n;
        while (n--) {
            *--q = *--p;
        }
    }
    return dst;
}

// 从后往前查找指定字符在内存中的位置
void *lib_memrchr(const void *s, int c, size_t n)
{
    const unsigned char *sp = (const unsigned char *)s + n - 1;  // 从末尾开始

    while (n--) {
        if (*sp == (unsigned char)c) return (void *)sp;  // 找到字符
        sp--;  // 向前搜索
    }

    return 0;  // 未找到
}

// 将内存区域设置为指定值
void *lib_memset(void *dst, int c, size_t n)
{
    char *q = (char *)dst;
    while (n--) {
        *q++ = c;  // 设置每个字节为指定值
    }
    return dst;
}

// 交换两个内存区域的内容
void lib_memswap(void *m1, void *m2, size_t n)
{
    char *p = (char *)m1;
    char *q = (char *)m2;
    char tmp;

    while (n--) {
        tmp = *p;   // 临时保存
        *p = *q;    // 交换内容
        *q = tmp;

        p++;
        q++;
    }
}

// 最小化的内存比较函数
int min_memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *c1 = s1, *c2 = s2;
    int d = 0;
    while (n--) {
        d = (int)*c1++ - (int)*c2++;  // 逐字节比较
        if (d) break;  // 发现差异立即退出
    }
    return d;
}

// 在内存中搜索子序列 - 使用优化的搜索算法
void *lib_memmem(const void *haystack, size_t n, const void *needle, size_t m)
{
    const unsigned char *y = (const unsigned char *)haystack;
    const unsigned char *x = (const unsigned char *)needle;

    size_t j, k, l;

    if (m > n || !m || !n) return 0;  // 参数有效性检查

    if (1 != m) {
        // 多字符搜索：使用Two-Way算法的简化版本
        if (x[0] == x[1]) {
            k = 2;  // 跳跃步长
            l = 1;  // 回退步长
        } else {
            k = 1;
            l = 2;
        }

        j = 0;
        while (j <= n - m) {
            if (x[1] != y[j + 1]) {
                j += k;  // 不匹配时跳跃
            } else {
                // 检查完整匹配
                if (!lib_memcmp(x + 2, y + j + 2, m - 2) && x[0] == y[j]) return (void *)&y[j];
                j += l;  // 部分匹配时小步前进
            }
        }
    } else
        // 单字符搜索
        do {
            if (*y == *x) return (void *)y;
            y++;
        } while (--n);
    return 0;  // 未找到
}

// 不区分大小写的字符串比较
int lib_strcasecmp(const char *s1, const char *s2)
{
    const unsigned char *c1 = (const unsigned char *)s1;
    const unsigned char *c2 = (const unsigned char *)s2;
    unsigned char ch;
    int d = 0;
    while (1) {
        d = toupper(ch = *c1++) - toupper(*c2++);  // 转换为大写后比较
        if (d || !ch) break;  // 不相等或到达字符串末尾
    }
    return d;
}

// 在字符串中查找指定字符的首次出现
char *lib_strchr(const char *s, int c)
{
    while (*s != (char)c) {
        if (!*s) return 0;  // 到达字符串末尾仍未找到
        s++;
    }
    return (char *)s;  // 返回字符位置
}

// 比较两个字符串
int lib_strcmp(const char *s1, const char *s2)
{
    const unsigned char *c1 = (const unsigned char *)s1;
    const unsigned char *c2 = (const unsigned char *)s2;
    unsigned char ch;
    int d = 0;
    while (1) {
        d = (int)(ch = *c1++) - (int)*c2++;  // 逐字符比较
        if (d || !ch) break;  // 不相等或到达字符串末尾
    }
    return d;  // 返回差值，0表示相等
}

// 复制字符串到目标位置
char *lib_strcpy(char *dst, const char *src)
{
    char *q = dst;
    const char *p = src;
    char ch;
    do {
        *q++ = ch = *p++;  // 复制字符直到遇到空字符
    } while (ch);

    return dst;  // 返回目标字符串
}

// 安全的字符串复制函数 - 限制复制长度并返回源字符串长度
size_t lib_strlcpy(char *dst, const char *src, size_t size)
{
    size_t bytes = 0;
    char *q = dst;
    const char *p = src;
    char ch;

    while ((ch = *p++)) {
        if (bytes + 1 < size) *q++ = ch;  // 仅在不超出缓冲区时复制

        bytes++;  // 统计源字符串长度
    }
    if (size) *q = '\0';  // 确保目标字符串以空字符结尾
    return bytes;  // 返回源字符串的实际长度
}

// 计算字符串长度
size_t lib_strlen(const char *s)
{
    const char *ss = s;
    while (*ss)  // 找到字符串末尾的空字符
        ss++;
    return ss - s;  // 返回字符数量
}

// 不区分大小写的有限长度字符串比较
int lib_strncasecmp(const char *s1, const char *s2, size_t n)
{
    const unsigned char *c1 = (const unsigned char *)s1;
    const unsigned char *c2 = (const unsigned char *)s2;
    unsigned char ch;
    int d = 0;
    while (n--) {
        d = toupper(ch = *c1++) - toupper(*c2++);  // 转换为大写后比较
        if (d || !ch) break;  // 不相等或到达字符串末尾
    }
    return d;
}

// 有限长度的字符串连接
char *lib_strncat(char *dst, const char *src, size_t n)
{
    char *q = lib_strchr(dst, '\0');  // 找到目标字符串末尾
    const char *p = src;
    char ch;
    while (n--) {
        *q++ = ch = *p++;  // 复制字符
        if (!ch) return dst;  // 遇到源字符串末尾
    }
    // *q = '\0';  // 注释掉的代码，应该手动添加结束符
    return dst;
}

// 连接两个字符串
char *lib_strcat(char *dst, const char *src)
{
    lib_strcpy(lib_strchr(dst, '\0'), src);  // 在目标字符串末尾追加源字符串
    return dst;
}

// 比较字符串的前n个字符
int lib_strncmp(const char *s1, const char *s2, size_t n)
{
    const unsigned char *c1 = (const unsigned char *)s1;
    const unsigned char *c2 = (const unsigned char *)s2;
    unsigned char ch;
    int d = 0;
    while (n--) {
        d = (int)(ch = *c1++) - (int)*c2++;  // 逐字符比较
        if (d || !ch) break;  // 不相等或到达字符串末尾
    }
    return d;
}

// 复制字符串的前n个字符
char *lib_strncpy(char *dst, const char *src, size_t n)
{
    char *q = dst;
    const char *p = src;
    char ch;
    while (n) {
        n--;
        *q++ = ch = *p++;  // 复制字符
        if (!ch) break;    // 遇到源字符串末尾提前退出
    }
    // *q = '\0';  // 注释掉的代码，不会自动添加结束符
    return dst;
}

// 计算有限长度字符串的实际长度
size_t lib_strnlen(const char *s, size_t maxlen)
{
    const char *ss = s;
    while ((maxlen > 0) && *ss) {
        ss++;
        maxlen--;  // 限制最大搜索长度
    }
    return ss - s;  // 返回实际长度
}

// 在字符串中查找任意指定字符的首次出现
char *lib_strpbrk(const char *s1, const char *s2)
{
    const char *c = s2;
    if (!*s1) return (char *)0;  // 空字符串直接返回
    while (*s1) {
        for (c = s2; *c; c++) {
            if (*s1 == *c) break;  // 找到匹配字符
        }
        if (*c) break;  // 找到则退出外层循环
        s1++;
    }
    if (*c == '\0') s1 = 0;  // 未找到任何匹配字符
    return (char *)s1;
}

// 从后往前查找字符在字符串中的最后一次出现
char *lib_strrchr(const char *s, int c)
{
    const char *found = 0;  // 记录最后找到的位置
    while (*s) {
        if (*s == (char)c) found = s;  // 更新最后找到的位置
        s++;
    }
    return (char *)found;  // 返回最后一次出现的位置
}

// 分割字符串 - 破坏性地从字符串中提取下一个标记
char *lib_strsep(char **stringp, const char *delim)
{
    char *s = *stringp;
    char *e;
    if (!s) return 0;  // 字符串为空
    e = lib_strpbrk(s, delim);  // 查找分隔符
    if (e) *e++ = '\0';  // 找到分隔符，替换为结束符并移动到下一位置
    *stringp = e;  // 更新字符串指针
    return s;  // 返回当前标记
}

// 计算字符串开头连续包含指定字符集中字符的长度
size_t lib_strspn(const char *s1, const char *s2)
{
    const char *s = s1;
    const char *c;
    while (*s1) {
        for (c = s2; *c; c++) {
            if (*s1 == *c) break;  // 当前字符在字符集中
        }
        if (*c == '\0') break;  // 当前字符不在字符集中，停止计数
        s1++;
    }
    return s1 - s;  // 返回匹配字符的数量
}

// 在字符串中查找子字符串
char *lib_strstr(const char *haystack, const char *needle)
{
    return (char *)lib_memmem(haystack, lib_strlen(haystack), needle, lib_strlen(needle));
}
