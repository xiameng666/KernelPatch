// ARM64指令处理头文件 - 定义ARM64指令编码、解码和生成相关的数据结构和函数声明
#ifndef __ASM_INSN_H
#define __ASM_INSN_H

#include <ktypes.h>

/* A64指令总是32位长度 */
#define AARCH64_INSN_SIZE 4

/*
 * ARM架构参考手册ARMv8 Profile-A, Issue A.a
 * 第C3.1节 "A64指令编码索引":
 * AArch64主编码表
 *  位位置
 *   28 27 26 25	编码组
 *   0  0  -  -		未分配
 *   1  0  0  -		数据处理，立即数
 *   1  0  1  -		分支，异常生成和系统指令
 *   -  1  -  0		加载和存储
 *   -  1  0  1		数据处理 - 寄存器
 *   0  1  1  1		数据处理 - SIMD和浮点
 *   1  1  1  1		数据处理 - SIMD和浮点
 * "-" 表示"不关心"
 */
// ARM64指令编码类别枚举
enum aarch64_insn_encoding_class
{
    AARCH64_INSN_CLS_UNKNOWN,   /* 未分配 */
    AARCH64_INSN_CLS_DP_IMM,    /* 数据处理 - 立即数 */
    AARCH64_INSN_CLS_DP_REG,    /* 数据处理 - 寄存器 */
    AARCH64_INSN_CLS_DP_FPSIMD, /* 数据处理 - SIMD和浮点 */
    AARCH64_INSN_CLS_LDST,      /* 加载和存储 */
    AARCH64_INSN_CLS_BR_SYS,    /* 分支，异常生成和系统指令 */
};

// ARM64指令提示操作枚举
enum aarch64_insn_hint_op
{
    AARCH64_INSN_HINT_NOP = 0x0 << 5,    // 空操作
    AARCH64_INSN_HINT_YIELD = 0x1 << 5,  // 让出处理器
    AARCH64_INSN_HINT_WFE = 0x2 << 5,    // 等待事件
    AARCH64_INSN_HINT_WFI = 0x3 << 5,    // 等待中断
    AARCH64_INSN_HINT_SEV = 0x4 << 5,    // 发送事件
    AARCH64_INSN_HINT_SEVL = 0x5 << 5,   // 发送本地事件
};

// ARM64指令立即数类型枚举
enum aarch64_insn_imm_type
{
    AARCH64_INSN_IMM_ADR,  // ADR指令立即数
    AARCH64_INSN_IMM_26,   // 26位立即数
    AARCH64_INSN_IMM_19,   // 19位立即数
    AARCH64_INSN_IMM_16,   // 16位立即数
    AARCH64_INSN_IMM_14,   // 14位立即数
    AARCH64_INSN_IMM_12,   // 12位立即数
    AARCH64_INSN_IMM_9,    // 9位立即数
    AARCH64_INSN_IMM_7,    // 7位立即数
    AARCH64_INSN_IMM_6,    // 6位立即数
    AARCH64_INSN_IMM_S,    // S立即数
    AARCH64_INSN_IMM_R,    // R立即数
    AARCH64_INSN_IMM_MAX   // 最大立即数类型
};

// ARM64指令寄存器类型枚举
enum aarch64_insn_register_type
{
    AARCH64_INSN_REGTYPE_RT,   // 目标寄存器
    AARCH64_INSN_REGTYPE_RN,   // 基址寄存器
    AARCH64_INSN_REGTYPE_RT2,  // 第二目标寄存器
    AARCH64_INSN_REGTYPE_RM,   // 修改寄存器
    AARCH64_INSN_REGTYPE_RD,   // 目的寄存器
    AARCH64_INSN_REGTYPE_RA,   // 累加寄存器
};

