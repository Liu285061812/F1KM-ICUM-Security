#include "icum_crypto.h"
#include "dr7f701649i.dvf.h"

#define AES_CTRL_CTX0          (0u << 16)
#define AES_CTRL_BYTE_ORDER    (1u << 9)
#define AES_CTRL_KEY_DEST      (1u << 24)
#define AES_CTRL_BCI_DEST      (0u << 24)
#define AES_CTRL_RST0          (1u << 0)
#define AES_CTRL_IDATDEST_MASK (3u << 24)

#define AES_DCTL_NEW_KEY       (1u << 0)
#define AES_DCTL_IVSEL         (1u << 2)
#define AES_DCTL_LAST          (1u << 15)
#define AES_DCTL_ATTR_EMPTY    (1u << 16)
#define AES_STR_BUSY           (1u << 0)
#define AES_CCTL_CMAC          8u

#define TRNG_SR_RRDY           (1u << 0)
#define TRNG_SR_RCHKERR        (1u << 2)
#define TRNG_SCLR_RRDY         (1u << 0)
#define TRNG_SCLR_RCHK          (1u << 1)
#define TRNG_CR_GE             (1u << 0)

static uint32 aes_pack_word(const unsigned char *data, unsigned int word_index)
{
    unsigned int offset;

    offset = word_index * 4u;
    return ((uint32)data[offset] << 24)
         | ((uint32)data[offset + 1u] << 16)
         | ((uint32)data[offset + 2u] << 8)
         | (uint32)data[offset + 3u];
}

static void aes_unpack_word(unsigned char *data,
                            unsigned int word_index,
                            uint32 word)
{
    unsigned int offset;

    offset = word_index * 4u;
    data[offset] = (unsigned char)(word >> 24);
    data[offset + 1u] = (unsigned char)(word >> 16);
    data[offset + 2u] = (unsigned char)(word >> 8);
    data[offset + 3u] = (unsigned char)word;
}

static int aes_wait_status(unsigned int busy, icum_u32 timeout)
{
    while (timeout != 0u) {
        if (((ICUM_AESB0STR & AES_STR_BUSY) != 0u) == (busy != 0u)) {
            return 1;
        }
        --timeout;
    }

    return (((ICUM_AESB0STR & AES_STR_BUSY) != 0u) == (busy != 0u));
}

int icum_aes128_ecb_crypt(const unsigned char *key,
                          const unsigned char *input,
                          unsigned char *output,
                          unsigned long length,
                          int decrypt,
                          icum_u32 timeout)
{
    unsigned long offset;
    unsigned long remaining;
    unsigned int word_index;
    uint32 control;
    uint32 context_control;

    if ((key == (const unsigned char *)0)
        || (input == (const unsigned char *)0)
        || (output == (unsigned char *)0)
        || (length == 0ul)
        || ((length & 15ul) != 0ul)
        || (timeout == 0u)) {
        return 0;
    }

    if (!aes_wait_status(0u, timeout)) {
        return 0;
    }

    ICUM_AESB0CTRL = AES_CTRL_RST0;
    while ((ICUM_AESB0CTRL & AES_CTRL_RST0) != 0u) {
        if (timeout == 0u) {
            return 0;
        }
        --timeout;
    }

    context_control = (decrypt != 0) ? (1u << 4) : 0u;
    ICUM_AESB0CCTL0 = context_control;

    control = AES_CTRL_CTX0 | AES_CTRL_BYTE_ORDER;
    ICUM_AESB0CTRL = control | AES_CTRL_KEY_DEST;
    for (word_index = 0u; word_index < 4u; ++word_index) {
        ICUM_AESB0IDAT = aes_pack_word(key, word_index);
    }

    ICUM_AESB0CTRL = control | AES_CTRL_BCI_DEST;
    offset = 0ul;
    remaining = length;
    while (remaining != 0ul) {
        ICUM_AESB0DCTL = (offset == 0ul) ? AES_DCTL_NEW_KEY : 0u;
        for (word_index = 0u; word_index < 4u; ++word_index) {
            ICUM_AESB0IDAT = aes_pack_word(input + offset, word_index);
        }

        if (!aes_wait_status(0u, timeout)) {
            ICUM_AESB0CTRL = AES_CTRL_RST0;
            return 0;
        }

        ICUM_AESB0CTRL = control;
        for (word_index = 0u; word_index < 4u; ++word_index) {
            aes_unpack_word(output + offset, word_index, ICUM_AESB0ODAT);
        }
        remaining -= 16ul;
        if (remaining != 0ul) {
            ICUM_AESB0CTRL = control | AES_CTRL_BCI_DEST;
            offset += 16ul;
        }
    }

    return 1;
}

