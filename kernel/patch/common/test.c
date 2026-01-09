// SELinux安全上下文测试模块 - 测试安全上下文转换功能
#include <log.h>
#include <linux/security.h>
#include <linux/string.h>

// 测试SELinux安全上下文到安全ID的转换
void test()
{
    logkd("=== start test ===");

    // 定义测试用的SELinux安全上下文字符串
    const char *sctx = "u:r:kernel:s0";

    uint32_t secid = 0;
    // 调用内核安全模块函数，将安全上下文字符串转换为安全ID
    int rc = security_secctx_to_secid(sctx, strlen(sctx), &secid);

    // 输出转换结果：安全ID和返回码
    logkd("secid: %d, rc: %d\n", secid, rc);

    logkd("=== end test ===");
}