/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2024 bmax121. All Rights Reserved.
 */

#include "symbol.h"
#include "common.h"

// 符号查找回调函数的用户数据结构
struct on_each_symbol_struct
{
    const char *symbol;    // 要查找的符号名称
    uint64_t addr;         // 找到的符号地址
};

/**
 * 符号查找回调函数 - 匹配带后缀的符号
 * 
 * @param index 符号索引
 * @param type 符号类型字符
 * @param symbol 当前符号名称
 * @param offset 符号偏移量
 * @param userdata 用户数据指针
 * @return 找到匹配符号返回 1，否则返回 0
 * 
 * 功能说明：
 * - 查找以指定名称开头的符号（支持编译器生成的后缀）
 * - 匹配模式：symbol_name.suffix 或 symbol_name$suffix
 * - 排除 .cfi_jt 跳转表符号
 * - 用于处理编译器优化产生的符号变体
 */
static int32_t on_each_symbol_callbackup(int32_t index, char type, const char *symbol, int32_t offset, void *userdata)
{
    struct on_each_symbol_struct *data = (struct on_each_symbol_struct *)userdata;
    int len = strlen(data->symbol);
    // 检查符号名称前缀匹配和后缀分隔符
    if (strstr(symbol, data->symbol) == symbol && (symbol[len] == '.' || symbol[len] == '$') &&
        !strstr(symbol, ".cfi_jt")) {
        tools_logi("%s -> %s: type: %c, offset: 0x%08x\n", data->symbol, symbol, type, offset);
        data->addr = offset;
        return 1;    // 找到匹配符号，停止搜索
    }
    return 0;
}

/**
 * 查找带后缀的符号（处理编译器生成的符号变体）
 * 
 * @param kallsym 内核符号表信息
 * @param img_buf 内核镜像缓冲区
 * @param symbol 要查找的基础符号名称
 * @return 符号偏移量，未找到返回 0
 * 
 * 功能说明：
 * - 用于查找编译器优化产生的符号变体
 * - 例如：func.isra.0, func.part.1, func$constprop.2 等
 * - 这些是 GCC/Clang 内联和特化优化的产物
 */
int32_t find_suffixed_symbol(kallsym_t *kallsym, char *img_buf, const char *symbol)
{
    struct on_each_symbol_struct udata = { symbol, 0 };
    on_each_symbol(kallsym, img_buf, &udata, on_each_symbol_callbackup);
    return udata.addr;
}

/**
 * 获取符号偏移量，未找到返回 0 而非负数
 * 
 * @param info 内核符号表信息
 * @param img 内核镜像
 * @param symbol 符号名称
 * @return 符号偏移量，未找到返回 0
 */
int32_t get_symbol_offset_zero(kallsym_t *info, char *img, char *symbol)
{
    int32_t offset = get_symbol_offset(info, img, symbol);
    return offset > 0 ? offset : 0;
}

/**
 * 获取符号偏移量，未找到则退出程序
 * 
 * @param info 内核符号表信息
 * @param img 内核镜像
 * @param symbol 符号名称
 * @return 符号偏移量（保证有效）
 * 
 * 功能说明：
 * - 用于查找关键符号，缺失时程序无法继续
 * - 自动处理错误情况并输出错误信息
 */
int32_t get_symbol_offset_exit(kallsym_t *info, char *img, char *symbol)
{
    int32_t offset = get_symbol_offset(info, img, symbol);
    if (offset >= 0) {
        return offset;
    } else {
        tools_loge_exit("no symbol %s\n", symbol);
    }
}

/**
 * 尝试查找符号，支持后缀变体查找
 * 
 * @param info 内核符号表信息
 * @param img 内核镜像
 * @param symbol 符号名称
 * @return 符号偏移量，未找到返回 0
 * 
 * 功能说明：
 * - 首先尝试精确匹配符号名称
 * - 如果精确匹配失败，尝试查找带后缀的变体
 * - 适用于处理编译器优化产生的符号变化
 */
