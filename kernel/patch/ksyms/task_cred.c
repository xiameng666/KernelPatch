/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <log.h>
#include <stdbool.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/vmalloc.h>
#include <baselib.h>
#include <linux/pid.h>
#include <asm/current.h>
#include <linux/security.h>
#include <syscall.h>
#include <uapi/linux/prctl.h>
#include <uapi/linux/magic.h>
#include <linux/capability.h>
#include <linux/seccomp.h>
#include <linux/sched/mm.h>
#include <ksyms.h>
#include <pgtable.h>
#include <symbol.h>
#include <linux/mm_types.h>
#include <asm/processor.h>

#define TASK_COMM_LEN 16          // 任务命令名长度
                                  
#define TASK_STRUCT_MAX_SIZE 0x1800  // task_struct最大预期大小
#define THREAD_INFO_MAX_SIZE 0x90    // thread_info最大预期大小
#define CRED_MAX_SIZE 0x100          // cred结构最大预期大小
#define MM_STRUCT_MAX_SIZE 0xb0      // mm_struct最大预期大小

// mm_struct结构偏移量定义 - 用于动态访问内存管理结构字段
struct mm_struct_offset mm_struct_offset = {
    .mmap_base_offset = -1,    // 内存映射基地址偏移量
    .task_size_offset = -1,    // 任务空间大小偏移量
    .pgd_offset = -1,          // 页全局目录偏移量
    .map_count_offset = -1,    // 映射计数偏移量
    .total_vm_offset = -1,     // 总虚拟内存偏移量
    .locked_vm_offset = -1,    // 锁定虚拟内存偏移量
    .pinned_vm_offset = -1,    // 固定虚拟内存偏移量
    .data_vm_offset = -1,      // 数据虚拟内存偏移量
    .exec_vm_offset = -1,      // 可执行虚拟内存偏移量
    .stack_vm_offset = -1,     // 栈虚拟内存偏移量
    .start_code_offset = -1,   // 代码段起始偏移量
    .end_code_offset = -1,     // 代码段结束偏移量
    .start_data_offset = -1,   // 数据段起始偏移量
    .end_data_offset = -1,     // 数据段结束偏移量
    .start_brk_offset = -1,    // 堆起始偏移量
    .brk_offset = -1,          // 当前堆偏移量
    .start_stack_offset = -1,  // 栈起始偏移量
    .arg_start_offset = -1,    // 参数起始偏移量
    .arg_end_offset = -1,      // 参数结束偏移量
    .env_start_offset = -1,    // 环境变量起始偏移量
    .env_end_offset = -1,      // 环境变量结束偏移量
};
KP_EXPORT_SYMBOL(mm_struct_offset);

// task_struct结构偏移量定义 - 用于动态访问任务结构字段
struct task_struct_offset task_struct_offset = {
    .pid_offset = -1,          // 进程ID偏移量
    .tgid_offset = -1,         // 线程组ID偏移量
    .thread_pid_offset = -1,   // 线程PID偏移量
    .ptracer_cred_offset = -1, // 跟踪器凭据偏移量
    .real_cred_offset = -1,    // 真实凭据偏移量
    .cred_offset = -1,         // 有效凭据偏移量
    .fs_offset = -1,           // 文件系统信息偏移量
    .files_offset = -1,        // 文件描述符表偏移量
    .loginuid_offset = -1,     // 登录UID偏移量
    .sessionid_offset = -1,    // 会话ID偏移量
    .comm_offset = -1,         // 命令名偏移量
    .seccomp_offset = -1,      // seccomp过滤器偏移量
    .security_offset = -1,     // 安全信息偏移量
    .stack_offset = -1,        // 栈偏移量
    .tasks_offset = -1,        // 任务链表偏移量
    .mm_offset = -1,           // 内存管理结构偏移量
    .active_mm_offset = -1,    // 活动内存管理结构偏移量
};
KP_EXPORT_SYMBOL(task_struct_offset);

