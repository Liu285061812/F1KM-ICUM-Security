#ifndef ICUM_CRYPTO_H
#define ICUM_CRYPTO_H

typedef unsigned long icum_u32;

/* AES is software-only; timeout is retained for API compatibility and ignored. */
/* ECB/CBC length must be a non-zero multiple of the 16-byte AES block size. */
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

#endif