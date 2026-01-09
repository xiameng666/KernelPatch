// pt_regs结构体偏移量解析模块 - 通过hook execve系统调用确定寄存器结构在内核栈中的位置

#include <ktypes.h>

#include <hook.h>
#include <syscall.h>
#include <asm/current.h>
#include <asm/ptrace.h>
#include <linux/ptrace.h>
#include <log.h>
#include <preset.h>

static int first_init_execed = 0;  // 标记是否已执行第一次init

/**
 * 处理第一次exec执行事件
 */
static void before_first_exec()
{
    log_boot("event: %s\n", EXTRA_EVENT_PRE_EXEC_INIT);
}

// https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2087
// SYSCALL_DEFINE3(execve, const char __user *, filename, const char __user *const __user *, argv,
//                 const char __user *const __user *, envp)

// https://elixir.bootlin.com/linux/v6.1/source/fs/exec.c#L2095
// SYSCALL_DEFINE5(execveat, int, fd, const char __user *, filename, const char __user *const __user *, argv,
//                 const char __user *const __user *, envp, int, flags)

/**
 * execve系统调用前置hook - 通过分析内核栈确定pt_regs结构偏移量
 * @param args hook参数结构体
 * @param udata 用户数据，包含系统调用号
 */
static void before_execve(hook_fargs3_t *args, void *udata)
{
    if (first_init_execed) return;  // 只在第一次执行时处理
    first_init_execed = 1;
    before_first_exec();

    log_boot("kernel stack:\n");

    // 获取系统调用参数
    uint64_t arg0 = syscall_argn(args, 0);
    uint64_t arg1 = syscall_argn(args, 1);
    uint64_t arg2 = syscall_argn(args, 2);
    uint64_t nr = (uint64_t)udata;  // 系统调用号

    // 计算内核栈地址范围
    unsigned long stack = (unsigned long)get_stack(current);
    uintptr_t addr = (uintptr_t)(thread_size + stack);

    // 在内核栈中搜索pt_regs结构体
    // 通过匹配系统调用参数来定位pt_regs的位置
    for (uintptr_t i = addr - sizeof(struct pt_regs) - 0x40; i < addr - 32 * 8; i += sizeof(uint32_t)) {
        uintptr_t val0 = *(uintptr_t *)i;       // 栈中的第一个值
        uintptr_t val1 = *(uintptr_t *)(i + 0x8);   // 栈中的第二个值
        uintptr_t val2 = *(uintptr_t *)(i + 0x10);  // 栈中的第三个值

        // 检查是否与系统调用参数匹配
        if ((arg0 == val0) && (val1 == arg1) && (val2 == arg2)) {
            struct pt_regs *regs = (struct pt_regs *)i;
            // 验证pt_regs结构的有效性
            if (regs->orig_x0 == arg0 && regs->syscallno == nr && regs->regs[8] == nr) {
                pt_regs_offset = addr - i;  // 计算偏移量
                break;
            }
        }
    }
    log_boot("    pt_regs offset: %x\n", pt_regs_offset);
}

/**
 * execv系统调用后置hook - 移除hook
 * @param args hook参数结构体
 * @param udata 用户数据
 */
static void after_execv(hook_fargs5_t *args, void *udata)
{
    // 解析完成后移除hook
    unhook_syscalln(__NR_execve, before_execve, after_execv);
    unhook_syscalln(__NR_execveat, before_execve, after_execv);
}

/**
 * 解析pt_regs结构体偏移量的主函数
 * @return 成功返回0，失败返回错误码
 */
int resolve_pt_regs()
{
    hook_err_t ret = 0;
    hook_err_t rc = HOOK_NO_ERR;

    // hook execve系统调用
    rc = hook_syscalln(__NR_execve, 3, before_execve, after_execv, (void *)__NR_execve);
    log_boot("hook __NR_execve rc: %d\n", rc);
    ret |= rc;

    // hook execveat系统调用
    rc = hook_syscalln(__NR_execveat, 5, before_execve, after_execv, (void *)__NR_execveat);
    log_boot("hook __NR_execveat rc: %d\n", rc);
    ret |= rc;

    return rc;
}