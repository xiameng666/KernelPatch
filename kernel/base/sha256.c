/*********************************************************************
* 文件名:    sha256.c
* 作者:      Brad Conte (brad AT bradconte.com)
* 版权:
* 免责声明:  此代码按"原样"提供，不提供任何保证。
* 详细信息:  SHA-256哈希算法的实现。
            SHA-256是SHA2规范中的三种算法之一。其他两种SHA-384和SHA-512
            在此实现中未提供。
            算法规范可在此处找到：
             * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
            此实现使用小端字节序。
*********************************************************************/

// SHA256哈希算法实现 - 用于根密钥验证和安全计算

/*************************** 头文件 ***************************/
#include "sha256.h"

/****************************** 宏定义 ******************************/
#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))   // 左循环移位
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))  // 右循环移位

#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))               // 选择函数
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z))) // 多数函数
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))  // Σ0函数
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))  // Σ1函数
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))      // σ0函数
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))    // σ1函数

/**************************** 变量 *****************************/
// SHA-256算法使用的64个32位常数（前64个素数的立方根的分数部分）
static const WORD k[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
                            0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
                            0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
                            0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
                            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
                            0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
                            0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
                            0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
                            0xc67178f2 };

/*********************** 函数定义 ***********************/
// SHA-256压缩函数 - 处理512位数据块
void sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
    WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    // 将输入数据转换为64个32位字（大端序）
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    // 扩展前16个字到64个字
    for (; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    // 初始化工作变量为当前哈希值
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    // 主循环：64轮压缩运算
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];  // 临时值1
        t2 = EP0(a) + MAJ(a, b, c);                    // 临时值2
        h = g;                                         // 循环移位
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // 将压缩结果加到当前哈希值上
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

// 初始化SHA-256上下文
void sha256_init(SHA256_CTX *ctx)
{
    ctx->datalen = 0;   // 数据长度
    ctx->bitlen = 0;    // 位长度
    // 设置初始哈希值（前8个素数的平方根的分数部分）
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

// 向SHA-256上下文添加数据
void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
    WORD i;

    // 逐字节处理输入数据
    for (i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];  // 存储数据
        ctx->datalen++;
        if (ctx->datalen == 64) {           // 缓冲区满64字节时处理
            sha256_transform(ctx, ctx->data);   // 执行压缩函数
            ctx->bitlen += 512;                 // 增加位计数
            ctx->datalen = 0;                   // 重置缓冲区
        }
    }
}

// 完成SHA-256计算并输出哈希值
void sha256_final(SHA256_CTX *ctx, BYTE hash[])
{
    WORD i;

    i = ctx->datalen;

    // 添加填充：首先添加一个1位（0x80）
    if (ctx->datalen < 56) {        // 如果剩余空间足够
        ctx->data[i++] = 0x80;      // 添加0x80
        while (i < 56)              // 用0填充到56字节
            ctx->data[i++] = 0x00;
    } else {                        // 如果剩余空间不够
        ctx->data[i++] = 0x80;      // 添加0x80
        while (i < 64)              // 用0填充到64字节
            ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);  // 处理当前块
        for (int i = 0; i < 56; i++)       // 清空新块的前56字节
            ctx->data[i] = 0;
    }

    // 在填充末尾添加原始消息的总长度（以位为单位，大端序）
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;         // 低8位
    ctx->data[62] = ctx->bitlen >> 8;    // 次低8位
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;   // 高8位
    sha256_transform(ctx, ctx->data);    // 处理最后一块

    // 由于此实现使用小端字节序而SHA使用大端序，
    // 在将最终状态复制到输出哈希时需要反转所有字节
    for (i = 0; i < 4; ++i) {
        hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;      // 状态字0
        hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;  // 状态字1
        hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;  // 状态字2
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff; // 状态字3
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff; // 状态字4
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff; // 状态字5
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff; // 状态字6
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff; // 状态字7
    }
}