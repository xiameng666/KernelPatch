/* 内核镜像补丁工具 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2024 bmax121. All Rights Reserved.
 */

#define _GNU_SOURCE
#define __USE_GNU

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "patch.h"
#include "kallsym.h"
#include "image.h"
#include "common.h"
#include "order.h"
#include "preset.h"
#include "symbol.h"
#include "kpm.h"
#include "sha256.h"

/**
 * @brief 读取内核文件到内存结构
 * @details 解析内核文件格式，处理UNCOMPRESSED_IMG头部
 * @param path 内核文件路径
 * @param kernel_file 输出的内核文件结构体
 */
void read_kernel_file(const char *path, kernel_file_t *kernel_file)
{
    int img_offset = 0;
    // 读取整个文件到内存
    read_file(path, &kernel_file->kfile, &kernel_file->kfile_len);
    
    // 检查是否是未压缩镜像格式，含有"UNCOMPRESSED_IMG"标识头
    kernel_file->is_uncompressed_img = kernel_file->kfile_len >= 20 &&
                                       !strncmp("UNCOMPRESSED_IMG", kernel_file->kfile, 16);
    
    // 如果是未压缩镜像，跳过20字节头部
    if (kernel_file->is_uncompressed_img) img_offset = 20;
    
    // 设置实际内核镜像数据的起始位置和长度
    kernel_file->kimg = kernel_file->kfile + img_offset;
    kernel_file->kimg_len = kernel_file->kfile_len - img_offset;
}

/**
 * @brief 更新内核文件镜像长度
 * @details 更新镜像长度并处理字节序问题
 * @param kernel_file 内核文件结构体
 * @param kimg_len 新的镜像长度
 * @param is_different_endian 是否需要字节序转换
 */
void update_kernel_file_img_len(kernel_file_t *kernel_file, int kimg_len, bool is_different_endian)
{
    // 更新镜像数据长度
    kernel_file->kimg_len = kimg_len;
    
    if (kernel_file->is_uncompressed_img) {
        // 如果是未压缩镜像，更新头部的长度字段，处理字节序
        *(uint32_t *)(kernel_file->kfile + 16) = (uint32_t)(is_different_endian ? i32swp(kimg_len) : kimg_len);
        // 文件总长度 = 镜像长度 + 20字节头部
        kernel_file->kfile_len = kimg_len + 20;
    } else {
        // 普通镜像文件，长度即为镜像长度
        kernel_file->kfile_len = kimg_len;
    }
}

/**
 * @brief 创建新的内核文件结构
 * @details 基于旧的内核文件创建新的，保留头部信息
 * @param kernel_file 输出的新内核文件结构体
 * @param old 原始内核文件结构体
 * @param kimg_len 新镜像长度
 * @param is_different_endian 是否需要字节序转换
 */
void new_kernel_file(kernel_file_t *kernel_file, kernel_file_t *old, int kimg_len, bool is_different_endian)
{
    // 计算头部长度（UNCOMPRESSED_IMG头或无头部）
    int prefix_len = old->kimg - old->kfile;
    int new_len = kimg_len + prefix_len;
    
    // 分配新的文件缓冲区
    kernel_file->kfile = (char *)malloc(new_len);
    kernel_file->kimg = kernel_file->kfile + prefix_len;
    
    // 复制原始头部数据
    memcpy(kernel_file->kfile, old->kfile, prefix_len);
    
    // 继承镜像格式标识
    kernel_file->is_uncompressed_img = old->is_uncompressed_img;
    
    // 更新长度信息
    update_kernel_file_img_len(kernel_file, kimg_len, is_different_endian);
}

/**
 * @brief 将内核文件写入磁盘
 * @details 将内存中的内核文件数据写入指定路径
 * @param kernel_file 内核文件结构体
 * @param path 目标文件路径
 */
void write_kernel_file(kernel_file_t *kernel_file, const char *path)
{
    // 调用通用文件写入函数
    write_file(path, kernel_file->kfile, kernel_file->kfile_len, false);
}

/**
 * @brief 释放内核文件结构体占用的内存
 * @details 清理动态分配的文件缓冲区并重置指针
 * @param kernel_file 待释放的内核文件结构体
 */
void free_kernel_file(kernel_file_t *kernel_file)
{
    // 释放文件数据缓冲区
    free(kernel_file->kfile);
    // 重置所有指针为NULL
    kernel_file->kfile = NULL;
    kernel_file->kimg = NULL;
}

/**
 * @brief 在内核镜像中查找预设配置结构
 * @details 通过魔数标识定位KernelPatch预设配置数据
 * @param kimg 内核镜像数据指针
 * @param kimg_len 内核镜像长度
 * @return 预设配置结构指针，未找到则返回NULL
 */
preset_t *get_preset(const char *kimg, int kimg_len)
{
    // 定义KernelPatch魔数标识
    char magic[MAGIC_LEN] = KP_MAGIC;
    // 在镜像中搜索魔数，返回预设配置结构指针
    return (preset_t *)memmem(kimg, kimg_len, magic, sizeof(magic));
}

