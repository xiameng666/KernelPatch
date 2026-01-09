/* ARM64指令编码解码和操作工具 */
/*
 * Copyright (C) 2013 Huawei Ltd.
 * Author: Jiang Liu <liuj97@gmail.com>
 *
 * Copyright (C) 2014-2016 Zi Shen Lim <zlim.lnx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Linux source: /arch/arm64/kernel/insn.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "insn.h"
#include "ptrace.h"
#include "fls_ffs.h"

// 调试宏：在检测到错误时输出信息并退出
#define BUG()                                                                           \
    do {                                                                                \
        fprintf(stdout, "BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        do {                                                                            \
        } while (0);                                                                    \
        exit(-EINVAL);                                                                  \
    } while (0)

// 条件断言宏：当条件为真时触发BUG
#define BUG_ON(condition)     \
    do {                      \
        if (condition) BUG(); \
    } while (0)

// 字节序转换宏（此处假设小端）
#define le32_to_cpu(x) (x)
#define cpu_to_le32(x) (x)

// 大小常量定义，用于内存对齐和计算
#define SZ_1 0x00000001
#define SZ_2 0x00000002
#define SZ_4 0x00000004
#define SZ_8 0x00000008
#define SZ_16 0x00000010
#define SZ_32 0x00000020
#define SZ_64 0x00000040
#define SZ_128 0x00000080
#define SZ_256 0x00000100
#define SZ_512 0x00000200

#define SZ_1K 0x00000400
#define SZ_2K 0x00000800
// 更大尺寸的常量定义
#define SZ_4K 0x00001000
#define SZ_8K 0x00002000
#define SZ_16K 0x00004000
#define SZ_32K 0x00008000
#define SZ_64K 0x00010000
#define SZ_128K 0x00020000
#define SZ_256K 0x00040000
#define SZ_512K 0x00080000

#define SZ_1M 0x00100000
#define SZ_2M 0x00200000
#define SZ_4M 0x00400000
#define SZ_8M 0x00800000
#define SZ_16M 0x01000000
#define SZ_32M 0x02000000
#define SZ_64M 0x04000000
#define SZ_128M 0x08000000
#define SZ_256M 0x10000000
#define SZ_512M 0x20000000

#define SZ_1G 0x40000000
#define SZ_2G 0x80000000

#define BITS_PER_LONG 64

/**
 * @brief 查找64位数中第一个置位bit的位置
 * @details 返回最低位1的位置，用于位操作
 * @param word 待查找的64位数
 * @return 第一个置位bit的位置
 */
static inline uint64_t __ffs64(u64 word)
{
    return __ffs((uint64_t)word);
}

// 提取64位数的高32位
#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
// 提取64位数的低32位
#define lower_32_bits(n) ((u32)(n))

/**
 * @brief 计算64位数中置位bit的数量
 * @details 使用位操作技巧快速计算1的个数
 * @param w 待计算的64位数
 * @return 置位bit的数量
 */
static inline uint64_t hweight64(u64 w)
{
    // 使用分治法计算汉明重量（1的个数）
    u64 res = w - ((w >> 1) & 0x5555555555555555ul);
    res = (res & 0x3333333333333333ul) + ((res >> 2) & 0x3333333333333333ul);
    res = (res + (res >> 4)) & 0x0F0F0F0F0F0F0F0Ful;
    res = res + (res >> 8);
    res = res + (res >> 16);
    return (res + (res >> 32)) & 0x00000000000000FFul;
}

/*
 * @brief 创建连续的位掩码
 * @details 从位置l开始到位置h结束创建位掩码
 *          例如GENMASK_ULL(39, 21)生成64位向量0x000000ffffe00000
 */
#ifndef _WIN32
// 在非Windows平台使用标准定义
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#else 
// Windows平台使用64位版本
#define GENMASK GENMASK_ULL
#define BITS_PER_LONG_LONG 64
#endif
// 64位位掩码生成宏
#define GENMASK_ULL(h, l) (((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

/*
 * #imm16 values used for BRK instruction generation
 * Allowed values for kgbd are 0x400 - 0x7ff
 * 0x100: for triggering a fault on purpose (reserved)
 * 0x400: for dynamic BRK instruction
 * 0x401: for compile time BRK instruction
 */
#define FAULT_BRK_IMM 0x100
#define KGDB_DYN_DBG_BRK_IMM 0x400
#define KGDB_COMPILED_DBG_BRK_IMM 0x401

/*
 * BRK instruction encoding
 * The #imm16 value should be placed at bits[20:5] within BRK ins
 */
#define AARCH64_BREAK_MON 0xd4200000

/*
 * BRK instruction for provoking a fault on purpose
 * Unlike kgdb, #imm16 value with unallocated handler is used for faulting.
 */
#define AARCH64_BREAK_FAULT (AARCH64_BREAK_MON | (FAULT_BRK_IMM << 5))

#define BIT(nr) (1ul << (nr))

#define AARCH64_INSN_SF_BIT BIT(31)
#define AARCH64_INSN_N_BIT BIT(22)
#define AARCH64_INSN_LSL_12 BIT(22)

// ARM64指令编码分类表 - 根据指令编码的高4位确定指令类别
static int aarch64_insn_encoding_class[] = {
    AARCH64_INSN_CLS_UNKNOWN, AARCH64_INSN_CLS_UNKNOWN, AARCH64_INSN_CLS_UNKNOWN, AARCH64_INSN_CLS_UNKNOWN,
    AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_REG,  AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_FPSIMD,
    AARCH64_INSN_CLS_DP_IMM,  AARCH64_INSN_CLS_DP_IMM,  AARCH64_INSN_CLS_BR_SYS,  AARCH64_INSN_CLS_BR_SYS,
    AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_REG,  AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_FPSIMD,
};

/**
 * @brief 获取ARM64指令的编码类别
 * @details 通过指令的bit[28:25]确定指令属于哪种类别
 * @param insn 32位ARM64指令编码
 * @return 指令的编码类别枚举值
 */
enum aarch64_insn_encoding_class aarch64_get_insn_class(u32 insn)
{
    return aarch64_insn_encoding_class[(insn >> 25) & 0xf];  // 提取bit[28:25]作为索引
}

/**
 * @brief 检查指令是否为NOP指令
 * @details NOP是HINT指令的别名，需要排除其他有意义的HINT指令
 * @param insn 32位ARM64指令编码
 * @return true表示是NOP指令，false表示不是
 */
bool aarch64_insn_is_nop(u32 insn)
{
    if (!aarch64_insn_is_hint(insn)) return false;  // 首先检查是否为HINT指令

    // 检查具体的HINT类型，排除有意义的HINT指令
    switch (insn & 0xFE0) {
    case AARCH64_INSN_HINT_YIELD:  // 让出处理器时间片
    case AARCH64_INSN_HINT_WFE:    // 等待事件
    case AARCH64_INSN_HINT_WFI:    // 等待中断
    case AARCH64_INSN_HINT_SEV:    // 发送事件
    case AARCH64_INSN_HINT_SEVL:   // 发送本地事件
        return false;               // 这些都不是NOP
    default:
        return true;                // 其他HINT指令视为NOP
    }
}

/**
 * @brief 检查指令是否为立即数分支指令
 * @details 判断指令是否使用立即数作为分支目标地址
 * @param insn 32位ARM64指令编码
 * @return true表示是立即数分支指令，false表示不是
 */
