/* TLSF内存分配器实现
 * Two Level Segregated Fit (TLSF) - O(1)时间复杂度的实时内存分配器
 * 
 * 这是一个高性能的动态内存分配算法，算法核心思想：
 * - 使用ffs/fls位操作指令进行快速查找
 * - 按大小将空闲块分组到不同的segregated list中
 * - 使用位图标记哪些大小类别有可用块
 * - 支持32位和64位架构的优化实现
 */

#include <stddef.h>
#include <stdint.h>
#include <log.h>

#include "tlsf.h"

// TODO: 待优化项目
#define printf logkd  // 重定向printf到内核日志
#define CHAR_BIT 8    // 字符位数定义
#define tlsf_assert(x)  // 断言宏（当前为空实现）

#if defined(__cplusplus)
#define tlsf_decl inline   // C++环境下使用inline
#else
#define tlsf_decl static   // C环境下使用static
#endif

/*
** 架构特定的位操作例程
**
** TLSF通过限制对大小合适的自由列表的搜索，
** 结合使用位掩码和架构特定位操作例程的高效自由列表查询，
** 实现了malloc和free操作的O(1)时间复杂度。
**
** 大多数现代处理器提供了计算前导零位数、
** 查找最低和最高设置位等指令。
** 当可用时将使用这些特定实现，
** 否则回退到相当高效的通用实现。
**
** 注意：TLSF规范依赖于ffs/fls返回值0..31。
** ffs/fls默认返回1-32，错误时返回0。
*/

/*
** 检测是否为32位或64位（LP/LLP）架构构建。
** 编译时没有可靠的可移植方法。
*/
#if defined(__alpha__) || defined(__ia64__) || defined(__x86_64__) || defined(_WIN64) || defined(__LP64__) || \
    defined(__LLP64__)
#define TLSF_64BIT
#endif

/*
** gcc 3.4 and above have builtin support, specialized for architecture.
** Some compilers masquerade as gcc; patchlevel test filters them out.
*/
#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && defined(__GNUC_PATCHLEVEL__)

#if defined(__SNC__)
/* SNC for Playstation 3. */

tlsf_decl int tlsf_ffs(unsigned int word)
{
    const unsigned int reverse = word & (~word + 1);
    const int bit = 32 - __builtin_clz(reverse);
    return bit - 1;
}

#else

tlsf_decl int tlsf_ffs(unsigned int word)
{
    return __builtin_ffs(word) - 1;
}

#endif

tlsf_decl int tlsf_fls(unsigned int word)
{
    const int bit = word ? 32 - __builtin_clz(word) : 0;
    return bit - 1;
}

#elif defined(_MSC_VER) && (_MSC_VER >= 1400) && (defined(_M_IX86) || defined(_M_X64))
/* Microsoft Visual C++ support on x86/X64 architectures. */

#include <intrin.h>

#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)

tlsf_decl int tlsf_fls(unsigned int word)
{
    unsigned long index;
    return _BitScanReverse(&index, word) ? index : -1;
}

tlsf_decl int tlsf_ffs(unsigned int word)
{
    unsigned long index;
    return _BitScanForward(&index, word) ? index : -1;
}

#elif defined(_MSC_VER) && defined(_M_PPC)
/* Microsoft Visual C++ support on PowerPC architectures. */

#include <ppcintrinsics.h>

tlsf_decl int tlsf_fls(unsigned int word)
{
    const int bit = 32 - _CountLeadingZeros(word);
    return bit - 1;
}

tlsf_decl int tlsf_ffs(unsigned int word)
{
    const unsigned int reverse = word & (~word + 1);
    const int bit = 32 - _CountLeadingZeros(reverse);
    return bit - 1;
}

#elif defined(__ARMCC_VERSION)
/* RealView Compilation Tools for ARM */

tlsf_decl int tlsf_ffs(unsigned int word)
{
    const unsigned int reverse = word & (~word + 1);
    const int bit = 32 - __clz(reverse);
    return bit - 1;
}

tlsf_decl int tlsf_fls(unsigned int word)
{
    const int bit = word ? 32 - __clz(word) : 0;
    return bit - 1;
}

#elif defined(__ghs__)
/* Green Hills support for PowerPC */

#include <ppc_ghs.h>

tlsf_decl int tlsf_ffs(unsigned int word)
{
    const unsigned int reverse = word & (~word + 1);
    const int bit = 32 - __CLZ32(reverse);
    return bit - 1;
}

tlsf_decl int tlsf_fls(unsigned int word)
{
    const int bit = word ? 32 - __CLZ32(word) : 0;
    return bit - 1;
}

#else
/* Fall back to generic implementation. */

tlsf_decl int tlsf_fls_generic(unsigned int word)
{
    int bit = 32;

    if (!word) bit -= 1;
    if (!(word & 0xffff0000)) {
        word <<= 16;
        bit -= 16;
    }
    if (!(word & 0xff000000)) {
        word <<= 8;
        bit -= 8;
    }
    if (!(word & 0xf0000000)) {
        word <<= 4;
        bit -= 4;
    }
    if (!(word & 0xc0000000)) {
        word <<= 2;
        bit -= 2;
    }
    if (!(word & 0x80000000)) {
        word <<= 1;
        bit -= 1;
    }

    return bit;
}

/* Implement ffs in terms of fls. */
tlsf_decl int tlsf_ffs(unsigned int word)
{
    return tlsf_fls_generic(word & (~word + 1)) - 1;
}

tlsf_decl int tlsf_fls(unsigned int word)
{
    return tlsf_fls_generic(word) - 1;
}

#endif

