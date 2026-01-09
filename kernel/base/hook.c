/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

// ARM64内联Hook系统实现 - 支持函数拦截和链式Hook

#include <hook.h>
#include <cache.h>
#include <pgtable.h>
#include <kpmalloc.h>
#include <io.h>
#include <symbol.h>
#include "hmem.h"

// 位域操作宏：提取32位数据的指定位域
#define bits32(n, high, low) ((uint32_t)((n) << (31u - (high))) >> (31u - (high) + (low)))
// 提取指定位的值
#define bit(n, st) (((n) >> (st)) & 1)
// 符号扩展：将指定长度的数值扩展为64位有符号数
#define sign64_extend(n, len) \
    (((uint64_t)((n) << (63u - (len - 1))) >> 63u) ? ((n) | (0xFFFFFFFFFFFFFFFF << (len))) : n)
// 向上对齐到指定边界
#define align_ceil(x, align) (((u64)(x) + (u64)(align) - 1) & ~((u64)(align) - 1))

typedef uint32_t inst_type_t;   // 指令类型定义
typedef uint32_t inst_mask_t;   // 指令掩码定义

// ARM64指令类型定义
#define INST_B 0x14000000          // 无条件跳转指令
#define INST_BC 0x54000000         // 条件跳转指令
#define INST_BL 0x94000000         // 带链接的跳转指令（函数调用）
#define INST_ADR 0x10000000        // 地址计算指令
#define INST_ADRP 0x90000000       // 页地址计算指令
#define INST_LDR_32 0x18000000     // 32位字面量加载指令
#define INST_LDR_64 0x58000000     // 64位字面量加载指令
#define INST_LDRSW_LIT 0x98000000  // 带符号扩展的字面量加载指令
#define INST_PRFM_LIT 0xD8000000   // 预取指令
#define INST_LDR_SIMD_32 0x1C000000   // SIMD 32位加载指令
#define INST_LDR_SIMD_64 0x5C000000   // SIMD 64位加载指令
#define INST_LDR_SIMD_128 0x9C000000  // SIMD 128位加载指令
#define INST_CBZ 0x34000000        // 零值比较跳转指令
#define INST_CBNZ 0x35000000       // 非零值比较跳转指令
#define INST_TBZ 0x36000000        // 位测试为零跳转指令
#define INST_TBNZ 0x37000000       // 位测试非零跳转指令
#define INST_HINT 0xD503201F       // 提示指令
#define INST_IGNORE 0x0            // 忽略的指令类型

// 对应的指令识别掩码
#define MASK_B 0xFC000000
#define MASK_BC 0xFF000010
#define MASK_BL 0xFC000000
#define MASK_ADR 0x9F000000
#define MASK_ADRP 0x9F000000
#define MASK_LDR_32 0xFF000000
#define MASK_LDR_64 0xFF000000
#define MASK_LDRSW_LIT 0xFF000000
#define MASK_PRFM_LIT 0xFF000000
#define MASK_LDR_SIMD_32 0xFF000000
#define MASK_LDR_SIMD_64 0xFF000000
#define MASK_LDR_SIMD_128 0xFF000000
#define MASK_CBZ 0x7F000000u
#define MASK_CBNZ 0x7F000000u
#define MASK_TBZ 0x7F000000u
#define MASK_TBNZ 0x7F000000u
#define MASK_HINT 0xFFFFF01F
#define MASK_IGNORE 0x0

// 指令掩码数组 - 用于指令识别
static inst_mask_t masks[] = {
    MASK_B,      MASK_BC,        MASK_BL,       MASK_ADR,         MASK_ADRP,        MASK_LDR_32,
    MASK_LDR_64, MASK_LDRSW_LIT, MASK_PRFM_LIT, MASK_LDR_SIMD_32, MASK_LDR_SIMD_64, MASK_LDR_SIMD_128,
    MASK_CBZ,    MASK_CBNZ,      MASK_TBZ,      MASK_TBNZ,        MASK_IGNORE,
};

// 指令类型数组 - 对应掩码的指令类型
static inst_type_t types[] = {
    INST_B,      INST_BC,        INST_BL,       INST_ADR,         INST_ADRP,        INST_LDR_32,
    INST_LDR_64, INST_LDRSW_LIT, INST_PRFM_LIT, INST_LDR_SIMD_32, INST_LDR_SIMD_64, INST_LDR_SIMD_128,
    INST_CBZ,    INST_CBNZ,      INST_TBZ,      INST_TBNZ,        INST_IGNORE,
};

// 重定位指令长度数组 - 每种指令类型重定位后的指令数量
static int32_t relo_len[] = { 6, 8, 8, 4, 4, 6, 6, 6, 8, 8, 8, 8, 6, 6, 6, 6, 2 };

// 符号扩展函数（已注释掉的实现）
// static uint64_t sign_extend(uint64_t x, uint32_t len)
// {
//     char sign_bit = bit(x, len - 1);
//     unsigned long sign_mask = 0 - sign_bit;
//     x |= ((sign_mask >> len) << len);
//     return x;
// }