bool aarch64_insn_is_branch_imm(u32 insn)
{
    return (aarch64_insn_is_b(insn) || aarch64_insn_is_bl(insn) || aarch64_insn_is_tbz(insn) ||
            aarch64_insn_is_tbnz(insn) || aarch64_insn_is_cbz(insn) || aarch64_insn_is_cbnz(insn) ||
            aarch64_insn_is_bcond(insn));
}

/**
 * @brief 检查指令是否使用字面量池
 * @details 判断指令是否需要从内存中加载字面量数据
 * @param insn 32位ARM64指令编码
 * @return true表示使用字面量池，false表示不使用
 */
bool aarch64_insn_uses_literal(u32 insn)
{
    // ldr/ldrsw (literal), prfm 指令使用字面量池
    return aarch64_insn_is_ldr_lit(insn) || aarch64_insn_is_ldrsw_lit(insn) || aarch64_insn_is_adr_adrp(insn) ||
           aarch64_insn_is_prfm_lit(insn);
}

/**
 * @brief 检查指令是否为分支指令
 * @details 判断指令是否会改变程序计数器，包括条件和无条件分支
 * @param insn 32位ARM64指令编码
 * @return true表示是分支指令，false表示不是
 */
bool aarch64_insn_is_branch(u32 insn)
{
    // b, bl, cb*, tb*, b.cond, br, blr 都是分支指令
    return aarch64_insn_is_b(insn) || aarch64_insn_is_bl(insn) || aarch64_insn_is_cbz(insn) ||
           aarch64_insn_is_cbnz(insn) || aarch64_insn_is_tbz(insn) || aarch64_insn_is_tbnz(insn) ||
           aarch64_insn_is_ret(insn) || aarch64_insn_is_br(insn) || aarch64_insn_is_blr(insn) ||
           aarch64_insn_is_bcond(insn);
}

/**
 * @brief 获取立即数字段的掩码和偏移
 * @details 根据立即数类型返回对应的位掩码和在指令中的位偏移
 * @param type 立即数类型枚举
 * @param maskp 输出位掩码的指针
 * @param shiftp 输出位偏移的指针
 * @return 0表示成功，-EINVAL表示无效类型
 */
static int aarch64_get_imm_shift_mask(enum aarch64_insn_imm_type type, u32 *maskp, int *shiftp)
{
    u32 mask;
    int shift;

    switch (type) {
    case AARCH64_INSN_IMM_26:  // 26位立即数，用于B/BL指令
        mask = BIT(26) - 1;
        shift = 0;
        break;
    case AARCH64_INSN_IMM_19:  // 19位立即数，用于条件分支指令
        mask = BIT(19) - 1;
        shift = 5;
        break;
    case AARCH64_INSN_IMM_16:  // 16位立即数，用于MOVZ/MOVN/MOVK指令
        mask = BIT(16) - 1;
        shift = 5;
        break;
    case AARCH64_INSN_IMM_14:  // 14位立即数，用于TBZ/TBNZ指令
        mask = BIT(14) - 1;
        shift = 5;
        break;
    case AARCH64_INSN_IMM_12:  // 12位立即数，用于ADD/SUB立即数指令
        mask = BIT(12) - 1;
        shift = 10;
        break;
    case AARCH64_INSN_IMM_9:   // 9位立即数，用于加载/存储指令
        mask = BIT(9) - 1;
        shift = 12;
        break;
    case AARCH64_INSN_IMM_7:   // 7位立即数，用于加载/存储对指令
        mask = BIT(7) - 1;
        shift = 15;
        break;
    case AARCH64_INSN_IMM_6:   // 6位立即数，用于位字段指令
    case AARCH64_INSN_IMM_S:   // S字段，用于位字段指令
        mask = BIT(6) - 1;
        shift = 10;
        break;
    case AARCH64_INSN_IMM_R:   // R字段，用于位字段指令
        mask = BIT(6) - 1;
        shift = 16;
        break;
    case AARCH64_INSN_IMM_N:   // N字段，用于逻辑立即数指令
        mask = 1;
        shift = 22;
        break;
    default:
        return -EINVAL;         // 无效的立即数类型
    }

    *maskp = mask;
    *shiftp = shift;

    return 0;
}

#define ADR_IMM_HILOSPLIT 2
#define ADR_IMM_SIZE SZ_2M
#define ADR_IMM_LOMASK ((1 << ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_HIMASK ((ADR_IMM_SIZE >> ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_LOSHIFT 29
#define ADR_IMM_HISHIFT 5

u64 aarch64_insn_decode_immediate(enum aarch64_insn_imm_type type, u32 insn)
{
    u32 immlo, immhi, mask;
    int shift;

    switch (type) {
    case AARCH64_INSN_IMM_ADR:
        shift = 0;
        immlo = (insn >> ADR_IMM_LOSHIFT) & ADR_IMM_LOMASK;
        immhi = (insn >> ADR_IMM_HISHIFT) & ADR_IMM_HIMASK;
        insn = (immhi << ADR_IMM_HILOSPLIT) | immlo;
        mask = ADR_IMM_SIZE - 1;
        break;
    default:
        if (aarch64_get_imm_shift_mask(type, &mask, &shift) < 0) {
            fprintf(stdout, "aarch64_insn_decode_immediate: unknown immediate encoding %d\n", type);
            return 0;
        }
    }

    return (insn >> shift) & mask;
}

u32 aarch64_insn_encode_immediate(enum aarch64_insn_imm_type type, u32 insn, u64 imm)
{
    u32 immlo, immhi, mask;
    int shift;

    if (insn == AARCH64_BREAK_FAULT) return AARCH64_BREAK_FAULT;

    switch (type) {
    case AARCH64_INSN_IMM_ADR:
        shift = 0;
        immlo = (imm & ADR_IMM_LOMASK) << ADR_IMM_LOSHIFT;
        imm >>= ADR_IMM_HILOSPLIT;
        immhi = (imm & ADR_IMM_HIMASK) << ADR_IMM_HISHIFT;
        imm = immlo | immhi;
        mask = ((ADR_IMM_LOMASK << ADR_IMM_LOSHIFT) | (ADR_IMM_HIMASK << ADR_IMM_HISHIFT));
        break;
    default:
        if (aarch64_get_imm_shift_mask(type, &mask, &shift) < 0) {
            fprintf(stdout, "aarch64_insn_encode_immediate: unknown immediate encoding %d\n", type);
            return AARCH64_BREAK_FAULT;
        }
    }

    /* Update the immediate field. */
    insn &= ~(mask << shift);
    insn |= (imm & mask) << shift;

    return insn;
}

u32 aarch64_insn_decode_register(enum aarch64_insn_register_type type, u32 insn)
{
    int shift;

    switch (type) {
    case AARCH64_INSN_REGTYPE_RT:
    case AARCH64_INSN_REGTYPE_RD:
        shift = 0;
        break;
    case AARCH64_INSN_REGTYPE_RN:
        shift = 5;
        break;
    case AARCH64_INSN_REGTYPE_RT2:
    case AARCH64_INSN_REGTYPE_RA:
        shift = 10;
        break;
    case AARCH64_INSN_REGTYPE_RM:
        shift = 16;
        break;
    default:
        fprintf(stdout, "%s: unknown register type encoding %d\n", __func__, type);
        return 0;
    }

    return (insn >> shift) & GENMASK(4, 0);
}

/**
 * @brief 编码寄存器到指令中
 * @details 将指定寄存器编号编码到指令的相应位置
 * @param type 寄存器类型（RT、RD、RN等）
 * @param insn 要修改的指令
 * @param reg 要编码的寄存器编号
 * @return 编码后的指令，失败返回AARCH64_BREAK_FAULT
 */