// cred结构偏移量定义 - 用于动态访问凭据结构字段
struct cred_offset cred_offset = {
    .usage_offset = -1,             // 引用计数偏移量
    .subscribers_offset = -1,       // 订阅者计数偏移量
    .magic_offset = -1,             // 魔数偏移量（用于验证结构完整性）
    .uid_offset = -1,               // 真实用户ID偏移量
    .gid_offset = -1,               // 真实组ID偏移量
    .suid_offset = -1,              // 保存的用户ID偏移量
    .sgid_offset = -1,              // 保存的组ID偏移量
    .euid_offset = -1,              // 有效用户ID偏移量
    .egid_offset = -1,              // 有效组ID偏移量
    .fsuid_offset = -1,             // 文件系统用户ID偏移量
    .fsgid_offset = -1,             // 文件系统组ID偏移量
    .securebits_offset = -1,        // 安全位标志偏移量
    .cap_inheritable_offset = -1,   // 可继承能力集偏移量
    .cap_permitted_offset = -1,     // 允许能力集偏移量
    .cap_effective_offset = -1,
    .cap_bset_offset = -1,
    .cap_ambient_offset = -1,

    .user_offset = -1,
    .user_ns_offset = -1,
    .ucounts_offset = -1,
    .group_info_offset = -1,

    .session_keyring_offset = -1,
    .process_keyring_offset = -1,
    .thread_keyring_offset = -1,
    .request_key_auth_offset = -1,

    .security_offset = -1,

    .rcu_offset = -1,
};
KP_EXPORT_SYMBOL(cred_offset);

struct task_struct *init_task = 0;
const struct cred *init_cred = 0;
const struct mm_struct *init_mm = 0;

int thread_size = 0;
KP_EXPORT_SYMBOL(thread_size);

int thread_info_in_task = 0;
KP_EXPORT_SYMBOL(thread_info_in_task);

int sp_el0_is_current = 0;
KP_EXPORT_SYMBOL(sp_el0_is_current);

int sp_el0_is_thread_info = 0;
KP_EXPORT_SYMBOL(sp_el0_is_thread_info);

int task_in_thread_info_offset = -1;
KP_EXPORT_SYMBOL(task_in_thread_info_offset);

int stack_in_task_offset = -1;
KP_EXPORT_SYMBOL(stack_in_task_offset);

int stack_end_offset = 0x90;
KP_EXPORT_SYMBOL(stack_end_offset);

static int16_t *bl_list = 0;
static int bl_cap = 0;                 // 黑名单容量

/**
 * @brief 重新初始化黑名单数组
 * @param num 黑名单数组的容量大小
 */
static void reinit_bllist(int num)
{
    bl_cap = num;  // 设置容量
    bl_list = (int16_t *)vmalloc(bl_cap * sizeof(int16_t));  // 分配内存
    // 初始化所有元素为-1（表示未使用）
    for (int i = 0; i < bl_cap; i++) {
        bl_list[i] = -1;
    }
}

/**
 * @brief 清理黑名单数组
 */
static void uninit_bllist()
{
    bl_cap = 0;        // 重置容量
    vfree(bl_list);    // 释放内存
}

/**
 * @brief 检查指定偏移量是否在黑名单中
 * @param off 要检查的偏移量
 * @return 1表示在黑名单中，0表示不在
 */
static int is_bl(int16_t off)
{
    // 遍历黑名单查找匹配的偏移量
    for (int i = 0; i < bl_cap; i++) {
        if (bl_list[i] < 0) break;      // 遇到未使用的槽位则停止
        if (bl_list[i] == off) return 1; // 找到匹配项
    }
    return 0;  // 未找到
}

/**
 * @brief 将偏移量添加到黑名单中
 * @param off 要添加的偏移量
 * @param size 数据大小（如果是8字节，会同时添加off+4）
 */
static void add_bll(int16_t off, int16_t size)
{
    // 查找第一个空闲槽位并添加偏移量
    for (int i = 0; i < bl_cap; i++) {
        if (bl_list[i] < 0) {
            bl_list[i] = off;
            if (size == 8) bl_list[i + 1] = off + 4;  // 8字节数据需要标记两个4字节位置
            break;
        }
    }
}

/**
 * @brief 解析cred结构体的字段偏移量
 * @return 成功返回0，失败返回错误码
 * @details 通过动态修改cred结构中的字段值并观察系统调用返回值的变化来确定字段偏移量
 */