/* 可能的64位版本的tlsf_fls */
#if defined(TLSF_64BIT)
// 64位环境下的前导零计算函数
tlsf_decl int tlsf_fls_sizet(size_t size)
{
    int high = (int)(size >> 32);  // 取高32位
    int bits = 0;
    if (high) {
        bits = 32 + tlsf_fls(high);   // 高位有值，加上32位偏移
    } else {
        bits = tlsf_fls((int)size & 0xffffffff);  // 只处理低32位
    }
    return bits;
}
#else
#define tlsf_fls_sizet tlsf_fls  // 32位环境直接使用基础版本
#endif

#undef tlsf_decl

/*
** 常量定义
*/

/* 公共常量：可以修改 */
enum tlsf_public
{
    /* 块大小线性细分数量的log2值。较大的值需要在控制结构中占用更多内存。
	** 典型值为4或5。
	*/
    SL_INDEX_COUNT_LOG2 = 5,
};

/* 私有常量：请勿修改 */
enum tlsf_private
{
#if defined(TLSF_64BIT)
    /* 64位环境：所有分配大小和地址按8字节对齐 */
    ALIGN_SIZE_LOG2 = 3,
#else
    /* 32位环境：所有分配大小和地址按4字节对齐 */
    ALIGN_SIZE_LOG2 = 2,
#endif
    ALIGN_SIZE = (1 << ALIGN_SIZE_LOG2),  // 对齐大小

/*
	** 我们支持分配大小最大到(1 << FL_INDEX_MAX)位。
	** 然而，由于我们线性细分第二级列表，而我们的最小大小粒度是4字节，
	** 对于小于SL_INDEX_COUNT * 4的大小创建第一级列表是没有意义的，
	** or (1 << (SL_INDEX_COUNT_LOG2 + 2)) bytes, as there we will be
	** trying to split size ranges into more slots than we have available.
	** Instead, we calculate the minimum threshold size, and place all
	** blocks below that size into the 0th first-level list.
	*/

#if defined(TLSF_64BIT)
    /*
	** TODO: We can increase this to support larger sizes, at the expense
	** of more overhead in the TLSF structure.
	*/
    FL_INDEX_MAX = 32,
#else
    FL_INDEX_MAX = 30,
#endif
    SL_INDEX_COUNT = (1 << SL_INDEX_COUNT_LOG2),
    FL_INDEX_SHIFT = (SL_INDEX_COUNT_LOG2 + ALIGN_SIZE_LOG2),
    FL_INDEX_COUNT = (FL_INDEX_MAX - FL_INDEX_SHIFT + 1),

    SMALL_BLOCK_SIZE = (1 << FL_INDEX_SHIFT),
};

/*
** Cast and min/max macros.
*/

#define tlsf_cast(t, exp) ((t)(exp))
#define tlsf_min(a, b) ((a) < (b) ? (a) : (b))
#define tlsf_max(a, b) ((a) > (b) ? (a) : (b))

/*
** Set assert macro, if it has not been provided by the user.
*/
#if !defined(tlsf_assert)
#define tlsf_assert assert
#endif

/*
** Static assertion mechanism.
*/

#define _tlsf_glue2(x, y) x##y
#define _tlsf_glue(x, y) _tlsf_glue2(x, y)
#define tlsf_static_assert(exp) typedef char _tlsf_glue(static_assert, __LINE__)[(exp) ? 1 : -1]

/* This code has been tested on 32- and 64-bit (LP/LLP) architectures. */
tlsf_static_assert(sizeof(int) * CHAR_BIT == 32);
tlsf_static_assert(sizeof(size_t) * CHAR_BIT >= 32);
tlsf_static_assert(sizeof(size_t) * CHAR_BIT <= 64);

/* SL_INDEX_COUNT must be <= number of bits in sl_bitmap's storage type. */
tlsf_static_assert(sizeof(unsigned int) * CHAR_BIT >= SL_INDEX_COUNT);

/* Ensure we've properly tuned our sizes. */
tlsf_static_assert(ALIGN_SIZE == SMALL_BLOCK_SIZE / SL_INDEX_COUNT);

/*
** Data structures and associated constants.
*/

/*
** Block header structure.
**
** There are several implementation subtleties involved:
** - The prev_phys_block field is only valid if the previous block is free.
** - The prev_phys_block field is actually stored at the end of the
**   previous block. It appears at the beginning of this structure only to
**   simplify the implementation.
** - The next_free / prev_free fields are only valid if the block is free.
*/
// 块头结构 - TLSF分配器的核心数据结构
typedef struct block_header_t
{
    /* 指向前一个物理块 */
    struct block_header_t *prev_phys_block;

    /* 此块的大小，不包括块头 */
    size_t size;

    /* 下一个和前一个空闲块 */
    struct block_header_t *next_free;
    struct block_header_t *prev_free;
} block_header_t;

/*
** 由于块大小总是至少为4的倍数，size字段的两个最低有效位用于存储块状态：
** - 位0：块是否忙碌或空闲
** - 位1：前一个块是否忙碌或空闲
*/
static const size_t block_header_free_bit = 1 << 0;      // 空闲块标记位
static const size_t block_header_prev_free_bit = 1 << 1; // 前一块空闲标记位

/*
** 暴露给已使用块的块头大小是size字段的大小。
** prev_phys_block字段存储在前一个空闲块*内部*。
*/
static const size_t block_header_overhead = sizeof(size_t);

/* 已使用块中的用户数据直接从size字段后开始 */
static const size_t block_start_offset = offsetof(block_header_t, size) + sizeof(size_t);

/*
** 空闲块必须足够大以存储其头部减去prev_phys_block字段的大小，
** 且不能大于FL_INDEX可寻址位数。
*/
static const size_t block_size_min = sizeof(block_header_t) - sizeof(block_header_t *);
static const size_t block_size_max = tlsf_cast(size_t, 1) << FL_INDEX_MAX;

