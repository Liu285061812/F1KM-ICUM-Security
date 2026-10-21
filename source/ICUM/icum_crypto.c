#include "icum_crypto.h"
#include "dr7f701649i.dvf.h"

#define TRNG_SR_RRDY           (1u << 0)
#define TRNG_SR_RCHKERR        (1u << 2)
#define TRNG_SCLR_RRDY         (1u << 0)
#define TRNG_SCLR_RCHK          (1u << 1)
#define TRNG_CR_GE             (1u << 0)

static unsigned char aes_gf_multiply(unsigned char left, unsigned char right)
{
    unsigned char result;
    unsigned int bit;
    unsigned char mask;

    result = 0u;
    for (bit = 0u; bit < 8u; ++bit) {
        mask = (unsigned char)(0u - (right & 1u));
        result ^= left & mask;
        mask = (unsigned char)(0u - (left >> 7));
        left = (unsigned char)((left << 1) ^ (0x1bu & mask));
        right >>= 1;
    }
    return result;
}

static unsigned char aes_gf_inverse(unsigned char value)
{
    unsigned char result;
    unsigned char base;
    unsigned int exponent;

    result = 1u;
    base = value;
    exponent = 254u;
    while (exponent != 0u) {
        if ((exponent & 1u) != 0u) result = aes_gf_multiply(result, base);
        base = aes_gf_multiply(base, base);
        exponent >>= 1;
    }
    return result;
}

static unsigned char aes_rotate_left(unsigned char value, unsigned int count)
{
    return (unsigned char)((value << count) | (value >> (8u - count)));
}

static unsigned char aes_sub_byte(unsigned char value)
{
    unsigned char inverse;

    inverse = aes_gf_inverse(value);
    return inverse ^ aes_rotate_left(inverse, 1u)
                  ^ aes_rotate_left(inverse, 2u)
                  ^ aes_rotate_left(inverse, 3u)
                  ^ aes_rotate_left(inverse, 4u) ^ 0x63u;
}

static unsigned char aes_inverse_sub_byte(unsigned char value)
{
    unsigned char inverse_affine;

    inverse_affine = aes_rotate_left(value, 1u)
                   ^ aes_rotate_left(value, 3u)
                   ^ aes_rotate_left(value, 6u) ^ 0x05u;
    return aes_gf_inverse(inverse_affine);
}

static void aes_expand_key(const unsigned char *key, unsigned char round_key[176])
{
    unsigned int generated;
    unsigned int index;
    unsigned char temporary[4];
    unsigned char round_constant;

    for (index = 0u; index < 16u; ++index) round_key[index] = key[index];
    generated = 16u;
    round_constant = 1u;
    while (generated < 176u) {
        for (index = 0u; index < 4u; ++index) {
            temporary[index] = round_key[generated - 4u + index];
        }
        if ((generated & 15u) == 0u) {
            unsigned char first;
            first = temporary[0];
            temporary[0] = aes_sub_byte(temporary[1]) ^ round_constant;
            temporary[1] = aes_sub_byte(temporary[2]);
            temporary[2] = aes_sub_byte(temporary[3]);
            temporary[3] = aes_sub_byte(first);
            round_constant = aes_gf_multiply(round_constant, 2u);
        }
        for (index = 0u; index < 4u; ++index) {
            round_key[generated] = round_key[generated - 16u] ^ temporary[index];
            ++generated;
        }
    }
}

static void aes_add_round_key(unsigned char state[16],
                              const unsigned char *round_key)
{
    unsigned int index;
    for (index = 0u; index < 16u; ++index) state[index] ^= round_key[index];
}

static void aes_shift_rows(unsigned char state[16], int inverse)
{
    unsigned char shifted[16];
    unsigned int row, column, source_column;

    for (row = 0u; row < 4u; ++row) {
        for (column = 0u; column < 4u; ++column) {
            source_column = (inverse != 0)
                          ? ((column + 4u - row) & 3u)
                          : ((column + row) & 3u);
            shifted[row + 4u * column] = state[row + 4u * source_column];
        }
    }
    for (row = 0u; row < 16u; ++row) state[row] = shifted[row];
}

static void aes_mix_columns(unsigned char state[16], int inverse)
{
    unsigned int column, offset;
    unsigned char a, b, c, d;

    for (column = 0u; column < 4u; ++column) {
        offset = column * 4u;
        a = state[offset];
        b = state[offset + 1u];
        c = state[offset + 2u];
        d = state[offset + 3u];
        if (inverse != 0) {
            state[offset] = aes_gf_multiply(a, 14u) ^ aes_gf_multiply(b, 11u)
                          ^ aes_gf_multiply(c, 13u) ^ aes_gf_multiply(d, 9u);
            state[offset + 1u] = aes_gf_multiply(a, 9u) ^ aes_gf_multiply(b, 14u)
                              ^ aes_gf_multiply(c, 11u) ^ aes_gf_multiply(d, 13u);
            state[offset + 2u] = aes_gf_multiply(a, 13u) ^ aes_gf_multiply(b, 9u)
                              ^ aes_gf_multiply(c, 14u) ^ aes_gf_multiply(d, 11u);
            state[offset + 3u] = aes_gf_multiply(a, 11u) ^ aes_gf_multiply(b, 13u)
                              ^ aes_gf_multiply(c, 9u) ^ aes_gf_multiply(d, 14u);
        } else {
            state[offset] = aes_gf_multiply(a, 2u) ^ aes_gf_multiply(b, 3u) ^ c ^ d;
            state[offset + 1u] = a ^ aes_gf_multiply(b, 2u)
                               ^ aes_gf_multiply(c, 3u) ^ d;
            state[offset + 2u] = a ^ b ^ aes_gf_multiply(c, 2u)
                               ^ aes_gf_multiply(d, 3u);
            state[offset + 3u] = aes_gf_multiply(a, 3u) ^ b ^ c
                               ^ aes_gf_multiply(d, 2u);
        }
    }
}

