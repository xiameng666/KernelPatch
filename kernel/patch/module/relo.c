// ARM64 ELF重定位处理模块
// 处理ARM64架构下的各种ELF重定位类型

#include <linux/printk.h>
#include <linux/elf.h>
#include <uapi/linux/elf.h>
#include <asm/elf.h>
#include <kpmalloc.h>
#include <linux/err.h>

#include "insn.h"

// ARM64指令立即数定义
#define AARCH64_INSN_IMM_MOVNZ AARCH64_INSN_IMM_MAX
#define AARCH64_INSN_IMM_MOVK AARCH64_INSN_IMM_16

// 字节序转换宏
#define le32_to_cpu(x) (x)
#define cpu_to_le32(x) (x)

// ARM64重定位操作类型枚举
enum aarch64_reloc_op
{
    RELOC_OP_NONE,  // 无操作
    RELOC_OP_ABS,   // 绝对地址
    RELOC_OP_PREL,  // 相对地址
    RELOC_OP_PAGE,  // 页面地址
};

// 执行重定位操作
static u64 do_reloc(enum aarch64_reloc_op reloc_op, void *place, u64 val)
{
    switch (reloc_op) {
    case RELOC_OP_ABS:   // 绝对地址：直接返回值
        return val;
    case RELOC_OP_PREL:  // 相对地址：目标地址减去当前位置
        return val - (u64)place;
    case RELOC_OP_PAGE:  // 页面地址：页面对齐的地址差
        return (val & ~0xfff) - ((u64)place & ~0xfff);
    case RELOC_OP_NONE:  // 无操作
        return 0;
    }

    pr_err("do_reloc: unknown relocation operation %d\n", reloc_op);
    return 0;
}

// 重定位数据
static int reloc_data(enum aarch64_reloc_op op, void *place, u64 val, int len)
{
    u64 imm_mask = (1 << len) - 1;
    s64 sval = do_reloc(op, place, val);

    switch (len) {
    case 16:  // 16位数据
        *(s16 *)place = sval;
        break;
    case 32:  // 32位数据
        *(s32 *)place = sval;
        break;
    case 64:  // 64位数据
        *(s64 *)place = sval;
        break;
    default:
        pr_err("Invalid length (%d) for data relocation\n", len);
        return 0;
    }
    /*
     * 提取高位值（包括符号位）并移位到第0位
     */
    sval = (s64)(sval & ~(imm_mask >> 1)) >> (len - 1);

    /*
     * 如果值无法用len位表示则发生溢出
     * （即底部len位不是符号扩展且顶部位不全为零）
     */
    if ((u64)(sval + 1) > 2) return -ERANGE;

    return 0;
}

// 重定位MOVW指令
static int reloc_insn_movw(enum aarch64_reloc_op op, void *place, u64 val, int lsb, enum aarch64_insn_imm_type imm_type)
{
    u64 imm, limit = 0;
    s64 sval;
    u32 insn = le32_to_cpu(*(u32 *)place);

    sval = do_reloc(op, place, val);
    sval >>= lsb;
    imm = sval & 0xffff;

    if (imm_type == AARCH64_INSN_IMM_MOVNZ) {
        /*
         * 对于有符号MOVW重定位，需要根据立即数是否小于零
         * 来操作指令编码
         */
        insn &= ~(3 << 29);
        if ((s64)imm >= 0) {
            // >=0: 将指令设置为MOVZ（操作码10b）
            insn |= 2 << 29;
        } else {
            /*
             * <0: 将指令设置为MOVN（操作码00b）
             * 由于已经屏蔽了操作码，只需要反转新的立即数字段
             */
            imm = ~imm;
        }
        imm_type = AARCH64_INSN_IMM_MOVK;
    }

    // 用新编码更新指令
    insn = aarch64_insn_encode_immediate(imm_type, insn, imm);
    *(u32 *)place = cpu_to_le32(insn);

    // 移出立即数字段
    sval >>= 16;

    /*
     * 对于无符号立即数，溢出检查很简单
     * 对于有符号立即数，符号位实际上是字段最高有效位之后的位
     * AARCH64_INSN_IMM_16立即数类型是无符号的
     */
    if (imm_type != AARCH64_INSN_IMM_16) {
        sval++;
        limit++;
    }

    // 根据立即数的符号检查高位
    if ((u64)sval > limit) return -ERANGE;

    return 0;
}

