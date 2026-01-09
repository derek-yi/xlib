/*
 * RC4 implementation
 * Issue date: 03/18/2006
 *
 * Copyright (C) 2006 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

typedef struct {
    uint8 se[256], sd[256];
    uint32 pose, posd;
    uint8 te, td;
} rc4_ctx;

#if 1

char base46_map[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                     'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                     'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                     'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

char* base64_encode(char* plain) 
{
    int counts = 0;  // 改为int
    unsigned char buffer[3];  // 改为unsigned char
    int plain_len = strlen(plain);
    char* cipher = malloc((plain_len * 4 / 3 + 4) * sizeof(char));
    int i = 0, c = 0;
    
    for(i = 0; i < plain_len; i++) {
        buffer[counts++] = (unsigned char)plain[i];
        if(counts == 3) {
            cipher[c++] = base46_map[buffer[0] >> 2];
            cipher[c++] = base46_map[((buffer[0] & 0x03) << 4) | (buffer[1] >> 4)];
            cipher[c++] = base46_map[((buffer[1] & 0x0f) << 2) | (buffer[2] >> 6)];
            cipher[c++] = base46_map[buffer[2] & 0x3f];
            counts = 0;
        }
    }
    
    if(counts > 0) {
        cipher[c++] = base46_map[buffer[0] >> 2];
        if(counts == 1) {
            cipher[c++] = base46_map[(buffer[0] & 0x03) << 4];
            cipher[c++] = '=';
        } else {
            cipher[c++] = base46_map[((buffer[0] & 0x03) << 4) | (buffer[1] >> 4)];
            cipher[c++] = base46_map[(buffer[1] & 0x0f) << 2];
        }
        cipher[c++] = '=';
    }
    
    cipher[c] = '\0';
    return cipher;
}

char* base64_decode(char* cipher) 
{
    int counts = 0;  // 改为int
    unsigned char buffer[4];  // 改为unsigned char
    char* plain = malloc(strlen(cipher) * 3 / 4 + 1);  // 分配额外空间
    int i = 0, p = 0;
    unsigned char k;  // 改为unsigned char

    for(i = 0; cipher[i] != '\0'; i++) {
        for(k = 0 ; k < 64 && base46_map[k] != cipher[i]; k++);
        if(k == 64 && cipher[i] != '=') {
            // 非法Base64字符
            free(plain);
            return NULL;
        }
        buffer[counts++] = k;
        if(counts == 4) {
            plain[p++] = (buffer[0] << 2) + (buffer[1] >> 4);
            if(buffer[2] != 64)
                plain[p++] = (buffer[1] << 4) + (buffer[2] >> 2);
            if(buffer[3] != 64)
                plain[p++] = (buffer[2] << 6) + buffer[3];
            counts = 0;
        }
    }
    plain[p] = '\0';
    return plain;
}

//https://github.com/ogay/rc4
static const uint8 rc4_table[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
    0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
    0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
    0xfc, 0xfd, 0xfe, 0xff};

#define SWAP(x, y) \
{                  \
    tmp = x;       \
    x = y;         \
    y = tmp;       \
}

#define RC4_CRYPT()                                             \
{                                                               \
    t += s[(uint8) i];                                          \
    SWAP(s[(uint8) i], s[t]);                                   \
    new_dst[i] = new_src[i] ^ s[(uint8) (s[(uint8) i] + s[t])]; \
}

void rc4_ks(rc4_ctx *ctx, const uint8 *key, uint32 key_len)
{
    uint32 i;
    uint8 *s;
    uint8 t, tmp;

    t = 0;
    s = ctx->se;
    //assert(key_len > 0 && key_len <= 256);

    ctx->pose = 1;
    ctx->posd = 1;
    ctx->te = 0;
    ctx->td = 0;
    memcpy(s, rc4_table, 256);
    for (i = 0; i < 256; i++) {
        t += s[i] + key[i % key_len];
        SWAP(s[i], s[t]);
    }

    memcpy(ctx->sd, s, 256);
}

void rc4_encrypt(rc4_ctx *ctx, const uint8 *src, uint8 *dst, uint32 len)
{
    uint32 i;
    uint32 pos;
    const uint8 *new_src;
    uint8 *s, *new_dst;
    uint8 t, tmp;

    pos = ctx->pose;
    s = ctx->se;
    t = ctx->te;

    new_src = src - pos;
    new_dst = dst - pos;
    for (i = pos; i < len + pos; i++) {
        RC4_CRYPT();
    }

    ctx->pose = i;
    ctx->te = t;
}

void rc4_decrypt(rc4_ctx *ctx, const uint8 *src, uint8 *dst, uint32 len)
{
    uint32 i;
    uint32 pos;
    const uint8 *new_src;
    uint8 *s, *new_dst;
    uint8 t, tmp;

    pos = ctx->posd;
    s = ctx->sd;
    t = ctx->td;

    new_src = src - pos;
    new_dst = dst - pos;
    for (i = pos; i < len + pos; i++) {
        RC4_CRYPT();
    }

    ctx->posd = i;
    ctx->td = t;
}

//https://www.toolhelper.cn/SymmetricEncryption/RC4
int main()
{
    char *key = "0123456789abcdefg";
    rc4_ctx ctx;
    char *pt = "{\"lic_enable\":1, \"expired_day\":\"20250101\", \"credit_max\":1000}";
    uint8 dst[512];
    uint8 decode[512];
    char *bs_str, *new_str;
    int i, data_len;
    
    data_len = strlen(pt);
    
    rc4_ks(&ctx, (uint8*)key, strlen(key));
    rc4_encrypt(&ctx, (uint8*)pt, dst, data_len);
    
    printf("rc4_encrypt: ");
    for (i = 0; i < data_len; i++) {
        printf("0x%02x ", dst[i]);
    }
    printf("\nstrlen(pt) %d\n", data_len);
    
    // 在Base64编码前确保dst以null结尾
    dst[data_len] = '\0';
    bs_str = base64_encode((char*)dst);
    printf("bs_str: %s\n", bs_str);
    
    // 解码并检查
    new_str = base64_decode(bs_str);
    if(new_str == NULL) {
        printf("Base64 decode failed!\n");
        return -1;
    }
    
    // 重新初始化RC4上下文用于解密（或者重用同一个）
    rc4_ctx ctx2;
    rc4_ks(&ctx2, (uint8*)key, strlen(key));
    rc4_decrypt(&ctx2, (uint8*)new_str, decode, strlen(new_str));
    
    // 确保解码后的字符串以null结尾
    decode[data_len] = '\0';
    printf("decode: %s\n", decode);
    
    // 清理内存
    free(bs_str);
    free(new_str);
    
    return 0;
}


#endif