// ARM64通用寄存器枚举（X0-X30，SP，ZR）
enum aarch64_insn_register
{
    AARCH64_INSN_REG_0 = 0,
    AARCH64_INSN_REG_1 = 1,
    AARCH64_INSN_REG_2 = 2,
    AARCH64_INSN_REG_3 = 3,
    AARCH64_INSN_REG_4 = 4,
    AARCH64_INSN_REG_5 = 5,
    AARCH64_INSN_REG_6 = 6,
    AARCH64_INSN_REG_7 = 7,
    AARCH64_INSN_REG_8 = 8,
    AARCH64_INSN_REG_9 = 9,
    AARCH64_INSN_REG_10 = 10,
    AARCH64_INSN_REG_11 = 11,
    AARCH64_INSN_REG_12 = 12,
    AARCH64_INSN_REG_13 = 13,
    AARCH64_INSN_REG_14 = 14,
    AARCH64_INSN_REG_15 = 15,
    AARCH64_INSN_REG_16 = 16,
    AARCH64_INSN_REG_17 = 17,
    AARCH64_INSN_REG_18 = 18,
    AARCH64_INSN_REG_19 = 19,
    AARCH64_INSN_REG_20 = 20,
    AARCH64_INSN_REG_21 = 21,
    AARCH64_INSN_REG_22 = 22,
    AARCH64_INSN_REG_23 = 23,
    AARCH64_INSN_REG_24 = 24,
    AARCH64_INSN_REG_25 = 25,
    AARCH64_INSN_REG_26 = 26,
    AARCH64_INSN_REG_27 = 27,
    AARCH64_INSN_REG_28 = 28,
    AARCH64_INSN_REG_29 = 29,
    AARCH64_INSN_REG_FP = 29, /* 帧指针 */
    AARCH64_INSN_REG_30 = 30,
    AARCH64_INSN_REG_LR = 30, /* 链接寄存器 */
    AARCH64_INSN_REG_ZR = 31, /* 零寄存器：作为源寄存器 */
    AARCH64_INSN_REG_SP = 31  /* 堆栈指针：作为加载/存储基址寄存器 */
};

// ARM64指令变体枚举（32位/64位）
enum aarch64_insn_variant
{
    AARCH64_INSN_VARIANT_32BIT,  // 32位指令变体
    AARCH64_INSN_VARIANT_64BIT   // 64位指令变体
};

// ARM64条件码枚举
enum aarch64_insn_condition
{
    AARCH64_INSN_COND_EQ = 0x0, /* 等于 == */
    AARCH64_INSN_COND_NE = 0x1, /* 不等于 != */
    AARCH64_INSN_COND_CS = 0x2, /* 无符号大于等于 >= */
    AARCH64_INSN_COND_CC = 0x3, /* 无符号小于 < */
    AARCH64_INSN_COND_MI = 0x4, /* 负数 < 0 */
    AARCH64_INSN_COND_PL = 0x5, /* 正数或零 >= 0 */
    AARCH64_INSN_COND_VS = 0x6, /* 溢出 */
    AARCH64_INSN_COND_VC = 0x7, /* 无溢出 */
    AARCH64_INSN_COND_HI = 0x8, /* 无符号大于 > */
    AARCH64_INSN_COND_LS = 0x9, /* 无符号小于等于 <= */
    AARCH64_INSN_COND_GE = 0xa, /* 有符号大于等于 >= */
    AARCH64_INSN_COND_LT = 0xb, /* 有符号小于 < */
    AARCH64_INSN_COND_GT = 0xc, /* 有符号大于 > */
    AARCH64_INSN_COND_LE = 0xd, /* 有符号小于等于 <= */
    AARCH64_INSN_COND_AL = 0xe, /* 总是 */
};

// ARM64分支指令类型枚举
enum aarch64_insn_branch_type
{
    AARCH64_INSN_BRANCH_NOLINK,      // 无链接分支（简单跳转）
    AARCH64_INSN_BRANCH_LINK,        // 链接分支（调用函数）
    AARCH64_INSN_BRANCH_RETURN,      // 返回分支
    AARCH64_INSN_BRANCH_COMP_ZERO,   // 比较零分支
    AARCH64_INSN_BRANCH_COMP_NONZERO, // 比较非零分支
};

