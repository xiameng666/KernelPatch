/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * register_utils.c - ARM64 debug register probing
 *
 * HW capabilities, debug register dump, ESR parsing, CPU info
 */

#include "kdbg-probe.h"

/* ================================================================
 * 1. CPU info
 * ================================================================ */

void probe_cpu_info(void)
{
    pr_info("kdbg-probe: ========== CPU INFO ==========\n");

    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    pr_info("kdbg-probe: MPIDR_EL1            = 0x%llx\n", mpidr);
    pr_info("kdbg-probe:   CPU Aff0=%d Aff1=%d\n",
            (int)(mpidr & 0xFF), (int)((mpidr >> 8) & 0xFF));

    uint64_t midr;
    asm volatile("mrs %0, midr_el1" : "=r"(midr));
    pr_info("kdbg-probe: MIDR_EL1             = 0x%llx\n", midr);
    pr_info("kdbg-probe:   Implementer=0x%x Part=0x%x Variant=%d Rev=%d\n",
            (int)((midr >> 24) & 0xFF),
            (int)((midr >> 4) & 0xFFF),
            (int)((midr >> 20) & 0xF),
            (int)(midr & 0xF));
}

/* ================================================================
 * 2. MDSCR_EL1 - Monitor Debug System Control Register
 *
 *  bit[0]  SS  - Software Step enable
 *  bit[13] KDE - Kernel Debug Enable (EL1 debug exceptions)
 *  bit[15] MDE - Monitor Debug Enable (EL0 HW breakpoint/watchpoint)
 *  bit[12] TDCC- Trap Debug Comms Channel
 * ================================================================ */

void print_mdscr(void)
{
    uint64_t mdscr;
    asm volatile("mrs %0, mdscr_el1" : "=r"(mdscr));

    pr_info("kdbg-probe: ---- MDSCR_EL1 = 0x%llx ----\n", mdscr);
    pr_info("kdbg-probe:   SS   (bit0)  = %d  %s\n",
            (int)(mdscr & 1),
            (mdscr & 1) ? "[ACTIVE] single-step enabled" : "single-step off");
    pr_info("kdbg-probe:   TDCC (bit12) = %d  %s\n",
            (int)((mdscr >> 12) & 1),
            ((mdscr >> 12) & 1) ? "debug comms trapped" : "debug comms normal");
    pr_info("kdbg-probe:   KDE  (bit13) = %d  %s\n",
            (int)((mdscr >> 13) & 1),
            ((mdscr >> 13) & 1) ? "[ACTIVE] kernel debug enabled" : "kernel debug off");
    pr_info("kdbg-probe:   MDE  (bit15) = %d  %s\n",
            (int)((mdscr >> 15) & 1),
            ((mdscr >> 15) & 1) ? "[ACTIVE] EL0 debug enabled" : "EL0 debug off");

    /* summary */
    if (mdscr == 0) {
        pr_info("kdbg-probe:   => all debug features OFF (idle state)\n");
    } else {
        pr_info("kdbg-probe:   => debug activity detected!\n");
    }
}

/* ================================================================
 * 3. Hardware capability probe (ID_AA64DFR0_EL1)
 * ================================================================ */

void probe_hw_capabilities(void)
{
    pr_info("kdbg-probe: ========== HW CAPABILITY ==========\n");

    /* ID_AA64DFR0_EL1 */
    uint64_t dfr0;
    asm volatile("mrs %0, id_aa64dfr0_el1" : "=r"(dfr0));

    int debug_ver = (int)(dfr0 & 0xF);
    int num_brps  = (int)((dfr0 >> 12) & 0xF) + 1;
    int num_wrps  = (int)((dfr0 >> 20) & 0xF) + 1;
    int num_ctx   = (int)((dfr0 >> 28) & 0xF) + 1;

    pr_info("kdbg-probe: ID_AA64DFR0_EL1     = 0x%llx\n", dfr0);
    pr_info("kdbg-probe:   DebugVer           = %d (%s)\n",
            debug_ver,
            debug_ver >= 8 ? "v8.0+" :
            debug_ver >= 9 ? "v8.2+" : "unknown");
    pr_info("kdbg-probe:   HW Breakpoints     = %d slots\n", num_brps);
    pr_info("kdbg-probe:   Watchpoints        = %d slots\n", num_wrps);
    pr_info("kdbg-probe:   Context CMP        = %d slots\n", num_ctx);

    print_mdscr();
}

/* ================================================================
 * 3. Debug register state dump (DBGBVR/DBGBCR, DBGWVR/DBGWCR)
 * ================================================================ */