/**
 * @brief 获取已补丁内核镜像的版本号
 * @details 从内核镜像中读取KernelPatch版本信息
 * @param kpimg_path 内核镜像文件路径
 * @return 版本号（major << 16 | minor << 8 | patch）
 */
uint32_t get_kpimg_version(const char *kpimg_path)
{
    char *kpimg = NULL;
    int kpimg_len = 0;
    
    // 读取内核镜像文件
    read_file(kpimg_path, &kpimg, &kpimg_len);
    
    // 查找预设配置结构
    preset_t *preset = get_preset(kpimg, kpimg_len);
    if (!preset) tools_loge_exit("not patched kernel image\n");
    
    // 提取版本信息并转换为32位整数
    version_t ver = preset->header.kp_version;
    uint32_t version = (ver.major << 16) + (ver.minor << 8) + ver.patch;
    return version;
}

/**
 * @brief 解析额外项类型字符串
 * @details 将字符串转换为对应的额外项类型枚举值
 * @param extra_str 额外项类型字符串
 * @return 对应的枚举值，无效时返回EXTRA_TYPE_NONE
 */
int extra_str_type(const char *extra_str)
{
    int extra_type = EXTRA_TYPE_NONE;
    
    // 根据字符串匹配对应的类型
    if (!strcmp(extra_str, EXTRA_TYPE_KPM_STR)) {
        extra_type = EXTRA_TYPE_KPM;          // KernelPatch模块
    } else if (!strcmp(extra_str, EXTRA_TYPE_EXEC_STR)) {
        extra_type = EXTRA_TYPE_EXEC;         // 可执行文件
    } else if (!strcmp(extra_str, EXTRA_TYPE_SHELL_STR)) {
        extra_type = EXTRA_TYPE_SHELL;        // Shell脚本
    } else if (!strcmp(extra_str, EXTRA_TYPE_RAW_STR)) {
        extra_type = EXTRA_TYPE_RAW;          // 原始数据
    } else if (!strcmp(extra_str, EXTRA_TYPE_ANDROID_RC_STR)) {
        extra_type = EXTRA_TYPE_ANDROID_RC;   // Android RC脚本
    } else {
        // 未知类型保持EXTRA_TYPE_NONE
    }
    return extra_type;
}

/**
 * @brief 将额外项类型枚举转换为字符串
 * @details 获取枚举值对应的字符串表示
 * @param extra_type 额外项类型枚举值
 * @return 对应的字符串，无效时返回NULL
 */
const char *extra_type_str(extra_item_type extra_type)
{
    switch (extra_type) {
    case EXTRA_TYPE_KPM:
        return EXTRA_TYPE_KPM_STR;          // "kpm"
    case EXTRA_TYPE_EXEC:
        return EXTRA_TYPE_EXEC_STR;         // "exec"
    case EXTRA_TYPE_SHELL:
        return EXTRA_TYPE_SHELL_STR;        // "shell"
    case EXTRA_TYPE_RAW:
        return EXTRA_TYPE_RAW_STR;          // "raw"
    case EXTRA_TYPE_ANDROID_RC:
        return EXTRA_TYPE_ANDROID_RC_STR;   // "android-rc"
    default:
        return EXTRA_TYPE_NONE_STR;         // "none"
    }
}

/**
 * @brief 将字节数组转换为十六进制字符串
 * @details 每个字节转换为两位十六进制字符
 * @param data 字节数组指针
 * @param len 数组长度
 * @return 动态分配的十六进制字符串，使用后需要释放
 */
static char *bytes_to_hexstr(const unsigned char *data, int len)
{
    // 分配缓冲区：每字节对应2个字符 + 结束符
    char *buf = (char *)malloc(2 * len + 1);
    buf[2 * len] = '\0';
    
    // 逐字节转换为十六进制字符
    for (int i = 0; i < len; i++) {
        sprintf(&buf[2 * i], "%02x", data[i]);
    }
    return buf;
}

/**
 * @brief 打印预设配置信息
 * @details 输出KernelPatch的版本、编译时间、配置标志等信息
 * @param preset 预设配置结构体指针
 */