int resolve_cred_offset()
{
    log_boot("struct cred: \n");

    reinit_bllist(128);  // 初始化偏移量黑名单，容量128

    // 分配临时内存并复制初始凭据结构
    struct cred *cred = (struct cred *)vmalloc(CRED_MAX_SIZE);
    struct cred *cred1 = (struct cred *)vmalloc(CRED_MAX_SIZE);
    struct task_struct *task = vmalloc(TASK_STRUCT_MAX_SIZE);
    lib_memcpy(cred, init_cred, CRED_MAX_SIZE);     // 复制初始凭据作为测试基准
    lib_memcpy(cred1, init_cred, CRED_MAX_SIZE);    // 复制副本用于比较
    lib_memcpy(task, init_task, TASK_STRUCT_MAX_SIZE);  // 复制任务结构

    // 设置任务的凭据指针指向我们的测试凭据
    *(struct cred **)((uintptr_t)task + task_struct_offset.cred_offset) = cred;
    *(struct cred **)((uintptr_t)task + task_struct_offset.real_cred_offset) = cred;

    const struct task_struct *backup = override_current(task);  // 临时切换当前任务

    // 解析能力位相关偏移量：cap_inheritable, cap_permitted, cap_effective
    kernel_cap_t effective, inheritable, permitted;
    cap_capget(task, &effective, &inheritable, &permitted);  // 获取当前能力位
    full_cap.val = effective.val;  // 保存完整能力位值
    log_boot("    full_cap capability: %x\n", full_cap.val);

    // 设置测试用的新能力位值
    kernel_cap_t new_cap_e = { 0xff }, new_cap_i = { 0xf }, new_cap_p = { 0xfff };
    cap_capset(cred1, cred, &new_cap_e, &new_cap_i, &new_cap_p);  // 应用新能力位到cred1

    // 遍历cred结构查找能力位字段的偏移量
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;  // 跳过已在黑名单中的偏移量
        kernel_cap_t cap = *(kernel_cap_t *)((uintptr_t)cred + i);   // 原始值
        kernel_cap_t cap1 = *(kernel_cap_t *)((uintptr_t)cred1 + i); // 修改后的值
        
        // 通过对比原始值和修改值来确定字段偏移量
        if (cap.val == effective.val && cap1.val == new_cap_e.val) {
            cred_offset.cap_effective_offset = i;      // 有效能力位偏移量
            add_bll(i, sizeof(kernel_cap_t));         // 添加到黑名单
            continue;
        }
        if (cap.val == inheritable.val && cap1.val == new_cap_i.val) {
            cred_offset.cap_inheritable_offset = i;    // 可继承能力位偏移量
            add_bll(i, sizeof(kernel_cap_t));         // 添加到黑名单
            continue;
        }
        if (cap.val == permitted.val && cap1.val == new_cap_p.val) {
            cred_offset.cap_permitted_offset = i;      // 允许能力位偏移量
            add_bll(i, sizeof(kernel_cap_t));         // 添加到黑名单
            continue;
        }
    }

    // 解析cap_bset（边界能力集）偏移量
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;  // 跳过已知偏移量
        kernel_cap_t cap1 = *(kernel_cap_t *)((uintptr_t)cred1 + i);
        if (cap1.val == effective.val) {  // cap_bset通常与effective相同
            cred_offset.cap_bset_offset = i;
            add_bll(i, sizeof(kernel_cap_t));
        }
    }
    log_boot("    cap_effective offset: %x\n", cred_offset.cap_effective_offset);
    log_boot("    cap_inheritable offset: %x\n", cred_offset.cap_inheritable_offset);
    log_boot("    cap_permitted offset: %x\n", cred_offset.cap_permitted_offset);
    log_boot("    cap_bset offset: %x\n", cred_offset.cap_bset_offset);

    // 解析securebits偏移量 - 通过修改值并调用prctl检测
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;  // 跳过已知偏移量
        unsigned *sbitsp = (unsigned *)((uintptr_t)cred + i);
        unsigned oribits = *sbitsp;  // 备份原始值
        *sbitsp = 1158;              // 设置测试值
        unsigned sbits = cap_task_prctl(PR_GET_SECUREBITS, 0, 0, 0, 0);  // 获取securebits
        if (sbits != 1158) {         // 如果获取的值不是测试值，说明不是securebits字段
            *sbitsp = oribits;       // 恢复原始值
            continue;
        }
        *sbitsp = oribits;           // 恢复原始值
        cred_offset.securebits_offset = i;  // 记录偏移量
        add_bll(i, sizeof(unsigned));      // 添加到黑名单
        break;
    }
    log_boot("    securebits offset: %x\n", cred_offset.securebits_offset);

    // 解析UID/GID相关偏移量 - 通过修改值并调用相应系统调用检测
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;  // 跳过已知偏移量
        uid_t *uidp = (uid_t *)((uintptr_t)cred + i);
        if (*uidp) continue;     // 跳过非零值字段
        *uidp = 1158;           // 设置测试值
        
        // 通过系统调用检测字段类型
        if (raw_syscall0(__NR_geteuid) == 1158) {
            cred_offset.euid_offset = i;    // 有效用户ID
        } else if (raw_syscall0(__NR_getuid) == 1158) {
            cred_offset.uid_offset = i;     // 真实用户ID
        } else if (raw_syscall0(__NR_getegid) == 1158) {
            cred_offset.egid_offset = i;    // 有效组ID
        } else if (raw_syscall0(__NR_getgid) == 1158) {
            cred_offset.gid_offset = i;     // 真实组ID
        } else {
            *uidp = 0;  // 恢复原始值
            continue;
        }
        *uidp = 0;      // 恢复原始值
        add_bll(i, sizeof(uid_t));  // 添加到黑名单
    }
    log_boot("    uid offset: %x\n", cred_offset.uid_offset);
    log_boot("    euid offset: %x\n", cred_offset.euid_offset);
    log_boot("    gid offset: %x\n", cred_offset.gid_offset);
    log_boot("    egid offset: %x\n", cred_offset.egid_offset);

    // 解析fsuid偏移量 - 文件系统用户ID
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;  // 跳过已知偏移量
        uid_t *uidp = (uid_t *)((uintptr_t)cred + i);
        uid_t backup = *uidp;    // 备份原始值
        *uidp = 1158;           // 设置测试值
        uid_t old_uid = raw_syscall1(__NR_setfsuid, -1);  // 调用setfsuid获取旧值
        *uidp = backup;         // 恢复原始值
        if (old_uid == 1158) {  // 如果返回的旧值是测试值，说明找到了fsuid字段
            cred_offset.fsuid_offset = i;
            add_bll(i, sizeof(uid_t));
            break;
        }
    }
    log_boot("    fsuid offset: %x\n", cred_offset.fsuid_offset);

    // 解析fsgid偏移量 - 文件系统组ID
    struct cred *new_cred = *(struct cred **)((uintptr_t)task + task_struct_offset.cred_offset);
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;  // 跳过已知偏移量
        gid_t *gidp = (gid_t *)((uintptr_t)new_cred + i);
        gid_t backup = *gidp;    // 备份原始值
        *gidp = 1158;           // 设置测试值
        gid_t old_gid = raw_syscall1(__NR_setfsgid, -1);  // 调用setfsgid获取旧值
        *gidp = backup;         // 恢复原始值
        if (old_gid == 1158) {  // 如果返回的旧值是测试值，说明找到了fsgid字段
            cred_offset.fsgid_offset = i;
            add_bll(i, sizeof(gid_t));
            break;
        }
    }
    log_boot("    fsgid offset: %x\n", cred_offset.fsgid_offset);

    // suid
    raw_syscall3(__NR_setresuid, 0, 0, 1158);
    new_cred = *(struct cred **)((uintptr_t)task + task_struct_offset.cred_offset);
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;
        uid_t *uidp = (uid_t *)((uintptr_t)new_cred + i);
        if (*uidp == 1158) {
            cred_offset.suid_offset = i;
            *uidp = 0;
            add_bll(i, sizeof(uid_t));
            break;
        }
    }
    log_boot("    suid offset: %x\n", cred_offset.suid_offset);

    // sgid
    raw_syscall3(__NR_setresgid, 0, 0, 1158);
    new_cred = *(struct cred **)((uintptr_t)task + task_struct_offset.cred_offset);
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;
        gid_t *uidp = (gid_t *)((uintptr_t)new_cred + i);
        if (*uidp == 1158) {
            cred_offset.sgid_offset = i;
            *uidp = 0;
            add_bll(i, sizeof(gid_t));
            break;
        }
    }
    log_boot("    sgid offset: %x\n", cred_offset.sgid_offset);

    // cap_ambient
    new_cred = *(struct cred **)((uintptr_t)task + task_struct_offset.cred_offset);
    *(kernel_cap_t *)((uintptr_t)new_cred + cred_offset.cap_effective_offset) = full_cap;
    *(kernel_cap_t *)((uintptr_t)new_cred + cred_offset.cap_inheritable_offset) = full_cap;
    *(kernel_cap_t *)((uintptr_t)new_cred + cred_offset.cap_permitted_offset) = full_cap;
    *(unsigned *)((uintptr_t)new_cred + cred_offset.securebits_offset) = 0;
    cap_task_prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, 0xf, 0, 0);  // 提升环境能力
    new_cred = *(struct cred **)((uintptr_t)task + task_struct_offset.cred_offset);
    for (int i = 0; i < CRED_MAX_SIZE; i += sizeof(uint32_t)) {
        if (is_bl(i)) continue;  // 跳过已知偏移量
        kernel_cap_t cap = *(kernel_cap_t *)((uintptr_t)cred + i);       // 原始值
        kernel_cap_t new_cap = *(kernel_cap_t *)((uintptr_t)new_cred + i); // 修改后的值
        if (!cap.val && new_cap.val == (1 << 0xf)) {  // 检查是否从空变为设置的能力位
            cred_offset.cap_ambient_offset = i;
            add_bll(i, sizeof(kernel_cap_t));
        }
    }
    log_boot("    cap_ambient offset: %x\n", cred_offset.cap_ambient_offset);

    revert_current(backup);  // 恢复当前任务上下文

    // 释放分配的内存
    vfree(cred);
    vfree(cred1);
    vfree(task);

    uninit_bllist();  // 清理黑名单
    return 0;  // 成功返回
}