// ARM64数据大小类型枚举
enum aarch64_insn_size_type
{
    AARCH64_INSN_SIZE_8,  // 8位数据
    AARCH64_INSN_SIZE_16, // 16位数据
    AARCH64_INSN_SIZE_32, // 32位数据
    AARCH64_INSN_SIZE_64, // 64位数据
};

// ARM64加载/存储指令类型枚举
enum aarch64_insn_ldst_type
{
    AARCH64_INSN_LDST_LOAD_REG_OFFSET,       // 加载寄存器偏移
    AARCH64_INSN_LDST_STORE_REG_OFFSET,      // 存储寄存器偏移
    AARCH64_INSN_LDST_LOAD_PAIR_PRE_INDEX,   // 加载寄存器对预索引
    AARCH64_INSN_LDST_STORE_PAIR_PRE_INDEX,  // 存储寄存器对预索引
    AARCH64_INSN_LDST_LOAD_PAIR_POST_INDEX,  // 加载寄存器对后索引
    AARCH64_INSN_LDST_STORE_PAIR_POST_INDEX, // 存储寄存器对后索引
};

// ARM64加法/减法指令类型枚举
enum aarch64_insn_adsb_type
{
    AARCH64_INSN_ADSB_ADD,          // 加法
    AARCH64_INSN_ADSB_SUB,          // 减法
    AARCH64_INSN_ADSB_ADD_SETFLAGS, // 加法并设置标志位
    AARCH64_INSN_ADSB_SUB_SETFLAGS  // 减法并设置标志位
};

// ARM64移动宽数据指令类型枚举
enum aarch64_insn_movewide_type
{
    AARCH64_INSN_MOVEWIDE_ZERO,    // 移动并清零其他位
    AARCH64_INSN_MOVEWIDE_KEEP,    // 移动并保持其他位
    AARCH64_INSN_MOVEWIDE_INVERSE  // 移动并反转其他位
};

// ARM64位域操作指令类型枚举
enum aarch64_insn_bitfield_type
{
    AARCH64_INSN_BITFIELD_MOVE,          // 位域移动
    AARCH64_INSN_BITFIELD_MOVE_UNSIGNED, // 无符号位域移动
    AARCH64_INSN_BITFIELD_MOVE_SIGNED    // 有符号位域移动
};

// ARM64单操作数数据处理指令类型枚举
enum aarch64_insn_data1_type
{
    AARCH64_INSN_DATA1_REVERSE_16, // 16位字节反转
    AARCH64_INSN_DATA1_REVERSE_32, // 32位字节反转
    AARCH64_INSN_DATA1_REVERSE_64, // 64位字节反转
};

// ARM64双操作数数据处理指令类型枚举
enum aarch64_insn_data2_type
{
    AARCH64_INSN_DATA2_UDIV, // 无符号除法
    AARCH64_INSN_DATA2_SDIV, // 有符号除法
    AARCH64_INSN_DATA2_LSLV, // 变量逻辑左移
    AARCH64_INSN_DATA2_LSRV, // 变量逻辑右移
    AARCH64_INSN_DATA2_ASRV, // 变量算术右移
    AARCH64_INSN_DATA2_RORV, // 变量循环右移
};

// ARM64三操作数数据处理指令类型枚举
enum aarch64_insn_data3_type
{
    AARCH64_INSN_DATA3_MADD, // 乘加运算
    AARCH64_INSN_DATA3_MSUB, // 乘减运算
};