/* TLSF控制结构 - 管理所有空闲链表的核心结构 */
typedef struct control_t
{
    /* 空链表指向此块以表示它们是空闲的 */
    block_header_t block_null;

    /* 空闲链表的位图 */
    unsigned int fl_bitmap;                             // 第一级位图
    unsigned int sl_bitmap[FL_INDEX_COUNT];             // 第二级位图数组

    /* 空闲链表的头部 */
    block_header_t *blocks[FL_INDEX_COUNT][SL_INDEX_COUNT]; // 二维空闲链表数组
} control_t;

/* 用于指针算术时的类型转换 */
typedef ptrdiff_t tlsfptr_t;

/*
** block_header_t成员函数
*/

// 获取块大小（清除状态位）
static size_t block_size(const block_header_t *block)
{
    return block->size & ~(block_header_free_bit | block_header_prev_free_bit);
}

static void block_set_size(block_header_t *block, size_t size)
{
    const size_t oldsize = block->size;
    block->size = size | (oldsize & (block_header_free_bit | block_header_prev_free_bit));
}

static int block_is_last(const block_header_t *block)
{
    return block_size(block) == 0;
}

static int block_is_free(const block_header_t *block)
{
    return tlsf_cast(int, block->size &block_header_free_bit);
}

static void block_set_free(block_header_t *block)
{
    block->size |= block_header_free_bit;
}

static void block_set_used(block_header_t *block)
{
    block->size &= ~block_header_free_bit;
}

static int block_is_prev_free(const block_header_t *block)
{
    return tlsf_cast(int, block->size &block_header_prev_free_bit);
}

static void block_set_prev_free(block_header_t *block)
{
    block->size |= block_header_prev_free_bit;
}

static void block_set_prev_used(block_header_t *block)
{
    block->size &= ~block_header_prev_free_bit;
}

static block_header_t *block_from_ptr(const void *ptr)
{
    return tlsf_cast(block_header_t *, tlsf_cast(unsigned char *, ptr) - block_start_offset);
}

static void *block_to_ptr(const block_header_t *block)
{
    return tlsf_cast(void *, tlsf_cast(unsigned char *, block) + block_start_offset);
}

/* Return location of next block after block of given size. */
static block_header_t *offset_to_block(const void *ptr, size_t size)
{
    return tlsf_cast(block_header_t *, tlsf_cast(tlsfptr_t, ptr) + size);
}

/* Return location of previous block. */
static block_header_t *block_prev(const block_header_t *block)
{
    tlsf_assert(block_is_prev_free(block) && "previous block must be free");
    return block->prev_phys_block;
}

/* Return location of next existing block. */
static block_header_t *block_next(const block_header_t *block)
{
    block_header_t *next = offset_to_block(block_to_ptr(block), block_size(block) - block_header_overhead);
    tlsf_assert(!block_is_last(block));
    return next;
}

/* Link a new block with its physical neighbor, return the neighbor. */
static block_header_t *block_link_next(block_header_t *block)
{
    block_header_t *next = block_next(block);
    next->prev_phys_block = block;
    return next;
}

static void block_mark_as_free(block_header_t *block)
{
    /* Link the block to the next block, first. */
    block_header_t *next = block_link_next(block);
    block_set_prev_free(next);
    block_set_free(block);
}

static void block_mark_as_used(block_header_t *block)
{
    block_header_t *next = block_next(block);
    block_set_prev_used(next);
    block_set_used(block);
}

static size_t align_up(size_t x, size_t align)
{
    tlsf_assert(0 == (align & (align - 1)) && "must align to a power of two");
    return (x + (align - 1)) & ~(align - 1);
}

static size_t align_down(size_t x, size_t align)
{
    tlsf_assert(0 == (align & (align - 1)) && "must align to a power of two");
    return x - (x & (align - 1));
}

static void *align_ptr(const void *ptr, size_t align)
{
    const tlsfptr_t aligned = (tlsf_cast(tlsfptr_t, ptr) + (align - 1)) & ~(align - 1);
    tlsf_assert(0 == (align & (align - 1)) && "must align to a power of two");
    return tlsf_cast(void *, aligned);
}

/*
** 调整分配大小使其对齐到字大小，且不小于内部最小值
*/
static size_t adjust_request_size(size_t size, size_t align)
{
    size_t adjust = 0;
    if (size) {
        const size_t aligned = align_up(size, align);

        /* 对齐大小不能超过最大块大小，否则会超出sl_bitmap范围 */
        if (aligned < block_size_max) {
            adjust = tlsf_max(aligned, block_size_min);
        }
    }
    return adjust;
}

/*
** TLSF工具函数。大多数情况下，这些是白皮书文档的直接翻译实现。
*/

// 映射块大小到分组索引 - 将size映射到对应的fl(第一级)和sl(第二级)索引
static void mapping_insert(size_t size, int *fli, int *sli)
{
    int fl, sl;
    if (size < SMALL_BLOCK_SIZE) {
        /* 将小块存储在第一个列表中 */
        fl = 0;
        sl = tlsf_cast(int, size) / (SMALL_BLOCK_SIZE / SL_INDEX_COUNT);
    } else {
        fl = tlsf_fls_sizet(size);  // 查找最高位的位置
        sl = tlsf_cast(int, size >> (fl - SL_INDEX_COUNT_LOG2)) ^ (1 << SL_INDEX_COUNT_LOG2);
        fl -= (FL_INDEX_SHIFT - 1);
    }
    *fli = fl;
    *sli = sl;
}

/* 此版本向上舍入到下一个块大小（用于分配） */
static void mapping_search(size_t size, int *fli, int *sli)
{
    if (size >= SMALL_BLOCK_SIZE) {
        // 向上舍入到下一个大小类别的边界
        const size_t round = (1 << (tlsf_fls_sizet(size) - SL_INDEX_COUNT_LOG2)) - 1;
        size += round;
    }
    mapping_insert(size, fli, sli);  // 调用实际的映射函数
}

