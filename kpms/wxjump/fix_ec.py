path = r'C:\Users\24151\Documents\GitHub\xiaojia-hide\KernelPatch\kpms\wxjump\wxjump.c'
with open(path, 'rb') as f:
    data = f.read()

# Replace the fault handler dispatch logic:
# After ESR permission fault check, add EC filter for EL0 only
# EC=0x24: Data Abort from lower EL (EL0)
# EC=0x20: Instruction Abort from lower EL (EL0)
# EC=0x25/0x21: from current EL (EL1) -> skip, let kernel handle

old = (b'    pr_info("wxjump: [fault] far=%lx esr=%lx EC=%lx DFSC=%lx\\n",\r\n'
       b'            far, esr, (esr >> 26) & 0x3FUL, esr & 0x3FUL);\r\n'
       b'\r\n'
       b'    if ((esr >> 26) == 0x20) {')

new = (b'    unsigned long ec = (esr >> 26) & 0x3FUL;\r\n'
       b'    unsigned long dfsc = esr & 0x3FUL;\r\n'
       b'\r\n'
       b'    pr_info("wxjump: [fault] far=%lx esr=%lx EC=%lx DFSC=%lx\\n",\r\n'
       b'            far, esr, ec, dfsc);\r\n'
       b'\r\n'
       b'    /* Only handle EL0 faults. EL1 faults (EC=0x21/0x25) are from kernel\r\n'
       b'     * (e.g. EPAN blocking EL1 access to UXN=0 pages) -> let kernel handle */\r\n'
       b'    if (ec != 0x24 && ec != 0x20) {\r\n'
       b'        pr_info("wxjump: [fault] skipping EL1 fault (EC=%lx)\\n", ec);\r\n'
       b'        kfn_mmput(mm);\r\n'
       b'        return;\r\n'
       b'    }\r\n'
       b'\r\n'
       b'    if (ec == 0x20) {')

assert old in data, 'fault dispatch block not found'
data = data.replace(old, new, 1)
print('Fix: Added EC filter for EL0-only fault handling')

# Also fix the second branch: the old code checked (esr >> 26) == 0x20
# but now we use ec variable, and the data abort check changes
old2 = (b'    } else if (!(esr & 0x40)) {\r\n'
        b'        /* Data Abort, WnR=0')
new2 = (b'    } else if (ec == 0x24 && !(esr & 0x40)) {\r\n'
        b'        /* Data Abort from EL0, WnR=0')
assert old2 in data, 'data abort branch not found'
data = data.replace(old2, new2, 1)
print('Fix: Updated data abort branch to check EC=0x24')

# Also add logging to read/exec fault handlers
old3 = b'    /* switch to orig page: UXN + read-only */'
if old3 not in data:
    old3 = b'    ret = wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_UXN_USER_RO);'
    new3 = (b'    pr_info("wxjump: [read_fault] switching %lx to ORIG_R\\n", page_addr);\r\n'
            b'    ret = wxjump_switch_mapping(vma, page_addr, pi->orig_pfn, PTE_UXN_USER_RO);')
    assert old3 in data, 'read fault switch not found'
    data = data.replace(old3, new3, 1)
else:
    # Insert before the comment
    new3 = (b'    pr_info("wxjump: [read_fault] switching %lx to ORIG_R\\n", page_addr);\r\n'
            b'    /* switch to orig page: UXN + read-only */')
    data = data.replace(old3, new3, 1)
print('Fix: Added read_fault logging')

with open(path, 'wb') as f:
    f.write(data)
print('All fixes applied')