#define READ_DBGBVR(n, out) asm volatile("mrs %0, dbgbvr" #n "_el1" : "=r"(out))
#define READ_DBGBCR(n, out) asm volatile("mrs %0, dbgbcr" #n "_el1" : "=r"(out))
#define READ_DBGWVR(n, out) asm volatile("mrs %0, dbgwvr" #n "_el1" : "=r"(out))
#define READ_DBGWCR(n, out) asm volatile("mrs %0, dbgwcr" #n "_el1" : "=r"(out))

#define DUMP_BP(n) do { \
    uint64_t bvr, bcr; \
    READ_DBGBVR(n, bvr); \
    READ_DBGBCR(n, bcr); \
    int enabled = (int)(bcr & 1); \
    int pmc = (int)((bcr >> 1) & 3); \
    const char *pmc_str = pmc == 1 ? "EL1" : pmc == 2 ? "EL0" : pmc == 3 ? "EL0+EL1" : "off"; \
    if (enabled) { \
        pr_info("kdbg-probe:   BP[%d] addr=0x%llx  ctrl=0x%llx  [ACTIVE %s]\n", \
                n, bvr, bcr, pmc_str); \
    } else { \
        pr_info("kdbg-probe:   BP[%d] addr=0x%llx  ctrl=0x%llx  [idle]\n", \
                n, bvr, bcr); \
    } \
} while(0)

#define DUMP_WP(n) do { \
    uint64_t wvr, wcr; \
    READ_DBGWVR(n, wvr); \
    READ_DBGWCR(n, wcr); \
    int enabled = (int)(wcr & 1); \
    int lsc = (int)((wcr >> 3) & 3); \
    const char *lsc_str = lsc == 1 ? "load" : lsc == 2 ? "store" : lsc == 3 ? "load+store" : "off"; \
    if (enabled) { \
        pr_info("kdbg-probe:   WP[%d] addr=0x%llx  ctrl=0x%llx  [ACTIVE %s]\n", \
                n, wvr, wcr, lsc_str); \
    } else { \
        pr_info("kdbg-probe:   WP[%d] addr=0x%llx  ctrl=0x%llx  [idle]\n", \
                n, wvr, wcr); \
    } \
} while(0)

void probe_debug_registers(void)
{
    pr_info("kdbg-probe: ========== DEBUG REGISTERS ==========\n");

    pr_info("kdbg-probe: --- HW Breakpoints DBGBVR/DBGBCR ---\n");
    DUMP_BP(0);
    DUMP_BP(1);
    DUMP_BP(2);
    DUMP_BP(3);
    DUMP_BP(4);
    DUMP_BP(5);

    pr_info("kdbg-probe: --- Watchpoints DBGWVR/DBGWCR ---\n");
    DUMP_WP(0);
    DUMP_WP(1);
    DUMP_WP(2);
    DUMP_WP(3);
}

/* ================================================================
 * 4. ESR (Exception Syndrome Register) parser
 *
 *  63       32 31    26 25 24                    0
 *  [reserved ] [ EC  ] [IL] [       ISS          ]
 *
 *  EC  (6b) = Exception Class    — what type of exception
 *  IL  (1b) = Instruction Length — 0=16bit(T32), 1=32bit(A64)
 *  ISS (25b)= Instruction Specific Syndrome — details per EC
 *
 *  For debug exceptions, bits[29:27] = DBG_ESR_EVT
 *  which indexes into debug_fault_info[]:
 *    0 = HW breakpoint
 *    1 = single-step
 *    2 = watchpoint
 *    6 = BRK instruction
 * ================================================================ */

/* EC values for debug exceptions */
#define EC_BREAKPT_LOW   0x30   /* EL0 HW breakpoint */
#define EC_BREAKPT_CUR   0x31   /* EL1 HW breakpoint */
#define EC_SOFTSTP_LOW   0x32   /* EL0 single-step */
#define EC_SOFTSTP_CUR   0x33   /* EL1 single-step */
#define EC_WATCHPT_LOW   0x34   /* EL0 watchpoint */
#define EC_WATCHPT_CUR   0x35   /* EL1 watchpoint */
#define EC_BRK64         0x3C   /* AArch64 BRK */
#define EC_SVC64         0x15   /* AArch64 SVC (syscall) */
#define EC_DABT_LOW      0x24   /* EL0 data abort */
#define EC_IABT_LOW      0x20   /* EL0 instruction abort */

static const char *ec_to_string(unsigned int ec)
{
    switch (ec) {
    case 0x00: return "Unknown";
    case 0x01: return "WFI/WFE trapped";
    case 0x0E: return "Illegal execution state";
    case EC_SVC64:       return "SVC (syscall, AArch64)";
    case 0x18: return "MSR/MRS/SysReg trap";
    case EC_IABT_LOW:    return "Instruction Abort (EL0)";
    case 0x21: return "Instruction Abort (EL1)";
    case 0x22: return "PC Alignment fault";
    case EC_DABT_LOW:    return "Data Abort (EL0)";
    case 0x25: return "Data Abort (EL1)";
    case 0x26: return "SP Alignment fault";
    case EC_BREAKPT_LOW: return "HW Breakpoint (EL0)";
    case EC_BREAKPT_CUR: return "HW Breakpoint (EL1)";
    case EC_SOFTSTP_LOW: return "Single-Step (EL0)";
    case EC_SOFTSTP_CUR: return "Single-Step (EL1)";
    case EC_WATCHPT_LOW: return "Watchpoint (EL0)";
    case EC_WATCHPT_CUR: return "Watchpoint (EL1)";
    case 0x38: return "BKPT (AArch32)";
    case EC_BRK64:       return "BRK (AArch64)";
    default:   return "(other)";
    }
}