static u32 aarch64_insn_encode_register(enum aarch64_insn_register_type type, u32 insn, enum aarch64_insn_register reg)
{
    int shift;

    if (insn == AARCH64_BREAK_FAULT) return AARCH64_BREAK_FAULT;

    if (reg < AARCH64_INSN_REG_0 || reg > AARCH64_INSN_REG_SP) {
        fprintf(stdout, "%s: unknown register encoding %d\n", __func__, reg);
        return AARCH64_BREAK_FAULT;
    }

    switch (type) {
    case AARCH64_INSN_REGTYPE_RT:  // 目标寄存器，位[4:0]
    case AARCH64_INSN_REGTYPE_RD:  // 目的寄存器，位[4:0]
        shift = 0;
        break;
    case AARCH64_INSN_REGTYPE_RN:  // 基地址寄存器，位[9:5]
        shift = 5;
        break;
    case AARCH64_INSN_REGTYPE_RT2: // 第二目标寄存器，位[14:10]
    case AARCH64_INSN_REGTYPE_RA:  // 累加器寄存器，位[14:10]
        shift = 10;
        break;
    case AARCH64_INSN_REGTYPE_RM:  // 偏移寄存器，位[20:16]
    case AARCH64_INSN_REGTYPE_RS:  // 源寄存器，位[20:16]
        shift = 16;
        break;
    default:
        fprintf(stdout, "%s: unknown register type encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    insn &= ~(GENMASK(4, 0) << shift);  // 清除原有寄存器字段
    insn |= reg << shift;               // 设置新的寄存器编号

    return insn;
}

/**
 * @brief 编码加载/存储指令的数据大小
 * @details 将数据大小编码到指令的size字段（位[31:30]）
 * @param type 数据大小类型
 * @param insn 要修改的指令
 * @return 编码后的指令，失败返回AARCH64_BREAK_FAULT
 */
static u32 aarch64_insn_encode_ldst_size(enum aarch64_insn_size_type type, u32 insn)
{
    u32 size;

    switch (type) {
    case AARCH64_INSN_SIZE_8:   // 8位（字节）
        size = 0;
        break;
    case AARCH64_INSN_SIZE_16:  // 16位（半字）
        size = 1;
        break;
    case AARCH64_INSN_SIZE_32:  // 32位（字）
        size = 2;
        break;
    case AARCH64_INSN_SIZE_64:  // 64位（双字）
        size = 3;
        break;
    default:
        fprintf(stdout, "%s: unknown size encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    insn &= ~GENMASK(31, 30);  // 清除size字段
    insn |= size << 30;        // 设置新的size值

    return insn;
}

/**
 * @brief 计算分支指令的偏移量
 * @details 计算从PC到目标地址的偏移，并检查是否在有效范围内
 * @param pc 当前程序计数器地址
 * @param addr 目标地址
 * @param range 允许的偏移范围
 * @return 计算的偏移量，超出范围返回range
 */
static inline int64_t branch_imm_common(uint64_t pc, uint64_t addr, int64_t range)
{
    int64_t offset;

    // ARM64指令必须4字节对齐
    if ((pc & 0x3) || (addr & 0x3)) {
        fprintf(stdout, "%s: A64 instructions must be word aligned\n", __func__);
        return range;
    }

    offset = ((long)addr - (long)pc);  // 计算偏移量

    // 检查偏移量是否在允许范围内
    if (offset < -range || offset >= range) {
        fprintf(stdout, "%s: offset out of range\n", __func__);
        return range;
    }

    return offset;
}

/**
 * @brief 生成立即数分支指令
 * @details 生成B或BL指令，支持±128M偏移范围
 * @param pc 当前程序计数器地址
 * @param addr 目标地址
 * @param type 分支类型（B或BL）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_branch_imm(uint64_t pc, uint64_t addr, enum aarch64_insn_branch_type type)
{
    u32 insn;
    int64_t offset;

    /*
     * B/BL支持[-128M, 128M)偏移范围
     * ARM64虚拟地址安排保证所有内核和模块代码都在±128M范围内
     */
    offset = branch_imm_common(pc, addr, SZ_128M);
    if (offset >= SZ_128M) return AARCH64_BREAK_FAULT;

    switch (type) {
    case AARCH64_INSN_BRANCH_LINK:      // BL指令（带链接的分支）
        insn = aarch64_insn_get_bl_value();
        break;
    case AARCH64_INSN_BRANCH_NOLINK:    // B指令（不带链接的分支）
        insn = aarch64_insn_get_b_value();
        break;
    default:
        fprintf(stdout, "%s: unknown branch encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    // 编码26位立即数偏移（除以4，因为指令4字节对齐）
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_26, insn, offset >> 2);
}

/**
 * @brief 生成比较分支指令
 * @details 生成CBZ或CBNZ指令，支持±1M偏移范围
 * @param pc 当前程序计数器地址
 * @param addr 目标地址
 * @param reg 要比较的寄存器
 * @param variant 指令变体（32位或64位）
 * @param type 分支类型（CBZ或CBNZ）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_comp_branch_imm(uint64_t pc, uint64_t addr, enum aarch64_insn_register reg,
                                     enum aarch64_insn_variant variant, enum aarch64_insn_branch_type type)
{
    u32 insn;
    int64_t offset;

    // CBZ/CBNZ支持±1M偏移范围
    offset = branch_imm_common(pc, addr, SZ_1M);
    if (offset >= SZ_1M) return AARCH64_BREAK_FAULT;

    switch (type) {
    case AARCH64_INSN_BRANCH_COMP_ZERO:     // CBZ指令（比较为零则分支）
        insn = aarch64_insn_get_cbz_value();
        break;
    case AARCH64_INSN_BRANCH_COMP_NONZERO:  // CBNZ指令（比较非零则分支）
        insn = aarch64_insn_get_cbnz_value();
        break;
    default:
        fprintf(stdout, "%s: unknown branch encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位寄存器操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位寄存器操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码寄存器字段
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT, insn, reg);

    // 编码19位立即数偏移
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_19, insn, offset >> 2);
}

/**
 * @brief 生成条件分支指令
 * @details 生成B.cond指令，支持±1M偏移范围
 * @param pc 当前程序计数器地址
 * @param addr 目标地址
 * @param cond 分支条件码
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_cond_branch_imm(uint64_t pc, uint64_t addr, enum aarch64_insn_condition cond)
{
    u32 insn;
    int64_t offset;

    // 条件分支支持±1M偏移范围
    offset = branch_imm_common(pc, addr, SZ_1M);

    insn = aarch64_insn_get_bcond_value();  // 获取B.cond指令模板

    // 检查条件码有效性
    if (cond < AARCH64_INSN_COND_EQ || cond > AARCH64_INSN_COND_AL) {
        fprintf(stdout, "%s: unknown condition encoding %d\n", __func__, cond);
        return AARCH64_BREAK_FAULT;
    }
    insn |= cond;  // 设置条件码字段

    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_19, insn, offset >> 2);
}

/**
 * @brief 生成提示指令
 * @details 生成HINT指令，用于向处理器提供执行提示
 * @param op 提示操作码
 * @return 生成的HINT指令编码
 */
u32 aarch64_insn_gen_hint(enum aarch64_insn_hint_op op)
{
    return aarch64_insn_get_hint_value() | op;
}

/**
 * @brief 生成NOP指令
 * @details 生成空操作指令，等同于HINT #0
 * @return NOP指令编码
 */
u32 aarch64_insn_gen_nop(void)
{
    return aarch64_insn_gen_hint(AARCH64_INSN_HINT_NOP);
}

/**
 * @brief 生成寄存器分支指令
 * @details 生成BR、BLR或RET指令，分支目标地址来自寄存器
 * @param reg 包含分支目标地址的寄存器
 * @param type 分支类型（BR、BLR或RET）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_branch_reg(enum aarch64_insn_register reg, enum aarch64_insn_branch_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_BRANCH_NOLINK:    // BR指令（不带链接的寄存器分支）
        insn = aarch64_insn_get_br_value();
        break;
    case AARCH64_INSN_BRANCH_LINK:      // BLR指令（带链接的寄存器分支）
        insn = aarch64_insn_get_blr_value();
        break;
    case AARCH64_INSN_BRANCH_RETURN:    // RET指令（函数返回）
        insn = aarch64_insn_get_ret_value();
        break;
    default:
        fprintf(stdout, "%s: unknown branch encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    // 编码分支目标寄存器
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, reg);
}

/**
 * @brief 生成寄存器偏移的加载/存储指令
 * @details 生成LDR/STR指令，使用寄存器作为偏移量
 * @param reg 数据寄存器
 * @param base 基地址寄存器
 * @param offset 偏移寄存器
 * @param size 数据大小
 * @param type 操作类型（加载或存储）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_load_store_reg(enum aarch64_insn_register reg, enum aarch64_insn_register base,
                                    enum aarch64_insn_register offset, enum aarch64_insn_size_type size,
                                    enum aarch64_insn_ldst_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_LDST_LOAD_REG_OFFSET:     // LDR指令（寄存器偏移）
        insn = aarch64_insn_get_ldr_reg_value();
        break;
    case AARCH64_INSN_LDST_STORE_REG_OFFSET:    // STR指令（寄存器偏移）
        insn = aarch64_insn_get_str_reg_value();
        break;
    default:
        fprintf(stdout, "%s: unknown load/store encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    // 编码数据大小
    insn = aarch64_insn_encode_ldst_size(size, insn);

    // 编码目标/源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT, insn, reg);

    // 编码基地址寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, base);

    // 编码偏移寄存器
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, offset);
}

/**
 * @brief 生成加载/存储对指令
 * @details 生成LDP/STP指令，同时操作两个寄存器
 * @param reg1 第一个数据寄存器
 * @param reg2 第二个数据寄存器
 * @param base 基地址寄存器
 * @param offset 立即数偏移（字节单位）
 * @param variant 指令变体（32位或64位）
 * @param type 操作类型（预索引、后索引或偏移）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_load_store_pair(enum aarch64_insn_register reg1, enum aarch64_insn_register reg2,
                                     enum aarch64_insn_register base, int offset, enum aarch64_insn_variant variant,
                                     enum aarch64_insn_ldst_type type)
{
    u32 insn;
    int shift;

    switch (type) {
    case AARCH64_INSN_LDST_LOAD_PAIR_PRE_INDEX:    // LDP预索引
        insn = aarch64_insn_get_ldp_pre_value();
        break;
    case AARCH64_INSN_LDST_STORE_PAIR_PRE_INDEX:   // STP预索引
        insn = aarch64_insn_get_stp_pre_value();
        break;
    case AARCH64_INSN_LDST_LOAD_PAIR_POST_INDEX:   // LDP后索引
        insn = aarch64_insn_get_ldp_post_value();
        break;
    case AARCH64_INSN_LDST_STORE_PAIR_POST_INDEX:  // STP后索引
        insn = aarch64_insn_get_stp_post_value();
        break;
    default:
        fprintf(stdout, "%s: unknown load/store encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:            // 32位寄存器操作
        // 偏移必须是4的倍数，范围[-256, 252]
        if ((offset & 0x3) || (offset < -256) || (offset > 252)) {
            fprintf(stdout, "%s: offset must be multiples of 4 in the range of [-256, 252] %d\n", __func__, offset);
            return AARCH64_BREAK_FAULT;
        }
        shift = 2;  // 32位操作，偏移右移2位编码
        break;
    case AARCH64_INSN_VARIANT_64BIT:            // 64位寄存器操作
        // 偏移必须是8的倍数，范围[-512, 504]
        if ((offset & 0x7) || (offset < -512) || (offset > 504)) {
            fprintf(stdout, "%s: offset must be multiples of 8 in the range of [-512, 504] %d\n", __func__, offset);
            return AARCH64_BREAK_FAULT;
        }
        shift = 3;                              // 64位操作，偏移右移3位编码
        insn |= AARCH64_INSN_SF_BIT;           // 设置SF位表示64位操作
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码第一个寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT, insn, reg1);

    // 编码第二个寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT2, insn, reg2);

    // 编码基地址寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, base);

    // 编码7位立即数偏移
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_7, insn, offset >> shift);
}

/**
 * @brief 生成独占加载/存储指令
 * @details 生成LDXR/STXR等独占访问指令，用于原子操作
 * @param reg 数据寄存器
 * @param base 基地址寄存器
 * @param state 状态寄存器（用于存储指令）
 * @param size 数据大小
 * @param type 操作类型（独占加载或存储）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_load_store_ex(enum aarch64_insn_register reg, enum aarch64_insn_register base,
                                   enum aarch64_insn_register state, enum aarch64_insn_size_type size,
                                   enum aarch64_insn_ldst_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_LDST_LOAD_EX:     // 独占加载（LDXR）
        insn = aarch64_insn_get_load_ex_value();
        break;
    case AARCH64_INSN_LDST_STORE_EX:    // 独占存储（STXR）
        insn = aarch64_insn_get_store_ex_value();
        break;
    default:
        fprintf(stdout, "%s: unknown load/store exclusive encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    // 编码数据大小
    insn = aarch64_insn_encode_ldst_size(size, insn);

    // 编码数据寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT, insn, reg);

    // 编码基地址寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, base);

    // RT2字段设置为零寄存器（未使用）
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT2, insn, AARCH64_INSN_REG_ZR);

    // 编码状态寄存器（用于存储指令的结果状态）
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RS, insn, state);
}

/**
 * @brief 编码预取指令的立即数字段
 * @details 将预取类型、目标和策略编码为5位立即数
 * @param type 预取类型（PLD、PLI、PST）
 * @param target 目标缓存级别（L1、L2、L3）
 * @param policy 预取策略（KEEP、STRM）
 * @param insn 要修改的指令
 * @return 编码后的指令，失败返回AARCH64_BREAK_FAULT
 */
static u32 aarch64_insn_encode_prfm_imm(enum aarch64_insn_prfm_type type, enum aarch64_insn_prfm_target target,
                                        enum aarch64_insn_prfm_policy policy, u32 insn)
{
    u32 imm_type = 0, imm_target = 0, imm_policy = 0;

    switch (type) {
    case AARCH64_INSN_PRFM_TYPE_PLD:        // 预取数据加载
        break;
    case AARCH64_INSN_PRFM_TYPE_PLI:        // 预取指令加载
        imm_type = BIT(0);
        break;
    case AARCH64_INSN_PRFM_TYPE_PST:        // 预取数据存储
        imm_type = BIT(1);
        break;
    default:
        fprintf(stdout, "%s: unknown prfm type encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (target) {
    case AARCH64_INSN_PRFM_TARGET_L1:       // L1缓存
        break;
    case AARCH64_INSN_PRFM_TARGET_L2:       // L2缓存
        imm_target = BIT(0);
        break;
    case AARCH64_INSN_PRFM_TARGET_L3:       // L3缓存
        imm_target = BIT(1);
        break;
    default:
        fprintf(stdout, "%s: unknown prfm target encoding %d\n", __func__, target);
        return AARCH64_BREAK_FAULT;
    }

    switch (policy) {
    case AARCH64_INSN_PRFM_POLICY_KEEP:     // 保持在缓存中
        break;
    case AARCH64_INSN_PRFM_POLICY_STRM:     // 流式访问，临时保持
        imm_policy = BIT(0);
        break;
    default:
        fprintf(stdout, "%s: unknown prfm policy encoding %d\n", __func__, policy);
        return AARCH64_BREAK_FAULT;
    }

    /* 在此情况下，imm5被编码到Rt字段中 */
    insn &= ~GENMASK(4, 0);
    insn |= imm_policy | (imm_target << 1) | (imm_type << 3);

    return insn;
}

/**
 * @brief 生成预取指令
 * @details 生成PRFM指令，用于数据预取优化
 * @param base 基地址寄存器
 * @param type 预取类型
 * @param target 目标缓存级别
 * @param policy 预取策略
 * @return 生成的PRFM指令编码
 */
u32 aarch64_insn_gen_prefetch(enum aarch64_insn_register base, enum aarch64_insn_prfm_type type,
                              enum aarch64_insn_prfm_target target, enum aarch64_insn_prfm_policy policy)
{
    u32 insn = aarch64_insn_get_prfm_value();

    // 设置64位大小（预取操作固定为64位）
    insn = aarch64_insn_encode_ldst_size(AARCH64_INSN_SIZE_64, insn);

    // 编码预取参数到立即数字段
    insn = aarch64_insn_encode_prfm_imm(type, target, policy, insn);

    // 编码基地址寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, base);

    // 编码12位立即数偏移（此处为0）
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_12, insn, 0);
}

/**
 * @brief 生成立即数加法/减法指令
 * @details 生成ADD/SUB立即数指令，支持12位立即数
 * @param dst 目标寄存器
 * @param src 源寄存器
 * @param imm 立即数值
 * @param variant 指令变体（32位或64位）
 * @param type 操作类型（ADD或SUB）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_add_sub_imm(enum aarch64_insn_register dst, enum aarch64_insn_register src, int imm,
                                 enum aarch64_insn_variant variant, enum aarch64_insn_adsb_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_ADSB_ADD:             // ADD立即数指令
        insn = aarch64_insn_get_add_imm_value();
        break;
    case AARCH64_INSN_ADSB_SUB:             // SUB立即数指令
        insn = aarch64_insn_get_sub_imm_value();
        break;
    case AARCH64_INSN_ADSB_ADD_SETFLAGS:    // ADDS立即数指令（设置标志位）
        insn = aarch64_insn_get_adds_imm_value();
        break;
    case AARCH64_INSN_ADSB_SUB_SETFLAGS:    // SUBS立即数指令（设置标志位）
        insn = aarch64_insn_get_subs_imm_value();
        break;
    default:
        fprintf(stdout, "%s: unknown add/sub encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    /* 不能编码超过24位的值（12位 + 12位移位） */
    if (imm & ~(BIT(24) - 1)) goto out;

    /* 如果高12位有数据... */
    if (imm & ~(SZ_4K - 1)) {
        /* ...而低12位也有数据 -> 错误 */
        if (imm & (SZ_4K - 1)) goto out;

        imm >>= 12;                         // 右移12位
        insn |= AARCH64_INSN_LSL_12;       // 设置LSL #12位
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);

    // 编码12位立即数
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_12, insn, imm);

out:
    fprintf(stdout, "%s: invalid immediate encoding %d\n", __func__, imm);
    return AARCH64_BREAK_FAULT;
}

/**
 * @brief 生成位字段操作指令
 * @details 生成BFM、SBFM、UBFM等位字段操作指令
 * @param dst 目标寄存器
 * @param src 源寄存器
 * @param immr 右旋转量
 * @param imms 提取长度减1
 * @param variant 指令变体（32位或64位）
 * @param type 位字段操作类型
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_bitfield(enum aarch64_insn_register dst, enum aarch64_insn_register src, int immr, int imms,
                              enum aarch64_insn_variant variant, enum aarch64_insn_bitfield_type type)
{
    u32 insn;
    u32 mask;

    switch (type) {
    case AARCH64_INSN_BITFIELD_MOVE:        // BFM指令（位字段移动）
        insn = aarch64_insn_get_bfm_value();
        break;
    case AARCH64_INSN_BITFIELD_MOVE_UNSIGNED:  // UBFM指令（无符号位字段移动）
        insn = aarch64_insn_get_ubfm_value();
        break;
    case AARCH64_INSN_BITFIELD_MOVE_SIGNED:    // SBFM指令（有符号位字段移动）
        insn = aarch64_insn_get_sbfm_value();
        break;
    default:
        fprintf(stdout, "%s: unknown bitfield encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        mask = GENMASK(4, 0);               // 5位掩码
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT | AARCH64_INSN_N_BIT;  // 设置SF和N位
        mask = GENMASK(5, 0);               // 6位掩码
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 检查immr值是否在有效范围内
    if (immr & ~mask) {
        fprintf(stdout, "%s: invalid immr encoding %d\n", __func__, immr);
        return AARCH64_BREAK_FAULT;
    }
    // 检查imms值是否在有效范围内
    if (imms & ~mask) {
        fprintf(stdout, "%s: invalid imms encoding %d\n", __func__, imms);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);

    // 编码immr字段（右旋转量）
    insn = aarch64_insn_encode_immediate(AARCH64_INSN_IMM_R, insn, immr);

    // 编码imms字段（提取长度减1）
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_S, insn, imms);
}

/**
 * @brief 生成移动立即数指令
 * @details 生成MOVZ、MOVK、MOVN指令，用于加载16位立即数
 * @param dst 目标寄存器
 * @param imm 16位立即数值
 * @param shift 移位量（0、16、32、48）
 * @param variant 指令变体（32位或64位）
 * @param type 移动操作类型
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_movewide(enum aarch64_insn_register dst, int imm, int shift, enum aarch64_insn_variant variant,
                              enum aarch64_insn_movewide_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_MOVEWIDE_ZERO:        // MOVZ指令（其他位清零）
        insn = aarch64_insn_get_movz_value();
        break;
    case AARCH64_INSN_MOVEWIDE_KEEP:        // MOVK指令（保持其他位）
        insn = aarch64_insn_get_movk_value();
        break;
    case AARCH64_INSN_MOVEWIDE_INVERSE:     // MOVN指令（立即数取反）
        insn = aarch64_insn_get_movn_value();
        break;
    default:
        fprintf(stdout, "%s: unknown movewide encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    // 检查立即数是否为16位值
    if (imm & ~(SZ_64K - 1)) {
        fprintf(stdout, "%s: invalid immediate encoding %d\n", __func__, imm);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        // 32位操作只允许移位0或16
        if (shift != 0 && shift != 16) {
            fprintf(stdout, "%s: invalid shift encoding %d\n", __func__, shift);
            return AARCH64_BREAK_FAULT;
        }
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        // 64位操作允许移位0、16、32、48
        if (shift != 0 && shift != 16 && shift != 32 && shift != 48) {
            fprintf(stdout, "%s: invalid shift encoding %d\n", __func__, shift);
            return AARCH64_BREAK_FAULT;
        }
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    insn |= (shift >> 4) << 21;  // 编码hw字段（移位量/16）

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码16位立即数
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_16, insn, imm);
}

/**
 * @brief 生成移位寄存器加法/减法指令
 * @details 生成ADD/SUB寄存器移位指令，第二操作数是移位寄存器
 * @param dst 目标寄存器
 * @param src 第一源寄存器
 * @param reg 第二源寄存器（要移位的）
 * @param shift 移位量
 * @param variant 指令变体（32位或64位）
 * @param type 操作类型（ADD或SUB）
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_add_sub_shifted_reg(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                                         enum aarch64_insn_register reg, int shift, enum aarch64_insn_variant variant,
                                         enum aarch64_insn_adsb_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_ADSB_ADD:             // ADD寄存器指令
        insn = aarch64_insn_get_add_value();
        break;
    case AARCH64_INSN_ADSB_SUB:             // SUB寄存器指令
        insn = aarch64_insn_get_sub_value();
        break;
    case AARCH64_INSN_ADSB_ADD_SETFLAGS:    // ADDS寄存器指令（设置标志位）
        insn = aarch64_insn_get_adds_value();
        break;
    case AARCH64_INSN_ADSB_SUB_SETFLAGS:    // SUBS寄存器指令（设置标志位）
        insn = aarch64_insn_get_subs_value();
        break;
    default:
        fprintf(stdout, "%s: unknown add/sub encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        // 32位操作移位量范围[0, 31]
        if (shift & ~(SZ_32 - 1)) {
            fprintf(stdout, "%s: invalid shift encoding %d\n", __func__, shift);
            return AARCH64_BREAK_FAULT;
        }
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        // 64位操作移位量范围[0, 63]
        if (shift & ~(SZ_64 - 1)) {
            fprintf(stdout, "%s: invalid shift encoding %d\n", __func__, shift);
            return AARCH64_BREAK_FAULT;
        }
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码第一源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);

    // 编码第二源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg);

    // 编码6位移位量
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_6, insn, shift);
}

/**
 * @brief 生成单操作数数据处理指令
 * @details 生成REV16、REV32、REV64等数据处理指令
 * @param dst 目标寄存器
 * @param src 源寄存器
 * @param variant 指令变体（32位或64位）
 * @param type 数据处理类型
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_data1(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_variant variant, enum aarch64_insn_data1_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_DATA1_REVERSE_16:     // REV16指令（按16位反转字节序）
        insn = aarch64_insn_get_rev16_value();
        break;
    case AARCH64_INSN_DATA1_REVERSE_32:     // REV32指令（按32位反转字节序）
        insn = aarch64_insn_get_rev32_value();
        break;
    case AARCH64_INSN_DATA1_REVERSE_64:     // REV64指令（按64位反转字节序）
        if (variant != AARCH64_INSN_VARIANT_64BIT) {
            fprintf(stdout, "%s: invalid variant for reverse64 %d\n", __func__, variant);
            return AARCH64_BREAK_FAULT;
        }
        insn = aarch64_insn_get_rev64_value();
        break;
    default:
        fprintf(stdout, "%s: unknown data1 encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码源寄存器
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);
}

/**
 * @brief 生成双操作数数据处理指令
 * @details 生成UDIV、SDIV、LSLV、LSRV、ASRV、RORV等指令
 * @param dst 目标寄存器
 * @param src 第一源寄存器
 * @param reg 第二源寄存器
 * @param variant 指令变体（32位或64位）
 * @param type 数据处理类型
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_data2(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_register reg, enum aarch64_insn_variant variant,
                           enum aarch64_insn_data2_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_DATA2_UDIV:           // UDIV指令（无符号除法）
        insn = aarch64_insn_get_udiv_value();
        break;
    case AARCH64_INSN_DATA2_SDIV:           // SDIV指令（有符号除法）
        insn = aarch64_insn_get_sdiv_value();
        break;
    case AARCH64_INSN_DATA2_LSLV:           // LSLV指令（寄存器逻辑左移）
        insn = aarch64_insn_get_lslv_value();
        break;
    case AARCH64_INSN_DATA2_LSRV:           // LSRV指令（寄存器逻辑右移）
        insn = aarch64_insn_get_lsrv_value();
        break;
    case AARCH64_INSN_DATA2_ASRV:           // ASRV指令（寄存器算术右移）
        insn = aarch64_insn_get_asrv_value();
        break;
    case AARCH64_INSN_DATA2_RORV:           // RORV指令（寄存器循环右移）
        insn = aarch64_insn_get_rorv_value();
        break;
    default:
        fprintf(stdout, "%s: unknown data2 encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码第一源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);

    // 编码第二源寄存器
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg);
}

/**
 * @brief 生成三操作数数据处理指令
 * @details 生成MADD、MSUB等三操作数指令
 * @param dst 目标寄存器
 * @param src 第一源寄存器
 * @param reg1 第二源寄存器
 * @param reg2 第三源寄存器（累加器）
 * @param variant 指令变体（32位或64位）
 * @param type 数据处理类型
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_data3(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_register reg1, enum aarch64_insn_register reg2,
                           enum aarch64_insn_variant variant, enum aarch64_insn_data3_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_DATA3_MADD:           // MADD指令（乘加：dst = reg1 * reg2 + src）
        insn = aarch64_insn_get_madd_value();
        break;
    case AARCH64_INSN_DATA3_MSUB:           // MSUB指令（乘减：dst = src - reg1 * reg2）
        insn = aarch64_insn_get_msub_value();
        break;
    default:
        fprintf(stdout, "%s: unknown data3 encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码累加器寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RA, insn, src);

    // 编码第一乘数寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, reg1);

    // 编码第二乘数寄存器
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg2);
}

/**
 * @brief 生成移位寄存器逻辑指令
 * @details 生成AND、BIC、ORR、ORN、EOR、EON等逻辑指令
 * @param dst 目标寄存器
 * @param src 第一源寄存器
 * @param reg 第二源寄存器（要移位的）
 * @param shift 移位量
 * @param variant 指令变体（32位或64位）
 * @param type 逻辑操作类型
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_logical_shifted_reg(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                                         enum aarch64_insn_register reg, int shift, enum aarch64_insn_variant variant,
                                         enum aarch64_insn_logic_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_LOGIC_AND:            // AND指令（按位与）
        insn = aarch64_insn_get_and_value();
        break;
    case AARCH64_INSN_LOGIC_BIC:            // BIC指令（位清除）
        insn = aarch64_insn_get_bic_value();
        break;
    case AARCH64_INSN_LOGIC_ORR:            // ORR指令（按位或）
        insn = aarch64_insn_get_orr_value();
        break;
    case AARCH64_INSN_LOGIC_ORN:            // ORN指令（按位或非）
        insn = aarch64_insn_get_orn_value();
        break;
    case AARCH64_INSN_LOGIC_EOR:            // EOR指令（按位异或）
        insn = aarch64_insn_get_eor_value();
        break;
    case AARCH64_INSN_LOGIC_EON:            // EON指令（按位异或非）
        insn = aarch64_insn_get_eon_value();
        break;
    case AARCH64_INSN_LOGIC_AND_SETFLAGS:   // ANDS指令（按位与并设置标志位）
        insn = aarch64_insn_get_ands_value();
        break;
    case AARCH64_INSN_LOGIC_BIC_SETFLAGS:   // BICS指令（位清除并设置标志位）
        insn = aarch64_insn_get_bics_value();
        break;
    default:
        fprintf(stdout, "%s: unknown logical encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位操作
        // 32位操作移位量范围[0, 31]
        if (shift & ~(SZ_32 - 1)) {
            fprintf(stdout, "%s: invalid shift encoding %d\n", __func__, shift);
            return AARCH64_BREAK_FAULT;
        }
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位操作
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        // 64位操作移位量范围[0, 63]
        if (shift & ~(SZ_64 - 1)) {
            fprintf(stdout, "%s: invalid shift encoding %d\n", __func__, shift);
            return AARCH64_BREAK_FAULT;
        }
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码第一源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);

    // 编码第二源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg);

    // 编码6位移位量
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_6, insn, shift);
}

/**
 * @brief 解码分支指令的立即数字段并返回字节偏移
 * @details 解码分支指令中的偏移量，返回有符号值用于计算新的分支目标
 * @param insn 32位ARM64指令编码
 * @return 有符号字节偏移量
 */
s32 aarch64_get_branch_offset(u32 insn)
{
    s32 imm;

    // B和BL指令使用26位立即数
    if (aarch64_insn_is_b(insn) || aarch64_insn_is_bl(insn)) {
        imm = aarch64_insn_decode_immediate(AARCH64_INSN_IMM_26, insn);
        return (imm << 6) >> 4;  // 符号扩展并转换为字节偏移
    }

    // CBZ、CBNZ和条件分支指令使用19位立即数
    if (aarch64_insn_is_cbz(insn) || aarch64_insn_is_cbnz(insn) || aarch64_insn_is_bcond(insn)) {
        imm = aarch64_insn_decode_immediate(AARCH64_INSN_IMM_19, insn);
        return (imm << 13) >> 11;  // 符号扩展并转换为字节偏移
    }

    // TBZ和TBNZ指令使用14位立即数
    if (aarch64_insn_is_tbz(insn) || aarch64_insn_is_tbnz(insn)) {
        imm = aarch64_insn_decode_immediate(AARCH64_INSN_IMM_14, insn);
        return (imm << 18) >> 16;  // 符号扩展并转换为字节偏移
    }

    /* 未处理的指令类型 */
    BUG();
}

/**
 * @brief 编码分支偏移到指令的立即数字段
 * @details 将分支偏移编码到指令中并返回更新后的指令
 * @param insn 原始指令编码
 * @param offset 字节偏移量
 * @return 更新后的指令编码
 */
u32 aarch64_set_branch_offset(u32 insn, s32 offset)
{
    // B和BL指令
    if (aarch64_insn_is_b(insn) || aarch64_insn_is_bl(insn))
        return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_26, insn, offset >> 2);

    // CBZ、CBNZ和条件分支指令
    if (aarch64_insn_is_cbz(insn) || aarch64_insn_is_cbnz(insn) || aarch64_insn_is_bcond(insn))
        return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_19, insn, offset >> 2);

    // TBZ和TBNZ指令
    if (aarch64_insn_is_tbz(insn) || aarch64_insn_is_tbnz(insn))
        return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_14, insn, offset >> 2);

    /* 未处理的指令类型 */
    BUG();
}

