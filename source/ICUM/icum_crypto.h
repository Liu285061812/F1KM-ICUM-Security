#ifndef ICUM_CRYPTO_H
#define ICUM_CRYPTO_H

typedef unsigned long icum_u32;

#define ICUM_CRYPTO_TEST_AES_ENCRYPT_PASS 0x00000001ul
#define ICUM_CRYPTO_TEST_AES_DECRYPT_PASS 0x00000002ul
#define ICUM_CRYPTO_TEST_TRNG_PASS        0x00000004ul
#define ICUM_CRYPTO_TEST_CBC_ENCRYPT_PASS 0x00000008ul
#define ICUM_CRYPTO_TEST_CBC_DECRYPT_PASS 0x00000010ul
#define ICUM_CRYPTO_TEST_CMAC_EMPTY_PASS  0x00000020ul
#define ICUM_CRYPTO_TEST_CMAC_PARTIAL_PASS 0x00000040ul
#define ICUM_CRYPTO_TEST_CMAC_FULL_PASS   0x00000080ul
#define ICUM_CRYPTO_TEST_CMAC_MULTIBLOCK_PASS 0x00010000ul
#define ICUM_CRYPTO_TEST_AES_MULTIBLOCK_PASS 0x00040000ul
#define ICUM_CRYPTO_TEST_AES_ENCRYPT_FAIL 0x00000100ul
#define ICUM_CRYPTO_TEST_AES_DECRYPT_FAIL 0x00000200ul
#define ICUM_CRYPTO_TEST_TRNG_FAIL        0x00000400ul
#define ICUM_CRYPTO_TEST_CBC_ENCRYPT_FAIL 0x00000800ul
#define ICUM_CRYPTO_TEST_CBC_DECRYPT_FAIL 0x00001000ul
#define ICUM_CRYPTO_TEST_CMAC_EMPTY_FAIL  0x00002000ul
#define ICUM_CRYPTO_TEST_CMAC_PARTIAL_FAIL 0x00004000ul
#define ICUM_CRYPTO_TEST_CMAC_FULL_FAIL   0x00008000ul
#define ICUM_CRYPTO_TEST_CMAC_MULTIBLOCK_FAIL 0x00020000ul
#define ICUM_CRYPTO_TEST_AES_MULTIBLOCK_FAIL 0x00080000ul

/* ECB length must be a non-zero multiple of the 16-byte AES block size. */
int icum_aes128_ecb_crypt(const unsigned char *key,
                          const unsigned char *input,
                          unsigned char *output,
                          unsigned long length,
                          int decrypt,
                          icum_u32 timeout);
int icum_aes128_cbc_crypt(const unsigned char *key,
                          const unsigned char *iv,
                          const unsigned char *input,
                          unsigned char *output,
                          unsigned long length,
                          int decrypt,
                          icum_u32 timeout);
int icum_aes128_cmac(const unsigned char *key,
                     const unsigned char *message,
                     unsigned long message_length,
                     unsigned char *tag,
                     icum_u32 timeout);
int icum_trng_read_words(icum_u32 *output,
                         unsigned long word_count,
                         icum_u32 timeout);
icum_u32 icum_crypto_self_test(icum_u32 timeout);

#endif