static void xor_block(unsigned char *output,
                      const unsigned char *left,
                      const unsigned char *right)
{
    unsigned int index;

    for (index = 0u; index < 16u; ++index) {
        output[index] = left[index] ^ right[index];
    }
}

int icum_aes128_cbc_crypt(const unsigned char *key,
                          const unsigned char *iv,
                          const unsigned char *input,
                          unsigned char *output,
                          unsigned long length,
                          int decrypt,
                          icum_u32 timeout)
{
    unsigned long offset;
    unsigned int index;
    unsigned char chain[16];
    unsigned char block[16];
    unsigned char transformed[16];

    if ((key == (const unsigned char *)0)
        || (iv == (const unsigned char *)0)
        || (input == (const unsigned char *)0)
        || (output == (unsigned char *)0)
        || (length == 0ul)
        || ((length & 15ul) != 0ul)
        || (timeout == 0u)) {
        return 0;
    }

    for (index = 0u; index < 16u; ++index) {
        chain[index] = iv[index];
    }

    for (offset = 0ul; offset < length; offset += 16ul) {
        for (index = 0u; index < 16u; ++index) {
            block[index] = input[offset + index];
        }

        if (decrypt != 0) {
            if (!icum_aes128_ecb_crypt(key, block, transformed,
                                        16ul, 1, timeout)) {
                return 0;
            }
            xor_block(transformed, transformed, chain);
            for (index = 0u; index < 16u; ++index) {
                chain[index] = block[index];
                output[offset + index] = transformed[index];
            }
        } else {
            xor_block(transformed, block, chain);
            if (!icum_aes128_ecb_crypt(key, transformed, block,
                                       16ul, 0, timeout)) {
                return 0;
            }
            for (index = 0u; index < 16u; ++index) {
                chain[index] = block[index];
                output[offset + index] = block[index];
            }
        }
    }

    return 1;
}

int icum_aes128_cmac(const unsigned char *key,
                     const unsigned char *message,
                     unsigned long message_length,
                     unsigned char *tag,
                     icum_u32 timeout)
{
    unsigned long block_count;
    unsigned long block_index;
    unsigned long final_offset;
    unsigned int index;
    unsigned int final_bytes;
    uint32 control;
    uint32 data_control;
    unsigned char block[16];

    if ((key == (const unsigned char *)0)
        || ((message == (const unsigned char *)0) && (message_length != 0ul))
        || (tag == (unsigned char *)0)
        || (timeout == 0u)) {
        return 0;
    }

    if (!aes_wait_status(0u, timeout)) {
        return 0;
    }

    ICUM_AESB0CTRL = AES_CTRL_RST0;
    while ((ICUM_AESB0CTRL & AES_CTRL_RST0) != 0u) {
        if (timeout == 0u) {
            return 0;
        }
        --timeout;
    }

    ICUM_AESB0CCTL0 = AES_CCTL_CMAC;
    control = AES_CTRL_CTX0 | AES_CTRL_BYTE_ORDER;
    ICUM_AESB0CTRL = control | AES_CTRL_KEY_DEST;
    for (index = 0u; index < 4u; ++index) {
        ICUM_AESB0IDAT = aes_pack_word(key, index);
    }

    ICUM_AESB0CTRL = control | AES_CTRL_BCI_DEST;
    block_count = (message_length == 0ul)
                ? 1ul
                : (1ul + ((message_length - 1ul) / 16ul));
    final_offset = (block_count - 1ul) * 16ul;

    if (message_length == 0ul) {
        data_control = AES_DCTL_IVSEL | AES_DCTL_NEW_KEY
                     | AES_DCTL_LAST | AES_DCTL_ATTR_EMPTY;
        ICUM_AESB0DCTL = data_control;
        for (index = 0u; index < 16u; ++index) {
            block[index] = 0u;
        }
        for (index = 0u; index < 4u; ++index) {
            ICUM_AESB0IDAT = aes_pack_word(block, index);
        }
        if (!aes_wait_status(0u, timeout)) {
            ICUM_AESB0CTRL = AES_CTRL_RST0;
            return 0;
        }
    } else {
        for (block_index = 0ul; block_index < block_count; ++block_index) {
            final_bytes = (block_index + 1ul == block_count)
                        ? (unsigned int)(message_length - final_offset)
                        : 16u;
            data_control = (block_index == 0ul)
                         ? (AES_DCTL_IVSEL | AES_DCTL_NEW_KEY)
                         : 0u;
            if (block_index + 1ul == block_count) {
                data_control |= AES_DCTL_LAST;
                if (final_bytes != 16u) {
                    data_control |= (uint32)(final_bytes * 8u) << 8;
                }
            }
            ICUM_AESB0DCTL = data_control;

            for (index = 0u; index < 16u; ++index) {
                block[index] = (index < final_bytes)
                             ? message[block_index * 16ul + index]
                             : 0u;
            }
            for (index = 0u; index < 4u; ++index) {
                ICUM_AESB0IDAT = aes_pack_word(block, index);
            }

            if (!aes_wait_status(0u, timeout)) {
                ICUM_AESB0CTRL = AES_CTRL_RST0;
                return 0;
            }

            if (block_index + 1ul < block_count) {
                ICUM_AESB0CTRL = control;
                for (index = 0u; index < 4u; ++index) {
                    aes_unpack_word(block, index, ICUM_AESB0ODAT);
                }
                ICUM_AESB0CTRL = control | AES_CTRL_BCI_DEST;
            }
        }
    }

    ICUM_AESB0CTRL = control;
    for (index = 0u; index < 4u; ++index) {
        aes_unpack_word(tag, index, ICUM_AESB0ODAT);
    }

    return 1;
}

