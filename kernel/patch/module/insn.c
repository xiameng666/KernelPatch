// ARM64指令处理模块实现
// 提供ARM64指令生成、编码、解码和修补功能

#include <linux/kernel.h>
#include <linux/stop_machine.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno.h>
#include <asm/cacheflush.h>
#include <pgtable.h>
#include <cache.h>
#include <linux/panic.h>
#include <asm/atomic.h>

#include <ktypes.h>

#include "insn.h"

// 错误处理宏定义
#define BUG()                                                                  \
    do {                                                                       \
        printk("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        do {                                                                   \
        } while (0);                                                           \
        panic("BUG INSN!");                                                    \
    } while (0)

#define BUG_ON(condition)               \
    do {                                \
        if (unlikely(condition)) BUG(); \
    } while (0)

// 字节序转换宏
#define le32_to_cpu(x) (x)
#define cpu_to_le32(x) (x)

// 内存大小常量定义
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

/*
 * 创建连续位掩码，从位位置@l开始到位置@h结束
 * 例如：GENMASK_ULL(39, 21) 产生64位向量 0x000000ffffe00000
 */
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define GENMASK_ULL(h, l) (((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

/*
 * 用于BRK指令生成的#imm16值
 * kgdb允许的值为0x400 - 0x7ff
 * 0x100: 故意触发错误（保留）
 * 0x400: 动态BRK指令
 * 0x401: 编译时BRK指令
 */
#define FAULT_BRK_IMM 0x100
#define KGDB_DYN_DBG_BRK_IMM 0x400
#define KGDB_COMPILED_DBG_BRK_IMM 0x401

/*
 * BRK指令编码
 * #imm16值应放置在BRK指令的bits[20:5]位置
 */
#define AARCH64_BREAK_MON 0xd4200000

/*
 * 故意触发错误的BRK指令
 * 与kgdb不同，使用未分配处理程序的#imm16值来触发错误
 */
#define AARCH64_BREAK_FAULT (AARCH64_BREAK_MON | (FAULT_BRK_IMM << 5))

// ARM64指令位域定义
#define AARCH64_INSN_SF_BIT BIT(31)  // 64位/32位标志位
#define AARCH64_INSN_N_BIT BIT(22)   // 否定标志位

// 指令编码类别查找表
static int aarch64_insn_encoding_class[] = {
    AARCH64_INSN_CLS_UNKNOWN, AARCH64_INSN_CLS_UNKNOWN, AARCH64_INSN_CLS_UNKNOWN, AARCH64_INSN_CLS_UNKNOWN,
    AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_REG,  AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_FPSIMD,
    AARCH64_INSN_CLS_DP_IMM,  AARCH64_INSN_CLS_DP_IMM,  AARCH64_INSN_CLS_BR_SYS,  AARCH64_INSN_CLS_BR_SYS,
    AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_REG,  AARCH64_INSN_CLS_LDST,    AARCH64_INSN_CLS_DP_FPSIMD,
};

// 获取指令的编码类别
enum aarch64_insn_encoding_class aarch64_get_insn_class(u32 insn)
{
    return aarch64_insn_encoding_class[(insn >> 25) & 0xf];
}

/* NOP是HINT的别名 */
// 检查指令是否为NOP指令
bool aarch64_insn_is_nop(u32 insn)
{
    if (!aarch64_insn_is_hint(insn)) return false;

    switch (insn & 0xFE0) {
    case AARCH64_INSN_HINT_YIELD:
    case AARCH64_INSN_HINT_WFE:
    case AARCH64_INSN_HINT_WFI:
    case AARCH64_INSN_HINT_SEV:
    case AARCH64_INSN_HINT_SEVL:
        return false;
    default:
        return true;
    }
}

// 从指定地址读取指令
void aarch64_insn_read(void *addr, u32 *insnp)
{
    u32 val = *(u32 *)addr;
    *insnp = le32_to_cpu(val);
}

// 向指定地址写入指令
void aarch64_insn_write(void *addr, u32 insn)
{
    insn = cpu_to_le32(insn);
    *(u32 *)addr = le32_to_cpu(insn);
}

// 检查指令是否可以安全热补丁
static bool __aarch64_insn_hotpatch_safe(u32 insn)
{
    if (aarch64_get_insn_class(insn) != AARCH64_INSN_CLS_BR_SYS) return false;

    return aarch64_insn_is_b(insn) || aarch64_insn_is_bl(insn) || aarch64_insn_is_svc(insn) ||
           aarch64_insn_is_hvc(insn) || aarch64_insn_is_smc(insn) || aarch64_insn_is_brk(insn) ||
           aarch64_insn_is_nop(insn);
}

/*
 * ARM Architecture Reference Manual for ARMv8 Profile-A, Issue A.a
 * Section B2.6.5 "Concurrent modification and execution of instructions":
 * Concurrent modification and execution of instructions can lead to the
 * resulting instruction performing any behavior that can be achieved by
 * executing any sequence of instructions that can be executed from the
 * same Exception level, except where the instruction before modification
 * and the instruction after modification is a B, BL, NOP, BKPT, SVC, HVC,
 * or SMC instruction.
 */
bool aarch64_insn_hotpatch_safe(u32 old_insn, u32 new_insn)
{
    return __aarch64_insn_hotpatch_safe(old_insn) && __aarch64_insn_hotpatch_safe(new_insn);
}

int aarch64_insn_patch_text_nosync(void *addr, u32 insn)
{
    u32 *tp = addr;
    int ret;
    if ((uintptr_t)tp & 0x3) return -EINVAL;
    aarch64_insn_write(tp, insn);
    flush_icache_range((uintptr_t)tp, (uintptr_t)tp + AARCH64_INSN_SIZE);
    return ret;
}

struct aarch64_insn_patch
{
    void **text_addrs;
    u32 *new_insns;
    int insn_cnt;
    atomic_t cpu_count;
};

static int aarch64_insn_patch_text_cb(void *arg)
{
    int i, ret = 0;
    struct aarch64_insn_patch *pp = arg;

    /* The first CPU becomes master */
    if (atomic_inc_return(&pp->cpu_count) == 1) {
        for (i = 0; ret == 0 && i < pp->insn_cnt; i++)
            ret = aarch64_insn_patch_text_nosync(pp->text_addrs[i], pp->new_insns[i]);
        /*
		 * aarch64_insn_patch_text_nosync() calls flush_icache_range(),
		 * which ends with "dsb; isb" pair guaranteeing global
		 * visibility.
		 */
        /* Notify other processors with an additional increment. */
        atomic_inc(&pp->cpu_count);
    } else {
        // while (atomic_read(&pp->cpu_count) <= num_online_cpus())
        //     cpu_relax();
        // isb();
    }

    return ret;
}

int aarch64_insn_patch_text_sync(void *addrs[], u32 insns[], int cnt)
{
    struct aarch64_insn_patch patch = {
        .text_addrs = addrs,
        .new_insns = insns,
        .insn_cnt = cnt,
        .cpu_count = ATOMIC_INIT(0),
    };

    if (cnt <= 0) return -EINVAL;

    // return stop_machine(aarch64_insn_patch_text_cb, &patch, cpu_online_mask);
    return stop_machine(aarch64_insn_patch_text_cb, &patch, 0);
}

int aarch64_insn_patch_text(void *addrs[], u32 insns[], int cnt)
{
    int ret;
    u32 insn;

    /* Unsafe to patch multiple instructions without synchronizaiton */
    if (cnt == 1) {
        aarch64_insn_read(addrs[0], &insn);
        if (aarch64_insn_hotpatch_safe(insn, insns[0])) {
            /*
			 * ARMv8 architecture doesn't guarantee all CPUs see
			 * the new instruction after returning from function
			 * aarch64_insn_patch_text_nosync(). So send IPIs to
			 * all other CPUs to achieve instruction
			 * synchronization.
			 */
            ret = aarch64_insn_patch_text_nosync(addrs[0], insns[0]);
            // kick_all_cpus_sync();
            return ret;
        }
    }

    return aarch64_insn_patch_text_sync(addrs, insns, cnt);
}

u32 aarch64_insn_encode_immediate(enum aarch64_insn_imm_type type, u32 insn, u64 imm)
{
    u32 immlo, immhi, lomask, himask, mask;
    int shift;

    switch (type) {
    case AARCH64_INSN_IMM_ADR:
        lomask = 0x3;
        himask = 0x7ffff;
        immlo = imm & lomask;
        imm >>= 2;
        immhi = imm & himask;
        imm = (immlo << 24) | (immhi);
        mask = (lomask << 24) | (himask);
        shift = 5;
        break;
    case AARCH64_INSN_IMM_26:
        mask = BIT(26) - 1;
        shift = 0;
        break;
    case AARCH64_INSN_IMM_19:
        mask = BIT(19) - 1;
        shift = 5;
        break;
    case AARCH64_INSN_IMM_16:
        mask = BIT(16) - 1;
        shift = 5;
        break;
    case AARCH64_INSN_IMM_14:
        mask = BIT(14) - 1;
        shift = 5;
        break;
    case AARCH64_INSN_IMM_12:
        mask = BIT(12) - 1;
        shift = 10;
        break;
    case AARCH64_INSN_IMM_9:
        mask = BIT(9) - 1;
        shift = 12;
        break;
    case AARCH64_INSN_IMM_7:
        mask = BIT(7) - 1;
        shift = 15;
        break;
    case AARCH64_INSN_IMM_6:
    case AARCH64_INSN_IMM_S:
        mask = BIT(6) - 1;
        shift = 10;
        break;
    case AARCH64_INSN_IMM_R:
        mask = BIT(6) - 1;
        shift = 16;
        break;
    default:
        logke("aarch64_insn_encode_immediate: unknown immediate encoding %d\n", type);
        return 0;
    }

    /* Update the immediate field. */
    insn &= ~(mask << shift);
    insn |= (imm & mask) << shift;

    return insn;
}

// 编码寄存器到指令中
static u32 aarch64_insn_encode_register(enum aarch64_insn_register_type type, u32 insn, enum aarch64_insn_register reg)
{
    int shift;

    if (reg < AARCH64_INSN_REG_0 || reg > AARCH64_INSN_REG_SP) {
        logke("%s: unknown register encoding %d\n", __func__, reg);
        return 0;
    }

    switch (type) {
    case AARCH64_INSN_REGTYPE_RT:  // 目标寄存器
    case AARCH64_INSN_REGTYPE_RD:  // 目的寄存器
        shift = 0;
        break;
    case AARCH64_INSN_REGTYPE_RN:  // 第一操作数寄存器
        shift = 5;
        break;
    case AARCH64_INSN_REGTYPE_RT2: // 第二目标寄存器
    case AARCH64_INSN_REGTYPE_RA:  // 累加器寄存器
        shift = 10;
        break;
    case AARCH64_INSN_REGTYPE_RM:  // 第二操作数寄存器
        shift = 16;
        break;
    default:
        logke("%s: unknown register type encoding %d\n", __func__, type);
        return 0;
    }

    insn &= ~(GENMASK(4, 0) << shift);
    insn |= reg << shift;

    return insn;
}

static u32 aarch64_insn_encode_ldst_size(enum aarch64_insn_size_type type, u32 insn)
{
    u32 size;

    switch (type) {
    case AARCH64_INSN_SIZE_8:
        size = 0;
        break;
    case AARCH64_INSN_SIZE_16:
        size = 1;
        break;
    case AARCH64_INSN_SIZE_32:
        size = 2;
        break;
    case AARCH64_INSN_SIZE_64:
        size = 3;
        break;
    default:
        logke("%s: unknown size encoding %d\n", __func__, type);
        return 0;
    }

    insn &= ~GENMASK(31, 30);
    insn |= size << 30;

    return insn;
}

static inline long branch_imm_common(unsigned long pc, unsigned long addr, long range)
{
    long offset;
    /*
	 * PC: A 64-bit Program Counter holding the address of the current
	 * instruction. A64 instructions must be word-aligned.
	 */
    BUG_ON((pc & 0x3) || (addr & 0x3));

    offset = ((long)addr - (long)pc);
    BUG_ON(offset < -range || offset >= range);

    return offset;
}

/**
 * 生成分支立即数指令
 * 根据指定的PC和目标地址生成B或BL指令
 * @param pc 当前指令地址
 * @param addr 目标跳转地址
 * @param type 分支类型（有链接或无链接）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_branch_imm(unsigned long pc, unsigned long addr, enum aarch64_insn_branch_type type)
{
    u32 insn;
    long offset;

    /*
     * B/BL支持[-128M, 128M)偏移范围
     * ARM64虚拟地址布局保证所有内核和模块代码都在+/-128M范围内
     */
    offset = branch_imm_common(pc, addr, SZ_128M);

    switch (type) {
    case AARCH64_INSN_BRANCH_LINK:  // 带链接的分支（函数调用）
        insn = aarch64_insn_get_bl_value();
        break;
    case AARCH64_INSN_BRANCH_NOLINK:  // 无链接分支（普通跳转）
        insn = aarch64_insn_get_b_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码26位立即数，偏移除以4（指令4字节对齐）
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_26, insn, offset >> 2);
}

/**
 * 生成比较并分支立即数指令
 * 生成CBZ或CBNZ指令，用于寄存器值与零的比较分支
 * @param pc 当前指令地址
 * @param addr 目标跳转地址
 * @param reg 比较的寄存器
 * @param variant 指令变体（32位或64位）
 * @param type 分支类型（等于零或不等于零）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_comp_branch_imm(unsigned long pc, unsigned long addr, enum aarch64_insn_register reg,
                                     enum aarch64_insn_variant variant, enum aarch64_insn_branch_type type)
{
    u32 insn;
    long offset;

    // CBZ/CBNZ指令的跳转范围限制为+/-1M
    offset = branch_imm_common(pc, addr, SZ_1M);

    switch (type) {
    case AARCH64_INSN_BRANCH_COMP_ZERO:  // 寄存器值为零时跳转
        insn = aarch64_insn_get_cbz_value();
        break;
    case AARCH64_INSN_BRANCH_COMP_NONZERO:  // 寄存器值非零时跳转
        insn = aarch64_insn_get_cbnz_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作
        insn |= AARCH64_INSN_SF_BIT;
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT, insn, reg);

    // 编码19位立即数偏移
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_19, insn, offset >> 2);
}

/**
 * 生成条件分支立即数指令
 * 生成B.cond指令，根据条件码进行分支
 * @param pc 当前指令地址
 * @param addr 目标跳转地址
 * @param cond 分支条件（EQ、NE、LT等）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_cond_branch_imm(unsigned long pc, unsigned long addr, enum aarch64_insn_condition cond)
{
    u32 insn;
    long offset;

    // 条件分支指令的跳转范围限制为+/-1M
    offset = branch_imm_common(pc, addr, SZ_1M);

    insn = aarch64_insn_get_bcond_value();

    // 验证条件码范围
    BUG_ON(cond < AARCH64_INSN_COND_EQ || cond > AARCH64_INSN_COND_AL);
    insn |= cond;  // 条件码占低4位

    // 编码19位立即数偏移
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_19, insn, offset >> 2);
}

// 生成提示指令
u32 aarch64_insn_gen_hint(enum aarch64_insn_hint_op op)
{
    return aarch64_insn_get_hint_value() | op;
}

// 生成NOP指令
u32 aarch64_insn_gen_nop(void)
{
    return aarch64_insn_gen_hint(AARCH64_INSN_HINT_NOP);
}

// 生成分支寄存器指令
u32 aarch64_insn_gen_branch_reg(enum aarch64_insn_register reg, enum aarch64_insn_branch_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_BRANCH_NOLINK:  // 无链接分支
        insn = aarch64_insn_get_br_value();
        break;
    case AARCH64_INSN_BRANCH_LINK:
        insn = aarch64_insn_get_blr_value();
        break;
    case AARCH64_INSN_BRANCH_RETURN:
        insn = aarch64_insn_get_ret_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, reg);
}

// 生成加载/存储寄存器指令
u32 aarch64_insn_gen_load_store_reg(enum aarch64_insn_register reg, enum aarch64_insn_register base,
                                    enum aarch64_insn_register offset, enum aarch64_insn_size_type size,
                                    enum aarch64_insn_ldst_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_LDST_LOAD_REG_OFFSET:
        insn = aarch64_insn_get_ldr_reg_value();
        break;
    case AARCH64_INSN_LDST_STORE_REG_OFFSET:
        insn = aarch64_insn_get_str_reg_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    insn = aarch64_insn_encode_ldst_size(size, insn);

    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT, insn, reg);

    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, base);

    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, offset);
}

// 生成加载/存储寄存器对指令
u32 aarch64_insn_gen_load_store_pair(enum aarch64_insn_register reg1, enum aarch64_insn_register reg2,
                                     enum aarch64_insn_register base, int offset, enum aarch64_insn_variant variant,
                                     enum aarch64_insn_ldst_type type)
{
    u32 insn;
    int shift;

    switch (type) {
    case AARCH64_INSN_LDST_LOAD_PAIR_PRE_INDEX:
        insn = aarch64_insn_get_ldp_pre_value();
        break;
    case AARCH64_INSN_LDST_STORE_PAIR_PRE_INDEX:
        insn = aarch64_insn_get_stp_pre_value();
        break;
    case AARCH64_INSN_LDST_LOAD_PAIR_POST_INDEX:
        insn = aarch64_insn_get_ldp_post_value();
        break;
    case AARCH64_INSN_LDST_STORE_PAIR_POST_INDEX:
        insn = aarch64_insn_get_stp_post_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:
        /* offset must be multiples of 4 in the range [-256, 252] */
        BUG_ON(offset & 0x3);
        BUG_ON(offset < -256 || offset > 252);
        shift = 2;
        break;
    case AARCH64_INSN_VARIANT_64BIT:
        /* offset must be multiples of 8 in the range [-512, 504] */
        BUG_ON(offset & 0x7);
        BUG_ON(offset < -512 || offset > 504);
        shift = 3;
        insn |= AARCH64_INSN_SF_BIT;
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT, insn, reg1);

    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RT2, insn, reg2);

    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, base);

    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_7, insn, offset >> shift);
}