/**
 * 重定位指令立即数
 * 为指令的立即数字段应用重定位
 * @param op 重定位操作类型
 * @param place 重定位位置
 * @param val 重定位值
 * @param lsb 最低有效位偏移
 * @param len 立即数位长度
 * @param imm_type 立即数类型
 * @return 成功返回0，溢出返回-ERANGE
 */
static int reloc_insn_imm(enum aarch64_reloc_op op, void *place, u64 val, int lsb, int len,
                          enum aarch64_insn_imm_type imm_type)
{
    u64 imm, imm_mask;
    s64 sval;
    u32 insn = le32_to_cpu(*(u32 *)place);

    // 计算重定位值
    sval = do_reloc(op, place, val);
    sval >>= lsb;
    
    // 提取值位并移位到第0位
    imm_mask = (BIT(lsb + len) - 1) >> lsb;
    imm = sval & imm_mask;
    
    // 更新指令的立即数字段
    insn = aarch64_insn_encode_immediate(imm_type, insn, imm);
    *(u32 *)place = cpu_to_le32(insn);
    
    /*
     * 提取高位值（包括符号位）并移位到第0位
     */
    sval = (s64)(sval & ~(imm_mask >> 1)) >> (len - 1);
    
    /*
     * 如果高位不全等于值的符号位则发生溢出
     */
    if ((u64)(sval + 1) >= 2) return -ERANGE;

    return 0;
}

/**
 * 应用ELF重定位（不带加数版本）
 * 处理不带加数的重定位项，当前为空实现
 * @param sechdrs 节头表
 * @param strtab 字符串表
 * @param symindex 符号表索引
 * @param relsec 重定位节索引
 * @param me 模块结构
 * @return 始终返回0
 */
int apply_relocate(Elf64_Shdr *sechdrs, const char *strtab, unsigned int symindex, unsigned int relsec,
                   struct module *me)
{
    return 0;
};

/**
 * 应用ELF重定位（带加数版本）
 * 处理ARM64架构的各种ELF重定位类型
 * @param sechdrs 节头表
 * @param strtab 字符串表  
 * @param symindex 符号表索引
 * @param relsec 重定位节索引
 * @param me 模块结构
 * @return 成功返回0，失败返回负数错误码
 */
int apply_relocate_add(Elf64_Shdr *sechdrs, const char *strtab, unsigned int symindex, unsigned int relsec,
                       struct module *me)
{
    unsigned int i;
    int ovf;
    bool overflow_check;
    Elf64_Sym *sym;
    void *loc;
    u64 val;
    Elf64_Rela *rel = (void *)sechdrs[relsec].sh_addr;