// 检查地址是否在trampoline范围内
static int is_in_tramp(hook_t *hook, uint64_t addr)
{
    uint64_t tramp_start = hook->origin_addr;
    uint64_t tramp_end = tramp_start + hook->tramp_insts_num * 4;
    if (addr >= tramp_start && addr < tramp_end) {
        return 1;
    }
    return 0;
}

// 在trampoline中重定位地址 - 如果地址在trampoline范围内，返回重定位后的地址
static uint64_t relo_in_tramp(hook_t *hook, uint64_t addr)
{
    uint64_t tramp_start = hook->origin_addr;
    uint64_t tramp_end = tramp_start + hook->tramp_insts_num * 4;
    if (!(addr >= tramp_start && addr < tramp_end)) return addr;
    
    // 计算地址在trampoline中的指令索引
    uint32_t addr_inst_index = (addr - tramp_start) / 4;
    uint64_t fix_addr = hook->relo_addr;
    
    // 累加前面指令的重定位长度，得到正确的重定位地址
    for (int i = 0; i < addr_inst_index; i++) {
        inst_type_t inst = hook->origin_insts[i];
        // 识别指令类型并累加重定位长度
        for (int j = 0; j < sizeof(relo_len) / sizeof(relo_len[0]); j++) {
            if ((inst & masks[j]) == types[j]) {
                fix_addr += relo_len[j] * 4;
                break;
            }
        }
    }
    return fix_addr;
}

#ifdef HOOK_INTO_BRANCH_FUNC

// 单次分支函数地址解析 - 处理函数开头的跳转指令
static uint64_t branch_func_addr_once(uint64_t addr)
{
    uint64_t ret = addr;
    uint32_t inst = *(uint32_t *)addr;
    if ((inst & MASK_B) == INST_B) {
        // 解析B指令的跳转地址
        uint64_t imm26 = bits32(inst, 25, 0);
        uint64_t imm64 = sign64_extend(imm26 << 2u, 28u);
        ret = addr + imm64;
    } else if (inst == ARM64_BTI_C || inst == ARM64_BTI_J || inst == ARM64_BTI_JC) {
        // 跳过BTI（分支目标识别）指令
        ret = addr + 4;
    } else {
        // 其他指令不处理
    }
    return ret;
}

// 递归解析分支函数地址 - 跟踪所有跳转直到找到真实函数地址
uint64_t branch_func_addr(uint64_t addr)
{
    uint64_t ret;
    for (;;) {
        ret = branch_func_addr_once(addr);
        if (ret == addr) break;  // 没有更多跳转时停止
        addr = ret;
    }
    return ret;
}

#endif

// 重定位分支指令（B/BC/BL）
static __noinline hook_err_t relo_b(hook_t *hook, uint64_t inst_addr, uint32_t inst, inst_type_t type)
{
    uint32_t *buf = hook->relo_insts + hook->relo_insts_num;
    uint64_t imm64;
    
    // 根据指令类型解析跳转偏移
    if (type == INST_BC) {
        // 条件跳转指令：19位立即数
        uint64_t imm19 = bits32(inst, 23, 5);
        imm64 = sign64_extend(imm19 << 2u, 21u);
    } else {
        // 无条件跳转/带链接跳转：26位立即数
        uint64_t imm26 = bits32(inst, 25, 0);
        imm64 = sign64_extend(imm26 << 2u, 28u);
    }
    
    // 计算目标地址并处理trampoline内的重定位
    uint64_t addr = inst_addr + imm64;
    addr = relo_in_tramp(hook, addr);

    uint32_t idx = 0;
    if (type == INST_BC) {
        // 条件跳转的重定位：先做条件跳转，不满足条件则跳过
        buf[idx++] = (inst & 0xFF00001F) | 0x40u; // B.<cond> #8
        buf[idx++] = 0x14000006; // B #24
    }
    
    // 加载目标地址到X17寄存器并跳转
    buf[idx++] = 0x58000051; // LDR X17, #8
    buf[idx++] = 0x14000003; // B #12
    buf[idx++] = addr & 0xFFFFFFFF;      // 目标地址低32位
    buf[idx++] = addr >> 32u;           // 目标地址高32位
    
    if (type == INST_BL) {
        // 带链接跳转：需要设置返回地址（X30寄存器）
        buf[idx++] = 0x1000001E; // ADR X30, .
        buf[idx++] = 0x910033DE; // ADD X30, X30, #12
        buf[idx++] = 0xD65F0220; // RET X17
    } else {
        buf[idx++] = 0xD65F0220; // RET X17
    }
    buf[idx++] = ARM64_NOP;
    return HOOK_NO_ERR;
}