// ARM64逻辑运算指令类型枚举
enum aarch64_insn_logic_type
{
    AARCH64_INSN_LOGIC_AND,          // 逻辑与
    AARCH64_INSN_LOGIC_BIC,          // 位清除（AND NOT）
    AARCH64_INSN_LOGIC_ORR,          // 逻辑或
    AARCH64_INSN_LOGIC_ORN,          // 逻辑或非
    AARCH64_INSN_LOGIC_EOR,          // 逻辑异或
    AARCH64_INSN_LOGIC_EON,          // 逻辑异或非
    AARCH64_INSN_LOGIC_AND_SETFLAGS, // 逻辑与并设置标志位
    AARCH64_INSN_LOGIC_BIC_SETFLAGS  // 位清除并设置标志位
};

// ARM64指令操作宏定义
// 用于生成指令检查和值获取函数
#define __AARCH64_INSN_FUNCS(abbr, mask, val)                        \
    static __always_inline bool aarch64_insn_is_##abbr(u32 code)     \
    {                                                                \
        return (code & (mask)) == (val);                             \
    }                                                                \
    static __always_inline u32 aarch64_insn_get_##abbr##_value(void) \
    {                                                                \
        return (val);                                                \
    }

// 定义各种ARM64指令的检查函数和值获取函数
__AARCH64_INSN_FUNCS(str_reg, 0x3FE0EC00, 0x38206800)  // 存储寄存器（寄存器偏移）
__AARCH64_INSN_FUNCS(ldr_reg, 0x3FE0EC00, 0x38606800)  // 加载寄存器（寄存器偏移）
__AARCH64_INSN_FUNCS(stp_post, 0x7FC00000, 0x28800000) // 存储寄存器对（后索引）
__AARCH64_INSN_FUNCS(ldp_post, 0x7FC00000, 0x28C00000) // 加载寄存器对（后索引）
__AARCH64_INSN_FUNCS(stp_pre, 0x7FC00000, 0x29800000)  // 存储寄存器对（预索引）
__AARCH64_INSN_FUNCS(ldp_pre, 0x7FC00000, 0x29C00000)  // 加载寄存器对（预索引）
__AARCH64_INSN_FUNCS(add_imm, 0x7F000000, 0x11000000)  // 立即数加法
__AARCH64_INSN_FUNCS(adds_imm, 0x7F000000, 0x31000000) // 立即数加法（设置标志位）
__AARCH64_INSN_FUNCS(sub_imm, 0x7F000000, 0x51000000)  // 立即数减法
__AARCH64_INSN_FUNCS(subs_imm, 0x7F000000, 0x71000000) // 立即数减法（设置标志位）
__AARCH64_INSN_FUNCS(movn, 0x7F800000, 0x12800000)     // 移动取反
__AARCH64_INSN_FUNCS(sbfm, 0x7F800000, 0x13000000)     // 有符号位域移动
__AARCH64_INSN_FUNCS(bfm, 0x7F800000, 0x33000000)      // 位域移动
__AARCH64_INSN_FUNCS(movz, 0x7F800000, 0x52800000)     // 移动清零
__AARCH64_INSN_FUNCS(ubfm, 0x7F800000, 0x53000000)     // 无符号位域移动
__AARCH64_INSN_FUNCS(movk, 0x7F800000, 0x72800000)     // 移动保持
__AARCH64_INSN_FUNCS(add, 0x7F200000, 0x0B000000)      // 寄存器加法
__AARCH64_INSN_FUNCS(adds, 0x7F200000, 0x2B000000)     // 寄存器加法（设置标志位）
__AARCH64_INSN_FUNCS(sub, 0x7F200000, 0x4B000000)      // 寄存器减法
__AARCH64_INSN_FUNCS(subs, 0x7F200000, 0x6B000000)     // 寄存器减法（设置标志位）
__AARCH64_INSN_FUNCS(madd, 0x7FE08000, 0x1B000000)     // 乘加运算
__AARCH64_INSN_FUNCS(msub, 0x7FE08000, 0x1B008000)     // 乘减运算
__AARCH64_INSN_FUNCS(udiv, 0x7FE0FC00, 0x1AC00800)     // 无符号除法
__AARCH64_INSN_FUNCS(sdiv, 0x7FE0FC00, 0x1AC00C00)     // 有符号除法
__AARCH64_INSN_FUNCS(lslv, 0x7FE0FC00, 0x1AC02000)     // 变量逻辑左移
__AARCH64_INSN_FUNCS(lsrv, 0x7FE0FC00, 0x1AC02400)     // 变量逻辑右移
__AARCH64_INSN_FUNCS(asrv, 0x7FE0FC00, 0x1AC02800)     // 变量算术右移
__AARCH64_INSN_FUNCS(rorv, 0x7FE0FC00, 0x1AC02C00)     // 变量循环右移
__AARCH64_INSN_FUNCS(rev16, 0x7FFFFC00, 0x5AC00400)    // 16位字节反转
__AARCH64_INSN_FUNCS(rev32, 0x7FFFFC00, 0x5AC00800)    // 32位字节反转
__AARCH64_INSN_FUNCS(rev64, 0x7FFFFC00, 0x5AC00C00)    // 64位字节反转
__AARCH64_INSN_FUNCS(and, 0x7F200000, 0x0A000000)      // 逻辑与
__AARCH64_INSN_FUNCS(bic, 0x7F200000, 0x0A200000)      // 位清除
__AARCH64_INSN_FUNCS(orr, 0x7F200000, 0x2A000000)      // 逻辑或
__AARCH64_INSN_FUNCS(orn, 0x7F200000, 0x2A200000)      // 逻辑或非
__AARCH64_INSN_FUNCS(eor, 0x7F200000, 0x4A000000)      // 逻辑异或
__AARCH64_INSN_FUNCS(eon, 0x7F200000, 0x4A200000)      // 逻辑异或非
__AARCH64_INSN_FUNCS(ands, 0x7F200000, 0x6A000000)     // 逻辑与（设置标志位）
__AARCH64_INSN_FUNCS(bics, 0x7F200000, 0x6A200000)     // 位清除（设置标志位）
__AARCH64_INSN_FUNCS(b, 0xFC000000, 0x14000000)        // 无条件分支
__AARCH64_INSN_FUNCS(bl, 0xFC000000, 0x94000000)       // 分支链接
__AARCH64_INSN_FUNCS(cbz, 0xFE000000, 0x34000000)      // 比较零分支
__AARCH64_INSN_FUNCS(cbnz, 0xFE000000, 0x35000000)     // 比较非零分支
__AARCH64_INSN_FUNCS(bcond, 0xFF000010, 0x54000000)    // 条件分支
__AARCH64_INSN_FUNCS(svc, 0xFFE0001F, 0xD4000001)      // 系统调用
__AARCH64_INSN_FUNCS(hvc, 0xFFE0001F, 0xD4000002)      // 虚拟化调用
__AARCH64_INSN_FUNCS(smc, 0xFFE0001F, 0xD4000003)      // 安全监控调用
__AARCH64_INSN_FUNCS(brk, 0xFFE0001F, 0xD4200000)      // 软件断点
__AARCH64_INSN_FUNCS(hint, 0xFFFFF01F, 0xD503201F)     // 提示指令
__AARCH64_INSN_FUNCS(br, 0xFFFFFC1F, 0xD61F0000)       // 分支到寄存器
__AARCH64_INSN_FUNCS(blr, 0xFFFFFC1F, 0xD63F0000)      // 分支链接到寄存器
__AARCH64_INSN_FUNCS(ret, 0xFFFFFC1F, 0xD65F0000)      // 返回指令