// 搜索合适的空闲块 - 基于给定的fl/sl索引查找可用的内存块
static block_header_t *search_suitable_block(control_t *control, int *fli, int *sli)
{
    int fl = *fli;
    int sl = *sli;

    /*
	** 首先，在与给定fl/sl索引关联的列表中搜索块
	*/
    unsigned int sl_map = control->sl_bitmap[fl] & (~0U << sl);  // 获取二级位图中可用的块
    if (!sl_map) {
        /* 不存在合适的块。在下一个更大的一级列表中搜索 */
        const unsigned int fl_map = control->fl_bitmap & (~0U << (fl + 1));
        if (!fl_map) {
            /* 没有可用的空闲块，内存已耗尽 */
            return 0;
        }

        fl = tlsf_ffs(fl_map);  // 找到第一个可用的一级索引
        *fli = fl;
        sl_map = control->sl_bitmap[fl];  // 获取对应的二级位图
    }
    tlsf_assert(sl_map && "internal error - second level bitmap is null");
    sl = tlsf_ffs(sl_map);  // 找到第一个可用的二级索引
    *sli = sl;

    /* 返回空闲列表中的第一个块 */
    return control->blocks[fl][sl];
}

/* 从空闲列表中移除一个空闲块 */
static void remove_free_block(control_t *control, block_header_t *block, int fl, int sl)
{
    block_header_t *prev = block->prev_free;  // 前一个空闲块
    block_header_t *next = block->next_free;  // 后一个空闲块
    tlsf_assert(prev && "prev_free field can not be null");
    tlsf_assert(next && "next_free field can not be null");
    next->prev_free = prev;  // 更新后续块的前指针
    prev->next_free = next;  // 更新前置块的后指针

    /* 如果此块是空闲列表的头部，设置新的头部 */
    if (control->blocks[fl][sl] == block) {
        control->blocks[fl][sl] = next;

        /* 如果新的头部为空，清除位图标记 */
        if (next == &control->block_null) {
            control->sl_bitmap[fl] &= ~(1U << sl);  // 清除二级位图中的标记

            /* 如果二级位图现在为空，清除一级位图标记 */
            if (!control->sl_bitmap[fl]) {
                control->fl_bitmap &= ~(1U << fl);
            }
        }
    }
}

/* 将空闲块插入到空闲块列表中 */
static void insert_free_block(control_t *control, block_header_t *block, int fl, int sl)
{
    block_header_t *current = control->blocks[fl][sl];
    tlsf_assert(current && "free list cannot have a null entry");
    tlsf_assert(block && "cannot insert a null entry into the free list");
    block->next_free = current;  // 新块指向当前头部
    block->prev_free = &control->block_null;  // 新块的前指针指向空节点
    current->prev_free = block;  // 当前头部的前指针指向新块

    tlsf_assert(block_to_ptr(block) == align_ptr(block_to_ptr(block), ALIGN_SIZE) && "block not aligned properly");
    /*
	** 将新块插入到列表头部，并适当标记一级和二级位图
	*/
    control->blocks[fl][sl] = block;  // 更新列表头部指针
    control->fl_bitmap |= (1U << fl);  // 设置一级位图标记
    control->sl_bitmap[fl] |= (1U << sl);  // 设置二级位图标记
}

/* 从空闲列表中移除给定块 */
static void block_remove(control_t *control, block_header_t *block)
{
    int fl, sl;
    mapping_insert(block_size(block), &fl, &sl);  // 获取块的分组索引
    remove_free_block(control, block, fl, sl);  // 从对应列表中移除
}

/* 将给定块插入到空闲列表中 */
static void block_insert(control_t *control, block_header_t *block)
{
    int fl, sl;
    mapping_insert(block_size(block), &fl, &sl);  // 获取块的分组索引
    insert_free_block(control, block, fl, sl);  // 插入到对应列表中
}

// 检查块是否可以分割成指定大小
static int block_can_split(block_header_t *block, size_t size)
{
    return block_size(block) >= sizeof(block_header_t) + size;  // 剩余空间足够容纳新的块头
}

/* 将块分割为两个，第二个块为空闲 */
static block_header_t *block_split(block_header_t *block, size_t size)
{
    /* 计算剩余块中的剩余空间大小 */
    block_header_t *remaining = offset_to_block(block_to_ptr(block), size - block_header_overhead);

    const size_t remain_size = block_size(block) - (size + block_header_overhead);

    tlsf_assert(block_to_ptr(remaining) == align_ptr(block_to_ptr(remaining), ALIGN_SIZE) &&
                "remaining block not aligned properly");

    tlsf_assert(block_size(remaining) == remain_size + size + block_header_overhead);
    block_set_size(remaining, remain_size);  // 设置剩余块的大小
    tlsf_assert(block_size(remaining) >= block_size_min && "block split with invalid size");

    block_set_size(block, size);  // 设置原块的新大小
    block_mark_as_free(remaining);  // 将剩余块标记为空闲

    return remaining;  // 返回剩余的空闲块
}

/* 将空闲块的存储空间合并到相邻的前一个空闲块中 */
static block_header_t *block_absorb(block_header_t *prev, block_header_t *block)
{
    tlsf_assert(!block_is_last(prev) && "previous block can't be last");
    /* 注意：保持标志位不变 */
    prev->size += block_size(block) + block_header_overhead;  // 合并大小
    block_link_next(prev);  // 重新链接下一个块
    return prev;  // 返回合并后的前块
}

/* 将刚释放的块与相邻的前一个空闲块合并 */
static block_header_t *block_merge_prev(control_t *control, block_header_t *block)
{
    if (block_is_prev_free(block)) {  // 如果前一个块是空闲的
        block_header_t *prev = block_prev(block);  // 获取前一个块
        tlsf_assert(prev && "prev physical block can't be null");
        tlsf_assert(block_is_free(prev) && "prev block is not free though marked as such");
        block_remove(control, prev);  // 从空闲列表中移除前块
        block = block_absorb(prev, block);  // 将当前块合并到前块
    }

    return block;  // 返回合并后的块
}

