/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2024 1f2003d5. All Rights Reserved.
 * Copyright (C) 2024 sekaiacg. All Rights Reserved.
 */

#include "sepolicy_flags.h"

#include <ksyms.h>
#include <uapi/scdefs.h>
#include <linux/spinlock.h>
#include <linux/capability.h>
#include <linux/security.h>
#include <asm/current.h>
#include <asm/thread_info.h>
#include <uapi/asm-generic/errno.h>
#include <hook.h>
#include <linux/string.h>
#include <predata.h>

/*
 * @see: https://android-review.googlesource.com/c/kernel/common/+/3009995
 * Android SELinux策略标志修复
 * 用于修复Android系统中SELinux策略配置标志的兼容性问题
 */

/**
 * @brief policydb_write函数的前置hook回调
 * @details 在策略数据库写入前保存文件指针的数据地址
 *          用于在后置回调中访问写入的数据
 * 
 * @param args hook参数结构体，包含policydb_write的参数
 * @param udata 用户自定义数据指针
 */
static void before_policydb_write(hook_fargs2_t *args, void *udata)
{
    // 获取策略文件结构体指针（第二个参数）
    struct _policy_file *fp = (struct _policy_file *)args->arg1;
    
    // 在本地存储中保存数据指针，供后置回调使用
    // fp->data指向策略数据缓冲区
    args->local.data0 = (uint64_t)fp->data;
}

/**
 * @brief policydb_write函数的后置hook回调  
 * @details 在策略数据库写入后修复配置标志
 *          确保Android网络相关的SELinux权限被正确设置
 * 
 * @param args hook参数结构体，包含函数参数和返回值
 * @param udata 用户自定义数据指针
 */
static void after_policydb_write(hook_fargs2_t *args, void *udata)
{
    // 获取策略数据库结构体指针（第一个参数）
    struct _policydb *p = (struct _policydb *)args->arg0;
    
    // 从本地存储获取数据缓冲区指针
    char *data = (char *)args->local.data0;

    // 只有在写入成功时才进行修复（返回值为0表示成功）
    if (!args->ret) {
        // 定位到配置字段在数据中的偏移位置
        __le32 *config = (__le32 *)(data + POLICYDB_CONFIG_OFFSET);
        __le32 before_config = *config;
        
        // 检查现有配置中是否已经设置了相应标志
        bool android_netlink_route_exists = before_config & POLICYDB_CONFIG_ANDROID_NETLINK_ROUTE;
        bool android_netlink_getneigh_exists = before_config & POLICYDB_CONFIG_ANDROID_NETLINK_GETNEIGH;
        
        // 如果策略数据库中启用了android_netlink_route，但配置中没有设置标志，则添加
        if (p->android_netlink_route == 1 && !android_netlink_route_exists) {
            *config |= POLICYDB_CONFIG_ANDROID_NETLINK_ROUTE;
        }
        
        // 如果策略数据库中启用了android_netlink_getneigh，但配置中没有设置标志，则添加
        if (p->android_netlink_getneigh == 1 && !android_netlink_getneigh_exists) {
            *config |= POLICYDB_CONFIG_ANDROID_NETLINK_GETNEIGH;
        }
    }
}

/**
 * @brief Android SELinux策略标志修复功能初始化
 * @details 通过hook policydb_write函数来修复Android SELinux策略中的配置标志
 *          解决某些Android版本中网络权限配置不一致的问题
 * 
 * @return 返回0表示成功，-1表示失败
 */
int android_sepolicy_flags_fix()
{
    // 查找policydb_write函数的地址
    // 这个函数负责将策略数据库写入到文件或缓冲区
    unsigned long policydb_write_addr = kallsyms_lookup_name("policydb_write");

    if (likely(policydb_write_addr)) {
        // 安装hook，拦截policydb_write函数调用
        // 使用hook_wrap2安装前置和后置回调
        hook_err_t err = hook_wrap2((void *)policydb_write_addr, before_policydb_write, after_policydb_write, 0);

        // 检查hook安装是否成功
        if (unlikely(err != HOOK_NO_ERR)) {
            log_boot("hook policydb_write_addr: %llx, error: %d\n", policydb_write_addr, err);
            return -1;
        }
    }

    return 0;
}