void print_preset_info(preset_t *preset)
{
    setup_header_t *header = &preset->header;
    setup_preset_t *setup = &preset->setup;
    
    // 提取版本号并转换为32位表示
    version_t ver = header->kp_version;
    uint32_t ver_num = (ver.major << 16) + (ver.minor << 8) + ver.patch;
    
    // 解析配置标志
    bool is_android = header->config_flags & CONFIG_ANDROID;
    bool is_debug = header->config_flags & CONFIG_DEBUG;

    // 输出基本信息
    fprintf(stdout, INFO_KP_IMG_SESSION "\n");
    fprintf(stdout, "version=0x%x\n", ver_num);
    fprintf(stdout, "compile_time=%s\n", header->compile_time);
    fprintf(stdout, "config=%s,%s\n", is_android ? "android" : "linux", is_debug ? "debug" : "release");
    fprintf(stdout, "superkey=%s\n", setup->superkey);

    // 新版本支持root超级密钥
    if (ver_num > 0xa04) {
        char *hexstr = bytes_to_hexstr(setup->root_superkey, ROOT_SUPER_KEY_HASH_LEN);
        fprintf(stdout, "root_superkey=%s\n", hexstr);
        free(hexstr);
    }

    // 输出附加配置信息
    fprintf(stdout, INFO_ADDITIONAL_SESSION "\n");
    char *addition = setup->additional;
    
    // 兼容旧版本的偏移调整
    if (ver_num <= 0xa04) {
        addition -= (ROOT_SUPER_KEY_HASH_LEN + SETUP_PRESERVE_LEN);
    }
    
    // 解析并输出附加配置项
    char *pos = addition;
    while (pos < addition + ADDITIONAL_LEN) {
        int len = *pos;
        if (!len) break;  // 遇到长度0表示结束
        pos++;
        
        // 临时终止字符串并输出
        char backup = *(pos + len);
        *(pos + len) = 0;
        fprintf(stdout, "%s\n", pos);
        *(pos + len) = backup;
        pos += len;
    }
}

/**
 * @brief 从文件路径打印KernelPatch镜像信息
 * @details 读取文件并验证KernelPatch标识，输出配置信息
 * @param kpimg_path 内核镜像文件路径
 * @return 0表示成功，-ENOENT表示不是KernelPatch镜像
 */
int print_kp_image_info_path(const char *kpimg_path)
{
    int rc = 0;
    char *kpimg;
    int len = 0;
    
    // 读取镜像文件
    read_file(kpimg_path, &kpimg, &len);
    preset_t *preset = (preset_t *)kpimg;
    
    // 验证是否为有效的KernelPatch镜像
    if (get_preset(kpimg, len) != preset) {
        rc = -ENOENT;  // 不是KernelPatch镜像
    } else {
        // 输出预设信息
        print_preset_info(preset);
        fprintf(stdout, "\n");
        free(kpimg);
    }
    return rc;
}

/**
 * @brief 解析内核镜像补丁信息
 * @details 分析内核镜像，提取版本、Banner、预设配置和嵌入项信息
 * @param kimg 内核镜像数据指针
 * @param kimg_len 内核镜像长度
 * @param pimg 输出的补丁镜像信息结构体
 * @return 0表示成功
 */
int parse_image_patch_info(const char *kimg, int kimg_len, patched_kimg_t *pimg)
{
    pimg->kimg = kimg;
    pimg->kimg_len = kimg_len;

    // 获取内核基本信息（架构、字节序等）
    kernel_info_t *kinfo = &pimg->kinfo;
    if (get_kernel_info(kinfo, kimg, kimg_len)) tools_loge_exit("get_kernel_info error\n");

    // 查找Linux版本banner字符串
    char linux_banner_prefix[] = "Linux version ";
    size_t prefix_len = strlen(linux_banner_prefix);
    const char *imgend = pimg->kimg + pimg->kimg_len;
    const char *banner = (char *)pimg->kimg;
    
    // 在镜像中搜索banner，验证格式正确性
    while ((banner = (char *)memmem(banner + 1, imgend - banner, linux_banner_prefix, prefix_len)) != NULL) {
        // 检查版本号格式：数字.数字
        if (isdigit(*(banner + prefix_len)) && *(banner + prefix_len + 1) == '.') {
            pimg->banner = banner;
            break;
        }
    }
    if (!pimg->banner) tools_loge_exit("can't find linux banner\n");

    // 检查是否已打补丁
    preset_t *old_preset = get_preset(kimg, kimg_len);
    pimg->preset = old_preset;

    if (!old_preset) {
        // 新的未打补丁的内核镜像
        tools_logi("new kernel image ...\n");
        pimg->ori_kimg_len = pimg->kimg_len;
        return 0;
    }

    // 已打补丁的内核镜像，恢复原始信息
    tools_logi("patched kernel image ...\n");
    
    // 获取保存的原始镜像长度，处理字节序
    int32_t saved_kimg_len = old_preset->setup.kimg_size;
    if (is_be() ^ kinfo->is_be) saved_kimg_len = i32swp(saved_kimg_len);

    // 验证长度一致性（对齐到4K边界）
    int align_kimg_len = (char *)old_preset - kimg;
    if (align_kimg_len != (int)align_ceil(saved_kimg_len, SZ_4K)) tools_loge_exit("saved kernel image size error\n");
    pimg->ori_kimg_len = saved_kimg_len;

    // 恢复原始内核头部数据
    memcpy((char *)kimg, old_preset->setup.header_backup, sizeof(old_preset->setup.header_backup));

    // 解析嵌入的额外项（KPM模块、脚本等）
    int extra_offset = align_kimg_len + old_preset->setup.kpimg_size;
    if (extra_offset > kimg_len) tools_loge_exit("kpimg length mismatch\n");
    if (extra_offset == kimg_len) return 0;  // 没有额外项

    // 获取额外项总大小
    int32_t extra_size = old_preset->setup.extra_size;
    if (is_be() ^ kinfo->is_be) extra_size = i32swp(extra_size);
    const char *item_pos = kimg + extra_offset;

    // 遍历所有嵌入项
    while (item_pos < kimg + extra_offset + extra_size) {
        patch_extra_item_t *item = (patch_extra_item_t *)item_pos;
        
        // 验证魔数和类型
        if (strcmp(EXTRA_HDR_MAGIC, item->magic)) break;
        if (item->type == EXTRA_TYPE_NONE) break;
        
        // 记录有效的嵌入项
        pimg->embed_item[pimg->embed_item_num++] = item;
        
        // 移动到下一项：头部 + 参数 + 内容
        item_pos += sizeof(patch_extra_item_t);
        item_pos += item->args_size;
        item_pos += item->con_size;
    }

    return 0;
}