/* 将刚释放的块与相邻的空闲块合并 */
static block_header_t *block_merge_next(control_t *control, block_header_t *block)
{
    block_header_t *next = block_next(block);  // 获取下一个块
    tlsf_assert(next && "next physical block can't be null");

    if (block_is_free(next)) {  // 如果下一个块是空闲的
        tlsf_assert(!block_is_last(block) && "previous block can't be last");
        block_remove(control, next);  // 从空闲列表中移除下一个块
        block = block_absorb(block, next);  // 将下一个块合并到当前块
    }

    return block;  // 返回合并后的块
}

/* 修剪块末尾的空余空间，返回给内存池 */
static void block_trim_free(control_t *control, block_header_t *block, size_t size)
{
    tlsf_assert(block_is_free(block) && "block must be free");
    if (block_can_split(block, size)) {  // 如果可以分割
        block_header_t *remaining_block = block_split(block, size);  // 分割块
        block_link_next(block);  // 重新链接
        block_set_prev_free(remaining_block);  // 设置剩余块的前置状态
        block_insert(control, remaining_block);  // 将剩余块插入空闲列表
    }
}

/* 修剪已使用块末尾的空余空间，返回给内存池 */
static void block_trim_used(control_t *control, block_header_t *block, size_t size)
{
    tlsf_assert(!block_is_free(block) && "block must be used");
    if (block_can_split(block, size)) {  // 如果可以分割
        /* 如果下一个块是空闲的，我们必须合并 */
        block_header_t *remaining_block = block_split(block, size);  // 分割块
        block_set_prev_used(remaining_block);  // 设置剩余块的前置状态为已使用

        remaining_block = block_merge_next(control, remaining_block);  // 与下一个块合并
        block_insert(control, remaining_block);  // 将合并后的块插入空闲列表
    }
}

// 修剪空闲块的头部，返回尾部块
static block_header_t *block_trim_free_leading(control_t *control, block_header_t *block, size_t size)
{
    block_header_t *remaining_block = block;
    if (block_can_split(block, size)) {  // 如果可以分割
        /* 我们需要第2个块 */
        remaining_block = block_split(block, size - block_header_overhead);  // 分割获取第二个块
        block_set_prev_free(remaining_block);  // 设置剩余块的前置状态

        block_link_next(block);  // 重新链接第一个块
        block_insert(control, block);  // 将第一个块插入空闲列表
    }

    return remaining_block;  // 返回剩余块（第二个块）
}

// 定位合适的空闲块 - 根据请求大小查找可用的空闲内存块
static block_header_t *block_locate_free(control_t *control, size_t size)
{
    int fl = 0, sl = 0;  // 一级和二级索引
    block_header_t *block = 0;

    if (size) {
        mapping_search(size, &fl, &sl);  // 查找合适的大小分组

        /*
		** mapping_search可能会调整size的值，所以对于过大的size值，
		** 有时可能会产生超出block数组范围的索引。
		** 因此，我们在这里进行保护，因为这是mapping_search的唯一调用点。
		** 注意我们不需要检查sl，因为它来自模运算，保证总是在范围内。
		*/
        if (fl < FL_INDEX_COUNT) {
            block = search_suitable_block(control, &fl, &sl);  // 搜索合适的块
        }
    }

    if (block) {
        tlsf_assert(block_size(block) >= size);
        remove_free_block(control, block, fl, sl);  // 从空闲列表中移除找到的块
    }

    return block;  // 返回找到的块或NULL
}

// 准备使用的块 - 将空闲块准备为可使用状态
static void *block_prepare_used(control_t *control, block_header_t *block, size_t size)
{
    void *p = 0;
    if (block) {
        tlsf_assert(size && "size must be non-zero");
        block_trim_free(control, block, size);  // 修剪块到合适大小
        block_mark_as_used(block);  // 标记块为已使用
        p = block_to_ptr(block);  // 获取用户可用的指针
    }
    return p;  // 返回用户指针
}

/* 清空结构并将所有空列表指向空块 */
static void control_construct(control_t *control)
{
    int i, j;

    // 初始化空块节点为自指向的循环链表
    control->block_null.next_free = &control->block_null;
    control->block_null.prev_free = &control->block_null;

    // 清空所有位图和列表
    control->fl_bitmap = 0;  // 清空一级位图
    for (i = 0; i < FL_INDEX_COUNT; ++i) {
        control->sl_bitmap[i] = 0;  // 清空二级位图
        for (j = 0; j < SL_INDEX_COUNT; ++j) {
            control->blocks[i][j] = &control->block_null;  // 所有列表指向空块
        }
    }
}

// 完整性检查结构
typedef struct integrity_t
{
    int prev_status;  // 前一个块的状态
    int status;       // 当前状态
} integrity_t;

#define tlsf_insist(x)  \
    {                   \
        tlsf_assert(x); \
        if (!(x)) {     \
            status--;   \
        }               \
    }

// 完整性检查的遍历回调函数
static void integrity_walker(void *ptr, size_t size, int used, void *user)
{
    block_header_t *block = block_from_ptr(ptr);  // 从用户指针获取块头
    integrity_t *integ = tlsf_cast(integrity_t *, user);
    const int this_prev_status = block_is_prev_free(block) ? 1 : 0;  // 前置块是否空闲
    const int this_status = block_is_free(block) ? 1 : 0;            // 当前块是否空闲
    const size_t this_block_size = block_size(block);                // 当前块大小

    int status = 0;
    (void)used;  // 抑制未使用变量警告
    
    // 检查前置状态一致性和块大小正确性
    tlsf_insist(integ->prev_status == this_prev_status && "prev status incorrect");
    tlsf_insist(size == this_block_size && "block size incorrect");

    integ->prev_status = this_status;  // 更新前置状态
    integ->status += status;           // 累计状态
}

