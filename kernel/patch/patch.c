// 内核补丁核心模块 - 负责协调和管理所有补丁功能的初始化

#include <log.h>
#include <ksyms.h>
#include <kallsyms.h>
#include <hook.h>
#include <accctl.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/cred.h>
#include <linux/capability.h>
#include <syscall.h>
#include <module.h>
#include <predata.h>
#include <linux/string.h>

/**
 * 打印启动日志 - 在内核panic时输出KernelPatch的启动信息
 */
void print_bootlog()
{
    const char *log = get_boot_log();  // 获取启动日志
    char buf[1024];
    int off = 0;
    char c;
    // 逐字符处理日志，遇到换行符时输出一行
    for (int i = 0; (c = log[i]); i++) {
        if (c == '\n') {
            buf[off++] = c;
            buf[off] = '\0';

            printk("KP %s", buf);  // 使用printk输出日志行
            off = 0;
        } else {
            buf[off++] = log[i];
        }
    }
}

/**
 * 内核panic前的hook函数 - 在系统崩溃前输出KernelPatch信息
 * @param args hook参数结构体
 * @param udata 用户数据
 */
void before_panic(hook_fargs12_t *args, void *udata)
{
    printk("==== Start KernelPatch for Kernel panic ====\n");
    print_bootlog();  // 输出完整的启动日志
    printk("==== End KernelPatch for Kernel panic ====\n");
}

// 外部函数声明 - 各个子系统的初始化函数
void linux_misc_symbol_init();    // Linux杂项符号初始化
void linux_libs_symbol_init();    // Linux库符号初始化

// 各个子系统初始化函数声明
int resolve_struct();             // 解析内核结构体
int task_observer();              // 任务观察器初始化
int bypass_kcfi();                // 绕过KCFI保护
int bypass_selinux();             // 绕过SELinux安全策略
int resolve_pt_regs();            // 解析pt_regs结构
int supercall_install();          // 安装超级调用
void module_init();               // 模块系统初始化
void syscall_init();              // 系统调用初始化
int kstorage_init();              // 内核存储初始化
int su_compat_init();             // su兼容性初始化

#ifdef ANDROID
int android_user_init();          // Android用户初始化
int android_sepolicy_flags_fix(); // Android SEPolicy标志修复
#endif

/**
 * rest_init前的hook函数 - 在内核初始化早期阶段执行KernelPatch初始化
 * @param args hook参数结构体
 * @param udata 用户数据
 */
static void before_rest_init(hook_fargs4_t *args, void *udata)
{
    int rc = 0;
    log_boot("entering init ...\n");

    // 逐步初始化各个子系统，任何步骤失败都会跳转到out
    if ((rc = bypass_kcfi())) goto out;
    log_boot("bypass_kcfi done: %d\n", rc);

    if ((rc = resolve_struct())) goto out;
    log_boot("resolve_struct done: %d\n", rc);

    if ((rc = bypass_selinux())) goto out;
    log_boot("bypass_selinux done: %d\n", rc);

    if ((rc = task_observer())) goto out;
    log_boot("task_observer done: %d\n", rc);

    rc = supercall_install();  // 安装超级调用系统
    log_boot("supercall_install done: %d\n", rc);

    rc = kstorage_init();      // 初始化内核存储
    log_boot("kstorage_init done: %d\n", rc);

    rc = su_compat_init();     // 初始化su兼容性
    log_boot("su_compat_init done: %d\n", rc);

    rc = resolve_pt_regs();    // 解析寄存器结构
    log_boot("resolve_pt_regs done: %d\n", rc);

#ifdef ANDROID
    rc = android_sepolicy_flags_fix();  // 修复Android SEPolicy标志
    log_boot("android_sepolicy_flags_fix done: %d\n", rc);

    rc = android_user_init();           // 初始化Android用户系统
    log_boot("android_user_init done: %d\n", rc);
#endif

out:
    return;
}

/**
 * 内核初始化前额外事件处理 - 处理KPM模块的预加载
 * @param extra 额外项配置
 * @param args 参数字符串
 * @param data 数据指针
 * @param udata 用户数据
 * @return 处理结果，0表示成功
 */
static int extra_event_pre_kernel_init(const patch_extra_item_t *extra, const char *args, const void *data, void *udata)
{
    if (extra->type == EXTRA_TYPE_KPM) {  // 如果是KPM类型的额外项
        // 检查事件名称是否匹配或为空
        if (!strcmp(EXTRA_EVENT_PRE_KERNEL_INIT, extra->event) || !extra->event[0]) {
            // 加载KPM模块
            int rc = load_module(data, extra->con_size, args, EXTRA_EVENT_PRE_KERNEL_INIT, 0);
            log_boot("load kpm: %s, rc: %d\n", extra->name, rc);
        }
    }
    return 0;
}

/**
 * kernel_init前的hook函数 - 处理内核初始化前的额外事件
 * @param args hook参数结构体
 * @param udata 用户数据
 */
static void before_kernel_init(hook_fargs4_t *args, void *udata)
{
    log_boot("event: %s\n", EXTRA_EVENT_PRE_KERNEL_INIT);
    on_each_extra_item(extra_event_pre_kernel_init, 0);  // 遍历并处理额外项
}

/**
 * kernel_init后的hook函数 - 内核初始化完成后的处理
 * @param args hook参数结构体
 * @param udata 用户数据
 */
static void after_kernel_init(hook_fargs4_t *args, void *udata)
{
    log_boot("event: %s\n", EXTRA_EVENT_POST_KERNEL_INIT);
}

/**
 * 主补丁函数 - KernelPatch的核心入口点
 * @return 返回值，0表示成功，非0表示失败
 */
int patch()
{
    // 初始化符号和模块系统
    linux_libs_symbol_init();  // 初始化Linux库符号
    linux_misc_symbol_init();  // 初始化Linux杂项符号
    module_init();              // 初始化模块系统
    syscall_init();             // 初始化系统调用

    hook_err_t rc = 0;

    // Hook panic函数以便在系统崩溃时输出调试信息
    unsigned long panic_addr = patch_config->panic;
    logkd("panic addr: %llx\n", panic_addr);
    if (panic_addr) {
        rc = hook_wrap12((void *)panic_addr, before_panic, 0, 0);
        log_boot("hook panic rc: %d\n", rc);
    }
    if (rc) return rc;

    // Hook rest_init或cgroup_init函数进行早期初始化
    unsigned long init_addr = patch_config->rest_init;
    if (!init_addr) init_addr = patch_config->cgroup_init;  // 备用地址
    if (init_addr) {
        rc = hook_wrap4((void *)init_addr, before_rest_init, 0, (void *)init_addr);
        log_boot("hook rest_init rc: %d\n", rc);
    }
    if (rc) return rc;

    // Hook kernel_init函数处理内核初始化事件
    unsigned long kernel_init_addr = patch_config->kernel_init;
    if (kernel_init_addr) {
        rc = hook_wrap4((void *)kernel_init_addr, before_kernel_init, after_kernel_init, 0);
        log_boot("hook kernel_init rc: %d\n", rc);
    }

    return rc;  // 返回最终的错误码
}