// 重定位地址计算指令（ADR/ADRP）
static __noinline hook_err_t relo_adr(hook_t *hook, uint64_t inst_addr, uint32_t inst, inst_type_t type)
{
    uint32_t *buf = hook->relo_insts + hook->relo_insts_num;

    // 提取目标寄存器和立即数
    uint32_t xd = bits32(inst, 4, 0);         // 目标寄存器
    uint64_t immlo = bits32(inst, 30, 29);    // 立即数低2位
    uint64_t immhi = bits32(inst, 23, 5);     // 立即数高19位
    uint64_t addr;

    if (type == INST_ADR) {
        // ADR指令：计算相对于PC的地址
        addr = inst_addr + sign64_extend((immhi << 2u) | immlo, 21u);
    } else {
        // ADRP指令：计算相对于PC的页地址
        addr = (inst_addr + sign64_extend((immhi << 14u) | (immlo << 12u), 33u)) & 0xFFFFFFFFFFFFF000;
        // ADRP的目标地址不能在trampoline范围内
        if (is_in_tramp(hook, addr)) return -HOOK_BAD_RELO;
    }
    
    // 生成重定位代码：直接加载计算好的地址
    buf[0] = 0x58000040u | xd; // LDR Xd, #8
    buf[1] = 0x14000003;       // B #12
    buf[2] = addr & 0xFFFFFFFF; // 地址低32位
    buf[3] = addr >> 32u;       // 地址高32位
    return HOOK_NO_ERR;
}

// 重定位字面量加载指令（LDR等）
static __noinline hook_err_t relo_ldr(hook_t *hook, uint64_t inst_addr, uint32_t inst, inst_type_t type)
{
    uint32_t *buf = hook->relo_insts + hook->relo_insts_num;

    // 提取目标寄存器和偏移量
    uint32_t rt = bits32(inst, 4, 0);        // 目标寄存器
    uint64_t imm19 = bits32(inst, 23, 5);    // 19位立即数偏移
    uint64_t offset = sign64_extend((imm19 << 2u), 21u);
    uint64_t addr = inst_addr + offset;

    // 检查地址是否在trampoline范围内（预取指令除外）
    if (is_in_tramp(hook, addr) && type != INST_PRFM_LIT) return -HOOK_BAD_RELO;

    addr = relo_in_tramp(hook, addr);

    if (type == INST_LDR_32 || type == INST_LDR_64 || type == INST_LDRSW_LIT) {
        // 通用寄存器加载指令的重定位
        buf[0] = 0x58000060u | rt; // LDR Xt, #12
        if (type == INST_LDR_32) {
            buf[1] = 0xB9400000 | rt | (rt << 5u); // LDR Wt, [Xt]  - 32位加载
        } else if (type == INST_LDR_64) {
            buf[1] = 0xF9400000 | rt | (rt << 5u); // LDR Xt, [Xt]  - 64位加载
        } else {
            // LDRSW_LIT - 带符号扩展的32位加载
            buf[1] = 0xB9800000 | rt | (rt << 5u); // LDRSW Xt, [Xt]
        }
        buf[2] = 0x14000004; // B #16
        buf[3] = ARM64_NOP;
        buf[4] = addr & 0xFFFFFFFF;  // 目标地址低32位
        buf[5] = addr >> 32u;        // 目标地址高32位
    } else {
        // SIMD寄存器加载指令和预取指令的重定位
        buf[0] = 0xA93F47F0; // STP X16, X17, [SP, -0x10]  - 保存寄存器
        buf[1] = 0x58000091; // LDR X17, #16
        if (type == INST_PRFM_LIT) {
            buf[2] = 0xF9800220 | rt; // PRFM Rt, [X17]  - 预取指令
        } else if (type == INST_LDR_SIMD_32) {
            buf[2] = 0xBD400220 | rt; // LDR St, [X17]   - SIMD 32位加载
        } else if (type == INST_LDR_SIMD_64) {
            buf[2] = 0xFD400220 | rt; // LDR Dt, [X17]   - SIMD 64位加载
        } else {
            // LDR_SIMD_128 - SIMD 128位加载
            buf[2] = 0x3DC00220u | rt; // LDR Qt, [X17]
        }
        buf[3] = 0xF85F83F1; // LDR X17, [SP, -0x8]  - 恢复X17寄存器
        buf[4] = 0x14000004; // B #16
        buf[5] = ARM64_NOP;
        buf[6] = addr & 0xFFFFFFFF;  // 目标地址低32位
        buf[7] = addr >> 32u;        // 目标地址高32位
    }
    return HOOK_NO_ERR;
}

// 重定位比较跳转指令（CBZ/CBNZ）
static __noinline hook_err_t relo_cb(hook_t *hook, uint64_t inst_addr, uint32_t inst, inst_type_t type)
{
    uint32_t *buf = hook->relo_insts + hook->relo_insts_num;

    // 提取19位立即数偏移并计算目标地址
    uint64_t imm19 = bits32(inst, 23, 5);
    uint64_t offset = sign64_extend((imm19 << 2u), 21u);
    uint64_t addr = inst_addr + offset;
    addr = relo_in_tramp(hook, addr);

    // 生成重定位代码：条件跳转到目标地址
    buf[0] = (inst & 0xFF00001F) | 0x40u; // CB(N)Z Rt, #8  - 条件满足跳转8字节
    buf[1] = 0x14000005; // B #20  - 条件不满足跳转20字节
    buf[2] = 0x58000051; // LDR X17, #8  - 加载目标地址
    buf[3] = 0xD65F0220; // RET X17  - 跳转到目标地址
    buf[4] = addr & 0xFFFFFFFF;  // 目标地址低32位
    buf[5] = addr >> 32u;        // 目标地址高32位
    return HOOK_NO_ERR;
}

