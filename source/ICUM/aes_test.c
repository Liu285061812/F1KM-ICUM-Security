#include "aes_test.h"

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