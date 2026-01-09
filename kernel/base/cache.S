// ARM64缓存操作汇编实现
#define ASM_NL	;

#define ALIGN	.align 4,0x90
#define ALIGN_STR	".align 4,0x90"

#define	DMA_BIDIRECTIONAL 0
#define DMA_TO_DEVICE 1
#define DMA_FROM_DEVICE 2
#define DMA_NONE = 3

#define USER(l, x...)				\
9999:	x;					\
	# todo:
	# .section __ex_table,"a";		\
	.align	3;				\
	.quad	9999b,l;			\
	.previous

// 函数入口点宏定义
#define ENTRY(name) \
	.globl name ASM_NL \
	ALIGN ASM_NL \
	name:

// 函数结束标记宏定义
#define END(name) \
	.size name, .-name

// 过程结束标记宏定义
#define ENDPROC(name) \
	.type name, @function ASM_NL \
	END(name)

// 从CTR寄存器获取最小D-cache行大小的宏
	.macro	dcache_line_size, reg, tmp
	mrs	\tmp, ctr_el0			// 读取CTR寄存器
	ubfm	\tmp, \tmp, #16, #19		// 提取缓存行大小编码
	mov	\reg, #4			// 每字4字节
	lsl	\reg, \reg, \tmp		// 计算实际缓存行大小
	.endm

// 从CTR寄存器获取最小I-cache行大小的宏
	.macro	icache_line_size, reg, tmp
	mrs	\tmp, ctr_el0			// 读取CTR寄存器
	and	\tmp, \tmp, #0xf		// 提取缓存行大小编码
	mov	\reg, #4			// 每字4字节
	lsl	\reg, \reg, \tmp		// 计算实际缓存行大小
	.endm

// 刷新整个D-cache
// 损坏的寄存器：x0-x7, x9-x11
ENTRY(__flush_dcache_all)
	dmb	sy				// 确保与先前内存访问的顺序
	mrs	x0, clidr_el1			// 读取clidr寄存器
	and	x3, x0, #0x7000000		// 从clidr提取loc字段
	lsr	x3, x3, #23			// 左对齐loc位字段
	cbz	x3, finished			// 如果loc为0，则无需清理
	mov	x10, #0				// 从缓存级别0开始清理
loop1:
	add	x2, x10, x10, lsr #1		// 计算3倍当前缓存级别
	lsr	x1, x0, x2			// 从clidr提取缓存类型位
	and	x1, x1, #7			// 仅保留当前缓存的类型位
	cmp	x1, #2				// 查看此级别的缓存类型
	b.lt	skip				// 如果无缓存或仅i-cache则跳过
	# todo:
	# save_and_disable_irqs x9		// 使CSSELR和CCSIDR访问原子化
	msr	csselr_el1, x10			// 在csselr中选择当前缓存级别
	isb					// isb同步新的cssr&csidr
	mrs	x1, ccsidr_el1			// 读取新的ccsidr
	# todo
	# restore_irqs x9
	and	x2, x1, #7			// 提取缓存行长度
	add	x2, x2, #4			// 添加4（行长度偏移）
	mov	x4, #0x3ff
	and	x4, x4, x1, lsr #3		// 找到way大小的最大值
	clz	w5, w4				// 找到way大小增量的位位置
	mov	x7, #0x7fff
	and	x7, x7, x1, lsr #13		// 提取索引大小的最大值
loop2:
	mov	x9, x4				// 创建最大way大小的工作副本
loop3:
	lsl	x6, x9, x5
	orr	x11, x10, x6			// 将way和缓存编号因子化到x11
	lsl	x6, x7, x2
	orr	x11, x11, x6			// 将索引编号因子化到x11
	dc	cisw, x11			// 通过set/way清理和无效化
	subs	x9, x9, #1			// 递减way
	b.ge	loop3
	subs	x7, x7, #1			// 递减索引
	b.ge	loop2
skip:
	add	x10, x10, #2			// 增加缓存编号
	cmp	x3, x10
	b.gt	loop1
finished:
	mov	x10, #0				// 切换回缓存级别0
	msr	csselr_el1, x10			// 在csselr中选择当前缓存级别
	dsb	sy
	isb
	ret
ENDPROC(__flush_dcache_all)

// 刷新整个缓存系统
// 现在使用从L1缓存向外工作的原子清理/无效化来实现数据缓存刷新
// 这是通过基于Set/Way的缓存维护指令完成的
// 指令缓存仍然可以通过单个指令无效化到统一点
ENTRY(flush_cache_all)
	mov	x12, lr          // 保存链接寄存器
	bl	__flush_dcache_all    // 调用D-cache刷新函数
	mov	x0, #0
	ic	ialluis          // I+BTB缓存无效化
	ret	x12              // 返回
ENDPROC(flush_cache_all)

// 刷新指定范围的指令缓存
// 确保指定区域内的I和D缓存一致
// 通常在代码写入内存区域后使用，即将执行时调用
// start - 区域的虚拟起始地址
// end   - 区域的虚拟结束地址
ENTRY(flush_icache_range)
	/* FALLTHROUGH */

// 刷新用户范围的缓存
// 确保指定区域内的I和D缓存一致
// 通常在代码写入内存区域后使用，即将执行时调用
// start - 区域的虚拟起始地址
// end   - 区域的虚拟结束地址
ENTRY(__flush_cache_user_range)
	dcache_line_size x2, x3      // 获取D-cache行大小
	sub	x3, x2, #1               // 计算对齐掩码
	bic	x4, x0, x3               // 对齐起始地址
