path = r'C:\Users\24151\Documents\GitHub\xiaojia-hide\KernelPatch\kpms\wxjump\wxjump.c'
with open(path, 'rb') as f:
    data = f.read()

# 1. Add fault counters as static globals (after the region_list definition)
old_list = b'static struct list_head region_list;'
new_list = (b'static LIST_HEAD(region_list);\r\n'
            b'\r\n'
            b'/* Fault counters (debug, no locking needed for approximate counts) */\r\n'
            b'static unsigned long wx_fault_read_count;\r\n'
            b'static unsigned long wx_fault_exec_count;\r\n'
            b'static unsigned long wx_fault_el1_skip_count;')
assert old_list in data, 'region_list not found'
data = data.replace(old_list, new_list, 1)
print('Added fault counters')

# 2. Replace hot-path pr_info in fault handler with counter increments
# Remove the verbose fault log
old_log = (b'    pr_info("wxjump: [fault] far=%lx esr=%lx EC=%lx DFSC=%lx\\n",\r\n'
           b'            far, esr, ec, dfsc);\r\n')
new_log = b''  # Remove entirely
assert old_log in data, 'fault log not found'
data = data.replace(old_log, new_log, 1)
print('Removed hot-path fault log')

# 3. Replace EL1 skip log with counter
old_skip = b'        pr_info("wxjump: [fault] skipping EL1 fault (EC=%lx)\\n", ec);\r\n'
new_skip = b'        wx_fault_el1_skip_count++;\r\n'
assert old_skip in data, 'EL1 skip log not found'
data = data.replace(old_skip, new_skip, 1)
print('Replaced EL1 skip log with counter')

# 4. Replace read_fault log with counter
old_rf = b'    pr_info("wxjump: [read_fault] switching %lx to ORIG_R\\n", page_addr);\r\n'
new_rf = b'    wx_fault_read_count++;\r\n'
assert old_rf in data, 'read_fault log not found'
data = data.replace(old_rf, new_rf, 1)
print('Replaced read_fault log with counter')

# 5. Add counter increment for exec faults - find the exec fault success path
old_exec = b'    pi->state = STATE_SHADOW_X;\r\n    wx_spin_unlock();\r\n\r\n    wxjump_put_region(region);\r\n    return 0;\r\n}'
new_exec = b'    pi->state = STATE_SHADOW_X;\r\n    wx_spin_unlock();\r\n\r\n    wx_fault_exec_count++;\r\n    wxjump_put_region(region);\r\n    return 0;\r\n}'
if old_exec in data:
    data = data.replace(old_exec, new_exec, 1)
    print('Added exec_fault counter')
else:
    print('WARN: exec fault counter pattern not found, skipping')

# 6. Add counter reporting in the control handler or module info
# Find the control handler to add a "stats" command
old_ctrl = b'    pr_info("wxjump: control: %s\\n", args ? args : "(null)");'
new_ctrl = (b'    pr_info("wxjump: control: %s\\n", args ? args : "(null)");\r\n'
            b'    pr_info("wxjump: stats: read_faults=%lu exec_faults=%lu el1_skips=%lu\\n",\r\n'
            b'            wx_fault_read_count, wx_fault_exec_count, wx_fault_el1_skip_count);')
assert old_ctrl in data, 'control handler not found'
data = data.replace(old_ctrl, new_ctrl, 1)
print('Added stats reporting in control handler')

with open(path, 'wb') as f:
    f.write(data)
print('\nAll fixes applied. Hot-path logging removed, counters added.')