/**
 * 在内存区域中查找swapper进程名偏移量
 * swapper是内核的idle进程，其comm字段包含特定的进程名
 * @param start 搜索起始地址
 * @param size 搜索区域大小
 * @return 找到返回偏移量，否则返回-1
 */
static int find_swapper_comm_offset(uint64_t start, int size)
{
    if (!is_kimg_range(start) || !is_kimg_range(start + size)) return -1;  // 检查地址范围有效性
    char swapper_comm[TASK_COMM_LEN] = "swapper";    // 标准swapper进程名
    char swapper_comm_1[TASK_COMM_LEN] = "swapper/0"; // CPU0上的swapper进程名
    for (uint64_t i = start; i < start + size; i += sizeof(uint32_t)) {
        // 查找匹配的进程名字符串
        if (!lib_strcmp(swapper_comm, (char *)i) || !lib_strcmp(swapper_comm_1, (char *)i)) {
            return i - start;  // 返回相对偏移量
        }
    }
    return -1;  // 未找到
}

/**
 * 解析task_struct结构体字段偏移量
 * 通过复制init_task并动态测试不同位置，确定各个关键字段的准确位置
 * 这种方法避免了硬编码偏移量，提供了跨内核版本的兼容性
 * @return 成功返回0，失败返回负值
 */