/**
 * @brief 获取ADRP指令的偏移量
 * @details 解码ADRP指令中的立即数并转换为字节偏移
 * @param insn ADRP指令编码
 * @return 页偏移量（乘以4096）
 */
s32 aarch64_insn_adrp_get_offset(u32 insn)
{
    BUG_ON(!aarch64_insn_is_adrp(insn));
    return aarch64_insn_decode_immediate(AARCH64_INSN_IMM_ADR, insn) << 12;
}

/**
 * @brief 设置ADRP指令的偏移量
 * @details 将偏移量编码到ADRP指令中
 * @param insn 原始ADRP指令编码
 * @param offset 页偏移量（字节）
 * @return 更新后的指令编码
 */
u32 aarch64_insn_adrp_set_offset(u32 insn, s32 offset)
{
    BUG_ON(!aarch64_insn_is_adrp(insn));
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_ADR, insn, offset >> 12);
}

/**
 * @brief 从MSR/MRS指令中提取系统寄存器数据
 * @details 提取Op/CR数据用于系统寄存器操作
 * @param insn MSR/MRS指令编码
 * @return 系统寄存器编号
 */
u32 aarch64_insn_extract_system_reg(u32 insn)
{
    return (insn & 0x1FFFE0) >> 5;
}

/**
 * @brief 检查AArch32指令是否为宽指令（32位）
 * @details 判断Thumb指令是否为32位宽指令
 * @param insn 16位Thumb指令编码
 * @return true表示是32位宽指令，false表示是16位指令
 */