// 重定位位测试跳转指令（TBZ/TBNZ）
static __noinline hook_err_t relo_tb(hook_t *hook, uint64_t inst_addr, uint32_t inst, inst_type_t type)
{
    uint32_t *buf = hook->relo_insts + hook->relo_insts_num;

    // 提取14位立即数偏移并计算目标地址
    uint64_t imm14 = bits32(inst, 18, 5);
    uint64_t offset = sign64_extend((imm14 << 2u), 16u);
    uint64_t addr = inst_addr + offset;
    addr = relo_in_tramp(hook, addr);

    // 生成重定位代码：位测试跳转到目标地址
    buf[0] = (inst & 0xFFF8001F) | 0x40u; // TB(N)Z Rt, #<imm>, #8  - 条件满足跳转8字节
    buf[1] = 0x14000005; // B #20  - 条件不满足跳转20字节
    buf[2] = 0x58000051; // LDR X17, #8  - 加载目标地址
    buf[3] = 0xd61f0220; // RET X17  - 跳转到目标地址
    buf[4] = addr & 0xFFFFFFFF;  // 目标地址低32位
    buf[5] = addr >> 32u;        // 目标地址高32位
    return HOOK_NO_ERR;
}

// 重定位忽略指令（直接复制指令）
static __noinline hook_err_t relo_ignore(hook_t *hook, uint64_t inst_addr, uint32_t inst, inst_type_t type)
{
    uint32_t *buf = hook->relo_insts + hook->relo_insts_num;
    buf[0] = inst;      // 直接复制原指令
    buf[1] = ARM64_NOP; // 填充NOP指令
    return HOOK_NO_ERR;
}

// 检查是否可以使用相对跳转
static uint32_t can_b_rel(uint64_t src_addr, uint64_t dst_addr)
{
#define B_REL_RANGE ((1 << 25) << 2)  // B指令的最大跳转范围
    return ((dst_addr >= src_addr) & (dst_addr - src_addr <= B_REL_RANGE)) ||
           ((src_addr >= dst_addr) & (src_addr - dst_addr <= B_REL_RANGE));
}

// 生成相对跳转指令
int32_t branch_relative(uint32_t *buf, uint64_t src_addr, uint64_t dst_addr)
{
    if (can_b_rel(src_addr, dst_addr)) {
        // 生成B指令，计算相对偏移
        buf[0] = 0x14000000u | (((dst_addr - src_addr) & 0x0FFFFFFFu) >> 2u); // B <label>
        buf[1] = ARM64_NOP;
        return 2;
    }
    return 0;
}
KP_EXPORT_SYMBOL(branch_relative);

// 生成绝对跳转指令
int32_t branch_absolute(uint32_t *buf, uint64_t addr)
{
    buf[0] = 0x58000051; // LDR X17, #8   - 加载目标地址到X17
    buf[1] = 0xd61f0220; // BR X17        - 跳转到X17指向的地址
    buf[2] = addr & 0xFFFFFFFF;  // 目标地址低32位
    buf[3] = addr >> 32u;        // 目标地址高32位
    return 4;
}
KP_EXPORT_SYMBOL(branch_absolute);

// 生成绝对返回指令
int32_t ret_absolute(uint32_t *buf, uint64_t addr)
{
    buf[0] = 0x58000051; // LDR X17, #8   - 加载目标地址到X17
    buf[1] = 0xD65F0220; // RET X17       - 返回到X17指向的地址
    buf[2] = addr & 0xFFFFFFFF;  // 目标地址低32位
    buf[3] = addr >> 32u;        // 目标地址高32位
    return 4;
}
KP_EXPORT_SYMBOL(ret_absolute);

// 生成从源地址到目标地址的跳转指令
int32_t branch_from_to(uint32_t *tramp_buf, uint64_t src_addr, uint64_t dst_addr)
{
#if 0
    // 优先尝试相对跳转（已注释掉）
    uint32_t len = branch_relative(tramp_buf, src_addr, dst_addr);
    if (len) return len;
#else
#if 0
    return branch_absolute(tramp_buf, dst_addr);
#else
    // 使用绝对返回指令
    return ret_absolute(tramp_buf, dst_addr);
#endif
#endif
}

// Hook链转换函数 - 无参数版本
typedef uint64_t (*transit0_func_t)();