/**
 * @brief 从文件路径解析内核镜像补丁信息
 * @details 读取内核文件并解析补丁信息，包装函数
 * @param kimg_path 内核镜像文件路径
 * @param pimg 输出的补丁镜像信息结构体
 * @return 0表示成功
 */
int parse_image_patch_info_path(const char *kimg_path, patched_kimg_t *pimg)
{
    if (!kimg_path) tools_loge_exit("empty kernel image\n");

    // 读取内核文件
    kernel_file_t kernel_file;
    read_kernel_file(kimg_path, &kernel_file);
    
    // 解析补丁信息
    int rc = parse_image_patch_info(kernel_file.kimg, kernel_file.kimg_len, pimg);
    
    // 清理资源
    free_kernel_file(&kernel_file);
    return rc;
}

/**
 * @brief 打印内核镜像补丁信息
 * @details 输出内核banner、补丁状态、预设配置和嵌入项详细信息
 * @param pimg 补丁镜像信息结构体
 * @return 0表示成功
 */
int print_image_patch_info(patched_kimg_t *pimg)
{
    int rc = 0;

    preset_t *preset = pimg->preset;

    // 输出内核基本信息
    fprintf(stdout, INFO_KERNEL_IMG_SESSION "\n");
    fprintf(stdout, "banner=%s", pimg->banner);

    // 确保banner以换行符结尾
    if (pimg->banner[strlen(pimg->banner) - 1] != '\n') fprintf(stdout, "\n");
    fprintf(stdout, "patched=%s\n", preset ? "true" : "false");

    if (preset) {
        // 输出预设配置信息
        print_preset_info(preset);

        // 输出嵌入项信息
        fprintf(stdout, INFO_EXTRA_SESSION "\n");
        fprintf(stdout, "num=%d\n", pimg->embed_item_num);

        // 遍历每个嵌入项
        for (int i = 0; i < pimg->embed_item_num; i++) {
            patch_extra_item_t *item = pimg->embed_item[i];
            const char *type = extra_type_str(item->type);
            
            // 输出嵌入项基本信息
            fprintf(stdout, INFO_EXTRA_SESSION_N "\n", i);
            fprintf(stdout, "index=%d\n", i);
            fprintf(stdout, "type=%s\n", type);
            fprintf(stdout, "name=%s\n", item->name);
            fprintf(stdout, "event=%s\n", item->event);
            fprintf(stdout, "priority=%d\n", item->priority);
            fprintf(stdout, "args_size=0x%x\n", item->args_size);
            fprintf(stdout, "args=%s\n", item->args_size > 0 ? (char *)item + sizeof(*item) : "");
            fprintf(stdout, "con_size=0x%x\n", item->con_size);

            // 如果是KPM模块，输出模块详细信息
            if (item->type == EXTRA_TYPE_KPM) {
                kpm_info_t kpm_info = { 0 };
                void *kpm = (kpm_info_t *)((uintptr_t)item + sizeof(patch_extra_item_t) + item->args_size);
                rc = get_kpm_info(kpm, item->con_size, &kpm_info);
                if (rc) tools_loge_exit("get kpm infomation error: %d\n", rc);
                fprintf(stdout, "version=%s\n", kpm_info.version);
                fprintf(stdout, "license=%s\n", kpm_info.license);
                fprintf(stdout, "author=%s\n", kpm_info.author);
                fprintf(stdout, "description=%s\n", kpm_info.description);
            }
        }
    }
    return rc;
}

/**
 * @brief 从文件路径打印内核镜像补丁信息
 * @details 读取文件并输出完整的补丁信息，包装函数
 * @param kimg_path 内核镜像文件路径
 * @return 0表示成功
 */
