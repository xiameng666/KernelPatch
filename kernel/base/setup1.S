/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 内核设置汇编函数 - ARM64内核启动和设置相关代码 */

#define __ASSEMBLY__
#include "./setup.h"
#include "./start.h"

.text
.align 2
.type memcpy8, %function
// 8位字节内存拷贝函数
// x0: 目标地址
// x1: 源地址  
// x2: 拷贝字节数
memcpy8:
    cmp x2, 0                    // 检查拷贝长度是否为0
    ble .l8end                   // 如果小于等于0则结束
.l8loop:
    ldrb w3, [x1], 1             // 从源地址加载1字节，并递增指针
    strb w3, [x0], 1             // 存储到目标地址，并递增指针
    subs x2, x2, 1               // 递减计数器并设置标志
    cbnz x2, .l8loop             // 如果不为0继续循环
.l8end:
    ret                          // 返回

.text
.align 2
.type rmemcpy32, %function
// 32位反向内存拷贝函数（从高地址向低地址拷贝）
// x0: 目标地址
// x1: 源地址
// x2: 拷贝字节数（必须是4的倍数）
rmemcpy32:
    subs x2, x2, #0x4            // 减去4字节并设置标志
    b.lt .r32end                 // 如果小于0则结束
.r32loop:
    ldr w3, [x1, x2]             // 从源地址+偏移加载4字节
    str w3, [x0, x2]             // 存储到目标地址+偏移
    subs x2, x2, #0x4            // 递减偏移4字节
    b.ge .r32loop                // 如果大于等于0继续循环
.r32end:
    ret                          // 返回