// 生成立即数加法/减法指令
u32 aarch64_insn_gen_add_sub_imm(enum aarch64_insn_register dst, enum aarch64_insn_register src, int imm,
                                 enum aarch64_insn_variant variant, enum aarch64_insn_adsb_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_ADSB_ADD:
        insn = aarch64_insn_get_add_imm_value();
        break;
    case AARCH64_INSN_ADSB_SUB:
        insn = aarch64_insn_get_sub_imm_value();
        break;
    case AARCH64_INSN_ADSB_ADD_SETFLAGS:
        insn = aarch64_insn_get_adds_imm_value();
        break;
    case AARCH64_INSN_ADSB_SUB_SETFLAGS:
        insn = aarch64_insn_get_subs_imm_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:
        break;
    case AARCH64_INSN_VARIANT_64BIT:
        insn |= AARCH64_INSN_SF_BIT;
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    BUG_ON(imm & ~(SZ_4K - 1));

    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);

    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_12, insn, imm);
}

/**
 * 生成位域操作指令
 * 生成BFM、UBFM、SBFM指令，用于位域提取和插入操作
 * @param dst 目标寄存器
 * @param src 源寄存器
 * @param immr 右旋转立即数
 * @param imms 位数立即数
 * @param variant 指令变体（32位或64位）
 * @param type 位域操作类型（有符号、无符号或普通移动）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_bitfield(enum aarch64_insn_register dst, enum aarch64_insn_register src, int immr, int imms,
                              enum aarch64_insn_variant variant, enum aarch64_insn_bitfield_type type)
{
    u32 insn;
    u32 mask;

    switch (type) {
    case AARCH64_INSN_BITFIELD_MOVE:  // 普通位域移动
        insn = aarch64_insn_get_bfm_value();
        break;
    case AARCH64_INSN_BITFIELD_MOVE_UNSIGNED:  // 无符号位域移动
        insn = aarch64_insn_get_ubfm_value();
        break;
    case AARCH64_INSN_BITFIELD_MOVE_SIGNED:  // 有符号位域移动
        insn = aarch64_insn_get_sbfm_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作，立即数范围0-31
        mask = GENMASK(4, 0);
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作，立即数范围0-63
        insn |= AARCH64_INSN_SF_BIT | AARCH64_INSN_N_BIT;
        mask = GENMASK(5, 0);
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 验证立即数范围
    BUG_ON(immr & ~mask);
    BUG_ON(imms & ~mask);

    // 编码寄存器字段
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);

    // 编码立即数字段
    insn = aarch64_insn_encode_immediate(AARCH64_INSN_IMM_R, insn, immr);
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_S, insn, imms);
}

/**
 * 生成移动宽数据指令
 * 生成MOVZ、MOVK、MOVN指令，用于向寄存器的16位段加载立即数
 * @param dst 目标寄存器
 * @param imm 16位立即数
 * @param shift 左移位数（必须是16的倍数）
 * @param variant 指令变体（32位或64位）
 * @param type 移动类型（清零、保持或反转）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_movewide(enum aarch64_insn_register dst, int imm, int shift, enum aarch64_insn_variant variant,
                              enum aarch64_insn_movewide_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_MOVEWIDE_ZERO:  // 清零其他位，设置指定16位
        insn = aarch64_insn_get_movz_value();
        break;
    case AARCH64_INSN_MOVEWIDE_KEEP:  // 保持其他位，仅设置指定16位
        insn = aarch64_insn_get_movk_value();
        break;
    case AARCH64_INSN_MOVEWIDE_INVERSE:  // 反转立即数后设置
        insn = aarch64_insn_get_movn_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 验证立即数范围（16位）
    BUG_ON(imm & ~(SZ_64K - 1));

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作，允许shift为0或16
        BUG_ON(shift != 0 && shift != 16);
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作，允许shift为0,16,32,48
        insn |= AARCH64_INSN_SF_BIT;
        BUG_ON(shift != 0 && shift != 16 && shift != 32 && shift != 48);
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码移位量（hw字段，除以16）
    insn |= (shift >> 4) << 21;

    // 编码目标寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);

    // 编码16位立即数
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_16, insn, imm);
}

/**
 * 生成移位寄存器加法/减法指令
 * 生成ADD/SUB指令，支持寄存器移位操作
 * @param dst 目标寄存器
 * @param src 第一个源寄存器
 * @param reg 第二个源寄存器（将被移位）
 * @param shift 移位量
 * @param variant 指令变体（32位或64位）
 * @param type 操作类型（加法、减法、带标志位的加减法）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_add_sub_shifted_reg(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                                         enum aarch64_insn_register reg, int shift, enum aarch64_insn_variant variant,
                                         enum aarch64_insn_adsb_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_ADSB_ADD:  // 加法
        insn = aarch64_insn_get_add_value();
        break;
    case AARCH64_INSN_ADSB_SUB:  // 减法
        insn = aarch64_insn_get_sub_value();
        break;
    case AARCH64_INSN_ADSB_ADD_SETFLAGS:  // 加法并设置标志位
        insn = aarch64_insn_get_adds_value();
        break;
    case AARCH64_INSN_ADSB_SUB_SETFLAGS:  // 减法并设置标志位
        insn = aarch64_insn_get_subs_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作，移位量0-31
        BUG_ON(shift & ~(SZ_32 - 1));
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作，移位量0-63
        insn |= AARCH64_INSN_SF_BIT;
        BUG_ON(shift & ~(SZ_64 - 1));
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码寄存器字段
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg);
    
    // 编码6位移位量
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_6, insn, shift);
}

/**
 * 生成单操作数数据处理指令
 * 生成REV16、REV32、REV64等字节反转指令
 * @param dst 目标寄存器
 * @param src 源寄存器
 * @param variant 指令变体（32位或64位）
 * @param type 数据处理类型（16位、32位、64位反转）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_data1(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_variant variant, enum aarch64_insn_data1_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_DATA1_REVERSE_16:  // 16位字节反转
        insn = aarch64_insn_get_rev16_value();
        break;
    case AARCH64_INSN_DATA1_REVERSE_32:  // 32位字节反转
        insn = aarch64_insn_get_rev32_value();
        break;
    case AARCH64_INSN_DATA1_REVERSE_64:  // 64位字节反转（仅64位模式）
        BUG_ON(variant != AARCH64_INSN_VARIANT_64BIT);
        insn = aarch64_insn_get_rev64_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作
        insn |= AARCH64_INSN_SF_BIT;
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码目标和源寄存器
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);
}

/**
 * 生成双操作数数据处理指令
 * 生成UDIV、SDIV、LSLV、LSRV等需要两个源寄存器的指令
 * @param dst 目标寄存器
 * @param src 第一个源寄存器
 * @param reg 第二个源寄存器
 * @param variant 指令变体（32位或64位）
 * @param type 数据处理类型（除法、移位等）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_data2(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_register reg, enum aarch64_insn_variant variant,
                           enum aarch64_insn_data2_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_DATA2_UDIV:  // 无符号除法
        insn = aarch64_insn_get_udiv_value();
        break;
    case AARCH64_INSN_DATA2_SDIV:  // 有符号除法
        insn = aarch64_insn_get_sdiv_value();
        break;
    case AARCH64_INSN_DATA2_LSLV:  // 逻辑左移（变量）
        insn = aarch64_insn_get_lslv_value();
        break;
    case AARCH64_INSN_DATA2_LSRV:  // 逻辑右移（变量）
        insn = aarch64_insn_get_lsrv_value();
        break;
    case AARCH64_INSN_DATA2_ASRV:  // 算术右移（变量）
        insn = aarch64_insn_get_asrv_value();
        break;
    case AARCH64_INSN_DATA2_RORV:  // 循环右移（变量）
        insn = aarch64_insn_get_rorv_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作
        insn |= AARCH64_INSN_SF_BIT;
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码三个寄存器字段
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg);
}

/**
 * 生成三操作数数据处理指令
 * 生成MADD、MSUB等需要三个源寄存器的指令
 * @param dst 目标寄存器
 * @param src 第一个源寄存器
 * @param reg1 第二个源寄存器
 * @param reg2 第三个源寄存器
 * @param variant 指令变体（32位或64位）
 * @param type 数据处理类型（乘加、乘减等）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_data3(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                           enum aarch64_insn_register reg1, enum aarch64_insn_register reg2,
                           enum aarch64_insn_variant variant, enum aarch64_insn_data3_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_DATA3_MADD:  // 乘加运算：dst = reg1 * reg2 + src
        insn = aarch64_insn_get_madd_value();
        break;
    case AARCH64_INSN_DATA3_MSUB:  // 乘减运算：dst = src - reg1 * reg2
        insn = aarch64_insn_get_msub_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作
        insn |= AARCH64_INSN_SF_BIT;
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码四个寄存器字段
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RA, insn, src);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, reg1);
    return aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg2);
}

/**
 * 生成移位寄存器逻辑运算指令
 * 生成AND、ORR、EOR等逻辑运算指令，支持寄存器移位
 * @param dst 目标寄存器
 * @param src 第一个源寄存器
 * @param reg 第二个源寄存器（将被移位）
 * @param shift 移位量
 * @param variant 指令变体（32位或64位）
 * @param type 逻辑运算类型（与、或、异或等）
 * @return 生成的指令编码
 */