bool aarch32_insn_is_wide(u32 insn)
{
    return insn >= 0xe800;
}

/**
 * @brief 从AArch32指令中提取寄存器编号
 * @details 根据偏移量从指令中提取4位寄存器编号
 * @param insn 32位AArch32指令编码
 * @param offset 寄存器字段在指令中的位偏移
 * @return 4位寄存器编号
 */
u32 aarch32_insn_extract_reg_num(u32 insn, int offset)
{
    return (insn & (0xf << offset)) >> offset;
}

#define OPC2_MASK 0x7
#define OPC2_OFFSET 5
/**
 * @brief 从MCR指令中提取OPC2字段
 * @details 提取协处理器指令的OPC2操作码
 * @param insn MCR指令编码
 * @return 3位OPC2值
 */
u32 aarch32_insn_mcr_extract_opc2(u32 insn)
{
    return (insn & (OPC2_MASK << OPC2_OFFSET)) >> OPC2_OFFSET;
}

#define CRM_MASK 0xf
/**
 * @brief 从MCR指令中提取CRm字段
 * @details 提取协处理器寄存器编号
 * @param insn MCR指令编码
 * @return 4位CRm值
 */
u32 aarch32_insn_mcr_extract_crm(u32 insn)
{
    return insn & CRM_MASK;
}