.text
.align 2
.type start_prepare, %function
// 启动准备函数 - 初始化内核补丁环境
// x0: 内核物理地址(kernel_pa)
start_prepare:
    stp x29, x30, [sp, -16]!     // 保存帧指针和链接寄存器
    stp x19, x20, [sp, -16]!     // 保存被调用者保存寄存器
    stp x21, x22, [sp, -16]!     // 保存被调用者保存寄存器
    stp x23, x24, [sp, -16]!     // 保存被调用者保存寄存器
    // kernel_pa
    mov x19, x0                  // 保存内核物理地址到x19
    
    // map_data
    adrp x9, map_data            // 获取map_data的页面地址
    add x9, x9, :lo12:map_data   // 获取map_data的完整地址

    // setup_preset
    adrp x10, setup_preset       // 获取setup_preset的页面地址
    add x10, x10, :lo12:setup_preset // 获取setup_preset的完整地址

    // start_preset
    adrp x11, start_preset       // 获取start_preset的页面地址
    add x11, x11, :lo12:start_preset // 获取start_preset的完整地址

    // header
    adrp x12, header             // 获取header的页面地址
    add x12, x12, :lo12:header   // 获取header的完整地址

    // start_preset.kernel_version = setup_preset.kernel_version;
    ldr w13, [x10, #setup_kernel_version_offset] // 从setup_preset加载内核版本
    str w13, [x11, #start_kernel_version_offset] // 存储到start_preset

    // start_preset.kallsyms_lookup_name_offset = setup_preset.set.kallsyms_lookup_name_offset;
    ldr x13, [x10, #setup_kallsyms_lookup_name_offset_offset] // 加载kallsyms查找函数偏移
    str x13, [x11, #start_kallsyms_lookup_name_offset_offset] // 存储到start_preset

    // start_preset.kernel_size = setup_preset.kernel_size;
    ldr x13, [x10, #setup_kernel_size_offset] // 加载内核大小
    str x13, [x11, #start_kernel_size_offset] // 存储到start_preset

    // start_preset.start_offset = setup_preset.start_offset;
    ldr x13, [x10, #setup_start_offset_offset] // 加载启动偏移
    str x13, [x11, #start_start_offset_offset] // 存储到start_preset
    mov x21, x13                 // 保存启动偏移到x21

    // start_preset.extra_size = setup_preset.extra_size;
    ldr x13, [x10, #setup_extra_size_offset] // 加载额外大小
    str x13, [x11, #start_extra_size_offset] // 存储到start_preset

    // start_preset.kernel_pa = kernel_pa;
    str x19, [x11, #start_kernel_pa_offset] // 存储内核物理地址到start_preset
    
    // start_preset.map_offset = setup_preset.map_offset
    ldr x13, [x10, #setup_map_offset_offset] // 加载映射偏移
    str x13, [x11, #start_map_offset_offset] // 存储到start_preset
    mov x20, x13                 // 保存映射偏移到x20

    // memcpy(&start_preset.setup, &setup_header, KP_HEADER_SIZE);
    add x0, x11, #start_header_offset // 计算start_preset.header地址
    add x1, x12, #0              // header地址
    mov x2, #KP_HEADER_SIZE      // 头部大小
    bl memcpy8                   // 调用内存拷贝函数

    // memcpy(start_preset.superkey, setup_preset.superkey, SUPER_KEY_LEN);
    add x0, x11, #start_superkey_offset // 计算start_preset.superkey地址
    add x1, x10, #setup_superkey_offset // 计算setup_preset.superkey地址
    mov x2, #SUPER_KEY_LEN       // 超级密钥长度
    bl memcpy8                   // 调用内存拷贝函数

    // memcpy(start_preset.root_superkey, setup_preset.root_superkey, ROOT_SUPER_KEY_HASH_LEN);
    add x0, x11, #start_root_superkey_offset // 计算start_preset.root_superkey地址
    add x1, x10, #setup_root_superkey_offset // 计算setup_preset.root_superkey地址
    mov x2, #ROOT_SUPER_KEY_HASH_LEN // 根超级密钥哈希长度
    bl memcpy8                   // 调用内存拷贝函数

    // memcpy(&start_preset.patch_config, &setup_preset.patch_config, sizeof(header.patch_config));
    add x0, x11, #start_patch_config_offset // 计算start_preset.patch_config地址
    add x1, x10, #setup_patch_config_offset // 计算setup_preset.patch_config地址
    mov x2, #PATCH_CONFIG_LEN    // 补丁配置长度
    bl memcpy8                   // 调用内存拷贝函数

    // backup map area
    // 备份映射区域
    // memcpy(start_preset.map_backup, kernel_pa + setup_preset.map_offset, (uint64_t)_map_end - (uint64_t)_map_start)
    adrp x13, _map_end           // 获取映射结束地址
    add x13, x13, :lo12:_map_end // 完整的映射结束地址
    adrp x14, _map_start         // 获取映射开始地址
    add x14, x14, :lo12:_map_start // 完整的映射开始地址
    sub x2, x13, x14             // 计算映射区域大小

    // start_preset.map_backup_len = (uint64_t)_map_end - (uint64_t)_map_start
    str x2, [x11, #start_map_backup_len_offset] // 存储映射备份长度
    add x0, x11, #start_map_backup_offset // 计算备份目标地址
    add x1, x19, x20             // 计算源地址（内核物理地址+映射偏移）
    bl memcpy8                   // 调用内存拷贝函数

    // uint64_t start_img_size = setup_preset.kpimg_size - (_kp_start - _link_base)
    // 计算启动镜像大小
    ldr x22, [x10, #setup_kpimg_size_offset] // 加载KP镜像大小
    adrp x23, _kp_start          // 获取KP启动地址
    add x23, x23, :lo12:_kp_start // 完整的KP启动地址
    adrp x24, _link_base         // 获取链接基地址
    add x24, x24, :lo12:_link_base // 完整的链接基地址
    sub x23, x23, x24            // 计算偏移
    sub x22, x22, x23            // 计算实际启动镜像大小

    // map_data.start_img_size = start_img_size;
    str x22, [x9, #map_start_img_size_offset] // 存储启动镜像大小到map_data

    // start and extra
    // 拷贝启动和额外数据
    // memcpy(kernel_pa + start_offset, (uint64_t)_kp_start, start_img_size + setup_preset.extra_size)
    add x0, x19, x21             // 计算目标地址（内核物理地址+启动偏移）
    adrp x1, _kp_start           // 获取KP启动地址
    add x1, x1, :lo12:_kp_start  // 完整的KP启动地址
    ldr x2, [x10, #setup_extra_size_offset] // 加载额外大小
    add x2, x2, x22              // 计算总拷贝大小（启动镜像+额外）
    bl rmemcpy32                 // 调用反向内存拷贝函数

    // Restore
    // 恢复寄存器
    ldp x23, x24, [sp], 16       // 恢复x23,x24
    ldp x21, x22, [sp], 16       // 恢复x21,x22
    ldp x19, x20, [sp], 16       // 恢复x19,x20
    ldp x29, x30, [sp], 16       // 恢复帧指针和链接寄存器
    ret                          // 返回

.text
.align 2
.type map_prepare, %function
// 映射准备函数 - 准备内核页表映射相关数据
// x0: 内核物理地址(kernel_pa)
map_prepare:
    stp x29, x30, [sp, -16]!     // 保存帧指针和链接寄存器
    stp x19, x20, [sp, -16]!     // 保存被调用者保存寄存器
    mov x19, x0                  // 保存内核物理地址
    // map_data
    adrp x9, map_data            // 获取map_data的页面地址
    add x9, x9, :lo12:map_data   // 获取map_data的完整地址
    // setup_preset
    adrp x10, setup_preset       // 获取setup_preset的页面地址
    add x10, x10, :lo12:setup_preset // 获取setup_preset的完整地址

    // map_data.kernel_pa = kernel_pa;
    str x19, [x9, #map_kernel_pa_offset] // 存储内核物理地址到map_data

    // map_data.map_offset = setup_preset.map_offset;
    ldr x11, [x10, #setup_map_offset_offset] // 加载映射偏移
    str x11, [x9, #map_map_offset_offset]   // 存储映射偏移到map_data
    mov x14, x11                 // 保存映射偏移到x14

    // map_data.paging_init_relo = setup_preset.paging_init_offset;
    ldr x11, [x10, #setup_paging_init_offset_offset] // 加载分页初始化偏移
    str x11, [x9, #map_paging_init_relo_offset]      // 存储到map_data
    mov x15, x11                 // 保存分页初始化偏移到x15

    // map_data.map_symbol = setup_preset.map_symbol
    add x0, x9, #map_map_symbol_offset  // 计算map_data.map_symbol地址
    add x1, x10, #setup_map_symbol_offset // 计算setup_preset.map_symbol地址
    mov x2, #MAP_SYMBOL_SIZE     // 映射符号大小
    bl memcpy8                   // 调用内存拷贝函数
    
#ifdef MAP_DEBUG
    // map_data.printk_relo = setup_preset.printk_offset;
    ldr x11, [x10, #setup_printk_offset_offset] // 加载printk偏移
    str x11, [x9, #map_printk_relo_offset]      // 存储到map_data
#endif
    // set start memory info
    // 设置启动内存信息
    // map_data.start_offset = setup_preset.start_offset;
    ldr x11, [x10, #setup_start_offset_offset]  // 加载启动偏移
    str x11, [x9, #map_start_offset_offset]     // 存储到map_data

    // map_data.start_size = (int64_t)(_kp_end - _kp_start);
    adrp x11, _kp_end            // 获取KP结束地址
    add x11, x11, :lo12:_kp_end  // 完整的KP结束地址
    adrp x12, _kp_start          // 获取KP开始地址
    add x12, x12, :lo12:_kp_start // 完整的KP开始地址
    sub x11, x11, x12            // 计算KP大小
    str x11, [x9, #map_start_size_offset] // 存储启动大小到map_data
    
    // map_data.extra_size = setup_preset.extra_size;
    ldr x11, [x10, #setup_extra_size_offset]   // 加载额外大小
    str x11, [x9, #map_extra_size_offset]      // 存储到map_data

    // map_data.alloc_size = HOOK_ALLOC_SIZE + MEMORY_ROX_SIZE + MEMORY_RW_SIZE;
    mov x11, #HOOK_ALLOC_SIZE    // 钩子分配大小
    add x11, x11, #MEMORY_ROX_SIZE // 加上只读执行内存大小
    add x11, x11, #MEMORY_RW_SIZE  // 加上读写内存大小
    str x11, [x9, #map_alloc_size_offset] // 存储总分配大小

    // backup and hook paging_init
    // 备份并钩取分页初始化函数
    // uint64_t paging_init_pa = paging_init_offset + kernel_pa;
    add x13, x15, x19            // 计算分页初始化物理地址
    // map_data.paging_init_backup = *(uint32_t *)(paging_init_pa);
    ldr w12, [x13]               // 加载原始指令

    mov w3, #0x201F              // NOP指令低16位
    movk w3, #0xD503, lsl#16     // NOP指令高16位
    orr w1, w3, #0x100           // 构造目标指令
    mov w2, #0xFFFFFD1F          // 掩码
    and w0, w12, w2              // 应用掩码
    // if ((map_data.paging_init_backup & 0xFFFFFD1F) == 0xD503211F)
    cmp w0, w1                   // 比较是否为特定指令
    b.ne .backup                 // 如果不是则跳转到备份
    // map_data.paging_init_backup = NOP
    mov w12, w3                  // 使用NOP指令
    // uint32_t *p = (uint32_t *)paging_init_pa + 1;
    add x11, x13, #4             // 移动到下一个指令
.cmp_auti:
    // while ((*p & 0xFFFFFD1F) != 0xD503211F) ++p;
    ldr w0, [x11], #4            // 加载指令并移动指针
    and w0, w0, w2               // 应用掩码
    cmp w0, w1                   // 比较指令
    b.ne .cmp_auti               // 继续查找直到找到目标指令
    // *p = NOP
    stur w3, [x11, #-4]          // 将目标指令替换为NOP

.backup:
    str w12, [x9, #map_paging_init_backup_offset] // 存储备份指令
    dsb ish                      // 数据同步屏障

    // uint64_t replace_offset = (uint64_t)(_paging_init - _map_start) + map_offset;
    adrp x11, _paging_init       // 获取_paging_init地址
    add x11, x11, :lo12:_paging_init // 完整的_paging_init地址
    adrp x12, _map_start         // 获取_map_start地址
    add x12, x12, :lo12:_map_start   // 完整的_map_start地址
    sub x11, x11, x12            // 计算偏移
    add x11, x11, x14            // 加上映射偏移

    // *(uint32_t *)paging_init_pa = B_REL(paging_init_offset, replace_offset);
    // 计算分支指令
    // #define B_REL(src, dst) (0x14000000u | (((dst - src) & 0x0FFFFFFFu) >> 2u))
    sub x15, x11, x15            // 计算相对偏移
    ubfx w15, w15, #2, #26       // 提取26位偏移并右移2位
    mov w12, #0x14000000         // 分支指令编码
    orr w15, w15, w12            // 组合成完整的分支指令
    str w15, [x13]               // 写入分支指令

    // relocate map
    // 重定位映射代码
    // memcpy(preset.map_offset + kernel_pa, (uint64_t)_map_start, (int64_t)(_map_end - _map_start));
    adrp x2, _map_end            // 获取映射结束地址
    add x2, x2, :lo12:_map_end   // 完整的映射结束地址
    adrp x1, _map_start          // 获取映射开始地址
    add x1, x1, :lo12:_map_start // 完整的映射开始地址
    sub x2, x2, x1               // 计算映射大小
    add x0, x19, x14             // 计算目标地址（内核物理地址+映射偏移）
    bl memcpy8                   // 调用内存拷贝函数

    // Restore
    // 恢复寄存器
    ldp x19, x20, [sp], 16       // 恢复x19,x20
    ldp x29, x30, [sp], 16       // 恢复帧指针和链接寄存器

    dsb ish                      // 数据同步屏障
    ret                          // 返回

.text
.align 2
.type setup, %function
// 主设置函数 - 执行内核补丁的主要设置工作
setup:
    // Save
    // 保存所有寄存器
    stp x29, x30, [sp, -16]!     // 保存帧指针和链接寄存器
    stp x0, x1, [sp, -16]!       // 保存参数寄存器
    stp x2, x3, [sp, -16]!       // 保存参数寄存器
    stp x4, x5, [sp, -16]!       // 保存参数寄存器
    stp x6, x7, [sp, -16]!       // 保存参数寄存器
    stp x8, x18, [sp, -16]!      // 保存工作寄存器
    stp x19, x20, [sp, -16]!     // 保存被调用者保存寄存器
    stp x21, x22, [sp, -16]!     // 保存被调用者保存寄存器
    stp x23, x24, [sp, -16]!     // 保存被调用者保存寄存器
    stp x25, x26, [sp, -16]!     // 保存被调用者保存寄存器
    stp x27, x28, [sp, -16]!     // 保存被调用者保存寄存器

    // _link_base
    adrp x9, _link_base          // 获取链接基地址
    add x9, x9, :lo12:_link_base // 完整的链接基地址

    // setup_preset
    adrp x10, setup_preset       // 获取setup_preset地址
    add x10, x10, :lo12:setup_preset // 完整的setup_preset地址
    mov x20, x10                 // 保存setup_preset地址

    // uint64 kernel_pa = (uint64_t)_link_base - setup_preset.setup_offset;
    ldr x11, [x10, #setup_setup_offset_offset] // 加载设置偏移
    sub x12, x9, x11             // 计算内核物理地址
    mov x19, x12                 // 保存内核物理地址

    mov x0, x19                  // 传递内核物理地址参数
    bl start_prepare             // 调用启动准备函数
    mov x0, x19                  // 传递内核物理地址参数
    bl map_prepare               // 调用映射准备函数

    // memcpy(kernel_pa, (uint64_t)setup_preset.header_backup, sizeof(setup_preset.header_backup));
    mov x0, x19                  // 目标地址（内核物理地址）
    add x1, x20, #setup_header_backup_offset // 源地址（头部备份）
    mov x2, #HDR_BACKUP_SIZE     // 头部备份大小
    bl memcpy8                   // 调用内存拷贝函数

    // I-cache = on or off,
    // 刷新指令缓存
    dsb ish                      // 数据同步屏障
    ic iallu                     // 无效化所有指令缓存
    dsb ish                      // 数据同步屏障
    isb                          // 指令同步屏障

    mov x16, x19                 // 保存内核物理地址到x16

    // Restore
    // 恢复所有寄存器
    ldp x27, x28, [sp], 16       // 恢复被调用者保存寄存器
    ldp x25, x26, [sp], 16       // 恢复被调用者保存寄存器
    ldp x23, x24, [sp], 16       // 恢复被调用者保存寄存器
    ldp x21, x22, [sp], 16       // 恢复被调用者保存寄存器
    ldp x19, x20, [sp], 16       // 恢复被调用者保存寄存器
    ldp x8, x18, [sp], 16        // 恢复工作寄存器
    ldp x6, x7, [sp], 16         // 恢复参数寄存器
    ldp x4, x5, [sp], 16         // 恢复参数寄存器
    ldp x2, x3, [sp], 16         // 恢复参数寄存器
    ldp x0, x1, [sp], 16         // 恢复参数寄存器
    ldp x29, x30, [sp], 16       // 恢复帧指针和链接寄存器

    // Restore sp
    // 恢复堆栈指针
    ldp x9, x10, [sp], 16        // 恢复原始堆栈指针
    mov sp, x9                   // 设置堆栈指针
    // _head
    br x16                       // 跳转到内核头部


.section .entry.text, "ax"
.global setup_entry
.type entry, %function
// 设置入口函数 - 内核补丁的主要入口点
// x0: 指向FDT blob的物理地址
setup_entry:
    // x0 = physical address to the FDT blob.
    // Preserve the arguments passed by the bootloader in x0 .. x3
    // 保留引导加载程序在x0..x3中传递的参数
    mov x9, sp                   // 保存当前堆栈指针
    adrp x11, stack              // 获取栈空间地址
    add x11, x11, :lo12:stack    // 完整的栈地址
    add x11, x11, STACK_SIZE     // 计算栈顶地址
    mov sp, x11                  // 设置新的堆栈指针
    stp x9, x10, [sp, -16]!      // 保存原始堆栈指针 
    b setup                      // 跳转到主设置函数

#undef __ASSEMBLY__