// 检查TLSF分配器的完整性
int tlsf_check(tlsf_t tlsf)
{
    int i, j;

    control_t *control = tlsf_cast(control_t *, tlsf);
    int status = 0;

    /* 检查空闲列表和位图的准确性 */
    for (i = 0; i < FL_INDEX_COUNT; ++i) {
        for (j = 0; j < SL_INDEX_COUNT; ++j) {
            const int fl_map = control->fl_bitmap & (1U << i);       // 一级位图标记
            const int sl_list = control->sl_bitmap[i];               // 二级位图
            const int sl_map = sl_list & (1U << j);                  // 二级位图标记
            const block_header_t *block = control->blocks[i][j];     // 对应的块列表

            /* 检查一级和二级列表的一致性 */
            if (!fl_map) {
                tlsf_insist(!sl_map && "second-level map must be null");
            }

            if (!sl_map) {
                tlsf_insist(block == &control->block_null && "block list must be null");
                continue;
            }

            /* 检查至少有一个空闲块 */
            tlsf_insist(sl_list && "no free blocks in second-level map");
            tlsf_insist(block != &control->block_null && "block should not be null");

            // 遍历空闲列表中的所有块
            while (block != &control->block_null) {
                int fli, sli;
                tlsf_insist(block_is_free(block) && "block should be free");
                tlsf_insist(!block_is_prev_free(block) && "blocks should have coalesced");
                tlsf_insist(!block_is_free(block_next(block)) && "blocks should have coalesced");
                tlsf_insist(block_is_prev_free(block_next(block)) && "block should be free");
                tlsf_insist(block_size(block) >= block_size_min && "block not minimum size");

                // 检查块是否在正确的索引列表中
                mapping_insert(block_size(block), &fli, &sli);
                tlsf_insist(fli == i && sli == j && "block size indexed in wrong list");
                block = block->next_free;  // 移动到下一个空闲块
            }
        }
    }

    return status;  // 返回检查结果
}

#undef tlsf_insist

// 默认的遍历回调函数 - 打印块信息
static void default_walker(void *ptr, size_t size, int used, void *user)
{
    (void)user;  // 抑制未使用变量警告
    printf("\t%p %s size: %x (%p)\n", ptr, used ? "used" : "free", (unsigned int)size, block_from_ptr(ptr));
}

// 遍历内存池中的所有块
void tlsf_walk_pool(pool_t pool, tlsf_walker walker, void *user)
{
    tlsf_walker pool_walker = walker ? walker : default_walker;  // 使用提供的或默认的遍历函数
    block_header_t *block = offset_to_block(pool, -(int)block_header_overhead);  // 获取第一个块

    // 遍历池中的每个块
    while (block && !block_is_last(block)) {
        pool_walker(block_to_ptr(block), block_size(block), !block_is_free(block), user);
        block = block_next(block);  // 移动到下一个块
    }
}

// 获取指定指针对应块的大小
size_t tlsf_block_size(void *ptr)
{
    size_t size = 0;
    if (ptr) {
        const block_header_t *block = block_from_ptr(ptr);  // 从用户指针获取块头
        size = block_size(block);  // 获取块的实际大小
    }
    return size;  // 返回块大小，如果指针为空则返回0
}

// 检查内存池的完整性
int tlsf_check_pool(pool_t pool)
{
    /* 检查块的物理结构是否正确 */
    integrity_t integ = { 0, 0 };  // 初始化完整性检查结构
    tlsf_walk_pool(pool, integrity_walker, &integ);  // 遍历池中所有块进行检查

    return integ.status;  // 返回检查状态
}

/*
** 传递给tlsf_create的内存块中TLSF结构的大小，
** 等于control_t结构的大小
*/
size_t tlsf_size(void)
{
    return sizeof(control_t);  // 返回控制结构的大小
}

// 获取TLSF的对齐大小要求
size_t tlsf_align_size(void)
{
    return ALIGN_SIZE;  // 返回对齐边界大小
}

// 获取最小块大小
size_t tlsf_block_size_min(void)
{
    return block_size_min;  // 返回最小可分配块大小
}

// 获取最大块大小
size_t tlsf_block_size_max(void)
{
    return block_size_max;  // 返回最大可分配块大小
}

/*
** 传递给tlsf_add_pool的内存块中TLSF结构的开销，
** 等于一个空闲块和哨兵块的开销之和
*/
size_t tlsf_pool_overhead(void)
{
    return 2 * block_header_overhead;  // 返回内存池管理开销
}

// 获取每个分配块的头部开销
size_t tlsf_alloc_overhead(void)
{
    return block_header_overhead;  // 返回块头开销大小
}