// 条件码检查函数 - 根据PSTATE标志位判断条件是否满足
static bool __check_eq(uint64_t pstate) { return (pstate & PSR_Z_BIT) != 0; }      // 相等（Z=1）
static bool __check_ne(uint64_t pstate) { return (pstate & PSR_Z_BIT) == 0; }      // 不等（Z=0）
static bool __check_cs(uint64_t pstate) { return (pstate & PSR_C_BIT) != 0; }      // 进位（C=1）
static bool __check_cc(uint64_t pstate) { return (pstate & PSR_C_BIT) == 0; }      // 无进位（C=0）
static bool __check_mi(uint64_t pstate) { return (pstate & PSR_N_BIT) != 0; }      // 负数（N=1）
static bool __check_pl(uint64_t pstate) { return (pstate & PSR_N_BIT) == 0; }      // 正数（N=0）
static bool __check_vs(uint64_t pstate) { return (pstate & PSR_V_BIT) != 0; }      // 溢出（V=1）
static bool __check_vc(uint64_t pstate) { return (pstate & PSR_V_BIT) == 0; }      // 无溢出（V=0）

// 高于（C=1且Z=0）
static bool __check_hi(uint64_t pstate)
{
    pstate &= ~(pstate >> 1); /* PSR_C_BIT &= ~PSR_Z_BIT */
    return (pstate & PSR_C_BIT) != 0;
}

