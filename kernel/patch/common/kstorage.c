/* 
 * 内核存储模块 - 提供线程安全的内核数据存储服务
 * 使用RCU锁机制保证读写并发安全
 * 支持分组管理，每个组内按ID索引存储数据
 */

#include <kstorage.h>

#include <linux/kernel.h>
#include <linux/rculist.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <compiler.h>
#include <stdbool.h>
#include <symbol.h>
#include <uapi/asm-generic/errno.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <kputils.h>

#define KSTRORAGE_MAX_GROUP_NUM 4  // 最大存储组数量

// static atomic64_t used_max_group = ATOMIC_INIT(0);
static int used_max_group = -1;  // 当前已使用的最大组号
static struct list_head kstorage_groups[KSTRORAGE_MAX_GROUP_NUM];  // 各组的链表头
static spinlock_t kstorage_glocks[KSTRORAGE_MAX_GROUP_NUM];  // 各组的自旋锁
static int group_sizes[KSTRORAGE_MAX_GROUP_NUM] = { 0 };  // 各组的元素数量
static spinlock_t used_max_group_lock;  // 组分配锁

// RCU回收回调函数
// 在RCU宽限期结束后释放kstorage结构体内存
static void reclaim_callback(struct rcu_head *rcu)
{
    struct kstorage *ks = container_of(rcu, struct kstorage, rcu);
    kvfree(ks);
}

// 尝试分配新的存储组
// 返回值: 成功时返回组ID(>=0)，失败时返回-1
int try_alloc_kstroage_group()
{
    spin_lock(&used_max_group_lock);
    // 检查是否还有可用的组
    if (used_max_group + 1 >= KSTRORAGE_MAX_GROUP_NUM) {
        spin_unlock(&used_max_group_lock);
        return -1;
    }
    // 分配新组
    used_max_group++;
    spin_unlock(&used_max_group_lock);
    return used_max_group;
}

// 获取指定组的元素数量
// gid: 组ID
// 返回值: 成功时返回元素数量，失败时返回-ENOENT
int kstorage_group_size(int gid)
{
    if (gid < 0 || gid >= KSTRORAGE_MAX_GROUP_NUM) return -ENOENT;
    return group_sizes[gid];
}

// 写入数据到指定的存储组和ID
// gid: 组ID
// did: 数据ID
// data: 数据指针
// offset: 数据偏移
// len: 数据长度
// data_is_user: 数据是否来自用户空间
// 返回值: 0=成功，<0=失败
int write_kstorage(int gid, long did, void *data, int offset, int len, bool data_is_user)
{
    int rc = -ENOENT;
    if (gid < 0 || gid >= KSTRORAGE_MAX_GROUP_NUM) return rc;

    struct list_head *head = &kstorage_groups[gid];
    spinlock_t *lock = &kstorage_glocks[gid];
    struct kstorage *pos = 0, *old = 0;

    rcu_read_lock();

    // 查找是否已存在相同ID的数据项
    list_for_each_entry(pos, head, list)
    {
        if (pos->did == did) {
            old = pos;
            break;
        }
    }

    // 分配新的存储结构
    struct kstorage *new = (struct kstorage *)vmalloc(sizeof(struct kstorage) + len);   
    if (!new) {
        rcu_read_unlock();
        return -ENOMEM;
    }
    new->gid = gid;
    new->did = did;
    new->dlen = 0;
    
    // 复制数据（处理用户空间数据）
    if (data_is_user) {
        void *drc = memdup_user(data + offset, len);
        if (IS_ERR(drc)) {
            rcu_read_unlock();
            return PTR_ERR(drc);
        }
        memcpy(new->data, drc, len);
        kvfree(drc);
    } else {
        memcpy(new->data, data + offset, len);
    }
    new->dlen = len;

    spin_lock(lock);
    if (old) { // 更新已存在的项
        list_replace_rcu(&old->list, &new->list);
    } else { // 添加新项
        list_add_rcu(&new->list, head);
        group_sizes[gid]++;
    }
    spin_unlock(lock);

    rcu_read_unlock();

    // 如果是更新操作，需要安全回收旧的数据结构
    if (old) {
        bool async = true;
        if (async) {
            // 异步回收：使用RCU延迟释放
            call_rcu(&old->rcu, reclaim_callback);
        } else {
            // 同步回收：等待RCU宽限期后立即释放
            synchronize_rcu();
            kvfree(old);
        }
    }
    return 0;
}
KP_EXPORT_SYMBOL(write_kstorage);

// 获取指定组和ID的存储项
// gid: 组ID
// did: 数据ID
// 返回值: 成功时返回kstorage指针，失败时返回错误指针
const struct kstorage *get_kstorage(int gid, long did)
{
    if (gid < 0 || gid >= KSTRORAGE_MAX_GROUP_NUM) return ERR_PTR(-ENOENT);

    struct list_head *head = &kstorage_groups[gid];
    struct kstorage *pos = 0;

    // 遍历链表查找匹配的ID
    list_for_each_entry(pos, head, list)
    {
        if (pos->did == did) {
            return pos;
        }
    }

    return ERR_PTR(-ENOENT);
}
KP_EXPORT_SYMBOL(get_kstorage);

