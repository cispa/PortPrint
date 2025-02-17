#ifndef VICTIMS_H_
#define VICTIMS_H_

#include <stdint.h>
#include <immintrin.h>
#include <emmintrin.h>

#define BIT_SET(val, bit) (((val) >> (bit)) & 0x1)
#define AES_MAX_KEYLENGTH    (15 * 16)
#define AES_MAX_KEYLENGTH_U32    (AES_MAX_KEYLENGTH / sizeof(uint32_t))

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 202311L
#define ENUM_SIZE : unsigned char
#endif
#endif
#ifndef ENUM_SIZE
#define ENUM_SIZE
#endif

typedef void __attribute__((noreturn)) (*victim_fun)(void);
static volatile unsigned char *sync_page = NULL;

enum source_project ENUM_SIZE {
    source_project_custom = 0,
    source_project_linux,
    source_project_openssl,

    source_project_error = 0xff,
};

enum cpu_extensions ENUM_SIZE {
    cpu_extensions_x86 = 0,
    cpu_extensions_sse3,
    cpu_extensions_sse4,
    cpu_extensions_avx,
    cpu_extensions_avx2,
    cpu_extensions_avx512,
    cpu_extensions_aesni,
    cpu_extensions_vaes,
    cpu_extensions_shani,
    cpu_extensions_as_compiled,
};

struct port_contention_victim {
    victim_fun run;
    enum source_project source;
    enum cpu_extensions extensions;
    const char *name;
};

struct crypto_aes_ctx {
    uint32_t key_enc[AES_MAX_KEYLENGTH_U32];
    uint32_t key_dec[AES_MAX_KEYLENGTH_U32];
    uint32_t key_length;
} __attribute__((aligned(32)));

union {
    uint64_t u64[2];
    __m128i u128;
} typedef aes_uint128 __attribute__((aligned(16)));

static const char *source_project_name(enum source_project project) {
    switch (project) {
        case source_project_custom:
            return "Custom";
        case source_project_linux:
            return "Linux";
        case source_project_openssl:
            return "OpenSSL";
        case source_project_error:
        default:
            ;
    }
    return "Error";
}

static unsigned char is_extension_supported(enum cpu_extensions extension) {
    unsigned int c1, a7, b7, c7, d7;

    asm volatile ("cpuid" : "=c" (c1) : "a" (1), "c"(0) : "rbx", "rdx");
    asm volatile ("cpuid" : "=a" (a7), "=b" (b7), "=c" (c7), "=d" (d7) : "a" (7), "c"(0));

    switch (extension) {
        case cpu_extensions_as_compiled:
        case cpu_extensions_x86:
            return 1;

        case cpu_extensions_sse3:
            return BIT_SET(c1, 0);
        case cpu_extensions_sse4:
            return BIT_SET(c1, 19) && BIT_SET(c1, 20);
        case cpu_extensions_avx:
            return BIT_SET(c1, 28);
        case cpu_extensions_aesni:
            return BIT_SET(c1, 25);
        case cpu_extensions_avx2:
            return BIT_SET(b7, 5);
        case cpu_extensions_avx512:
            return BIT_SET(b7, 16) && BIT_SET(b7, 17);
        case cpu_extensions_vaes:
            return BIT_SET(c7, 9);
        case cpu_extensions_shani:
            return BIT_SET(b7, 29);
        default: ;
    }

    return 0;
}

#define instantiate_victim(name, init_i, victim_invoke, block_type, block_num)  \
static void __attribute__((noreturn)) victim_fun_##name (void) {                \
    unsigned int i, num_dest;                                                   \
    void* context;                                                              \
    unsigned char __attribute__((aligned(32))) buf[32] = {1,0,};                \
    aes_uint128 icb = {.u64={0, 0}};                                            \
    aes_uint128 key = {.u64={0x1234567890abdef,0xcafebabecafebabe}};            \
    block_type x[block_num];                                                    \
    block_type y[block_num];                                                    \
    init_i;                                                                     \
                                                                                \
    *sync_page = 1;                                                             \
    for (;;) {                                                                  \
        i = 4;                                                                  \
        num_dest = 0;                                                           \
                                                                                \
        while (!*sync_page) { asm volatile("lfence");}                          \
        while(i--) {                                                            \
            asm volatile(".rept 512\nlfence\n.endr");                           \
            victim_invoke;                                                      \
            asm volatile(".rept 4096\nlfence\n.endr");                          \
        }                                                                       \
        *sync_page = 0;                                                         \
    }                                                                           \
}

// AES-NI
extern void aesni_ctr_enc(struct crypto_aes_ctx *ctx, uint8_t *out, const uint8_t *in, unsigned int len,
                                uint8_t *iv);

#define aesni_linux_init \
    __attribute__((aligned(0x10))) struct crypto_aes_ctx aes_ctx = {.key_length = sizeof(key)}; \
    memcpy(&aes_ctx.key_enc, &key, sizeof(key));                                                \
    memcpy(&aes_ctx.key_dec, &key, sizeof(key));
#define aesni_linux_invoke \
    aesni_ctr_enc(&aes_ctx, (void *) y, (void *) x, sizeof(x), (void *) &icb);
instantiate_victim(aesni_linux, aesni_linux_init, aesni_linux_invoke, aes_uint128, 0x100)
static struct port_contention_victim aesni_linux = {
    victim_fun_aesni_linux, source_project_linux, cpu_extensions_aesni, "AES-NI"
};

extern void aesni_ctr32_encrypt_blocks(const unsigned char *in, unsigned char *out, size_t blocks,
                                               const void *key, const unsigned char *ivec);

#define aesni_openssl_init ;
#define aesni_openssl_invoke aesni_ctr32_encrypt_blocks((void*)x, (void*)y, 0x100, &key, (void*)&icb);
instantiate_victim(aesni_openssl, aesni_openssl_init, aesni_openssl_invoke, aes_uint128, 0x100)
static struct port_contention_victim aesni_openssl = {
    victim_fun_aesni_openssl, source_project_openssl, cpu_extensions_aesni, "AES-NI"
};

static const struct port_contention_victim *victims[] = {
    &aesni_linux,
    &aesni_openssl,
};


#endif