int32_t try_get_symbol_offset_zero(kallsym_t *info, char *img, char *symbol)
{
    int32_t offset = get_symbol_offset(info, img, symbol);
    if (offset > 0) return offset;
    return find_suffixed_symbol(info, img, symbol);
}

/**
 * 选择内存映射区域（临时实现）
 * 
 * @param kallsym 内核符号表信息
 * @param image_buf 内核镜像缓冲区
 * @param map_start 输出映射起始地址
 * @param max_size 输出最大映射大小
 * 
 * 功能说明：
 * - 在内核镜像中选择合适的区域用于代码映射
 * - 当前使用 tcp_init_sock 函数位置作为参考点
 * - 确保映射区域对齐到 16 字节边界
 * - TODO: 需要更智能的区域选择算法
 */
void select_map_area(kallsym_t *kallsym, char *image_buf, int32_t *map_start, int32_t *max_size)
{
    int32_t addr = 0x200;
    // 使用 tcp_init_sock 作为映射区域的参考点
    addr = get_symbol_offset_exit(kallsym, image_buf, "tcp_init_sock");
    *map_start = align_ceil(addr, 16);    // 16 字节对齐
    *max_size = 0x800;                    // 2KB 映射区域
}

/**
 * 填充内存映射相关符号偏移量
 * 
 * @param kallsym 内核符号表信息
 * @param img_buf 内核镜像缓冲区
 * @param symbol 输出的映射符号结构体
 * @param target_is_be 目标架构是否为大端序
 * @return 成功返回 0
 * 
 * 功能说明：
 * - 查找内存块管理相关的关键符号
 * - 包括内存保留、释放、分配等核心函数
 * - 处理不同内核版本的符号变化
 * - 根据字节序要求进行数据转换
 */
int fillin_map_symbol(kallsym_t *kallsym, char *img_buf, map_symbol_t *symbol, int32_t target_is_be)
{
    // 内存块保留和释放函数（必需）
    symbol->memblock_reserve_relo = get_symbol_offset_exit(kallsym, img_buf, "memblock_reserve");
    symbol->memblock_free_relo = get_symbol_offset_exit(kallsym, img_buf, "memblock_free");

    // 内存块标记为不可映射（可选）
    symbol->memblock_mark_nomap_relo = get_symbol_offset_zero(kallsym, img_buf, "memblock_mark_nomap");

    // 物理和虚拟内存分配函数
    symbol->memblock_phys_alloc_relo = get_symbol_offset_zero(kallsym, img_buf, "memblock_phys_alloc_try_nid");
    symbol->memblock_virt_alloc_relo = get_symbol_offset_zero(kallsym, img_buf, "memblock_virt_alloc_try_nid");
    if (!symbol->memblock_phys_alloc_relo && !symbol->memblock_virt_alloc_relo)
        tools_loge_exit("no symbol memblock_alloc");

    // 尝试通用的内存分配函数作为后备
    uint64_t memblock_alloc_try_nid = get_symbol_offset_zero(kallsym, img_buf, "memblock_alloc_try_nid");

    // 如果专用分配函数不可用，使用通用函数
    if (!symbol->memblock_phys_alloc_relo) symbol->memblock_phys_alloc_relo = memblock_alloc_try_nid;
    if (!symbol->memblock_virt_alloc_relo) symbol->memblock_virt_alloc_relo = memblock_alloc_try_nid;
    if (!symbol->memblock_phys_alloc_relo && !symbol->memblock_virt_alloc_relo)
        tools_loge_exit("no symbol memblock_alloc");

    // 如果目标架构字节序与主机不同，转换所有偏移量
    if ((is_be() ^ target_is_be)) {
        for (int64_t *pos = (int64_t *)symbol; pos <= (int64_t *)symbol; pos++) {
            *pos = i64swp(*pos);
        }
    }
    return 0;
}

/**
 * 从候选符号数组中获取第一个可用符号的偏移量
 * 
 * @param kallsym 内核符号表信息
 * @param img_buf 内核镜像缓冲区
 * @param cand_arr 候选符号名称数组
 * @param cand_num 候选符号数量
 * @return 第一个找到的符号偏移量，未找到返回 0
 * 
 * 功能说明：
 * - 按顺序尝试候选符号列表
 * - 用于处理不同内核版本中符号名称的变化
 * - 实现符号查找的降级机制
 */