// 向TLSF分配器添加内存池
pool_t tlsf_add_pool(tlsf_t tlsf, void *mem, size_t bytes)
{
    block_header_t *block;
    block_header_t *next;

    const size_t pool_overhead = tlsf_pool_overhead();  // 获取池管理开销
    const size_t pool_bytes = align_down(bytes - pool_overhead, ALIGN_SIZE);  // 计算可用池大小

    // 检查内存对齐
    if (((ptrdiff_t)mem % ALIGN_SIZE) != 0) {
        printf("tlsf_add_pool: Memory must be aligned by %u bytes.\n", (unsigned int)ALIGN_SIZE);
        return 0;
    }

    // 检查池大小是否在有效范围内
    if (pool_bytes < block_size_min || pool_bytes > block_size_max) {
#if defined(TLSF_64BIT)
        printf("tlsf_add_pool: Memory size must be between 0x%x and 0x%x00 bytes.\n",
               (unsigned int)(pool_overhead + block_size_min), (unsigned int)((pool_overhead + block_size_max) / 256));
#else
        printf("tlsf_add_pool: Memory size must be between %u and %u bytes.\n",
               (unsigned int)(pool_overhead + block_size_min), (unsigned int)(pool_overhead + block_size_max));
#endif
        return 0;
    }

    /*
	** 创建主空闲块。稍微偏移块的起始位置，
	** 使prev_phys_block字段位于池外部 - 
	** 它永远不会被使用
	*/
    block = offset_to_block(mem, -(tlsfptr_t)block_header_overhead);  // 计算块的实际位置
    block_set_size(block, pool_bytes);  // 设置块大小
    block_set_free(block);              // 标记为空闲
    block_set_prev_used(block);         // 设置前置块为已使用状态
    block_insert(tlsf_cast(control_t *, tlsf), block);  // 将块插入空闲列表

    /* 分割块以创建大小为零的哨兵块 */
    next = block_link_next(block);  // 获取下一个块位置
    block_set_size(next, 0);        // 设置哨兵块大小为0
    block_set_used(next);           // 标记哨兵块为已使用
    block_set_prev_free(next);      // 设置前置块为空闲状态

    return mem;  // 返回内存池指针
}

// 从TLSF分配器中移除内存池
void tlsf_remove_pool(tlsf_t tlsf, pool_t pool)
{
    control_t *control = tlsf_cast(control_t *, tlsf);
    block_header_t *block = offset_to_block(pool, -(int)block_header_overhead);  // 获取池的主块

    int fl = 0, sl = 0;  // 分组索引

    // 验证块状态的正确性
    tlsf_assert(block_is_free(block) && "block should be free");
    tlsf_assert(!block_is_free(block_next(block)) && "next block should not be free");
    tlsf_assert(block_size(block_next(block)) == 0 && "next block size should be zero");

    mapping_insert(block_size(block), &fl, &sl);  // 获取块的分组索引
    remove_free_block(control, block, fl, sl);    // 从空闲列表中移除块
}

/*
** TLSF主接口
*/

#if TLSF_DEBUG
// 测试ffs/fls函数的正确性
int test_ffs_fls()
{
    /* 验证ffs/fls函数工作正常 */
    int rv = 0;  // 返回值，0表示所有测试通过
    rv += (tlsf_ffs(0) == -1) ? 0 : 0x1;           // 测试ffs(0)应返回-1
    rv += (tlsf_fls(0) == -1) ? 0 : 0x2;           // 测试fls(0)应返回-1  
    rv += (tlsf_ffs(1) == 0) ? 0 : 0x4;            // 测试ffs(1)应返回0
    rv += (tlsf_fls(1) == 0) ? 0 : 0x8;            // 测试fls(1)应返回0
    rv += (tlsf_ffs(0x80000000) == 31) ? 0 : 0x10; // 测试最高位
    rv += (tlsf_ffs(0x80008000) == 15) ? 0 : 0x20; // 测试中间位
    rv += (tlsf_fls(0x80000008) == 31) ? 0 : 0x40; // 测试最高位
    rv += (tlsf_fls(0x7FFFFFFF) == 30) ? 0 : 0x80; // 测试最高位-1

#if defined(TLSF_64BIT)
    rv += (tlsf_fls_sizet(0x80000000) == 31) ? 0 : 0x100;      // 64位测试
    rv += (tlsf_fls_sizet(0x100000000) == 32) ? 0 : 0x200;     // 64位测试
    rv += (tlsf_fls_sizet(0xffffffffffffffff) == 63) ? 0 : 0x400; // 64位最大值测试
#endif

    if (rv) {
        printf("test_ffs_fls: %x ffs/fls tests failed.\n", rv);  // 报告测试失败
    }
    return rv;  // 返回测试结果
}
#endif

// 创建TLSF分配器实例
tlsf_t tlsf_create(void *mem)
{
#if TLSF_DEBUG
    // 调试模式下测试位操作函数
    if (test_ffs_fls()) {
        return 0;  // 测试失败则返回NULL
    }
#endif

    // 检查内存对齐
    if (((tlsfptr_t)mem % ALIGN_SIZE) != 0) {
        printf("tlsf_create: Memory must be aligned to %u bytes.\n", (unsigned int)ALIGN_SIZE);
        return 0;
    }

    control_construct(tlsf_cast(control_t *, mem));  // 初始化控制结构

    return tlsf_cast(tlsf_t, mem);  // 返回TLSF实例
}

// 创建带内存池的TLSF分配器实例
tlsf_t tlsf_create_with_pool(void *mem, size_t bytes)
{
    tlsf_t tlsf = tlsf_create(mem);  // 创建TLSF实例
    tlsf_add_pool(tlsf, (char *)mem + tlsf_size(), bytes - tlsf_size());  // 添加内存池
    return tlsf;  // 返回实例
}

// 销毁TLSF分配器实例
void tlsf_destroy(tlsf_t tlsf)
{
    /* 无需执行任何操作 */
    (void)tlsf;  // 抑制未使用参数警告
}

// 获取TLSF实例的默认内存池
pool_t tlsf_get_pool(tlsf_t tlsf)
{
    return tlsf_cast(pool_t, (char *)tlsf + tlsf_size());  // 返回池指针
}

// TLSF内存分配函数
void *tlsf_malloc(tlsf_t tlsf, size_t size)
{
    control_t *control = tlsf_cast(control_t *, tlsf);
    const size_t adjust = adjust_request_size(size, ALIGN_SIZE);  // 调整请求大小
    block_header_t *block = block_locate_free(control, adjust);   // 查找合适的空闲块
    return block_prepare_used(control, block, adjust);           // 准备并返回用户指针
}