/*
 * print_esr - parse and print all fields of an ESR value
 */
static void print_esr(uint32_t esr)
{
    unsigned int ec  = (esr >> 26) & 0x3F;    /* bits[31:26] */
    unsigned int il  = (esr >> 25) & 1;        /* bit[25] */
    unsigned int iss = esr & 0x1FFFFFF;        /* bits[24:0] */

    pr_info("kdbg-probe: ---- ESR = 0x%08x ----\n", esr);
    pr_info("kdbg-probe:   EC  = 0x%02x  (%s)\n", ec, ec_to_string(ec));
    pr_info("kdbg-probe:   IL  = %d     (%s)\n", il, il ? "32-bit insn" : "16-bit insn");
    pr_info("kdbg-probe:   ISS = 0x%07x\n", iss);

    /* debug-specific fields */
    switch (ec) {
    case EC_BRK64: {
        /* ISS[15:0] = BRK #imm16 */
        uint16_t brk_imm = (uint16_t)(iss & 0xFFFF);
        const char *known = "user-defined";
        if (brk_imm == 0x004) known = "kprobes";
        else if (brk_imm == 0x005) known = "uprobes";
        else if (brk_imm == 0x006) known = "kprobes-SS";
        else if (brk_imm == 0x100) known = "fault";
        else if (brk_imm == 0x400) known = "kgdb-dynamic";
        else if (brk_imm == 0x800) known = "BUG/WARN";
        else if (brk_imm >= 0x900 && brk_imm <= 0x9FF) known = "KASAN";
        else if (brk_imm == 0xDB0) known = "** kdbg **";
        pr_info("kdbg-probe:   BRK #imm = 0x%03x  (%s)\n", brk_imm, known);
        break;
    }

    case EC_BREAKPT_LOW:
    case EC_BREAKPT_CUR: {
        int evt = (esr >> 27) & 0x7;
        pr_info("kdbg-probe:   DBG_ESR_EVT = %d  (table index -> breakpoint_handler)\n", evt);
        break;
    }

    case EC_SOFTSTP_LOW:
    case EC_SOFTSTP_CUR: {
        int evt = (esr >> 27) & 0x7;
        int ex = iss & 1;  /* ISS bit[0] = EX */
        pr_info("kdbg-probe:   DBG_ESR_EVT = %d  (table index -> single_step_handler)\n", evt);
        pr_info("kdbg-probe:   EX = %d  (%s)\n", ex,
                ex ? "step completed normally" : "step from exception return");
        break;
    }

    case EC_WATCHPT_LOW:
    case EC_WATCHPT_CUR: {
        int evt = (esr >> 27) & 0x7;
        int wnr = (iss >> 6) & 1;  /* ISS bit[6] = WnR */
        pr_info("kdbg-probe:   DBG_ESR_EVT = %d  (table index -> watchpoint_handler)\n", evt);
        pr_info("kdbg-probe:   WnR = %d  (%s)\n", wnr,
                wnr ? "write access" : "read access");
        break;
    }

    default:
        break;
    }
}

/* demo: print some example ESR values */
void probe_esr_examples(void)
{
    pr_info("kdbg-probe: ========== ESR DECODE EXAMPLES ==========\n");

    /* BRK #0xDB0 (kdbg software breakpoint)
     * EC=0x3C=0b111100, IL=1, ISS=0x0DB0
     * encoding: (0x3C << 26) | (1 << 25) | 0xDB0 = 0xF2000DB0 */
    print_esr(0xF2000DB0);

    /* HW breakpoint from EL0
     * EC=0x30=0b110000, IL=0, ISS=0
     * encoding: (0x30 << 26) = 0xC0000000 */
    print_esr(0xC0000000);

    /* Single-step from EL0, EX=1
     * EC=0x32=0b110010, IL=1, ISS=1
     * encoding: (0x32 << 26) | (1 << 25) | 1 = 0xCA000001 */
    print_esr(0xCA000001);

    /* Watchpoint from EL0, write access
     * EC=0x34=0b110100, IL=0, ISS bit6=1
     * encoding: (0x34 << 26) | 0x40 = 0xD0000040 */
    print_esr(0xD0000040);
}