static void aes_crypt_block(const unsigned char *key,
                            const unsigned char input[16],
                            unsigned char output[16],
                            int decrypt)
{
    unsigned char state[16];
    unsigned char round_key[176];
    unsigned int index, round;

    aes_expand_key(key, round_key);
    for (index = 0u; index < 16u; ++index) state[index] = input[index];
    if (decrypt == 0) {
        aes_add_round_key(state, round_key);
        for (round = 1u; round < 10u; ++round) {
            for (index = 0u; index < 16u; ++index) state[index] = aes_sub_byte(state[index]);
            aes_shift_rows(state, 0);
            aes_mix_columns(state, 0);
            aes_add_round_key(state, round_key + round * 16u);
        }
        for (index = 0u; index < 16u; ++index) state[index] = aes_sub_byte(state[index]);
        aes_shift_rows(state, 0);
        aes_add_round_key(state, round_key + 160u);
    } else {
        aes_add_round_key(state, round_key + 160u);
        for (round = 9u; round > 0u; --round) {
            aes_shift_rows(state, 1);
            for (index = 0u; index < 16u; ++index) {
                state[index] = aes_inverse_sub_byte(state[index]);
            }
            aes_add_round_key(state, round_key + round * 16u);
            aes_mix_columns(state, 1);
        }
        aes_shift_rows(state, 1);
        for (index = 0u; index < 16u; ++index) {
            state[index] = aes_inverse_sub_byte(state[index]);
        }
        aes_add_round_key(state, round_key);
    }
    for (index = 0u; index < 16u; ++index) output[index] = state[index];
}

int icum_aes128_ecb_crypt(const unsigned char *key,
                          const unsigned char *input,
                          unsigned char *output,
                          unsigned long length,
                          int decrypt,
                          icum_u32 timeout)
{
    unsigned long offset;

    if ((key == (const unsigned char *)0)
        || (input == (const unsigned char *)0)
        || (output == (unsigned char *)0)
        || (length == 0ul)
        || ((length & 15ul) != 0ul)) return 0;

    (void)timeout;
    for (offset = 0ul; offset < length; offset += 16ul) {
        aes_crypt_block(key, input + offset, output + offset, decrypt);
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
        || ((length & 15ul) != 0ul)) {
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
    unsigned long block_index, offset, remaining;
    unsigned int index;
    unsigned char chaining[16];
    unsigned char block[16];
    unsigned char subkey1[16];
    unsigned char subkey2[16];
    unsigned char zero[16];
    unsigned char reduction;

    if ((key == (const unsigned char *)0)
        || ((message == (const unsigned char *)0) && (message_length != 0ul))
        || (tag == (unsigned char *)0)) return 0;

    (void)timeout;
    for (index = 0u; index < 16u; ++index) zero[index] = 0u;
    aes_crypt_block(key, zero, subkey1, 0);
    reduction = (unsigned char)(0x87u & (0u - (subkey1[0] >> 7)));
    for (index = 0u; index < 15u; ++index) {
        subkey1[index] = (unsigned char)((subkey1[index] << 1)
                       | (subkey1[index + 1u] >> 7));
    }
    subkey1[15] = (unsigned char)((subkey1[15] << 1) ^ reduction);
    reduction = (unsigned char)(0x87u & (0u - (subkey1[0] >> 7)));
    for (index = 0u; index < 15u; ++index) {
        subkey2[index] = (unsigned char)((subkey1[index] << 1)
                       | (subkey1[index + 1u] >> 7));
    }
    subkey2[15] = (unsigned char)((subkey1[15] << 1) ^ reduction);

    for (index = 0u; index < 16u; ++index) chaining[index] = 0u;
    block_count = (message_length == 0ul)
                ? 1ul
                : 1ul + ((message_length - 1ul) / 16ul);
    for (block_index = 0ul; block_index + 1ul < block_count; ++block_index) {
        offset = block_index * 16ul;
        for (index = 0u; index < 16u; ++index) {
            block[index] = chaining[index] ^ message[offset + index];
        }
        aes_crypt_block(key, block, chaining, 0);
    }

    offset = (block_count - 1ul) * 16ul;
    remaining = message_length - offset;
    for (index = 0u; index < 16u; ++index) {
        block[index] = 0u;
    }
    if ((message_length != 0ul) && (remaining == 16ul)) {
        for (index = 0u; index < 16u; ++index) {
            block[index] = message[offset + index] ^ subkey1[index];
        }
    } else {
        for (index = 0u; index < (unsigned int)remaining; ++index) {
            block[index] = message[offset + index];
        }
        block[remaining] = 0x80u;
        for (index = 0u; index < 16u; ++index) block[index] ^= subkey2[index];
    }
    for (index = 0u; index < 16u; ++index) {
        block[index] ^= chaining[index];
    }
    aes_crypt_block(key, block, tag, 0);
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