int resolve_task_offset()
{
    log_boot("struct task_struct: \n");

    // 分配内存并复制init_task结构体用于测试
    struct task_struct *task = (struct task_struct *)vmalloc(TASK_STRUCT_MAX_SIZE);
    lib_memcpy(task, init_task, TASK_STRUCT_MAX_SIZE);

    const struct task_struct *backup = override_current(task);  // 临时替换当前任务

    // 解析cred和real_cred偏移量 - 通过查找init_cred指针
    int cred_offset[2];          // 存储找到的两个cred偏移量
    int cred_offset_idx = 0;     // 偏移量索引
    init_cred = get_task_cred(init_task); // 获取初始凭据 // todo: get_task_cred not export
    log_boot("    init_cred addr: %llx\n", init_cred);
    
    // 在init_task中查找指向init_cred的指针
    for (uintptr_t i = (uintptr_t)init_task; i < (uintptr_t)init_task + TASK_STRUCT_MAX_SIZE; i += sizeof(uint32_t)) {
        uintptr_t val = *(uintptr_t *)i;
        if (val == (uintptr_t)init_cred) {  // 找到指向init_cred的指针
            cred_offset[cred_offset_idx++] = i - (uintptr_t)init_task;
            if (cred_offset_idx >= 2) break;  // 找到两个偏移量就足够了
        }
    }

    // 通过修改测试确定哪个是cred，哪个是real_cred
    char flag_cred[CRED_MAX_SIZE];
    lib_memcpy(flag_cred, init_cred, sizeof(flag_cred));  // 复制init_cred作为测试标记
    *(uintptr_t *)((uintptr_t)init_task + cred_offset[0]) = (uintptr_t)flag_cred;  // 修改第一个偏移量
    
    if ((uintptr_t)init_cred == (uintptr_t)flag_cred) {  // 判断修改是否影响到了当前cred
        task_struct_offset.real_cred_offset = cred_offset[0];  // 第一个是real_cred
        task_struct_offset.cred_offset = cred_offset[1];       // 第二个是cred
    } else {
        task_struct_offset.real_cred_offset = cred_offset[1];  // 第一个是cred
        task_struct_offset.cred_offset = cred_offset[0];       // 第二个是real_cred
    }
    *(uintptr_t *)((uintptr_t)init_task + cred_offset[0]) = (uintptr_t)init_cred;  // 恢复原始值

    log_boot("    cred offset: %x\n", task_struct_offset.cred_offset);
    log_boot("    real_cred offset: %x\n", task_struct_offset.real_cred_offset);

    // 解析seccomp偏移量 - 安全计算模式字段
    if (kfunc(prctl_get_seccomp)) {  // 确保prctl_get_seccomp函数可用
        for (uintptr_t i = (uintptr_t)task; i < (uintptr_t)task + TASK_STRUCT_MAX_SIZE; i += sizeof(uint32_t)) {
            int *modep = (int *)i;
            int mode_back = *modep;  // 备份原始值
            if (mode_back) continue; // 跳过非零值字段
            *modep = 1158;          // 设置测试值
            int mode = prctl_get_seccomp();  // 获取seccomp模式
            if (mode == 1158) {     // 如果返回测试值，说明找到了seccomp字段
                task_struct_offset.seccomp_offset = i - (uintptr_t)task;
            }
            *modep = mode_back;     // 恢复原始值
        }
    }
    log_boot("    seccomp offset: %x\n", task_struct_offset.seccomp_offset);

    // active_mm
    // 解析active_mm偏移量 - 活动内存管理结构指针
    init_mm = (struct mm_struct *)kallsyms_lookup_name("init_mm");  // 获取初始内存管理结构
    if (init_mm) {  // 如果成功获取到init_mm
        // 在task结构中查找指向init_mm的指针
        for (uintptr_t i = (uintptr_t)task; i < (uintptr_t)task + TASK_STRUCT_MAX_SIZE; i += sizeof(uint32_t)) {
            uintptr_t active_mm = *(uintptr_t *)i;
            if (active_mm == (uintptr_t)init_mm) {  // 找到指向init_mm的字段
                task_struct_offset.active_mm_offset = i - (uintptr_t)task;  // 记录偏移量
                break;
            }
        }
    } else {
        // todo: 处理无法获取init_mm的情况
    }
    log_boot("    active_mm offset: %x\n", task_struct_offset.active_mm_offset);

    revert_current(backup);  // 恢复当前任务上下文
    vfree(task);            // 释放分配的内存
    return 0;               // 成功返回
}