static int get_cand_arr_symbol_offset_zero(kallsym_t *kallsym, char *img_buf, char **cand_arr, int cand_num)
{
    int offset = 0;
    for (int i = 0; i < cand_num; i++) {
        offset = get_symbol_offset_zero(kallsym, img_buf, cand_arr[i]);
        if (offset) break;
    }
    return offset;
}

/**
 * 填充补丁配置所需的关键符号偏移量
 * 
 * @param kallsym 内核符号表信息
 * @param img_buf 内核镜像缓冲区
 * @param imglen 内核镜像长度
 * @param symbol 输出的补丁配置结构体
 * @param target_is_be 目标架构是否为大端序
 * @param is_android 是否为 Android 内核
 * @return 成功返回 0
 * 
 * 功能说明：
 * - 查找内核补丁系统所需的关键函数符号
 * - 包括初始化、进程管理、安全控制等核心函数
 * - 处理不同内核版本和编译器优化的符号变化
 * - Android 特定符号的额外处理
 * - 根据字节序进行数据转换
 */
int fillin_patch_config(kallsym_t *kallsym, char *img_buf, int imglen, patch_config_t *symbol, int32_t target_is_be,
                        bool is_android)
{
    // 内核 panic 函数（用于调试和错误处理）
    symbol->panic = get_symbol_offset_zero(kallsym, img_buf, "panic");

    // 内核初始化相关函数 - rest_init 优先，cgroup_init 作为后备
    symbol->rest_init = try_get_symbol_offset_zero(kallsym, img_buf, "rest_init");
    if (!symbol->rest_init) symbol->cgroup_init = try_get_symbol_offset_zero(kallsym, img_buf, "cgroup_init");
    if (!symbol->rest_init && !symbol->cgroup_init) tools_loge_exit("no symbol rest_init");

    // 用户空间初始化函数
    symbol->kernel_init = try_get_symbol_offset_zero(kallsym, img_buf, "kernel_init");

    // CFI (Control Flow Integrity) 相关函数
    symbol->report_cfi_failure = get_symbol_offset_zero(kallsym, img_buf, "report_cfi_failure");
    symbol->__cfi_slowpath_diag = get_symbol_offset_zero(kallsym, img_buf, "__cfi_slowpath_diag");
    symbol->__cfi_slowpath = get_symbol_offset_zero(kallsym, img_buf, "__cfi_slowpath");

    // 进程复制函数 - copy_process 优先，cgroup_post_fork 作为后备
    symbol->copy_process = try_get_symbol_offset_zero(kallsym, img_buf, "copy_process");
    if (!symbol->copy_process) symbol->cgroup_post_fork = get_symbol_offset_zero(kallsym, img_buf, "cgroup_post_fork");
    if (!symbol->copy_process && !symbol->cgroup_post_fork) tools_loge_exit("no symbol copy_process");

    // SELinux AVC 拒绝函数（支持 GCC -fipa-sra 优化产生的变体，如 avc_denied.isra.5）
    symbol->avc_denied = try_get_symbol_offset_zero(kallsym, img_buf, "avc_denied");
    if (!symbol->avc_denied && is_android) tools_loge_exit("no symbol avc_denied");

    // SELinux 慢速审计函数
    symbol->slow_avc_audit = try_get_symbol_offset_zero(kallsym, img_buf, "slow_avc_audit");

    // 输入事件处理函数
    symbol->input_handle_event = get_symbol_offset_zero(kallsym, img_buf, "input_handle_event");

    // 如果目标架构字节序与主机不同，转换所有偏移量
    if ((is_be() ^ target_is_be)) {
        for (int64_t *pos = (int64_t *)symbol; pos <= (int64_t *)symbol; pos++) {
            *pos = i64swp(*pos);
        }
    }
    return 0;
}
