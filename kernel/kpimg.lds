/* KernelPatch ARM64 链接脚本 - 定义内核补丁镜像的内存布局 */
OUTPUT_ARCH(aarch64)

_link_base = 64k - 3 * 4k;   /* 链接基地址：64KB - 12KB */

SECTIONS 
{
    /DISCARD/ : { *(.discard) *(.discard.*) }  /* 丢弃不需要的段 */
    /DISCARD/ : {  /* 丢弃动态链接相关段 */
        *(.interp .dynamic)
        *(.dynsym .dynstr .hash .gnu.hash)
    }
    . = _link_base;              /* 设置当前位置为链接基地址 */
    _link_base = .;              /* 记录链接基地址 */

    .setup.data : {              /* 设置数据段 */
        _setup_start = .;        /* 记录设置开始地址 */
        base/setup.o(.setup.header)  /* 设置头部 */
        . = _setup_start + 64;   /* 偏移64字节 */
        base/setup.o(.setup.preset)  /* 设置预设数据 */
        base/setup.o(.setup.data)    /* 设置数据 */
        . = _setup_start + 4K;   /* 对齐到4KB边界 */
    }
    
    .setup.text : {             /* 设置代码段 */
        base/setup1.o(.entry.text)   /* 入口代码 */
        base/setup1.o(.text)         /* 设置代码 */
        base/setup.o(.text)          /* 设置代码 */
        _setup_end = .;          /* 记录设置结束地址 */
    }

    . = ALIGN(16);               /* 16字节对齐 */
    .setup.map : {               /* 设置映射段 */
        _map_start = .;          /* 记录映射开始地址 */
        base/map.o(.map.data)    /* 映射数据 */
        base/map.o(.map.text)    /* 映射代码 */
        base/map.o(.text)        /* 映射代码 */
        base/map1.o(.text)       /* 映射汇编代码 */
        . = ALIGN(16);           /* 16字节对齐 */
        _map_end = .;            /* 记录映射结束地址 */
    }
    ASSERT(SIZEOF(.setup.map) < 0xa00, "Size too large of .setup.map!")  /* 断言映射段大小小于2560字节 */

    . = ALIGN(64K);              /* 64KB对齐 */
    _kp_start = .;               /* 记录KP开始地址 */
    .kp.text : {                 /* KP代码段 */
        _kp_text_start = .;      /* 记录KP代码开始地址 */

        base/start.o(.start.text)     /* 启动代码 */
        base/start.o(.text)           /* 启动代码 */
        
        base/hook.o(.transit0.text);      /* 钩子传输0代码 */
        _transit0_end = .;               /* 记录传输0结束地址 */
        base/hook.o(.transit4.text);      /* 钩子传输4代码 */
        _transit4_end = .;               /* 记录传输4结束地址 */
        base/hook.o(.transit8.text);      /* 钩子传输8代码 */
        _transit8_end = .;               /* 记录传输8结束地址 */
        base/hook.o(.transit12.text);     /* 钩子传输12代码 */
        _transit12_end = .;              /* 记录传输12结束地址 */

        base/fphook.o(.fp.transit0.text);  /* 函数指针钩子传输0代码 */
        _fp_transit0_end = .;            /* 记录函数指针传输0结束地址 */
        base/fphook.o(.fp.transit4.text);  /* 函数指针钩子传输4代码 */
        _fp_transit4_end = .;            /* 记录函数指针传输4结束地址 */
        base/fphook.o(.fp.transit8.text);  /* 函数指针钩子传输8代码 */
        _fp_transit8_end = .;            /* 记录函数指针传输8结束地址 */
        base/fphook.o(.fp.transit12.text); /* 函数指针钩子传输12代码 */
        _fp_transit12_end = .;           /* 记录函数指针传输12结束地址 */

        base/*(.text)                    /* 所有base目录的代码段 */
        base/*(.rodata*)                 /* 所有base目录的只读数据段 */

        *(.text)                         /* 所有代码段 */
        *(.rodata*)                      /* 所有只读数据段 */
        /*  *(.got) */                   /* 全局偏移表（已注释） */

        . = ALIGN(16);                   /* 16字节对齐 */
        _kp_text_end = .;                /* 记录KP代码结束地址 */
    }

    . = ALIGN(64k);                      /* 64KB对齐 */
    .kp.data : {                         /* KP数据段 */
        _kp_data_start = .;              /* 记录KP数据开始地址 */
        base/start.o(.start.data)        /* 启动数据 */
        base/*(.data)                    /* 所有base目录的数据段 */
        base/*(.bss)                     /* 所有base目录的BSS段 */
        *(.data)                         /* 所有数据段 */
        *(.bss)                          /* 所有BSS段 */
        . = ALIGN(16);                   /* 16字节对齐 */
        _kp_symbol_start = .;            /* 记录KP符号开始地址 */
        *(.kp.symbol)                    /* KP符号段 */
        _kp_symbol_end = .;              /* 记录KP符号结束地址 */
        _kp_data_end = .;                /* 记录KP数据结束地址 */
    }

    .got.plt : { *(.got.plt) }           /* 过程链接表 */
    ASSERT(SIZEOF(.got.plt) == 0, "Unexpected GOT/PLT entries detected!")  /* 断言PLT表为空 */

    . = ALIGN(64k);                      /* 64KB对齐 */
    _kp_end = .;                         /* 记录KP结束地址 */

    _link_end = .;                       /* 记录链接结束地址 */

    .got : {                             /* 全局偏移表 */
        *(.got)
    }
    ASSERT(SIZEOF(.got) == 0, "Unexpected GOT detected!")  /* 断言GOT表为空 */
    
    .plt : {                             /* 过程链接表 */
        *(.plt) *(.plt.*) *(.iplt) *(.igot .igot.plt)
    }
    ASSERT(SIZEOF(.plt) == 0, "Unexpected run-time procedure linkages detected!")  /* 断言PLT表为空 */


    .data.rel.ro : {                     /* 只读重定位数据 */
        *(.data.rel.ro) 
    }
    ASSERT(SIZEOF(.data.rel.ro) == 0, "Unexpected RELRO detected!")  /* 断言RELRO为空 */
    
    .rela.dyn : {                        /* 动态重定位表 */
        *(.rela .rela*)
    }
    ASSERT(SIZEOF(.rela.dyn) == 0, "Unexpected RELRDYN detected!")  /* 断言动态重定位表为空 */
}