/**
 * 解析当前任务指针的获取方式和线程相关配置
 * 确定sp_el0寄存器的用途以及线程大小等关键参数
 * @return 成功返回0，失败返回负值
 */
int resolve_current()
{
    log_boot("current: \n");
    uint64_t sp_el0, sp;
    asm volatile("mrs %0, sp_el0" : "=r"(sp_el0));  // 读取sp_el0寄存器值
    asm volatile("mov %0, sp" : "=r"(sp));          // 读取当前栈指针

    log_boot("    sp_el0: %llx\n", sp_el0);
    log_boot("    sp: %llx\n", sp);

    // 初始化默认值
    sp_el0_is_current = 0;        // sp_el0是否直接指向当前任务
    sp_el0_is_thread_info = 0;    // sp_el0是否指向线程信息

    // 通过符号表查找关键数据结构地址
    init_task = (struct task_struct *)kallsyms_lookup_name("init_task");
    uint64_t init_thread_union_addr = kallsyms_lookup_name("init_thread_union");

#if 0
    init_task = 0;                // 调试时可用于强制重新查找
    init_thread_union_addr = 0;   // 调试时可用于强制重新查找
#endif

    log_boot("    init_task addr lookup: %llx\n", init_task);
    log_boot("    init_thread_union addr lookup: %llx\n", init_thread_union_addr);

    // 分析sp_el0寄存器的用途
    if (is_kimg_range(sp_el0)) {  // 如果sp_el0指向内核地址空间
        if (sp_el0 == init_thread_union_addr) {  // 指向初始线程联合体
            sp_el0_is_thread_info = 1;
            log_boot("    sp_el0: current_thread_info\n");
        } else if ((uint64_t)init_task == sp_el0 || (sp_el0 & (page_size - 1)) ||
                   (task_struct_offset.comm_offset = find_swapper_comm_offset(sp_el0, TASK_STRUCT_MAX_SIZE)) > 0) {
            // sp_el0直接指向当前任务结构或能找到swapper进程名
            sp_el0_is_current = 1;
            init_task = (struct task_struct *)sp_el0;  // 更新init_task指针

            log_boot("    sp_el0: current\n");
            log_boot("    init_task addr: %llx\n", init_task);
            if (task_struct_offset.comm_offset > 0) {
                log_boot("    comm_offset of task: %x\n", task_struct_offset.comm_offset);
            }
        } else {
            sp_el0_is_thread_info = 1;  // 默认认为是线程信息
            log_boot("    sp_el0: current_thread_info\n");
        }
    } else {
        // 使用栈指针进行分析
        log_boot("    sp_el0: useless\n");
    }

    // 确定THREAD_SIZE和end_of_stack以及CONFIG_THREAD_INFO_IN_TASK配置
    // 注意：在这里我们使用的栈空间很少，不用担心栈溢出
    int thread_shift_cand[] = { 14, 15, 16 };  // 可能的线程大小位移值（16KB, 32KB, 64KB）
    for (int i = 0; i < sizeof(thread_shift_cand) / sizeof(thread_shift_cand[0]); i++) {
        int tsz = 1 << thread_shift_cand[i];        // 计算线程大小
        uint64_t sp_low = sp & ~(tsz - 1);          // 计算栈底地址（对齐到线程大小）
        // uint64_t sp_high = sp_low + tsz;         // 栈顶地址（用户栈指针）
        uint64_t psp = sp_low;                      // 从栈底开始搜索
        
        // 在线程信息区域搜索栈结束魔数
        for (; psp < sp_low + THREAD_INFO_MAX_SIZE; psp += sizeof(uint32_t)) {
            if (*(uint64_t *)psp == STACK_END_MAGIC) {  // 找到栈结束魔数
                if (psp == sp_low) {                    // 魔数在栈底，说明thread_info在task中
                    thread_size = tsz;
                    stack_end_offset = 0;
                    thread_info_in_task = 1;
                } else {                                // 魔数不在栈底，说明thread_info独立
                    thread_size = tsz;
                    stack_end_offset = psp - sp_low;    // 计算栈结束偏移量
                    thread_info_in_task = 0;
                }
                break;
            }
        }
        if (thread_size > 0) {  // 找到有效的线程大小
            log_boot("    init stack end: %llx\n", psp);
            break;
        }
    }

    log_boot("    thread_size: %x\n", thread_size);
    log_boot("    stack_end_offset: %x\n", stack_end_offset);
    log_boot("    thread_info_in_task: %x\n", thread_info_in_task);

    // 确定task_in_thread_info_offset偏移量，通常是16字节，参见thread_info_be490
    if (!thread_info_in_task) {  // 如果thread_info不在task结构中
        uint64_t thread_info_addr = (uint64_t)current_thread_info_sp();  // 获取线程信息地址
        if (init_task) {  // 如果已知init_task地址
            // 在thread_info中查找指向init_task的指针
            for (uint64_t ptr = thread_info_addr; ptr < thread_info_addr + stack_end_offset; ptr += sizeof(uint32_t)) {
                uint64_t pv = *(uint64_t *)ptr;
                if (pv == (uint64_t)init_task) {  // 找到指向init_task的指针
                    task_in_thread_info_offset = ptr - thread_info_addr;  // 计算偏移量
                    break;
                }
            }
        } else { // 不太可能的情况：没有init_task地址
            // 通过查找swapper进程名来定位task结构
            for (uint64_t ptr = thread_info_addr; ptr < thread_info_addr + stack_end_offset; ptr += sizeof(uint32_t)) {
                uint64_t pv = *(uint64_t *)ptr;
                task_struct_offset.comm_offset = find_swapper_comm_offset(pv, TASK_STRUCT_MAX_SIZE);
                if (task_struct_offset.comm_offset > 0) {  // 找到包含swapper进程名的结构
                    init_task = (struct task_struct *)pv;    // 设置init_task
                    task_in_thread_info_offset = ptr - thread_info_addr;  // 计算偏移量
                    log_boot("    init_task addr: %llx\n", init_task);
                    log_boot("    comm_offset of task: %x\n", task_struct_offset.comm_offset);
                }
            }
        }
        log_boot("    task_in_thread_info_offset: %x\n", task_in_thread_info_offset);
    }

    // 如果还没有找到comm_offset，直接在init_task中查找
    if (task_struct_offset.comm_offset <= 0) {
        task_struct_offset.comm_offset = find_swapper_comm_offset((uint64_t)init_task, TASK_STRUCT_MAX_SIZE);
        log_boot("    comm_offset of task: %x\n", task_struct_offset.comm_offset);
    }

    // 解析栈偏移量 - 在task_struct中查找指向栈底的指针
    uint64_t stack_base = (sp & ~(thread_size - 1));  // 计算当前栈底地址
    for (uintptr_t i = (uintptr_t)init_task; i < (uintptr_t)init_task + TASK_STRUCT_MAX_SIZE; i += sizeof(uint32_t)) {
        uintptr_t val = *(uintptr_t *)i;
        if (stack_base == val) {  // 找到指向栈底的字段
            stack_in_task_offset = i - (uintptr_t)init_task;        // 全局偏移量
            task_struct_offset.stack_offset = stack_in_task_offset; // task结构中的栈偏移量
            break;
        }
    }
    log_boot("    stack offset of task: %x\n", task_struct_offset.stack_offset);

    return 0;  // 成功返回
}

