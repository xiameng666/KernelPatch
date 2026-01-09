// ARM64内存屏障定义头文件
#ifndef _KP_BARRIER_H_
#define _KP_BARRIER_H_

// 完整内存屏障，确保所有内存访问按顺序执行
#define mb() asm volatile("dmb ish" ::: "memory")
// 写内存屏障，确保写操作按顺序执行
#define wmb() asm volatile("dmb ishst" ::: "memory")
// 读内存屏障，确保读操作按顺序执行
#define rmb() asm volatile("dmb ishld" ::: "memory")

// SMP多核环境下的内存屏障，与内核实现保持一致
// 内核在ARM64上使用dmb变体实现SMP屏障，与上面的mb()/wmb()/rmb()基本相同
// 虽然内核对后者使用dsb，但无论如何，如果上面的mb()/wmb()/rmb()发生变化，确保下面的smp_*()不变
#define smp_mb() asm volatile("dmb ish" ::: "memory")
#define smp_wmb() asm volatile("dmb ishst" ::: "memory")
#define smp_rmb() asm volatile("dmb ishld" ::: "memory")

// 原子存储释放操作，确保存储操作在释放语义下可见
#define smp_store_release(p, v)                                                         \
    do {                                                                                \
        union                                                                           \
        {                                                                               \
            typeof(*p) __val;                                                           \
            char __c[1];                                                                \
        } __u = { .__val = (v) };                                                       \
        compiletime_assert_atomic_type(*p);                                             \
                                                                                        \
        switch (sizeof(*p)) {                                                           \
        case 1:                                                                         \
            asm volatile("stlrb %w1, %0" : "=Q"(*p) : "r"(*(u8 *)__u.__c) : "memory");  \
            break;                                                                      \
        case 2:                                                                         \
            asm volatile("stlrh %w1, %0" : "=Q"(*p) : "r"(*(u16 *)__u.__c) : "memory"); \
            break;                                                                      \
        case 4:                                                                         \
            asm volatile("stlr %w1, %0" : "=Q"(*p) : "r"(*(u32 *)__u.__c) : "memory");  \
            break;                                                                      \
        case 8:                                                                         \
            asm volatile("stlr %1, %0" : "=Q"(*p) : "r"(*(u64 *)__u.__c) : "memory");   \
            break;                                                                      \
        default:                                                                        \
            /* Only to shut up gcc ... */                                               \
            mb();                                                                       \
            break;                                                                      \
        }                                                                               \
    } while (0)

// 原子加载获取操作，确保加载操作在获取语义下完成
#define smp_load_acquire(p)                                                             \
    ({                                                                                  \
        union                                                                           \
        {                                                                               \
            typeof(*p) __val;                                                           \
            char __c[1];                                                                \
        } __u = { .__c = { 0 } };                                                       \
        compiletime_assert_atomic_type(*p);                                             \
                                                                                        \
        switch (sizeof(*p)) {                                                           \
        case 1:                                                                         \
            asm volatile("ldarb %w0, %1" : "=r"(*(u8 *)__u.__c) : "Q"(*p) : "memory");  \
            break;                                                                      \
        case 2:                                                                         \
            asm volatile("ldarh %w0, %1" : "=r"(*(u16 *)__u.__c) : "Q"(*p) : "memory"); \
            break;                                                                      \
        case 4:                                                                         \
            asm volatile("ldar %w0, %1" : "=r"(*(u32 *)__u.__c) : "Q"(*p) : "memory");  \
            break;                                                                      \
        case 8:                                                                         \
            asm volatile("ldar %0, %1" : "=r"(*(u64 *)__u.__c) : "Q"(*p) : "memory");   \
            break;                                                                      \
        default:                                                                        \
            /* Only to shut up gcc ... */                                               \
            mb();                                                                       \
            break;                                                                      \
        }                                                                               \
        __u.__val;                                                                      \
    })

#endif