int print_image_patch_info_path(const char *kimg_path)
{
    patched_kimg_t pimg = { 0 };
    kernel_file_t kernel_file;
    
    // 读取和解析内核文件
    read_kernel_file(kimg_path, &kernel_file);
    int rc = parse_image_patch_info(kernel_file.kimg, kernel_file.kimg_len, &pimg);
    
    // 输出补丁信息
    print_image_patch_info(&pimg);
    
    // 清理资源
    free_kernel_file(&kernel_file);
    return rc;
}

/**
 * @brief 额外配置项优先级比较函数
 * @details 用于qsort排序，按优先级降序排列（优先级高的在前）
 * @param a 第一个配置项指针
 * @param b 第二个配置项指针
 * @return 比较结果，负数表示a优先级更高
 */
static int extra_compare(const void *a, const void *b)
{
    extra_config_t *pa = (extra_config_t *)a;
    extra_config_t *pb = (extra_config_t *)b;
    // 返回负值表示按优先级降序排列
    return -(pa->priority - pb->priority);
}

/**
 * @brief 向内核镜像追加数据
 * @details 将数据复制到指定偏移位置并更新偏移
 * @param kimg 内核镜像缓冲区
 * @param data 待追加的数据
 * @param len 数据长度
 * @param offset 当前偏移位置指针，会被更新
 */
static void extra_append(char *kimg, const void *data, int len, int *offset)
{
    // 复制数据到当前偏移位置
    memcpy(kimg + *offset, data, len);
    // 更新偏移位置
    *offset += len;
}

/**
 * @brief 更新内核镜像补丁
 * @details 将KernelPatch应用到内核镜像，嵌入额外项并生成最终补丁镜像
 * @param kimg_path 原始内核镜像路径
 * @param kpimg_path KernelPatch镜像路径
 * @param out_path 输出补丁镜像路径
 * @param superkey 超级密钥字符串
 * @param root_key 是否使用root密钥
 * @param additional 附加配置字符串数组
 * @param extra_configs 额外配置项数组
 * @param extra_config_num 额外配置项数量
 * @return 0表示成功
 */