int icum_trng_read_words(icum_u32 *output,
                         unsigned long word_count,
                         icum_u32 timeout)
{
    unsigned long word_index;

    if ((output == (icum_u32 *)0)
        || (word_count == 0ul)
        || (timeout == 0u)) {
        return 0;
    }

    ICUM_TRNGC0CR = 0u;
    ICUM_TRNGC0SCLR = TRNG_SCLR_RRDY | TRNG_SCLR_RCHK;
    ICUM_TRNGC0CR = TRNG_CR_GE;

    for (word_index = 0ul; word_index < word_count; ++word_index) {
        while ((ICUM_TRNGC0SR & TRNG_SR_RRDY) == 0u) {
            if (timeout == 0u) {
                ICUM_TRNGC0CR = 0u;
                return 0;
            }
            --timeout;
        }

        output[word_index] = ICUM_TRNGC0RNRR;
        if ((ICUM_TRNGC0SR & TRNG_SR_RCHKERR) != 0u) {
            ICUM_TRNGC0CR = 0u;
            return 0;
        }
    }

    ICUM_TRNGC0CR = 0u;
    return 1;
}

static int bytes_equal(const unsigned char *left,
                       const unsigned char *right,
                       unsigned int length)
{
    unsigned int index;

    for (index = 0u; index < length; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }

    return 1;
}