/**
 * 解析mm_struct结构体字段偏移量
 * 主要解析页全局目录(pgd)字段的偏移量，用于内存管理
 * @return 成功返回0，失败返回负值
 */
int resolve_mm_struct_offset()
{
    if (!init_mm) return 0;  // 如果没有init_mm则跳过

    log_boot("struct mm_struct: \n");

    // 可选方法：从任务获取mm结构
    // struct mm_struct *mm = get_task_mm(init_task);
    // uintptr_t init_mm_addr = (uintptr_t)mm;

    uintptr_t init_mm_addr = (uintptr_t)init_mm;  // 使用init_mm地址
    if (!init_mm_addr) return 0;  // 地址无效则返回

    // 在mm_struct中查找pgd字段 - 页全局目录指针
    for (uintptr_t i = init_mm_addr; i < init_mm_addr + MM_STRUCT_MAX_SIZE; i += sizeof(uint32_t)) {
        uint64_t pgd = *(uintptr_t *)i;
        if (pgd == phys_to_kimg(pgd_pa)) {  // 检查是否为页全局目录物理地址对应的内核虚拟地址
            mm_struct_offset.pgd_offset = i - init_mm_addr;  // 记录pgd字段偏移量
        }
    }
    log_boot("    pgd offset: %x\n", mm_struct_offset.pgd_offset);
    return 0;  // 成功返回
}

/**
 * 统一的结构体偏移量解析入口函数
 * 按顺序解析current指针、task_struct、cred和mm_struct的字段偏移量
 * @return 成功返回0，失败返回对应的错误码
 */
int resolve_struct()
{
    full_cap = CAP_FULL_SET;  // 设置完整能力集

    int err = 0;  // 错误码

    // 按顺序执行各个解析步骤，任何一步失败都会跳转到错误处理
    if ((err = resolve_current())) goto out;        // 解析当前任务指针获取方式

    if ((err = resolve_task_offset())) goto out;    // 解析task_struct偏移量

    if ((err = resolve_cred_offset())) goto out;    // 解析cred结构偏移量

    resolve_mm_struct_offset();  // 解析mm_struct偏移量（不检查错误）

out:
    return err;  // 返回错误码（0表示成功）
}