    // 遍历所有重定位项
    for (i = 0; i < sechdrs[relsec].sh_size / sizeof(*rel); i++) {
        // loc对应AArch64 ELF文档中的P
        loc = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr + rel[i].r_offset;
        // sym是引用的ELF符号
        sym = (Elf64_Sym *)sechdrs[symindex].sh_addr + ELF64_R_SYM(rel[i].r_info);
        // val对应AArch64 ELF文档中的(S + A)
        val = sym->st_value + rel[i].r_addend;

        overflow_check = true;

        // 执行静态重定位
        switch (ELF64_R_TYPE(rel[i].r_info)) {
        // 空重定位
        case R_ARM_NONE:
        case R_AARCH64_NONE:
            ovf = 0;
            break;
            
        // 数据重定位
        case R_AARCH64_ABS64:  // 64位绝对地址
            overflow_check = false;
            ovf = reloc_data(RELOC_OP_ABS, loc, val, 64);
            break;
        case R_AARCH64_ABS32:  // 32位绝对地址
            ovf = reloc_data(RELOC_OP_ABS, loc, val, 32);
            break;
        case R_AARCH64_ABS16:  // 16位绝对地址
            ovf = reloc_data(RELOC_OP_ABS, loc, val, 16);
            break;
        case R_AARCH64_PREL64:  // 64位相对地址
            overflow_check = false;
            ovf = reloc_data(RELOC_OP_PREL, loc, val, 64);
            break;
        case R_AARCH64_PREL32:  // 32位相对地址
            ovf = reloc_data(RELOC_OP_PREL, loc, val, 32);
            break;
        case R_AARCH64_PREL16:  // 16位相对地址
            ovf = reloc_data(RELOC_OP_PREL, loc, val, 16);
            break;

        // MOVW指令重定位
        case R_AARCH64_MOVW_UABS_G0_NC:  // 无符号绝对地址G0位，不检查溢出
            overflow_check = false;
        case R_AARCH64_MOVW_UABS_G0:  // 无符号绝对地址G0位
            ovf = reloc_insn_movw(RELOC_OP_ABS, loc, val, 0, AARCH64_INSN_IMM_16);
            break;
        case R_AARCH64_MOVW_UABS_G1_NC:  // 无符号绝对地址G1位，不检查溢出
            overflow_check = false;
        case R_AARCH64_MOVW_UABS_G1:  // 无符号绝对地址G1位
            ovf = reloc_insn_movw(RELOC_OP_ABS, loc, val, 16, AARCH64_INSN_IMM_16);
            break;
        case R_AARCH64_MOVW_UABS_G2_NC:  // 无符号绝对地址G2位，不检查溢出
            overflow_check = false;
        case R_AARCH64_MOVW_UABS_G2:  // 无符号绝对地址G2位
            ovf = reloc_insn_movw(RELOC_OP_ABS, loc, val, 32, AARCH64_INSN_IMM_16);
            break;
        case R_AARCH64_MOVW_UABS_G3:  // 无符号绝对地址G3位
            // 使用高位，不会溢出
            overflow_check = false;
            ovf = reloc_insn_movw(RELOC_OP_ABS, loc, val, 48, AARCH64_INSN_IMM_16);
            break;
        case R_AARCH64_MOVW_SABS_G0:  // 有符号绝对地址G0位
            ovf = reloc_insn_movw(RELOC_OP_ABS, loc, val, 0, AARCH64_INSN_IMM_MOVNZ);
            break;
        case R_AARCH64_MOVW_SABS_G1:  // 有符号绝对地址G1位
            ovf = reloc_insn_movw(RELOC_OP_ABS, loc, val, 16, AARCH64_INSN_IMM_MOVNZ);
            break;
        case R_AARCH64_MOVW_SABS_G2:  // 有符号绝对地址G2位
            ovf = reloc_insn_movw(RELOC_OP_ABS, loc, val, 32, AARCH64_INSN_IMM_MOVNZ);
            break;
        case R_AARCH64_MOVW_PREL_G0_NC:  // 相对地址G0位，不检查溢出
            overflow_check = false;
            ovf = reloc_insn_movw(RELOC_OP_PREL, loc, val, 0, AARCH64_INSN_IMM_MOVK);
            break;
        case R_AARCH64_MOVW_PREL_G0:  // 相对地址G0位
            ovf = reloc_insn_movw(RELOC_OP_PREL, loc, val, 0, AARCH64_INSN_IMM_MOVNZ);
            break;
        case R_AARCH64_MOVW_PREL_G1_NC:  // 相对地址G1位，不检查溢出
            overflow_check = false;
            ovf = reloc_insn_movw(RELOC_OP_PREL, loc, val, 16, AARCH64_INSN_IMM_MOVK);
            break;
        case R_AARCH64_MOVW_PREL_G1:  // 相对地址G1位
            ovf = reloc_insn_movw(RELOC_OP_PREL, loc, val, 16, AARCH64_INSN_IMM_MOVNZ);
            break;
        case R_AARCH64_MOVW_PREL_G2_NC:  // 相对地址G2位，不检查溢出
            overflow_check = false;
            ovf = reloc_insn_movw(RELOC_OP_PREL, loc, val, 32, AARCH64_INSN_IMM_MOVK);
            break;
        case R_AARCH64_MOVW_PREL_G2:  // 相对地址G2位
            ovf = reloc_insn_movw(RELOC_OP_PREL, loc, val, 32, AARCH64_INSN_IMM_MOVNZ);
            break;
        case R_AARCH64_MOVW_PREL_G3:  // 相对地址G3位
            // 使用高位，不会溢出
            overflow_check = false;
            ovf = reloc_insn_movw(RELOC_OP_PREL, loc, val, 48, AARCH64_INSN_IMM_MOVNZ);
            break;
            
        // 立即数指令重定位
        case R_AARCH64_LD_PREL_LO19:  // 加载指令19位相对地址
            ovf = reloc_insn_imm(RELOC_OP_PREL, loc, val, 2, 19, AARCH64_INSN_IMM_19);
            break;
        case R_AARCH64_ADR_PREL_LO21:  // ADR指令21位相对地址
            ovf = reloc_insn_imm(RELOC_OP_PREL, loc, val, 0, 21, AARCH64_INSN_IMM_ADR);
            break;
        case R_AARCH64_ADR_PREL_PG_HI21_NC:  // ADRP指令高21位，不检查溢出
            overflow_check = false;
        case R_AARCH64_ADR_PREL_PG_HI21:  // ADRP指令高21位
            ovf = reloc_insn_imm(RELOC_OP_PAGE, loc, val, 12, 21, AARCH64_INSN_IMM_ADR);
            break;
        case R_AARCH64_ADD_ABS_LO12_NC:  // ADD指令低12位绝对地址，不检查溢出
        case R_AARCH64_LDST8_ABS_LO12_NC:  // 8位加载存储指令低12位
            overflow_check = false;
            ovf = reloc_insn_imm(RELOC_OP_ABS, loc, val, 0, 12, AARCH64_INSN_IMM_12);
            break;
        case R_AARCH64_LDST16_ABS_LO12_NC:  // 16位加载存储指令低12位
            overflow_check = false;
            ovf = reloc_insn_imm(RELOC_OP_ABS, loc, val, 1, 11, AARCH64_INSN_IMM_12);
            break;
        case R_AARCH64_LDST32_ABS_LO12_NC:  // 32位加载存储指令低12位
            overflow_check = false;
            ovf = reloc_insn_imm(RELOC_OP_ABS, loc, val, 2, 10, AARCH64_INSN_IMM_12);
            break;
        case R_AARCH64_LDST64_ABS_LO12_NC:  // 64位加载存储指令低12位
            overflow_check = false;
            ovf = reloc_insn_imm(RELOC_OP_ABS, loc, val, 3, 9, AARCH64_INSN_IMM_12);
            break;
        case R_AARCH64_LDST128_ABS_LO12_NC:  // 128位加载存储指令低12位
            overflow_check = false;
            ovf = reloc_insn_imm(RELOC_OP_ABS, loc, val, 4, 8, AARCH64_INSN_IMM_12);
            break;
        case R_AARCH64_TSTBR14:  // 测试分支指令14位
            ovf = reloc_insn_imm(RELOC_OP_PREL, loc, val, 2, 14, AARCH64_INSN_IMM_14);
            break;
        case R_AARCH64_CONDBR19:  // 条件分支指令19位
            ovf = reloc_insn_imm(RELOC_OP_PREL, loc, val, 2, 19, AARCH64_INSN_IMM_19);
            break;
        case R_AARCH64_JUMP26:  // 无条件跳转指令26位
        case R_AARCH64_CALL26:  // 函数调用指令26位
            ovf = reloc_insn_imm(RELOC_OP_PREL, loc, val, 2, 26, AARCH64_INSN_IMM_26);
            break;
        default:
            pr_err("unsupported RELA relocation: %llu\n", ELF64_R_TYPE(rel[i].r_info));
            return -ENOEXEC;
        }

        // 检查溢出
        if (overflow_check && ovf == -ERANGE) goto overflow;
    }
    return 0;
    
overflow:
    pr_err("overflow in relocation type %d val %llx\n", (int)ELF64_R_TYPE(rel[i].r_info), val);
    return -ENOEXEC;
}