icum_u32 icum_crypto_self_test(icum_u32 timeout)
{
    static const unsigned char key[16] = {
#if 0
        0x2bu, 0x7eu, 0x15u, 0x16u, 0x28u, 0xa6u, 0xd2u, 0xa6u,
        0xabu, 0xf7u, 0x15u, 0x88u, 0x09u, 0xcfu, 0x4fu, 0x3cu
#else
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u,
        0x08u, 0x09u, 0x0au, 0x0bu, 0x0cu, 0x0du, 0x0eu, 0x0fu
#endif
    };
    static const unsigned char plaintext[16] = {
#if 0
        0x6bu, 0xc0u, 0xbcu, 0xe1u, 0x2au, 0x45u, 0x99u, 0x91u,
        0xe1u, 0x34u, 0x74u, 0x1au, 0x7fu, 0x9eu, 0x19u, 0x25u
#else
        0x00u, 0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u, 0x77u,
        0x88u, 0x99u, 0xaau, 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu
#endif
        };
    static const unsigned char ciphertext_expected[16] = {
#if 0
        0x76u, 0x49u, 0xabu, 0xacu, 0x81u, 0x19u, 0xb2u, 0x46u,
        0xceu, 0xe9u, 0x8eu, 0x9bu, 0x12u, 0xe9u, 0x19u, 0x7du
#else
        0x69u, 0xc4u, 0xe0u, 0xd8u, 0x6au, 0x7bu, 0x04u, 0x30u,
        0xd8u, 0xcdu, 0xb7u, 0x80u, 0x70u, 0xb4u, 0xc5u, 0x5au
#endif
        };
    static const unsigned char cbc_key[16] = {
        0x2bu, 0x7eu, 0x15u, 0x16u, 0x28u, 0xaeu, 0xd2u, 0xa6u,
        0xabu, 0xf7u, 0x15u, 0x88u, 0x09u, 0xcfu, 0x4fu, 0x3cu
    };
    static const unsigned char cbc_iv[16] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u,
        0x08u, 0x09u, 0x0au, 0x0bu, 0x0cu, 0x0du, 0x0eu, 0x0fu
    };
    static const unsigned char cbc_plaintext[32] = {
        0x6bu, 0xc1u, 0xbeu, 0xe2u, 0x2eu, 0x40u, 0x9fu, 0x96u,
        0xe9u, 0x3du, 0x7eu, 0x11u, 0x73u, 0x93u, 0x17u, 0x2au,
        0xaeu, 0x2du, 0x8au, 0x57u, 0x1eu, 0x03u, 0xacu, 0x9cu,
        0x9eu, 0xb7u, 0x6fu, 0xacu, 0x45u, 0xafu, 0x8eu, 0x51u
    };
    static const unsigned char cbc_ciphertext_expected[32] = {
        0x76u, 0x49u, 0xabu, 0xacu, 0x81u, 0x19u, 0xb2u, 0x46u,
        0xceu, 0xe9u, 0x8eu, 0x9bu, 0x12u, 0xe9u, 0x19u, 0x7du,
        0x50u, 0x86u, 0xcbu, 0x9bu, 0x50u, 0x72u, 0x19u, 0xeeu,
        0x95u, 0xdbu, 0x11u, 0x3au, 0x91u, 0x76u, 0x78u, 0xb2u
        };
    static const unsigned char ecb_multiblock_expected[32] = {
        0x3au, 0xd7u, 0x7bu, 0xb4u, 0x0du, 0x7au, 0x36u, 0x60u,
        0xa8u, 0x9eu, 0xcau, 0xf3u, 0x24u, 0x66u, 0xefu, 0x97u,
        0xf5u, 0xd3u, 0xd5u, 0x85u, 0x03u, 0xb9u, 0x69u, 0x9du,
        0xe7u, 0x85u, 0x89u, 0x5au, 0x96u, 0xfdu, 0xbau, 0xafu
    };
    static const unsigned char cmac_empty_expected[16] = {
        0xbbu, 0x1du, 0x69u, 0x29u, 0xe9u, 0x59u, 0x37u, 0x28u,
        0x7fu, 0xa3u, 0x7du, 0x12u, 0x9bu, 0x75u, 0x67u, 0x46u
    };
    static const unsigned char cmac_full_expected[16] = {
        0x07u, 0x0au, 0x16u, 0xb4u, 0x6bu, 0x4du, 0x41u, 0x44u,
        0xf7u, 0x9bu, 0xddu, 0x9du, 0xd0u, 0x4au, 0x28u, 0x7cu
    };
    static const unsigned char cmac_multiblock_message[64] = {
        0x6bu, 0xc1u, 0xbeu, 0xe2u, 0x2eu, 0x40u, 0x9fu, 0x96u,
        0xe9u, 0x3du, 0x7eu, 0x11u, 0x73u, 0x93u, 0x17u, 0x2au,
        0xaeu, 0x2du, 0x8au, 0x57u, 0x1eu, 0x03u, 0xacu, 0x9cu,
        0x9eu, 0xb7u, 0x6fu, 0xacu, 0x45u, 0xafu, 0x8eu, 0x51u,
        0x30u, 0xc8u, 0x1cu, 0x46u, 0xa3u, 0x5cu, 0xe4u, 0x11u,
        0xe5u, 0xfbu, 0xc1u, 0x19u, 0x1au, 0x0au, 0x52u, 0xefu,
        0xf6u, 0x9fu, 0x24u, 0x45u, 0xdfu, 0x4fu, 0x9bu, 0x17u,
        0xadu, 0x2bu, 0x41u, 0x7bu, 0xe6u, 0x6cu, 0x37u, 0x10u
    };
    static const unsigned char cmac_multiblock_expected[16] = {
        0x51u, 0xf0u, 0xbeu, 0xbfu, 0x7eu, 0x3bu, 0x9du, 0x92u,
        0xfcu, 0x49u, 0x74u, 0x17u, 0x79u, 0x36u, 0x3cu, 0xfeu
    };
    static const unsigned char cmac_partial_message[40] = {
        0x6bu, 0xc1u, 0xbeu, 0xe2u, 0x2eu, 0x40u, 0x9fu, 0x96u,
        0xe9u, 0x3du, 0x7eu, 0x11u, 0x73u, 0x93u, 0x17u, 0x2au,
        0xaeu, 0x2du, 0x8au, 0x57u, 0x1eu, 0x03u, 0xacu, 0x9cu,
        0x9eu, 0xb7u, 0x6fu, 0xacu, 0x45u, 0xafu, 0x8eu, 0x51u,
        0x30u, 0xc8u, 0x1cu, 0x46u, 0xa3u, 0x5cu, 0xe4u, 0x11u
    };
    static const unsigned char cmac_partial_expected[16] = {
        0xdfu, 0xa6u, 0x67u, 0x47u, 0xdeu, 0x9au, 0xe6u, 0x30u,
        0x30u, 0xcau, 0x32u, 0x61u, 0x14u, 0x97u, 0xc8u, 0x27u
    };
    unsigned char ciphertext[16];
    unsigned char ecb_multiblock_ciphertext[32];
    unsigned char plaintext_result[16];
    unsigned char cbc_ciphertext[32];
    unsigned char cbc_plaintext_result[32];
    unsigned char cmac_result[16];
    icum_u32 random_words[4];
    icum_u32 result;

    result = 0u;
    if (icum_aes128_ecb_crypt(key, plaintext, ciphertext, 16ul, 0, timeout)
        && bytes_equal(ciphertext, ciphertext_expected, 16u)) {
        result |= ICUM_CRYPTO_TEST_AES_ENCRYPT_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_AES_ENCRYPT_FAIL;
    }

    if (icum_aes128_ecb_crypt(key,
                              ciphertext_expected,
                              plaintext_result,
                              16ul,
                              1,
                              timeout)
        && bytes_equal(plaintext_result, plaintext, 16u)) {
        result |= ICUM_CRYPTO_TEST_AES_DECRYPT_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_AES_DECRYPT_FAIL;
    }

    if (icum_aes128_ecb_crypt(cbc_key, cbc_plaintext,
                              ecb_multiblock_ciphertext, 32ul, 0, timeout)
        && bytes_equal(ecb_multiblock_ciphertext,
                       ecb_multiblock_expected, 32u)) {
        result |= ICUM_CRYPTO_TEST_AES_MULTIBLOCK_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_AES_MULTIBLOCK_FAIL;
    }

    if (icum_aes128_cbc_crypt(cbc_key, cbc_iv, cbc_plaintext,
                              cbc_ciphertext, 32ul, 0, timeout)
        && bytes_equal(cbc_ciphertext, cbc_ciphertext_expected, 32u)) {
        result |= ICUM_CRYPTO_TEST_CBC_ENCRYPT_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_CBC_ENCRYPT_FAIL;
    }

    if (icum_aes128_cbc_crypt(cbc_key, cbc_iv, cbc_ciphertext_expected,
                              cbc_plaintext_result, 32ul, 1, timeout)
        && bytes_equal(cbc_plaintext_result, cbc_plaintext, 32u)) {
        result |= ICUM_CRYPTO_TEST_CBC_DECRYPT_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_CBC_DECRYPT_FAIL;
    }

    if (icum_aes128_cmac(cbc_key, (const unsigned char *)0, 0ul,
                         cmac_result, timeout)
        && bytes_equal(cmac_result, cmac_empty_expected, 16u)) {
        result |= ICUM_CRYPTO_TEST_CMAC_EMPTY_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_CMAC_EMPTY_FAIL;
    }

    if (icum_aes128_cmac(cbc_key, cbc_plaintext, 16ul,
                         cmac_result, timeout)
        && bytes_equal(cmac_result, cmac_full_expected, 16u)) {
        result |= ICUM_CRYPTO_TEST_CMAC_FULL_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_CMAC_FULL_FAIL;
    }

    if (icum_aes128_cmac(cbc_key, cmac_multiblock_message, 64ul,
                         cmac_result, timeout)
        && bytes_equal(cmac_result, cmac_multiblock_expected, 16u)) {
        result |= ICUM_CRYPTO_TEST_CMAC_MULTIBLOCK_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_CMAC_MULTIBLOCK_FAIL;
    }

    if (icum_aes128_cmac(cbc_key, cmac_partial_message, 40ul,
                         cmac_result, timeout)
        && bytes_equal(cmac_result, cmac_partial_expected, 16u)) {
        result |= ICUM_CRYPTO_TEST_CMAC_PARTIAL_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_CMAC_PARTIAL_FAIL;
    }

    if (icum_trng_read_words(random_words, 4ul, timeout)
        && ((random_words[0] | random_words[1]
             | random_words[2] | random_words[3]) != 0u)) {
        result |= ICUM_CRYPTO_TEST_TRNG_PASS;
    } else {
        result |= ICUM_CRYPTO_TEST_TRNG_FAIL;
    }

    return result;
}