u32 aarch64_insn_gen_logical_shifted_reg(enum aarch64_insn_register dst, enum aarch64_insn_register src,
                                         enum aarch64_insn_register reg, int shift, enum aarch64_insn_variant variant,
                                         enum aarch64_insn_logic_type type)
{
    u32 insn;

    switch (type) {
    case AARCH64_INSN_LOGIC_AND:  // 逻辑与
        insn = aarch64_insn_get_and_value();
        break;
    case AARCH64_INSN_LOGIC_BIC:  // 位清除（与非）
        insn = aarch64_insn_get_bic_value();
        break;
    case AARCH64_INSN_LOGIC_ORR:  // 逻辑或
        insn = aarch64_insn_get_orr_value();
        break;
    case AARCH64_INSN_LOGIC_ORN:  // 或非
        insn = aarch64_insn_get_orn_value();
        break;
    case AARCH64_INSN_LOGIC_EOR:  // 逻辑异或
        insn = aarch64_insn_get_eor_value();
        break;
    case AARCH64_INSN_LOGIC_EON:  // 异或非
        insn = aarch64_insn_get_eon_value();
        break;
    case AARCH64_INSN_LOGIC_AND_SETFLAGS:  // 逻辑与并设置标志位
        insn = aarch64_insn_get_ands_value();
        break;
    case AARCH64_INSN_LOGIC_BIC_SETFLAGS:  // 位清除并设置标志位
        insn = aarch64_insn_get_bics_value();
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    switch (variant) {
    case AARCH64_INSN_VARIANT_32BIT:  // 32位操作，移位量0-31
        BUG_ON(shift & ~(SZ_32 - 1));
        break;
    case AARCH64_INSN_VARIANT_64BIT:  // 64位操作，移位量0-63
        insn |= AARCH64_INSN_SF_BIT;
        BUG_ON(shift & ~(SZ_64 - 1));
        break;
    default:
        BUG_ON(1);
        return AARCH64_BREAK_FAULT;
    }

    // 编码寄存器字段
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RD, insn, dst);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RN, insn, src);
    insn = aarch64_insn_encode_register(AARCH64_INSN_REGTYPE_RM, insn, reg);
    
    // 编码6位移位量
    return aarch64_insn_encode_immediate(AARCH64_INSN_IMM_6, insn, shift);
}