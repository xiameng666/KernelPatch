/*********************************************************************
* Filename:   sha256.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the SHA-256 hashing algorithm.
              SHA-256 is one of the three algorithms in the SHA2
              specification. The others, SHA-384 and SHA-512, are not
              offered in this implementation.
              Algorithm specification can be found here:
               * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              This implementation uses little endian byte order.
              
              中文说明：
              SHA-256 加密哈希算法实现，用于 KernelPatch 项目中的数据完整性验证
              该实现遵循 FIPS 180-2 标准，支持小端序字节顺序
              主要用于内核镜像和模块的哈希值计算和验证
*********************************************************************/

/*************************** HEADER FILES ***************************/
#include "sha256.h"

/****************************** MACROS ******************************/
// 32 位左循环移位操作
#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
// 32 位右循环移位操作
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))

// SHA-256 算法中的逻辑函数
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))         // 选择函数
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))  // 多数函数
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))    // Σ0 函数
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))    // Σ1 函数
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))        // σ0 函数
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))      // σ1 函数

/**************************** VARIABLES *****************************/
// SHA-256 算法的 64 个常数 K，来源于前 64 个质数的立方根的小数部分
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

/*********************** FUNCTION DEFINITIONS ***********************/
/**
 * SHA-256 核心变换函数
 * 
 * @param ctx SHA256 上下文结构体
 * @param data 64 字节的输入数据块
 * 
 * 功能说明：
 * - 实现 SHA-256 算法的核心压缩函数
 * - 将 64 字节数据块扩展为 64 个 32 位字
 * - 执行 64 轮哈希计算更新内部状态
 * - 每轮使用不同的常数和消息字进行计算
 */
void sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
    WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    // 将 64 字节输入数据转换为 16 个 32 位大端序字
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    
    // 扩展 16 个字为 64 个字（消息调度）
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

    // 执行 64 轮主循环计算
    for (i = 0; i < 64; ++i) {
        // 计算临时值 t1 和 t2
        t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        
        // 更新工作变量（循环移位）
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // 将工作变量加回哈希值（模 2^32 加法）
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/**
 * 初始化 SHA-256 上下文
 * 
 * @param ctx SHA256 上下文结构体指针
 * 
 * 功能说明：
 * - 重置数据长度和位长度计数器
 * - 设置 SHA-256 标准初始哈希值
 * - 这些初始值来源于前 8 个质数的平方根的小数部分
 */
void sha256_init(SHA256_CTX *ctx)
{
    ctx->datalen = 0;    // 当前数据块中的字节数
    ctx->bitlen = 0;     // 总处理位数
    
    // SHA-256 标准初始哈希值（H0 到 H7）
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

/**
 * 更新 SHA-256 哈希计算（处理输入数据）
 * 
 * @param ctx SHA256 上下文结构体指针
 * @param data 输入数据缓冲区
 * @param len 输入数据长度
 * 
 * 功能说明：
 * - 将输入数据添加到内部缓冲区
 * - 当缓冲区满 64 字节时触发变换函数
 * - 支持流式处理任意长度的数据
 * - 自动管理数据块边界和位长度计数
 */
void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
    WORD i;

    for (i = 0; i < len; ++i) {
        // 将字节添加到内部数据缓冲区
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        
        // 当缓冲区满 64 字节时处理完整块
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;    // 增加 512 位 (64 字节)
            ctx->datalen = 0;      // 重置缓冲区
        }
    }
}

/**
 * 完成 SHA-256 哈希计算并输出最终结果
 * 
 * @param ctx SHA256 上下文结构体指针
 * @param hash 输出的 32 字节哈希值缓冲区
 * 
 * 功能说明：
 * - 对剩余数据进行填充处理
 * - 添加消息长度信息到填充末尾
 * - 执行最后的变换计算
 * - 将内部状态转换为大端序字节输出
 * - 遵循 SHA-256 标准的填充和长度编码规则
 */
void sha256_final(SHA256_CTX *ctx, BYTE hash[])
{
    WORD i;

    i = ctx->datalen;

    // 对剩余数据进行填充
    if (ctx->datalen < 56) {
        // 如果剩余空间足够，直接填充
        ctx->data[i++] = 0x80;    // 添加强制的 1 位（0x80 = 10000000）
        while (i < 56)
            ctx->data[i++] = 0x00;    // 用零填充到 56 字节
    } else {
        // 如果剩余空间不够，需要额外的块
        ctx->data[i++] = 0x80;    // 添加强制的 1 位
        while (i < 64)
            ctx->data[i++] = 0x00;    // 填充到块末尾
        sha256_transform(ctx, ctx->data);    // 处理填充块
        
        // 清零新块准备添加长度信息
        for (int i = 0; i < 56; i++)
            ctx->data[i] = 0;
    }

    // 在填充末尾添加原始消息的总位长度（大端序 64 位）
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;         // 位长度低 8 位
    ctx->data[62] = ctx->bitlen >> 8;    // 位长度次低 8 位
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;   // 位长度高 8 位
    sha256_transform(ctx, ctx->data);    // 处理包含长度的最终块

    // 将内部状态转换为大端序字节序输出
    // 因为实现使用小端序而 SHA 规范要求大端序，所以需要字节序转换
    for (i = 0; i < 4; ++i) {
        hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;        // H0
        hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;    // H1
        hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;    // H2
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;   // H3
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;   // H4
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;   // H5
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;   // H6
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;   // H7
    }
}