1:
USER(9f, dc	cvau, x4	)		// 清理D行到PoU（统一点）
	add	x4, x4, x2               // 移动到下一个缓存行
	cmp	x4, x1                   // 检查是否到达结束地址
	b.lo	1b                   // 循环直到完成
	dsb	ish                      // 数据同步屏障

	icache_line_size x2, x3      // 获取I-cache行大小
	sub	x3, x2, #1               // 计算对齐掩码
	bic	x4, x0, x3               // 对齐起始地址
1:
USER(9f, ic	ivau, x4	)		// 无效化I行PoU
	add	x4, x4, x2               // 移动到下一个缓存行
	cmp	x4, x1                   // 检查是否到达结束地址
	b.lo	1b                   // 循环直到完成
9:						// 忽略任何缓存操作错误
	dsb	ish                      // 数据同步屏障
	isb                          // 指令同步屏障
	ret
ENDPROC(flush_icache_range)
ENDPROC(__flush_cache_user_range)

// 刷新D-cache区域
// 确保页面kaddr中保存的数据写回到相应页面
// kaddr - 内核地址
// size  - 问题区域的大小
ENTRY(__flush_dcache_area)
	dcache_line_size x2, x3      // 获取D-cache行大小
	add	x1, x0, x1               // 计算结束地址
	sub	x3, x2, #1               // 计算对齐掩码
	bic	x0, x0, x3               // 对齐起始地址
1:	dc	civac, x0			// 清理并无效化D行/统一行
	add	x0, x0, x2               // 移动到下一个缓存行
	cmp	x0, x1                   // 检查是否到达结束地址
	b.lo	1b                   // 循环直到完成
	dsb	sy                       // 系统数据同步屏障
	ret
ENDPROC(__flush_dcache_area)

// 无效化缓存范围
// start - 区域起始地址
// end   - 区域结束地址
ENTRY(__inval_cache_range)
	/* FALLTHROUGH */

// DMA无效化范围
// start - 区域的虚拟起始地址
// end   - 区域的虚拟结束地址
__dma_inv_range:
	dcache_line_size x2, x3      // 获取D-cache行大小
	sub	x3, x2, #1               // 计算对齐掩码
	tst	x1, x3			// 结束地址是否缓存行对齐？
	bic	x1, x1, x3               // 对齐结束地址
	b.eq	1f                   // 如果对齐则跳转
	dc	civac, x1			// 清理并无效化D/U行
1:	tst	x0, x3			// 起始地址是否缓存行对齐？
	bic	x0, x0, x3               // 对齐起始地址
	b.eq	2f                   // 如果对齐则跳转
	dc	civac, x0			// 清理并无效化D/U行
	b	3f                       // 跳转到循环
2:	dc	ivac, x0			// 无效化D/U行
3:	add	x0, x0, x2           // 移动到下一个缓存行
	cmp	x0, x1                   // 检查是否到达结束地址
	b.lo	2b                   // 循环直到完成
	dsb	sy                       // 系统数据同步屏障
	ret
ENDPROC(__inval_cache_range)
ENDPROC(__dma_inv_range)

/*
 *	__dma_clean_range(start, end)
 *	- start   - virtual start address of region
 *	- end     - virtual end address of region
 */
__dma_clean_range:
	dcache_line_size x2, x3
	sub	x3, x2, #1
	bic	x0, x0, x3
	# dc cvac, x0
	dc civac, x0
	add	x0, x0, x2
	cmp	x0, x1
	b.lo	1b
	dsb	sy
	ret
ENDPROC(__dma_clean_range)

// DMA刷新范围（清理并无效化）
// start - 区域的虚拟起始地址
// end   - 区域的虚拟结束地址
ENTRY(__dma_flush_range)
	dcache_line_size x2, x3      // 获取D-cache行大小
	sub	x3, x2, #1               // 计算对齐掩码
	bic	x0, x0, x3               // 对齐起始地址到缓存行边界
1:	dc	civac, x0		     // 清理并无效化D/U行
	add	x0, x0, x2               // 移动到下一个缓存行
	cmp	x0, x1                   // 检查是否到达结束地址
	b.lo	1b                   // 循环直到完成
	dsb	sy                       // 系统数据同步屏障
	ret
ENDPROC(__dma_flush_range)

// DMA映射区域
// start - 内核虚拟起始地址
// size  - 区域大小
// dir   - DMA方向
ENTRY(__dma_map_area)
	add	x1, x1, x0               // 计算结束地址
	cmp	w2, #DMA_FROM_DEVICE     // 检查DMA方向是否为设备到内存
	b.eq	__dma_inv_range      // 如果是则无效化范围
	b	__dma_clean_range        // 否则清理范围
ENDPROC(__dma_map_area)

// DMA取消映射区域
// start - 内核虚拟起始地址
// size  - 区域大小
// dir   - DMA方向
ENTRY(__dma_unmap_area)
	add	x1, x1, x0               // 计算结束地址
	cmp	w2, #DMA_TO_DEVICE       // 检查DMA方向是否为内存到设备
	b.ne	__dma_inv_range      // 如果不是则无效化范围
	ret                          // 否则直接返回
ENDPROC(__dma_unmap_area)