int patch_update_img(const char *kimg_path, const char *kpimg_path, const char *out_path, const char *superkey,
                     bool root_key, const char **additional, extra_config_t *extra_configs, int extra_config_num)
{
    set_log_enable(true);

    // 参数验证
    if (!kpimg_path) tools_loge_exit("empty kpimg\n");
    if (!out_path) tools_loge_exit("empty out image path\n");
    if (!superkey) tools_loge_exit("empty superkey\n");

    // 解析原始内核镜像
    patched_kimg_t pimg = { 0 };
    kernel_file_t kernel_file;
    read_kernel_file(kimg_path, &kernel_file);
    if (kernel_file.is_uncompressed_img) tools_logw("kernel image with UNCOMPRESSED_IMG header\n");

    int rc = parse_image_patch_info(kernel_file.kimg, kernel_file.kimg_len, &pimg);
    if (rc) tools_loge_exit("parse kernel image error\n");

    // 获取内核基本信息
    kernel_info_t *kinfo = &pimg.kinfo;
    int align_kernel_size = align_ceil(kinfo->kernel_size, SZ_4K);

    // kimg kallsym
    char *kallsym_kimg = (char *)malloc(pimg.ori_kimg_len);
    memcpy(kallsym_kimg, pimg.kimg, pimg.ori_kimg_len);
    kallsym_t kallsym = { 0 };
    if (analyze_kallsym_info(&kallsym, kallsym_kimg, pimg.ori_kimg_len, ARM64, 1)) {
        tools_loge_exit("analyze_kallsym_info error\n");
    }

    // kpimg
    char *kpimg = NULL;
    int kpimg_len = 0;
    read_file_align(kpimg_path, &kpimg, &kpimg_len, 0x10);

    // extra
    int extra_size = 0;
    int extra_num = 0;

    for (int i = 0; i < extra_config_num; i++) {
        extra_config_t *config = extra_configs + i;
        if (config->is_path && config->extra_type == EXTRA_TYPE_NONE) {
            tools_loge_exit("extra type none\n");
        }
        if (config->set_event && strnlen(config->set_event, EXTRA_EVENT_LEN) >= EXTRA_EVENT_LEN) {
            tools_loge_exit("extra event too long: %s\n", config->set_event);
        }
        if (config->set_name && strnlen(config->set_name, EXTRA_NAME_LEN) >= EXTRA_NAME_LEN) {
            tools_loge_exit("extra name too long: %s\n", config->set_event);
        }

        patch_extra_item_t *item = NULL;
        if (config->is_path) {
            // todo: free
            item = (patch_extra_item_t *)malloc(sizeof(patch_extra_item_t));
            memset(item, 0, sizeof(patch_extra_item_t));
            const char *path = config->path;
            char *data;
            int len = 0;
            read_file_align(path, &data, &len, EXTRA_ALIGN);
            config->data = data;
            item->con_size = len;
            // if name not set
            if (!config->set_name) {
                if (config->extra_type == EXTRA_TYPE_KPM) {
                    kpm_info_t kpm_info = { 0 };
                    int rc = get_kpm_info(data, len, &kpm_info);
                    if (rc) tools_loge_exit("can get infomation of kpm, path: %s\n", path);
                    strcpy(item->name, kpm_info.name);
                } else {
                    char *rsp = strrchr(path, '/');
                    strncpy(item->name, rsp ? rsp + 1 : path, EXTRA_NAME_LEN - 1);
                }
            }
        } else {
            const char *name = config->name;
            for (int j = 0; j < pimg.embed_item_num; j++) {
                item = pimg.embed_item[j];
                if (strcmp(name, item->name)) continue;
                if (is_be() ^ kinfo->is_be) {
                    item->type = i32swp(item->type);
                    item->priority = i32swp(item->priority);
                    item->con_size = i32swp(item->con_size);
                    item->args_size = i32swp(item->args_size);
                }
                if (!config->set_args && item->args_size > 0) {
                    config->set_args = (char *)item + sizeof(*item);
                }
                config->extra_type = item->type;
                config->data = (char *)item + sizeof(*item) + item->args_size;
                break;
            }
        }
        if (!item) tools_loge_exit("empty extra item\n");
        strcpy(item->magic, EXTRA_HDR_MAGIC);
        config->item = item;
        item->type = config->extra_type;
        if (config->set_args) item->args_size = align_ceil(strlen(config->set_args), EXTRA_ALIGN);
        if (config->set_name) strcpy(item->name, config->set_name);
        if (config->set_event) strcpy(item->event, config->set_event);
        if (config->priority) item->priority = config->priority;
    }

    qsort(extra_configs, extra_config_num, sizeof(extra_config_t), extra_compare);

    extra_size += sizeof(patch_extra_item_t); // ending with empty item

    for (int i = 0; i < extra_config_num; i++) {
        extra_config_t *config = extra_configs + i;
        extra_num++;
        extra_size += sizeof(patch_extra_item_t);
        extra_size += config->item->args_size;
        extra_size += config->item->con_size;
    }

    // copy to out image
    int ori_kimg_len = pimg.ori_kimg_len;
    int align_kimg_len = align_ceil(ori_kimg_len, SZ_4K);
    int out_img_len = align_kimg_len + kpimg_len;
    int out_all_len = out_img_len + extra_size;

    int start_offset = align_kernel_size;
    if (out_all_len > start_offset) {
        start_offset = align_ceil(out_all_len, SZ_4K);
        tools_logi("patch overlap, move start from 0x%x to 0x%x\n", align_kernel_size, start_offset);
    }
    tools_logi("layout kimg: 0x0,0x%x, kpimg: 0x%x,0x%x, extra: 0x%x,0x%x, end: 0x%x, start: 0x%x\n", ori_kimg_len,
               align_kimg_len, kpimg_len, out_img_len, extra_size, out_all_len, start_offset);

    kernel_file_t out_kernel_file;
    new_kernel_file(&out_kernel_file, &kernel_file, out_all_len, (bool)(is_be() ^ kinfo->is_be));
    memcpy(out_kernel_file.kimg, pimg.kimg, ori_kimg_len);
    memset(out_kernel_file.kimg + ori_kimg_len, 0, align_kimg_len - ori_kimg_len);
    memcpy(out_kernel_file.kimg + align_kimg_len, kpimg, kpimg_len);

    // set preset
    preset_t *preset = (preset_t *)(out_kernel_file.kimg + align_kimg_len);

    setup_header_t *header = &preset->header;
    version_t ver = header->kp_version;
    uint32_t ver_num = (ver.major << 16) + (ver.minor << 8) + ver.patch;
    bool is_android = header->config_flags & CONFIG_ANDROID;
    bool is_debug = header->config_flags & CONFIG_DEBUG;
    tools_logi("kpimg version: %x\n", ver_num);
    tools_logi("kpimg compile time: %s\n", header->compile_time);
    tools_logi("kpimg config: %s, %s\n", is_android ? "android" : "linux", is_debug ? "debug" : "release");

    setup_preset_t *setup = &preset->setup;
    memset(setup, 0, sizeof(preset->setup));

    setup->kernel_version.major = kallsym.version.major;
    setup->kernel_version.minor = kallsym.version.minor;
    setup->kernel_version.patch = kallsym.version.patch;
    setup->kimg_size = ori_kimg_len;
    setup->kpimg_size = kpimg_len;

    setup->kernel_size = kinfo->kernel_size;
    setup->page_shift = kinfo->page_shift;
    setup->setup_offset = align_kimg_len;
    setup->start_offset = start_offset;
    setup->extra_size = extra_size;

    int map_start, map_max_size;
    select_map_area(&kallsym, kallsym_kimg, &map_start, &map_max_size);
    setup->map_offset = map_start;
    setup->map_max_size = map_max_size;
    tools_logi("map_start: 0x%x, max_size: 0x%x\n", map_start, map_max_size);

    setup->kallsyms_lookup_name_offset = get_symbol_offset_exit(&kallsym, kallsym_kimg, "kallsyms_lookup_name");

    setup->printk_offset = get_symbol_offset_zero(&kallsym, kallsym_kimg, "printk");
    if (!setup->printk_offset) setup->printk_offset = get_symbol_offset_zero(&kallsym, kallsym_kimg, "_printk");
    if (!setup->printk_offset) tools_loge_exit("no symbol printk\n");

    if ((is_be() ^ kinfo->is_be)) {
        setup->kimg_size = i64swp(setup->kimg_size);
        setup->kernel_size = i64swp(setup->kernel_size);
        setup->page_shift = i64swp(setup->page_shift);
        setup->setup_offset = i64swp(setup->setup_offset);
        setup->start_offset = i64swp(setup->start_offset);
        setup->extra_size = i64swp(setup->extra_size);
        setup->map_offset = i64swp(setup->map_offset);
        setup->map_max_size = i64swp(setup->map_max_size);
        setup->kallsyms_lookup_name_offset = i64swp(setup->kallsyms_lookup_name_offset);
        setup->paging_init_offset = i64swp(setup->paging_init_offset);
        setup->printk_offset = i64swp(setup->printk_offset);
    }

    // map symbol
    fillin_map_symbol(&kallsym, kallsym_kimg, &setup->map_symbol, kinfo->is_be);

    // header backup
    memcpy(setup->header_backup, kallsym_kimg, sizeof(setup->header_backup));

    // start symbol
    fillin_patch_config(&kallsym, kallsym_kimg, ori_kimg_len, &setup->patch_config, kinfo->is_be, 0);

    // superkey
    if (!root_key) {
        tools_logi("superkey: %s\n", superkey);
        strncpy((char *)setup->superkey, superkey, SUPER_KEY_LEN - 1);
    } else {
        int len = SHA256_BLOCK_SIZE > ROOT_SUPER_KEY_HASH_LEN ? ROOT_SUPER_KEY_HASH_LEN : SHA256_BLOCK_SIZE;
        BYTE buf[SHA256_BLOCK_SIZE];
        SHA256_CTX ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, (const BYTE *)superkey, strnlen(superkey, SUPER_KEY_LEN));
        sha256_final(&ctx, buf);
        memcpy(setup->root_superkey, buf, len);
        char *hexstr = bytes_to_hexstr(setup->root_superkey, len);
        tools_logi("root superkey hash: %s\n", hexstr);
        free(hexstr);
    }

    // modify kernel entry
    int paging_init_offset = get_symbol_offset_exit(&kallsym, kallsym_kimg, "paging_init");
    setup->paging_init_offset = relo_branch_func(kallsym_kimg, paging_init_offset);
    int text_offset = align_kimg_len + SZ_4K;
    b((uint32_t *)(out_kernel_file.kimg + kinfo->b_stext_insn_offset), kinfo->b_stext_insn_offset, text_offset);

    // additional [len key=value] set
    char *addition_pos = setup->additional;
    for (int i = 0;; i++) {
        const char *kv = additional[i];
        if (!kv) break;
        if (!strchr(kv, '=')) tools_loge_exit("addition must be format of key=value\n");

        int kvlen = strlen(kv);
        if (kvlen > 127) tools_loge_exit("addition %s too long\n", kv);
        if (addition_pos + kvlen + 1 > setup->additional + ADDITIONAL_LEN) tools_loge_exit("no memory for addition\n");

        *addition_pos = (char)kvlen;
        addition_pos++;

        tools_logi("adding addition: %s\n", kv);
        strcpy(addition_pos, kv);
        addition_pos += kvlen;
    }

    // append extra
    int current_offset = out_img_len;
    for (int i = 0; i < extra_config_num; i++) {
        extra_config_t *config = extra_configs + i;
        patch_extra_item_t *item = config->item;
        const char *type = extra_type_str(item->type);
        tools_logi("embedding %s, name: %s, priority: %d, event: %s, args: %s, size: 0x%x+0x%x+0x%x\n", type,
                   item->name, item->priority, item->event, config->set_args ?: "", (int)sizeof(*item), item->args_size,
                   item->con_size);

        int args_len = item->args_size;
        int con_len = item->con_size;

        if (is_be() ^ kinfo->is_be) {
            item->type = i32swp(item->type);
            item->priority = i32swp(item->priority);
            item->con_size = i32swp(item->con_size);
            item->args_size = i32swp(item->args_size);
        }

        extra_append(out_kernel_file.kimg, (void *)item, sizeof(*item), &current_offset);
        if (args_len > 0) extra_append(out_kernel_file.kimg, (void *)config->set_args, args_len, &current_offset);
        extra_append(out_kernel_file.kimg, (void *)config->data, con_len, &current_offset);
    }

    // guard extra
    patch_extra_item_t empty_item = { 0 };
    extra_append(out_kernel_file.kimg, (void *)&empty_item, sizeof(empty_item), &current_offset);

    write_kernel_file(&out_kernel_file, out_path);

    // free
    free(kallsym_kimg);
    free(kpimg);
    free_kernel_file(&out_kernel_file);
    free_kernel_file(&kernel_file);

    tools_logi("patch done: %s\n", out_path);

    set_log_enable(false);
    return 0;
}