// TLSF对齐内存分配函数
void *tlsf_memalign(tlsf_t tlsf, size_t align, size_t size)
{
    control_t *control = tlsf_cast(control_t *, tlsf);
    const size_t adjust = adjust_request_size(size, ALIGN_SIZE);  // 调整请求大小

    /*
	** 我们必须分配额外的最小块大小字节，这样如果
	** 我们的空闲块留下的对齐间隙太小，我们可以
	** 修剪前导空闲块并将其释放回池中。我们必须
	** 这样做，因为前一个物理块正在使用中，因此
	** prev_phys_block字段无效，我们不能简单地调整
	** 该块的大小。
	*/
    const size_t gap_minimum = sizeof(block_header_t);  // 最小间隙大小
    const size_t size_with_gap = adjust_request_size(adjust + align + gap_minimum, align);  // 包含间隙的大小

    /*
	** 如果对齐小于或等于基础对齐，我们就完成了。
	** 如果我们请求0字节，返回null，就像tlsf_malloc(0)一样。
	*/
    const size_t aligned_size = (adjust && align > ALIGN_SIZE) ? size_with_gap : adjust;

    block_header_t *block = block_locate_free(control, aligned_size);  // 查找合适大小的块

    /* 这不能是静态断言 */
    tlsf_assert(sizeof(block_header_t) == block_size_min + block_header_overhead);

    if (block) {
        void *ptr = block_to_ptr(block);           // 获取用户指针
        void *aligned = align_ptr(ptr, align);     // 计算对齐后的指针
        size_t gap = tlsf_cast(size_t, tlsf_cast(tlsfptr_t, aligned) - tlsf_cast(tlsfptr_t, ptr));  // 计算间隙

        /* 如果间隙太小，偏移到下一个对齐边界 */
        if (gap && gap < gap_minimum) {
            const size_t gap_remain = gap_minimum - gap;  // 剩余间隙
            const size_t offset = tlsf_max(gap_remain, align);  // 计算偏移量
            const void *next_aligned = tlsf_cast(void *, tlsf_cast(tlsfptr_t, aligned) + offset);

            aligned = align_ptr(next_aligned, align);  // 重新对齐
            gap = tlsf_cast(size_t, tlsf_cast(tlsfptr_t, aligned) - tlsf_cast(tlsfptr_t, ptr));  // 重新计算间隙
        }

        if (gap) {
            tlsf_assert(gap >= gap_minimum && "gap size too small");
            block = block_trim_free_leading(control, block, gap);  // 修剪前导部分
        }
    }

    return block_prepare_used(control, block, adjust);  // 准备并返回对齐的指针
}

// TLSF内存释放函数
void tlsf_free(tlsf_t tlsf, void *ptr)
{
    /* 不要尝试释放NULL指针 */
    if (ptr) {
        control_t *control = tlsf_cast(control_t *, tlsf);
        block_header_t *block = block_from_ptr(ptr);  // 从用户指针获取块头
        tlsf_assert(!block_is_free(block) && "block already marked as free");
        block_mark_as_free(block);                    // 标记块为空闲
        block = block_merge_prev(control, block);     // 与前一个空闲块合并
        block = block_merge_next(control, block);     // 与后一个空闲块合并
        block_insert(control, block);                 // 将合并后的块插入空闲列表
    }
}

// 简单的内存复制函数实现
static void *memcpy(void *dst, const void *src, size_t n)
{
    const char *p = src;  // 源指针
    char *q = dst;        // 目标指针
    while (n--) {
        *q++ = *p++;  // 逐字节复制
    }
    return dst;  // 返回目标指针
}

/*
** TLSF块信息为我们提供了足够的信息来
** 提供合理智能的realloc实现，根据需要
** 增长或收缩当前分配的块。
**
** 此例程处理realloc的一些深奥边缘情况：
** - 带有null指针的非零大小将表现得像malloc
** - 带有非null指针的零大小将表现得像free
** - 无法满足的请求将保持原始缓冲区不变
** - 扩展的缓冲区大小将使新分配的区域内容未定义
*/
void *tlsf_realloc(tlsf_t tlsf, void *ptr, size_t size)
{
    control_t *control = tlsf_cast(control_t *, tlsf);
    void *p = 0;

    /* 零大小请求被视为free */
    if (ptr && size == 0) {
        tlsf_free(tlsf, ptr);  // 释放内存
    }
    /* NULL指针请求被视为malloc */
    else if (!ptr) {
        p = tlsf_malloc(tlsf, size);  // 分配新内存
    } else {
        block_header_t *block = block_from_ptr(ptr);  // 获取当前块
        block_header_t *next = block_next(block);     // 获取下一个块

        const size_t cursize = block_size(block);                               // 当前块大小
        const size_t combined = cursize + block_size(next) + block_header_overhead;  // 合并后大小
        const size_t adjust = adjust_request_size(size, ALIGN_SIZE);            // 调整请求大小

        tlsf_assert(!block_is_free(block) && "block already marked as free");

        /*
		** 如果下一个块正在使用，或者与当前块合并时，
		** 没有提供足够的空间，我们必须重新分配并复制。
		*/
        if (adjust > cursize && (!block_is_free(next) || adjust > combined)) {
            p = tlsf_malloc(tlsf, size);  // 分配新的更大块
            if (p) {
                const size_t minsize = tlsf_min(cursize, size);  // 计算需要复制的大小
                memcpy(p, ptr, minsize);  // 复制原有数据
                tlsf_free(tlsf, ptr);     // 释放原始块
            }
        } else {
            /* 我们需要扩展到下一个块吗？ */
            if (adjust > cursize) {
                block_merge_next(control, block);  // 合并下一个块
                block_mark_as_used(block);         // 标记为已使用
            }

            /* 修剪结果块并返回原始指针 */
            block_trim_used(control, block, adjust);  // 修剪到合适大小
            p = ptr;  // 返回原始指针
        }
    }

    return p;  // 返回结果指针
}