// 低于或相等（C=0或Z=1）
static bool __check_ls(uint64_t pstate)
{
    pstate &= ~(pstate >> 1); /* PSR_C_BIT &= ~PSR_Z_BIT */
    return (pstate & PSR_C_BIT) == 0;
}

// 大于或等于（N=V）
static bool __check_ge(uint64_t pstate)
{
    pstate ^= (pstate << 3); /* PSR_N_BIT ^= PSR_V_BIT */
    return (pstate & PSR_N_BIT) == 0;
}

// 小于（N≠V）
static bool __check_lt(uint64_t pstate)
{
    pstate ^= (pstate << 3); /* PSR_N_BIT ^= PSR_V_BIT */
    return (pstate & PSR_N_BIT) != 0;
}

// 大于（Z=0且N=V）
static bool __check_gt(uint64_t pstate)
{
    /*PSR_N_BIT ^= PSR_V_BIT */
    uint64_t temp = pstate ^ (pstate << 3);
    temp |= (pstate << 1); /*PSR_N_BIT |= PSR_Z_BIT */
    return (temp & PSR_N_BIT) == 0;
}

// 小于或等于（Z=1或N≠V）
static bool __check_le(uint64_t pstate)
{
    /*PSR_N_BIT ^= PSR_V_BIT */
    uint64_t temp = pstate ^ (pstate << 3);
    temp |= (pstate << 1); /*PSR_N_BIT |= PSR_Z_BIT */
    return (temp & PSR_N_BIT) != 0;
}