#undef __AARCH64_INSN_FUNCS

// 检查指令是否为NOP
bool aarch64_insn_is_nop(u32 insn);

// 从指定地址读取指令
void aarch64_insn_read(void *addr, u32 *insnp);
// 向指定地址写入指令
void aarch64_insn_write(void *addr, u32 insn);
// 获取指令的编码类别
enum aarch64_insn_encoding_class aarch64_get_insn_class(u32 insn);
// 编码立即数到指令中
u32 aarch64_insn_encode_immediate(enum aarch64_insn_imm_type type, u32 insn, u64 imm);
// 生成分支立即数指令
u32 aarch64_insn_gen_branch_imm(unsigned long pc, unsigned long addr, enum aarch64_insn_branch_type type);
// 生成比较分支立即数指令
u32 aarch64_insn_gen_comp_branch_imm(unsigned long pc, unsigned long addr, enum aarch64_insn_register reg,
                                     enum aarch64_insn_variant variant, enum aarch64_insn_branch_type type);
// 生成条件分支立即数指令
u32 aarch64_insn_gen_cond_branch_imm(unsigned long pc, unsigned long addr, enum aarch64_insn_condition cond);
// 生成提示指令
u32 aarch64_insn_gen_hint(enum aarch64_insn_hint_op op);
// 生成NOP指令
u32 aarch64_insn_gen_nop(void);
// 生成分支寄存器指令
u32 aarch64_insn_gen_branch_reg(enum aarch64_insn_register reg, enum aarch64_insn_branch_type type);
// 生成加载/存储寄存器指令
u32 aarch64_insn_gen_load_store_reg(enum aarch64_insn_register reg, enum aarch64_insn_register base,
                                    enum aarch64_insn_register offset, enum aarch64_insn_size_type size,
                                    enum aarch64_insn_ldst_type type);
