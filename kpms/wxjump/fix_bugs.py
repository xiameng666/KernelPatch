import sys

path = r'C:\Users\24151\Documents\GitHub\xiaojia-hide\KernelPatch\kpms\wxjump\wxjump.c'
with open(path, 'rb') as f:
    data = f.read()

# Fix 1: PTE_BASE_FLAGS AttrIndx=0 -> AttrIndx=4 (MT_NORMAL)
# 0xF03 bits[4:2]=000 -> 0xF13 bits[4:2]=100
old1 = b'#define PTE_BASE_FLAGS       0xF03ULL'
new1 = b'#define PTE_BASE_FLAGS       0xF13ULL'
assert old1 in data, 'PTE_BASE_FLAGS not found'
data = data.replace(old1, new1, 1)
print('Fix 1: PTE_BASE_FLAGS 0xF03 -> 0xF13 (AttrIndx=4 MT_NORMAL)')

# Fix 2: fault handler arg mapping: arg0=far(x0), arg1=esr(x1)
old2 = b'unsigned long far = fargs->arg1;\r\n    unsigned long esr = fargs->arg2;'
new2 = b'unsigned long far = fargs->arg0;    /* x0: FAR_EL1 */\r\n    unsigned long esr = fargs->arg1;    /* x1: ESR_EL1 */'
assert old2 in data, 'fault handler args not found'
data = data.replace(old2, new2, 1)
print('Fix 2: fault handler arg1/arg2 -> arg0/arg1')

# Fix 3: Add debug log after ESR check passes
old3 = b'    wxjump_put_region(region);\r\n\r\n    if ((esr >> 26) == 0x20) {'
new3 = (b'    wxjump_put_region(region);\r\n\r\n'
        b'    pr_info("wxjump: [fault] far=%lx esr=%lx EC=%lx DFSC=%lx\\n",\r\n'
        b'            far, esr, (esr >> 26) & 0x3FUL, esr & 0x3FUL);\r\n\r\n'
        b'    if ((esr >> 26) == 0x20) {')
assert old3 in data, 'fault dispatch block not found'
data = data.replace(old3, new3, 1)
print('Fix 3: Added fault handler debug logging')

with open(path, 'wb') as f:
    f.write(data)
print('All fixes applied successfully')