/**
 * @brief 撤销内核镜像补丁
 * @details 从已补丁的内核镜像恢复原始内核，移除KernelPatch
 * @param kimg_path 已补丁的内核镜像路径
 * @param out_path 输出原始内核镜像路径
 * @return 0表示成功
 */
int unpatch_img(const char *kimg_path, const char *out_path)
{
    if (!kimg_path) tools_loge_exit("empty kernel image\n");
    if (!out_path) tools_loge_exit("empty out image path\n");

    // 读取已补丁的内核文件
    kernel_file_t kernel_file;
    read_kernel_file(kimg_path, &kernel_file);

    // 查找预设配置，验证是否已补丁
    preset_t *preset = get_preset(kernel_file.kimg, kernel_file.kimg_len);
    if (!preset) tools_loge_exit("not patched kernel image\n");

    // 恢复原始内核头部数据
    memcpy(kernel_file.kimg, preset->setup.header_backup, sizeof(preset->setup.header_backup));
    
    // 恢复原始内核大小
    int kimg_size = preset->setup.kimg_size ?: ((char *)preset - kernel_file.kimg);
    update_kernel_file_img_len(&kernel_file, kimg_size, false);

    // 写入恢复的原始内核
    write_kernel_file(&kernel_file, out_path);
    free_kernel_file(&kernel_file);
    return 0;
}