uint64_t __attribute__((section(".transit0.text"))) __attribute__((__noinline__)) _transit0()
{
    uint64_t this_va;
    // 获取当前函数地址
    asm volatile("adr %0, ." : "=r"(this_va));
    uint32_t *vptr = (uint32_t *)this_va;
    
    // 向前查找NOP指令，定位到Hook链结构
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    
    // 通过容器偏移获取Hook链结构指针
    hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, hook_chain_t, transit);
    
    // 初始化函数参数结构
    hook_fargs0_t fargs;
    fargs.skip_origin = 0;
    fargs.chain = hook_chain;
    
    // 执行所有前置回调函数
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain0_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    
    // 如果没有跳过原函数，则执行原函数
    if (!fargs.skip_origin) {
        transit0_func_t origin_func = (transit0_func_t)hook_chain->hook.relo_addr;
        fargs.ret = origin_func();
    }
    
    // 执行所有后置回调函数（逆序）
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain0_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}
extern void _transit0_end();

// Hook链转换函数 - 4参数版本
typedef uint64_t (*transit4_func_t)(uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t __attribute__((section(".transit4.text"))) __attribute__((__noinline__))
_transit4(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));
    uint32_t *vptr = (uint32_t *)this_va;
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, hook_chain_t, transit);
    
    // 初始化4参数函数参数结构
    hook_fargs4_t fargs;
    fargs.skip_origin = 0;
    fargs.arg0 = arg0;
    fargs.arg1 = arg1;
    fargs.arg2 = arg2;
    fargs.arg3 = arg3;
    fargs.chain = hook_chain;
    
    // 执行前置回调
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain4_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    
    // 执行原函数（使用可能被修改的参数）
    if (!fargs.skip_origin) {
        transit4_func_t origin_func = (transit4_func_t)hook_chain->hook.relo_addr;
        fargs.ret = origin_func(fargs.arg0, fargs.arg1, fargs.arg2, fargs.arg3);
    }
    
    // 执行后置回调
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain4_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}

extern void _transit4_end();

// transit8:
typedef uint64_t (*transit8_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t __attribute__((section(".transit8.text"))) __attribute__((__noinline__))
_transit8(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6,
          uint64_t arg7)
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));
    uint32_t *vptr = (uint32_t *)this_va;
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, hook_chain_t, transit);
    hook_fargs8_t fargs;
    fargs.skip_origin = 0;
    fargs.arg0 = arg0;
    fargs.arg1 = arg1;
    fargs.arg2 = arg2;
    fargs.arg3 = arg3;
    fargs.arg4 = arg4;
    fargs.arg5 = arg5;
    fargs.arg6 = arg6;
    fargs.arg7 = arg7;
    fargs.chain = hook_chain;
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain8_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    if (!fargs.skip_origin) {
        transit8_func_t origin_func = (transit8_func_t)hook_chain->hook.relo_addr;
        fargs.ret =
            origin_func(fargs.arg0, fargs.arg1, fargs.arg2, fargs.arg3, fargs.arg4, fargs.arg5, fargs.arg6, fargs.arg7);
    }
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain8_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}

extern void _transit8_end();