// 总是真（无条件）
static bool __check_al(uint64_t pstate) { return true; }

/**
 * 注意：ARMv8 ARM将条件码0b1111称为"nv"，但声明它的行为与0b1110（"al"）相同
 */
pstate_check_t *const aarch32_opcode_cond_checks[16] = { __check_eq, __check_ne, __check_cs, __check_cc,
                                                         __check_mi, __check_pl, __check_vs, __check_vc,
                                                         __check_hi, __check_ls, __check_ge, __check_lt,
                                                         __check_gt, __check_le, __check_al, __check_al };

/**
 * @brief 检查值是否为连续的1的范围
 * @details 用于验证立即数编码的有效性（不处理全1或全0）
 * @param val 待检查的值
 * @return true表示是连续的1，false表示不是
 */
static bool range_of_ones(u64 val)
{
    /* 不处理全1或全0 */
    u64 sval = val >> __ffs64(val);

    /* Sean Eron Anderson的位操作技巧之一 */
    return ((sval + 1) & (sval)) == 0;
}

/**
 * @brief 编码逻辑立即数
 * @details 将立即数编码为ARM64逻辑指令的立即数字段
 * @param imm 要编码的立即数
 * @param variant 指令变体（32位或64位）
 * @param insn 要修改的指令
 * @return 编码后的指令，失败返回AARCH64_BREAK_FAULT
 */
static u32 aarch64_encode_immediate(u64 imm, enum aarch64_insn_variant variant, u32 insn)
{
    uint32_t immr, imms, n, ones, ror, esz, tmp;
    u64 mask = ~0UL;

    /* 不能编码全0或全1 */
    if (!imm || !~imm) return AARCH64_BREAK_FAULT;

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:
        if (upper_32_bits(imm)) return AARCH64_BREAK_FAULT;
        esz = 32;
        break;
    case AARCH64_INSN_VARIANT_64BIT:
        insn |= AARCH64_INSN_SF_BIT;
        esz = 64;
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    /*
     * Replicate()的逆向操作。尝试找到幂2步长的重复模式
     */
    for (tmp = esz / 2; tmp >= 2; tmp /= 2) {
        u64 emask = BIT(tmp) - 1;

        if ((imm & emask) != ((imm >> tmp) & emask)) break;

        esz = tmp;
        mask = emask;
    }

    /* 只有在编码64位值时才设置N */
    n = esz == 64;

    /* 将imm裁剪到元素大小 */
    imm &= mask;

    /* 计算需要编码的1的数量 */
    ones = hweight64(imm);

    /*
     * imms设置为(ones - 1)，前缀是一串1和一个0（如果适合的话）
     * 限制为6位
     */
    imms = ones - 1;
    imms |= 0xf << ffs(esz);
    imms &= BIT(6) - 1;

    /* 计算旋转量 */
    if (range_of_ones(imm)) {
        /*
         * 模式：0..01..10..0
         * 计算需要多少旋转来右对齐
         */
        ror = __ffs64(imm);
    } else {
        /*
         * 模式：0..01..10..01..1
         * 用1填充未使用的高位，检查结果是否为有效立即数
         * （全1且有连续的0范围）
         */
        imm |= ~mask;
        if (!range_of_ones(~imm)) return AARCH64_BREAK_FAULT;

        /*
         * 计算旋转量以获得连续的1集合，第一位设置在位置0
         */
        ror = fls(~imm);
    }

    /*
     * immr是我们需要旋转回原始1集合的位数
     * 注意这是相对于元素大小的...
     */
    immr = (esz - ror) % esz;

    insn = aarch64_insn_encode_immediate(AARCH64_INSN_IMM_N, insn, n);
    insn = aarch64_insn_encode_immediate(AARCH64_INSN_IMM_R, insn, immr);
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_S, insn, imms);
}

/**
 * @brief 生成逻辑立即数指令
 * @details 生成AND、ORR、EOR等逻辑立即数指令
 * @param type 逻辑操作类型
 * @param variant 指令变体（32位或64位）
 * @param Rn 源寄存器
 * @param Rd 目标寄存器
 * @param imm 立即数值
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_logical_immediate(enum aarch64_insn_logic_type type, enum aarch64_insn_variant variant,
                                       enum aarch64_insn_register Rn, enum aarch64_insn_register Rd, u64 imm)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_LOGIC_AND:           // AND立即数指令
        insn = aarch64_insn_get_and_imm_value();
        break;
    case AARCH64_INSN_LOGIC_ORR:           // ORR立即数指令
        insn = aarch64_insn_get_orr_imm_value();
        break;
    case AARCH64_INSN_LOGIC_EOR:           // EOR立即数指令
        insn = aarch64_insn_get_eor_imm_value();
        break;
    case AARCH64_INSN_LOGIC_AND_SETFLAGS:  // ANDS立即数指令（设置标志位）
        insn = aarch64_insn_get_ands_imm_value();
        break;
    default:
        fprintf(stdout, "%s: unknown logical encoding %d\n", __func__, type);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, Rd);
    // 编码源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, Rn);
    // 编码立即数
    return aarch64_encode_immediate(imm, variant, insn);
}

/**
 * @brief 生成位提取指令
 * @details 生成EXTR指令，从两个寄存器中提取连续位
 * @param variant 指令变体（32位或64位）
 * @param Rm 第二源寄存器
 * @param Rn 第一源寄存器
 * @param Rd 目标寄存器
 * @param lsb 最低有效位位置
 * @return 生成的指令编码，失败返回AARCH64_BREAK_FAULT
 */
u32 aarch64_insn_gen_extr(enum aarch64_insn_variant variant, enum aarch64_insn_register Rm,
                          enum aarch64_insn_register Rn, enum aarch64_insn_register Rd, u8 lsb)
{
    u32 insn;

    insn = aarch64_insn_get_extr_value();

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:        // 32位变体
        if (lsb > 31) return AARCH64_BREAK_FAULT;  // LSB不能超过31
        break;
    case AARCH64_INSN_VARIANT_64BIT:        // 64位变体
        if (lsb > 63) return AARCH64_BREAK_FAULT;  // LSB不能超过63
        insn |= AARCH64_INSN_SF_BIT;        // 设置SF位
        insn = aarch64_insn_encode_immediate(AARCH64_INSN_IMM_N, insn, 1);  // 设置N位
        break;
    default:
        fprintf(stdout, "%s: unknown variant encoding %d\n", __func__, variant);
        return AARCH64_BREAK_FAULT;
    }

    // 编码LSB位置到S字段
    insn = aarch64_insn_encode_immediate(AARCH64_INSN_IMM_S, insn, lsb);
    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, Rd);
    // 编码第一源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, Rn);
    // 编码第二源寄存器
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, Rm);
}