/**
 * @brief 重置超级密钥
 * @details 修改已补丁内核镜像中的超级密钥
 * @param kimg_path 已补丁的内核镜像路径
 * @param out_path 输出修改后的内核镜像路径
 * @param superkey 新的超级密钥字符串
 * @return 0表示成功
 */
int reset_key(const char *kimg_path, const char *out_path, const char *superkey)
{
    // 参数验证
    if (!kimg_path) tools_loge_exit("empty kernel image\n");
    if (!out_path) tools_loge_exit("empty out image path\n");
    if (!superkey) tools_loge_exit("empty superkey\n");

    if (strlen(superkey) <= 0) tools_loge_exit("empty superkey\n");
    if (strlen(superkey) >= SUPER_KEY_LEN) tools_loge_exit("too long superkey\n");

    // 读取已补丁的内核文件
    kernel_file_t kernel_file;
    read_kernel_file(kimg_path, &kernel_file);

    // 查找预设配置
    preset_t *preset = get_preset(kernel_file.kimg, kernel_file.kimg_len);
    if (!preset) tools_loge_exit("not patched kernel image\n");

    // 备份并替换超级密钥
    char *origin_key = strdup((char *)preset->setup.superkey);
    strcpy((char *)preset->setup.superkey, superkey);
    tools_logi("reset superkey: %s -> %s\n", origin_key, preset->setup.superkey);

    // 写入修改后的内核
    write_kernel_file(&kernel_file, out_path);

    // 清理资源
    free(origin_key);
    free_kernel_file(&kernel_file);

    return 0;
}

/**
 * @brief 导出内核符号表
 * @details 解析内核镜像中的kallsyms符号表并输出所有符号
 * @param kimg_path 内核镜像文件路径
 * @return 0表示成功，-1表示失败
 */
int dump_kallsym(const char *kimg_path)
{
    if (!kimg_path) tools_loge_exit("empty kernel image\n");
    set_log_enable(true);
    
    // 读取内核镜像文件
    kernel_file_t kernel_file;
    read_kernel_file(kimg_path, &kernel_file);

    // 分析kallsyms符号表信息
    kallsym_t kallsym;
    if (analyze_kallsym_info(&kallsym, kernel_file.kimg, kernel_file.kimg_len, ARM64, 1)) {
        fprintf(stdout, "analyze_kallsym_info error\n");
        return -1;
    }
    
    // 导出所有符号到标准输出
    dump_all_symbols(&kallsym, kernel_file.kimg);
    set_log_enable(false);
    
    // 清理资源
    free_kernel_file(&kernel_file);
    return 0;
}
int dump_ikconfig(const char *kimg_path)
{
    if (!kimg_path) tools_loge_exit("empty kernel image\n");
    set_log_enable(true);
    // read image files
    kernel_file_t kernel_file;
    read_kernel_file(kimg_path, &kernel_file);

    
    dump_all_ikconfig(kernel_file.kimg,kernel_file.kimg_len);
    set_log_enable(false);
    free_kernel_file(&kernel_file);
    return 0;
}