// 生成加载/存储寄存器对指令
u32 aarch64_insn_gen_load_store_pair(enum aarch64_insn_register reg1, enum aarch64_insn_register reg2,
                                     enum aarch64_insn_register base, int offset, enum aarch64_insn_variant variant,
                                     enum aarch64_insn_ldst_type type);
// 生成立即数加法/减法指令
u32 aarch64_insn_gen_add_sub_imm(enum aarch64_insn_register dst, enum aarch64_insn_register src, int imm,
                                 enum aarch64_insn_variant variant, enum aarch64_insn_adsb_type type);
// 生成位域操作指令
u32 aarch64_insn_gen_bitfield(enum aarch64_insn_register dst, enum aarch64_insn_register src, int immr, int imms,
                              enum aarch64_insn_variant variant, enum aarch64_insn_bitfield_type type);
// 生成移动宽数据指令
u32 aarch64_insn_gen_movewide(enum aarch64_insn_register dst, int imm, int shift, enum aarch64_insn_variant variant,
                              enum aarch64_insn_movewide_type type);
// 生成移位寄存器加法/减法指令
u32 aarch64_insn_gen_add_sub_shifted_reg(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                                         enum aarch64_insn_register reg, int shift, enum aarch64_insn_variant variant,
                                         enum aarch64_insn_adsb_type type);
// 生成单操作数数据处理指令
u32 aarch64_insn_gen_data1(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_variant variant, enum aarch64_insn_data1_type type);
// 生成双操作数数据处理指令
u32 aarch64_insn_gen_data2(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_register reg, enum aarch64_insn_variant variant,
                           enum aarch64_insn_data2_type type);
// 生成三操作数数据处理指令
u32 aarch64_insn_gen_data3(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_register reg1, enum aarch64_insn_register reg2,
                           enum aarch64_insn_variant variant, enum aarch64_insn_data3_type type);
// 生成移位寄存器逻辑运算指令
u32 aarch64_insn_gen_logical_shifted_reg(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                                         enum aarch64_insn_register reg, int shift, enum aarch64_insn_variant variant,
                                         enum aarch64_insn_logic_type type);

bool aarch64_insn_hotpatch_safe(u32 old_insn, u32 new_insn);

int aarch64_insn_patch_text_nosync(void *addr, u32 insn);
int aarch64_insn_patch_text_sync(void *addrs[], u32 insns[], int cnt);
int aarch64_insn_patch_text(void *addrs[], u32 insns[], int cnt);

#endif