// 对指定组的每个存储项执行回调函数
// gid: 组ID
// cb: 回调函数
// udata: 用户数据，传递给回调函数
// 返回值: 0=成功，其他=回调函数返回的错误码
int on_each_kstorage_elem(int gid, on_kstorage_cb cb, void *udata)
{
    if (gid < 0 || gid >= KSTRORAGE_MAX_GROUP_NUM) return -ENOENT;

    int rc = 0;

    struct list_head *head = &kstorage_groups[gid];
    struct kstorage *pos = 0;

    rcu_read_lock();

    // 遍历链表，对每个元素执行回调
    list_for_each_entry(pos, head, list)
    {
        int rc = cb(pos, udata);
        if (rc) break;  // 如果回调返回非零值，停止遍历
    }

    rcu_read_unlock();

    return rc;
}
KP_EXPORT_SYMBOL(on_each_kstorage_elem);

// 从指定组和ID读取数据
// gid: 组ID
// did: 数据ID
// data: 输出缓冲区
// offset: 读取偏移
// len: 读取长度
// data_is_user: 输出缓冲区是否在用户空间
// 返回值: 0=成功，<0=失败
int read_kstorage(int gid, long did, void *data, int offset, int len, bool data_is_user)
{
    int rc = 0;
    rcu_read_lock();

    // 获取存储项
    const struct kstorage *pos = get_kstorage(gid, did);

    if (IS_ERR(pos)) {
        rcu_read_unlock();
        return PTR_ERR(pos);
    }

    // 计算实际可读取的长度
    int min_len = pos->dlen - offset > len ? len : pos->dlen - offset;

    // 复制数据到输出缓冲区
    if (data_is_user) {
        // 复制到用户空间
        int cplen = compat_copy_to_user(data, pos->data + offset, min_len);
        if (cplen <= 0) {
            logkfe("compat_copy_to_user error: %d", cplen);
            rc = cplen;
        }
    } else {
        // 复制到内核空间
        memcpy(data, pos->data + offset, min_len);
    }

    rcu_read_unlock();
    return rc;
}
KP_EXPORT_SYMBOL(read_kstorage);

// 列出指定组中所有数据项的ID
// gid: 组ID
// ids: 输出ID数组
// idslen: ID数组长度
// data_is_user: ID数组是否在用户空间
// 返回值: 成功时返回实际ID数量，失败时返回负数
int list_kstorage_ids(int gid, long *ids, int idslen, bool data_is_user)
{
    if (gid < 0 || gid >= KSTRORAGE_MAX_GROUP_NUM) return -ENOENT;

    int cnt = 0;

    struct list_head *head = &kstorage_groups[gid];
    struct kstorage *pos = 0;

    rcu_read_lock();

    // 遍历链表，收集所有ID
    list_for_each_entry(pos, head, list)
    {
        if (cnt >= idslen) break;  // 防止缓冲区溢出

        if (data_is_user) {
            // 复制ID到用户空间
            int cplen = compat_copy_to_user(ids + cnt, &pos->did, sizeof(pos->did));
            if (cplen <= 0) {
                logkfe("compat_copy_to_user error: %d", cplen);
                cnt = cplen;
            }
        } else {
            // 复制ID到内核空间
            memcpy(ids + cnt, &pos->did, sizeof(pos->did));
        }
        cnt++;
    }

    rcu_read_unlock();

    return cnt;
}
KP_EXPORT_SYMBOL(list_kstorage_ids);

// 从指定组中删除指定ID的存储项
// gid: 组ID
// did: 数据ID
// 返回值: 0=成功，<0=失败
int remove_kstorage(int gid, long did)
{
    int rc = -ENOENT;
    if (gid < 0 || gid >= KSTRORAGE_MAX_GROUP_NUM) return rc;

    struct list_head *head = &kstorage_groups[gid];
    spinlock_t *lock = &kstorage_glocks[gid];
    struct kstorage *pos = 0;

    spin_lock(lock);

    // 查找要删除的项
    list_for_each_entry(pos, head, list)
    {
        if (pos->did == did) {
            // 从链表中删除（RCU安全）
            list_del_rcu(&pos->list);
            spin_unlock(lock);

            // 更新组大小
            group_sizes[gid]--;

            // 安全回收内存
            bool async = true;
            if (async) {
                // 异步回收：使用RCU延迟释放
                call_rcu(&pos->rcu, reclaim_callback);
            } else {
                // 同步回收：等待RCU宽限期后立即释放
                synchronize_rcu();
                kvfree(pos);
            }
            return 0;
        }
    }

    spin_unlock(lock);

    return 0;
}
KP_EXPORT_SYMBOL(remove_kstorage);

// 内核存储模块初始化函数
// 初始化所有存储组的链表头和锁
// 返回值: 0=成功
int kstorage_init()
{
    // 初始化所有组的链表头和自旋锁
    for (int i = 0; i < KSTRORAGE_MAX_GROUP_NUM; i++) {
        INIT_LIST_HEAD(&kstorage_groups[i]);
        spin_lock_init(&kstorage_glocks[i]);
    }
    // 初始化组分配锁
    spin_lock_init(&used_max_group_lock);

    return 0;
}