// transit12:
typedef uint64_t (*transit12_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                     uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t __attribute__((section(".transit12.text"))) __attribute__((__noinline__))
_transit12(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6,
           uint64_t arg7, uint64_t arg8, uint64_t arg9, uint64_t arg10, uint64_t arg11)
{
    uint64_t this_va;
    asm volatile("adr %0, ." : "=r"(this_va));
    uint32_t *vptr = (uint32_t *)this_va;
    while (*--vptr != ARM64_NOP) {
    };
    vptr--;
    hook_chain_t *hook_chain = local_container_of((uint64_t)vptr, hook_chain_t, transit);
    hook_fargs12_t fargs;
    fargs.skip_origin = 0;
    fargs.arg0 = arg0;
    fargs.arg1 = arg1;
    fargs.arg2 = arg2;
    fargs.arg3 = arg3;
    fargs.arg4 = arg4;
    fargs.arg5 = arg5;
    fargs.arg6 = arg6;
    fargs.arg7 = arg7;
    fargs.arg8 = arg8;
    fargs.arg9 = arg9;
    fargs.arg10 = arg10;
    fargs.arg11 = arg11;
    fargs.chain = hook_chain;
    for (int32_t i = 0; i < hook_chain->chain_items_max; i++) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain12_callback func = hook_chain->befores[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    if (!fargs.skip_origin) {
        transit12_func_t origin_func = (transit12_func_t)hook_chain->hook.relo_addr;
        fargs.ret = origin_func(fargs.arg0, fargs.arg1, fargs.arg2, fargs.arg3, fargs.arg4, fargs.arg5, fargs.arg6,
                                fargs.arg7, fargs.arg8, fargs.arg9, fargs.arg10, fargs.arg11);
    }
    for (int32_t i = hook_chain->chain_items_max - 1; i >= 0; i--) {
        if (hook_chain->states[i] != CHAIN_ITEM_STATE_READY) continue;
        hook_chain12_callback func = hook_chain->afters[i];
        if (func) func(&fargs, hook_chain->udata[i]);
    }
    return fargs.ret;
}

extern void _transit12_end();

// 重定位单个指令 - Hook系统的核心功能
static __noinline hook_err_t relocate_inst(hook_t *hook, uint64_t inst_addr, uint32_t inst)
{
    hook_err_t rc = HOOK_NO_ERR;
    inst_type_t it = INST_IGNORE;
    int len = 1;

    // 识别指令类型
    for (int j = 0; j < sizeof(relo_len) / sizeof(relo_len[0]); j++) {
        if ((inst & masks[j]) == types[j]) {
            it = types[j];
            len = relo_len[j];
            break;
        }
    }

    // 根据指令类型调用相应的重定位函数
    switch (it) {
    case INST_B:
    case INST_BC:
    case INST_BL:
        rc = relo_b(hook, inst_addr, inst, it);
        break;
    case INST_ADR:
    case INST_ADRP:
        rc = relo_adr(hook, inst_addr, inst, it);
        break;
    case INST_LDR_32:
    case INST_LDR_64:
    case INST_LDRSW_LIT:
    case INST_PRFM_LIT:
    case INST_LDR_SIMD_32:
    case INST_LDR_SIMD_64:
    case INST_LDR_SIMD_128:
        rc = relo_ldr(hook, inst_addr, inst, it);
        break;
    case INST_CBZ:
    case INST_CBNZ:
        rc = relo_cb(hook, inst_addr, inst, it);
        break;
    case INST_TBZ:
    case INST_TBNZ:
        rc = relo_tb(hook, inst_addr, inst, it);
        break;
    case INST_IGNORE:
    default:
        rc = relo_ignore(hook, inst_addr, inst, it);
        break;
    }

    hook->relo_insts_num += len;

    return rc;
}

// Hook准备阶段 - 分析和重定位目标函数的指令
hook_err_t hook_prepare(hook_t *hook)
{
    // 验证所有地址的有效性
    if (is_bad_address((void *)hook->func_addr)) return -HOOK_BAD_ADDRESS;
    if (is_bad_address((void *)hook->origin_addr)) return -HOOK_BAD_ADDRESS;
    if (is_bad_address((void *)hook->replace_addr)) return -HOOK_BAD_ADDRESS;
    if (is_bad_address((void *)hook->relo_addr)) return -HOOK_BAD_ADDRESS;

    // 备份原始指令
    for (int i = 0; i < TRAMPOLINE_NUM; i++) {
        hook->origin_insts[i] = *((uint32_t *)hook->origin_addr + i);
    }
    
    // 生成跳转到替换函数的trampoline代码
    hook->tramp_insts_num = branch_from_to(hook->tramp_insts, hook->origin_addr, hook->replace_addr);

    // 初始化重定位指令缓冲区（填充NOP）
    for (int i = 0; i < sizeof(hook->relo_insts) / sizeof(hook->relo_insts[0]); i++) {
        hook->relo_insts[i] = ARM64_NOP;
    }

    // 在重定位代码开头添加BTI（分支目标识别）指令
    uint32_t *bti = hook->relo_insts + hook->relo_insts_num;
    bti[0] = ARM64_BTI_JC;
    bti[1] = ARM64_NOP;
    hook->relo_insts_num += 2;

    // 重定位被替换的指令
    for (int i = 0; i < hook->tramp_insts_num; i++) {
        uint64_t inst_addr = hook->origin_addr + i * 4;
        uint32_t inst = hook->origin_insts[i];
        hook_err_t relo_res = relocate_inst(hook, inst_addr, inst);
        if (relo_res) {
            return -HOOK_BAD_RELO;
        }
    }

    // 在重定位代码末尾添加跳转回原函数剩余部分的代码
    uint64_t back_src_addr = hook->relo_addr + hook->relo_insts_num * 4;
    uint64_t back_dst_addr = hook->origin_addr + hook->tramp_insts_num * 4;
    uint32_t *buf = hook->relo_insts + hook->relo_insts_num;
    hook->relo_insts_num += branch_from_to(buf, back_src_addr, back_dst_addr);
    return HOOK_NO_ERR;
}
KP_EXPORT_SYMBOL(hook_prepare);

// Hook安装 - 将trampoline代码写入目标函数
void hook_install(hook_t *hook)
{
    uint64_t va = hook->origin_addr;
    uint64_t *entry = pgtable_entry_kernel(va);
    uint64_t ori_prot = *entry;
    
    // 临时修改页面保护属性为可写
    modify_entry_kernel(va, entry, (ori_prot | PTE_DBM) & ~PTE_RDONLY);
    
    // TODO: 应该使用cpu_stop_machine停止所有CPU
    // TODO: 可以直接使用aarch64_insn_patch_text_nosync, aarch64_insn_patch_text
    
    // 将trampoline指令写入原函数起始位置
    for (int32_t i = 0; i < hook->tramp_insts_num; i++) {
        *((uint32_t *)hook->origin_addr + i) = hook->tramp_insts[i];
    }
    
    // 刷新指令缓存确保修改生效
    flush_icache_all();
    
    // 恢复原始页面保护属性
    modify_entry_kernel(va, entry, ori_prot);
}
KP_EXPORT_SYMBOL(hook_install);

// Hook卸载 - 恢复原始指令
void hook_uninstall(hook_t *hook)
{
    uint64_t va = hook->origin_addr;
    uint64_t *entry = pgtable_entry_kernel(va);
    uint64_t ori_prot = *entry;
    
    // 临时修改页面保护属性为可写
    modify_entry_kernel(va, entry, (ori_prot | PTE_DBM) & ~PTE_RDONLY);
    flush_tlb_kernel_page(va);
    
    // 恢复原始指令
    for (int32_t i = 0; i < hook->tramp_insts_num; i++) {
        *((uint32_t *)hook->origin_addr + i) = hook->origin_insts[i];
    }
    
    // 刷新指令缓存确保修改生效
    flush_icache_all();
    
    // 恢复原始页面保护属性
    modify_entry_kernel(va, entry, ori_prot);
}
KP_EXPORT_SYMBOL(hook_uninstall);

// 主Hook函数 - 简单的函数替换Hook
hook_err_t hook(void *func, void *replace, void **backup)
{
    hook_err_t err = HOOK_NO_ERR;
    if (!func || !replace || !backup) {
        return -HOOK_BAD_ADDRESS;
    }
    
    // 解析函数真实地址（跳过可能的跳转指令）
    uint64_t origin_addr = branch_func_addr((uintptr_t)func);
    
    // 分配Hook结构体内存
    hook_t *hook = (hook_t *)hook_mem_zalloc(origin_addr, INLINE);
    if (!hook) return -HOOK_NO_MEM;
    
    // 初始化Hook结构
    hook->func_addr = (uint64_t)func;
    hook->origin_addr = origin_addr;
    hook->replace_addr = (uint64_t)replace;
    hook->relo_addr = (uint64_t)hook->relo_insts;
    *backup = (void *)hook->relo_addr;  // 返回重定位后的原函数地址
    
    logkv("Hook func: %llx, origin: %llx, replace: %llx, relocate: %llx, chain: %llx\n", hook->func_addr,
          hook->origin_addr, hook->replace_addr, hook->relo_addr, hook);
    
    // 准备Hook（重定位指令等）
    err = hook_prepare(hook);
    if (err) goto out;
    
    // 安装Hook
    hook_install(hook);
    logkv("Hook func: %llx succsseed\n", hook->func_addr);
    return HOOK_NO_ERR;
    
out:
    hook_mem_free(hook);
    logkv("Hook func: %llx failed, err: %d\n", hook->func_addr, err);
    return err;
}
KP_EXPORT_SYMBOL(hook);

// 取消Hook
void unhook(void *func)
{
    uint64_t origin = branch_func_addr((uint64_t)func);
    hook_t *hook = hook_get_mem_from_origin(origin);
    if (!hook) return;
    
    // 卸载Hook并释放内存
    hook_uninstall(hook);
    hook_mem_free(hook);
    logkv("Unhook func: %llx\n", func);
}
KP_EXPORT_SYMBOL(unhook);

// 准备Hook链的转换代码
static hook_err_t hook_chain_prepare(uint32_t *transit, int32_t argno)
{
    uint64_t transit_start, transit_end;
    
    // 根据参数数量选择对应的转换函数模板
    switch (argno) {
    case 0:
        transit_start = (uint64_t)_transit0;
        transit_end = (uint64_t)_transit0_end;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        transit_start = (uint64_t)_transit4;
        transit_end = (uint64_t)_transit4_end;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
        transit_start = (uint64_t)_transit8;
        transit_end = (uint64_t)_transit8_end;
        break;
    default:
        transit_start = (uint64_t)_transit12;
        transit_end = (uint64_t)_transit12_end;
        break;
    }

    int32_t transit_num = (transit_end - transit_start) / 4;
    // TODO: 添加断言检查
    if (transit_num >= TRANSIT_INST_NUM) return -HOOK_TRANSIT_NO_MEM;

    // 复制转换函数代码到Hook链的转换区域
    transit[0] = ARM64_BTI_JC;  // 添加BTI指令
    transit[1] = ARM64_NOP;
    for (int i = 0; i < transit_num; i++) {
        transit[i + 2] = ((uint32_t *)transit_start)[i];
    }
    return HOOK_NO_ERR;
}

// 向Hook链添加回调函数
hook_err_t hook_chain_add(hook_chain_t *chain, void *before, void *after, void *udata)
{
    for (int i = 0; i < HOOK_CHAIN_NUM; i++) {
        // 检查是否已存在相同的回调函数
        if ((before && chain->befores[i] == before) || (after && chain->afters[i] == after)) return -HOOK_DUPLICATED;

        // TODO: 应该使用原子操作或锁
        if (chain->states[i] == CHAIN_ITEM_STATE_EMPTY) {
            chain->states[i] = CHAIN_ITEM_STATE_BUSY;
            dsb(ish);  // 数据同步屏障
            
            // 设置回调函数和用户数据
            chain->udata[i] = udata;
            chain->befores[i] = before;
            chain->afters[i] = after;
            
            // 更新最大链项数
            if (i + 1 > chain->chain_items_max) {
                chain->chain_items_max = i + 1;
            }
            
            dsb(ish);  // 确保写入完成
            chain->states[i] = CHAIN_ITEM_STATE_READY;
            logkv("Wrap chain add: %llx, %llx, %llx successed\n", chain->hook.func_addr, before, after);
            return HOOK_NO_ERR;
        }
    }
    logkv("Wrap chain add: %llx, %llx, %llx failed\n", chain->hook.func_addr, before, after);
    return -HOOK_CHAIN_FULL;
}
KP_EXPORT_SYMBOL(hook_chain_add);

// 从Hook链移除回调函数
void hook_chain_remove(hook_chain_t *chain, void *before, void *after)
{
    for (int i = 0; i < HOOK_CHAIN_NUM; i++) {
        if (chain->states[i] == CHAIN_ITEM_STATE_READY)
            if ((before && chain->befores[i] == before) || (after && chain->afters[i] == after)) {
                chain->states[i] = CHAIN_ITEM_STATE_BUSY;
                dsb(ish);
                
                // 清空回调函数和用户数据
                chain->udata[i] = 0;
                chain->befores[i] = 0;
                chain->afters[i] = 0;
                
                dsb(ish);
                chain->states[i] = CHAIN_ITEM_STATE_EMPTY;
                break;
            }
    }
    logkv("Wrap chain remove: %llx, %llx, %llx\n", chain->hook.func_addr, before, after);
}
KP_EXPORT_SYMBOL(hook_chain_remove);

// Hook包装函数 - 支持前置和后置回调的高级Hook
// TODO: 需要加锁保护
hook_err_t hook_wrap(void *func, int32_t argno, void *before, void *after, void *udata)
{
    if (is_bad_address(func)) return -HOOK_BAD_ADDRESS;
    uint64_t faddr = (uint64_t)func;
    uint64_t origin = branch_func_addr(faddr);
    if (is_bad_address(func)) return -HOOK_BAD_ADDRESS;
    
    // 检查是否已存在Hook链
    hook_chain_t *chain = (hook_chain_t *)hook_get_mem_from_origin(origin);
    if (chain) return hook_chain_add(chain, before, after, udata);
    
    // 创建新的Hook链
    chain = (hook_chain_t *)hook_mem_zalloc(origin, INLINE_CHAIN);
    if (!chain) return -HOOK_NO_MEM;
    
    chain->chain_items_max = 0;
    hook_t *hook = &chain->hook;
    hook->func_addr = faddr;
    hook->origin_addr = origin;
    hook->replace_addr = (uint64_t)chain->transit;  // 替换为转换函数
    hook->relo_addr = (uint64_t)hook->relo_insts;
    
    logkv("Wrap func: %llx, origin: %llx, replace: %llx, relocate: %llx, chain: %llx\n", hook->func_addr,
          hook->origin_addr, hook->replace_addr, hook->relo_addr, chain);
    
    hook_err_t err = hook_prepare(hook);
    if (err) goto err;
    
    // 准备转换函数代码
    err = hook_chain_prepare(chain->transit, argno);
    if (err) goto err;
    
    // 添加回调函数到链中
    err = hook_chain_add(chain, before, after, udata);
    if (err) goto err;
    
    // 安装Hook链
    hook_chain_install(chain);
    logkv("Wrap func: %llx succsseed\n", hook->func_addr);
    return HOOK_NO_ERR;
    
err:
    hook_mem_free(chain);
    logkv("Wrap func: %llx failed, err: %d\n", hook->func_addr, err);
    return err;
}
KP_EXPORT_SYMBOL(hook_wrap);

// 取消Hook包装
void hook_unwrap_remove(void *func, void *before, void *after, int remove)
{
    if (is_bad_address(func)) return;
    uint64_t faddr = (uint64_t)func;
    uint64_t origin = branch_func_addr(faddr);
    if (is_bad_address(func)) return;
    
    hook_chain_t *chain = (hook_chain_t *)hook_get_mem_from_origin(origin);
    if (!chain) return;
    
    // 从链中移除回调函数
    hook_chain_remove(chain, before, after);
    if (!remove) return;
    
    // 检查是否所有链项都为空，如果是则完全移除Hook链
    // TODO: 这部分代码需要优化
    for (int i = 0; i < HOOK_CHAIN_NUM; i++) {
        if (chain->states[i] != CHAIN_ITEM_STATE_EMPTY) return;
    }
    
    // 卸载Hook链
    hook_chain_uninstall(chain);
    // TODO: 这里不安全，需要确保没有其他线程在使用
    hook_mem_free(chain);
    logkv("Unwrap func: %llx\n", func);
}
KP_EXPORT_SYMBOL(hook_unwrap_remove);
