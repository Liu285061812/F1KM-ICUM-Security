#include "icum_rsa.h"
#include "icum_crypto.h"
#include "device.h"

typedef unsigned long long rsa_u64;

#define RSA_LIMBS (ICUM_RSA_MAX_BYTES / 4ul)

typedef struct
{
    uint32 word[RSA_LIMBS];
} rsa_number;

static void rsa_zero(rsa_number *value)
{
    unsigned int i;
    for (i = 0u; i < RSA_LIMBS; ++i) value->word[i] = 0u;
}

static void rsa_from_bytes(rsa_number *value,
                           const unsigned char *bytes,
                           unsigned long length)
{
    unsigned long i;
    rsa_zero(value);
    for (i = 0ul; i < length; ++i) {
        unsigned long reverse;
        reverse = length - 1ul - i;
        value->word[i / 4ul] |= (uint32)bytes[reverse] << ((i & 3ul) * 8ul);
    }
}

static void rsa_to_bytes(const rsa_number *value,
                         unsigned char *bytes,
                         unsigned long length)
{
    unsigned long i;
    for (i = 0ul; i < length; ++i) {
        bytes[length - 1ul - i] =
            (unsigned char)(value->word[i / 4ul] >> ((i & 3ul) * 8ul));
    }
}

static int rsa_compare(const rsa_number *left,
                       const rsa_number *right,
                       unsigned int limbs)
{
    unsigned int i;
    for (i = limbs; i != 0u; --i) {
        if (left->word[i - 1u] > right->word[i - 1u]) return 1;
        if (left->word[i - 1u] < right->word[i - 1u]) return -1;
    }
    return 0;
}

static void rsa_subtract(rsa_number *left,
                         const rsa_number *right,
                         unsigned int limbs)
{
    unsigned int i;
    uint32 borrow;
    rsa_u64 subtrahend;
    rsa_u64 current;

    borrow = 0u;
    for (i = 0u; i < limbs; ++i) {
        subtrahend = (rsa_u64)right->word[i] + borrow;
        current = left->word[i];
        left->word[i] = (uint32)(current - subtrahend);
        borrow = (current < subtrahend) ? 1u : 0u;
    }
}

static void rsa_mod_double(rsa_number *value,
                           const rsa_number *modulus,
                           unsigned int limbs)
{
    rsa_number distance;
    uint32 carry;
    unsigned int i;

    distance = *modulus;
    rsa_subtract(&distance, value, limbs);
    if (rsa_compare(value, &distance, limbs) >= 0) {
        rsa_subtract(value, &distance, limbs);
        return;
    }
    carry = 0u;
    for (i = 0u; i < limbs; ++i) {
        uint32 next;
        next = value->word[i] >> 31;
        value->word[i] = (value->word[i] << 1) | carry;
        carry = next;
    }
}

static uint32 rsa_montgomery_inverse(uint32 value)
{
    uint32 inverse;
    unsigned int i;
    inverse = 1u;
    for (i = 0u; i < 5u; ++i) {
        inverse *= 2u - value * inverse;
    }
    return 0u - inverse;
}

static void rsa_montgomery_multiply(rsa_number *result,
                                    const rsa_number *left,
                                    const rsa_number *right,
                                    const rsa_number *modulus,
                                    unsigned int limbs,
                                    uint32 n0_inverse)
{
    uint32 temporary[RSA_LIMBS + 2u];
    uint32 reduced[RSA_LIMBS];
    rsa_u64 accumulator;
    uint32 carry, multiplier, borrow, reduce, select_mask;
    unsigned int i, j;

    for (i = 0u; i < limbs + 2u; ++i) temporary[i] = 0u;
    for (i = 0u; i < limbs; ++i) {
        carry = 0u;
        for (j = 0u; j < limbs; ++j) {
            accumulator = (rsa_u64)left->word[j] * right->word[i]
                        + temporary[j] + carry;
            temporary[j] = (uint32)accumulator;
            carry = (uint32)(accumulator >> 32);
        }
        accumulator = (rsa_u64)temporary[limbs] + carry;
        temporary[limbs] = (uint32)accumulator;
        temporary[limbs + 1u] += (uint32)(accumulator >> 32);

        multiplier = temporary[0] * n0_inverse;
        carry = 0u;
        for (j = 0u; j < limbs; ++j) {
            accumulator = (rsa_u64)multiplier * modulus->word[j]
                        + temporary[j] + carry;
            if (j != 0u) temporary[j - 1u] = (uint32)accumulator;
            carry = (uint32)(accumulator >> 32);
        }
        accumulator = (rsa_u64)temporary[limbs] + carry;
        temporary[limbs - 1u] = (uint32)accumulator;
        temporary[limbs] = temporary[limbs + 1u]
                         + (uint32)(accumulator >> 32);
        temporary[limbs + 1u] = 0u;
    }

    borrow = 0u;
    for (i = 0u; i < limbs; ++i) {
        rsa_u64 subtrahend;
        subtrahend = (rsa_u64)modulus->word[i] + borrow;
        reduced[i] = (uint32)((rsa_u64)temporary[i] - subtrahend);
        borrow = ((rsa_u64)temporary[i] < subtrahend) ? 1u : 0u;
    }
    reduce = (temporary[limbs] != 0u) | (borrow ^ 1u);
    select_mask = 0u - reduce;
    for (i = 0u; i < limbs; ++i) {
        result->word[i] = (reduced[i] & select_mask)
                        | (temporary[i] & ~select_mask);
    }
}

static unsigned int rsa_modulus_bits(const unsigned char *modulus,
                                     unsigned long length)
{
    unsigned int bits;
    unsigned char top;
    unsigned int i;
    bits = (unsigned int)(length * 8ul);
    top = modulus[0];
    for (i = 0u; i < 8u; ++i) {
        if ((top & (0x80u >> i)) != 0u) return bits - i;
    }
    return 0u;
}

static int rsa_modexp(const unsigned char *modulus_bytes,
                      unsigned long modulus_length,
                      const unsigned char *exponent_bytes,
                      unsigned long exponent_length,
                      const unsigned char *input_bytes,
                      unsigned char *output_bytes,
                      int private_operation)
{
    rsa_number modulus, base, r, r2, one, result, factor, temporary;
    unsigned int limbs, exponent_bits, bit, i;
    unsigned long exponent_offset;
    unsigned int exponent_bit;
    uint32 n0_inverse, select_mask;

    if ((modulus_length < ICUM_RSA_MIN_BYTES)
        || (modulus_length > ICUM_RSA_MAX_BYTES)
        || ((modulus_length & 3ul) != 0ul)
        || (modulus_bytes[0] == 0u)
        || ((modulus_bytes[modulus_length - 1ul] & 1u) == 0u)
        || (exponent_length == 0ul)) {
        return 0;
    }
    limbs = (unsigned int)(modulus_length / 4ul);
    rsa_from_bytes(&modulus, modulus_bytes, modulus_length);
    rsa_from_bytes(&base, input_bytes, modulus_length);
    if (rsa_compare(&base, &modulus, limbs) >= 0) return 0;
    if ((modulus.word[0] & 1u) == 0u) return 0;
    n0_inverse = rsa_montgomery_inverse(modulus.word[0]);

    rsa_zero(&r);
    r.word[0] = 1u;
    for (i = 0u; i < limbs * 32u; ++i) rsa_mod_double(&r, &modulus, limbs);
    r2 = r;
    for (i = 0u; i < limbs * 32u; ++i) rsa_mod_double(&r2, &modulus, limbs);

    rsa_montgomery_multiply(&factor, &base, &r2, &modulus, limbs, n0_inverse);
    result = r;
    exponent_bits = (unsigned int)(exponent_length * 8ul);
    if (private_operation == 0) {
        while ((exponent_bits > 1u) && (exponent_bytes[0] == 0u)) {
            ++exponent_bytes;
            --exponent_length;
            exponent_bits -= 8u;
        }
    }
    for (bit = 0u; bit < exponent_bits; ++bit) {
        exponent_offset = (unsigned long)(bit / 8u);
        exponent_bit = (unsigned int)(7u - (bit & 7u));
        rsa_montgomery_multiply(&temporary, &result, &result,
                                &modulus, limbs, n0_inverse);
        if (private_operation != 0) {
            rsa_montgomery_multiply(&r2, &temporary, &factor,
                                    &modulus, limbs, n0_inverse);
            select_mask = 0u - (uint32)
                ((exponent_bytes[exponent_offset] & (1u << exponent_bit)) != 0u);
            for (i = 0u; i < limbs; ++i) {
                result.word[i] = (r2.word[i] & select_mask)
                               | (temporary.word[i] & ~select_mask);
            }
        } else {
            result = temporary;
        }
        if ((private_operation == 0)
            && ((exponent_bytes[exponent_offset] & (1u << exponent_bit)) != 0u)) {
            rsa_montgomery_multiply(&temporary, &result, &factor,
                                    &modulus, limbs, n0_inverse);
            result = temporary;
        }
    }
    rsa_zero(&one);
    one.word[0] = 1u;
    rsa_montgomery_multiply(&temporary, &result, &one,
                            &modulus, limbs, n0_inverse);
    rsa_to_bytes(&temporary, output_bytes, modulus_length);
    return 1;
}

static int rsa_key_valid(const unsigned char *modulus,
                         unsigned long modulus_length)
{
    return (modulus != (const unsigned char *)0)
        && (modulus_length >= ICUM_RSA_MIN_BYTES)
        && (modulus_length <= ICUM_RSA_MAX_BYTES)
        && ((modulus_length & 3ul) == 0ul)
        && ((modulus[0] & 0x80u) != 0u)
        && ((modulus[modulus_length - 1ul] & 1u) != 0u);
}

static unsigned long rsa_exponent_bytes(unsigned long exponent,
                                        unsigned char bytes[4])
{
    unsigned long length;
    bytes[0] = (unsigned char)(exponent >> 24);
    bytes[1] = (unsigned char)(exponent >> 16);
    bytes[2] = (unsigned char)(exponent >> 8);
    bytes[3] = (unsigned char)exponent;
    length = 4ul;
    while ((length > 1ul) && (bytes[4ul - length] == 0u)) --length;
    return length;
}

static int rsa_public_operation(const icum_rsa_public_key *key,
                               const unsigned char *input,
                               unsigned char *output)
{
    unsigned char exponent[4];
    unsigned long exponent_length;
    if ((key == (const icum_rsa_public_key *)0)
        || (input == (const unsigned char *)0)
        || (output == (unsigned char *)0)
        || !rsa_key_valid(key->modulus, key->modulus_length)
        || (key->public_exponent < 3ul)
        || ((key->public_exponent & 1ul) == 0ul)) return 0;
    exponent_length = rsa_exponent_bytes(key->public_exponent, exponent);
    return rsa_modexp(key->modulus, key->modulus_length,
                      exponent + 4ul - exponent_length, exponent_length,
                      input, output, 0);
}

static int rsa_private_operation(const icum_rsa_private_key *key,
                                 const unsigned char *input,
                                 unsigned char *output)
{
    if ((key == (const icum_rsa_private_key *)0)
        || (input == (const unsigned char *)0)
        || (output == (unsigned char *)0)
        || (key->private_exponent == (const unsigned char *)0)
        || !rsa_key_valid(key->modulus, key->modulus_length)) return 0;
    return rsa_modexp(key->modulus, key->modulus_length,
                      key->private_exponent, key->modulus_length,
                      input, output, 1);
}

static int rsa_random_bytes(unsigned char *output,
                            unsigned long length,
                            unsigned long timeout)
{
    unsigned long words[ICUM_RSA_MAX_BYTES / 4ul];
    unsigned long word_count, i;
    uint32 value;

    if ((output == (unsigned char *)0) || (length > ICUM_RSA_MAX_BYTES)) return 0;
    if (length == 0ul) return 1;
    word_count = (length + 3ul) / 4ul;
    if (!icum_trng_read_words(words, word_count, (icum_u32)timeout)) return 0;
    for (i = 0ul; i < length; ++i) {
        value = (uint32)words[i / 4ul];
        output[i] = (unsigned char)(value >> ((3ul - (i & 3ul)) * 8ul));
    }
    return 1;
}

static int rsa_random_nonzero(unsigned char *output,
                              unsigned long length,
                              unsigned long timeout)
{
    unsigned long i, attempts;
    uint32 word;

    if (!rsa_random_bytes(output, length, timeout)) return 0;
    for (i = 0ul; i < length; ++i) {
        attempts = 0ul;
        while (output[i] == 0u) {
            if ((++attempts > 32ul)
                || !icum_trng_read_words(&word, 1ul, (icum_u32)timeout)) return 0;
            output[i] = (unsigned char)word;
            if (output[i] == 0u) output[i] = (unsigned char)(word >> 8);
            if (output[i] == 0u) output[i] = (unsigned char)(word >> 16);
            if (output[i] == 0u) output[i] = (unsigned char)(word >> 24);
        }
    }
    return 1;
}

int icum_rsa_encrypt_pkcs1_v15(const icum_rsa_public_key *key,
                               const unsigned char *message,
                               unsigned long message_length,
                               unsigned char *ciphertext,
                               unsigned long timeout)
{
    unsigned char encoded[ICUM_RSA_MAX_BYTES];
    unsigned long length, padding_length, i;

    if ((key == (const icum_rsa_public_key *)0)
        || !rsa_key_valid(key->modulus, key->modulus_length)
        || ((message == (const unsigned char *)0) && (message_length != 0ul))
        || (ciphertext == (unsigned char *)0) || (timeout == 0ul)) return 0;
    length = key->modulus_length;
    if ((message_length > length - 11ul)
        || (length > sizeof(encoded))) return 0;
    padding_length = length - message_length - 3ul;
    encoded[0] = 0u; encoded[1] = 2u;
    if (!rsa_random_nonzero(encoded + 2u, padding_length, timeout)) return 0;
    encoded[2ul + padding_length] = 0u;
    for (i = 0ul; i < message_length; ++i)
        encoded[3ul + padding_length + i] = message[i];
    return rsa_public_operation(key, encoded, ciphertext);
}

int icum_rsa_decrypt_pkcs1_v15(const icum_rsa_private_key *key,
                               const unsigned char *ciphertext,
                               unsigned long ciphertext_length,
                               unsigned char *message,
                               unsigned long message_capacity,
                               unsigned long *message_length)
{
    unsigned char encoded[ICUM_RSA_MAX_BYTES];
    unsigned long length, separator, i;
    unsigned int invalid;

    if ((key == (const icum_rsa_private_key *)0)
        || !rsa_key_valid(key->modulus, key->modulus_length)
        || (ciphertext == (const unsigned char *)0)
        || (message == (unsigned char *)0)
        || (message_length == (unsigned long *)0)
        || (ciphertext_length != key->modulus_length)) return 0;
    length = key->modulus_length;
    if (!rsa_private_operation(key, ciphertext, encoded)) return 0;
    invalid = (encoded[0] != 0u) | (encoded[1] != 2u);
    separator = length;
    for (i = 2ul; i < length; ++i) {
        if ((separator == length) && (encoded[i] == 0u)) separator = i;
    }
    invalid |= (separator == length);
    invalid |= (separator < 10ul);
    if (invalid != 0u) return 0;
    *message_length = length - separator - 1ul;
    if (*message_length > message_capacity) return 0;
    for (i = 0ul; i < *message_length; ++i)
        message[i] = encoded[separator + 1ul + i];
    return 1;
}

static int rsa_digest_info(icum_hash_algorithm hash,
                           const unsigned char *message,
                           unsigned long message_length,
                           unsigned char *encoded,
                           unsigned long *encoded_length)
{
    static const unsigned char sha1_prefix[15] =
        {0x30,0x21,0x30,0x09,0x06,0x05,0x2b,0x0e,0x03,0x02,0x1a,0x05,0x00,0x04,0x14};
    static const unsigned char sha256_prefix[19] =
        {0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};
    static const unsigned char sha384_prefix[19] =
        {0x30,0x41,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x02,0x05,0x00,0x04,0x30};
    static const unsigned char sha512_prefix[19] =
        {0x30,0x51,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x03,0x05,0x00,0x04,0x40};
    const unsigned char *prefix;
    unsigned long prefix_length, digest_length, i;
    unsigned char digest[64];

    digest_length = icum_hash_digest_size(hash);
    if ((digest_length == 0ul)
        || !icum_hash_compute(hash, message, message_length, digest)) return 0;
    if (hash == ICUM_HASH_SHA1) {
        prefix = sha1_prefix; prefix_length = sizeof(sha1_prefix);
    } else if (hash == ICUM_HASH_SHA256) {
        prefix = sha256_prefix; prefix_length = sizeof(sha256_prefix);
    } else if (hash == ICUM_HASH_SHA384) {
        prefix = sha384_prefix; prefix_length = sizeof(sha384_prefix);
    } else {
        prefix = sha512_prefix; prefix_length = sizeof(sha512_prefix);
    }
    for (i=0ul;i<prefix_length;++i) encoded[i]=prefix[i];
    for (i=0ul;i<digest_length;++i) encoded[prefix_length+i]=digest[i];
    *encoded_length=prefix_length+digest_length;
    return 1;
}

int icum_rsa_sign_pkcs1_v15(const icum_rsa_private_key *key,
                            icum_hash_algorithm hash,
                            const unsigned char *message,
                            unsigned long message_length,
                            unsigned char *signature)
{
    unsigned char encoded[ICUM_RSA_MAX_BYTES], digest_info[83];
    unsigned long length, info_length, padding_length, i;

    if ((key == (const icum_rsa_private_key *)0)
        || !rsa_key_valid(key->modulus, key->modulus_length)
        || (signature == (unsigned char *)0)
        || ((message == (const unsigned char *)0) && (message_length != 0ul))
        || !rsa_digest_info(hash,message,message_length,digest_info,&info_length)) return 0;
    length=key->modulus_length;
    if (info_length > length-11ul) return 0;
    padding_length=length-info_length-3ul;
    encoded[0]=0u; encoded[1]=1u;
    for(i=0ul;i<padding_length;++i) encoded[2ul+i]=0xffu;
    encoded[2ul+padding_length]=0u;
    for(i=0ul;i<info_length;++i) encoded[3ul+padding_length+i]=digest_info[i];
    return rsa_private_operation(key,encoded,signature);
}

int icum_rsa_verify_pkcs1_v15(const icum_rsa_public_key *key,
                              icum_hash_algorithm hash,
                              const unsigned char *message,
                              unsigned long message_length,
                              const unsigned char *signature)
{
    unsigned char encoded[ICUM_RSA_MAX_BYTES], expected[83];
    unsigned long length, info_length, i, separator;
    unsigned int mismatch;

    if ((key == (const icum_rsa_public_key *)0)
        || !rsa_key_valid(key->modulus,key->modulus_length)
        || (signature == (const unsigned char *)0)
        || ((message == (const unsigned char *)0) && (message_length != 0ul))
        || !rsa_digest_info(hash,message,message_length,expected,&info_length)
        || !rsa_public_operation(key,signature,encoded)) return 0;
    length=key->modulus_length;
    if ((encoded[0]!=0u)||(encoded[1]!=1u)) return 0;
    separator=2ul;
    while ((separator<length)&&(encoded[separator]==0xffu)) ++separator;
    if ((separator<10ul)||(separator>=length)||(encoded[separator]!=0u)
        || (length-separator-1ul!=info_length)) return 0;
    mismatch=0u;
    for(i=0ul;i<info_length;++i) mismatch |= encoded[separator+1ul+i]^expected[i];
    return mismatch==0u;
}

static int rsa_mgf1(icum_hash_algorithm hash,
                    const unsigned char *seed,
                    unsigned long seed_length,
                    unsigned char *mask,
                    unsigned long mask_length)
{
    unsigned char input[68], digest[64];
    unsigned long digest_length, offset, take, i;
    uint32 counter;
    digest_length=icum_hash_digest_size(hash);
    if ((digest_length==0ul)||(seed_length>64ul)
        || (seed_length+4ul>sizeof(input))) return 0;
    for(i=0ul;i<seed_length;++i) input[i]=seed[i];
    offset=0ul; counter=0u;
    while(offset<mask_length) {
        input[seed_length]=(unsigned char)(counter>>24);
        input[seed_length+1ul]=(unsigned char)(counter>>16);
        input[seed_length+2ul]=(unsigned char)(counter>>8);
        input[seed_length+3ul]=(unsigned char)counter;
        if(!icum_hash_compute(hash,input,seed_length+4ul,digest)) return 0;
        take=mask_length-offset;
        if(take>digest_length) take=digest_length;
        for(i=0ul;i<take;++i) mask[offset+i]=digest[i];
        offset+=take;
        ++counter;
    }
    return 1;
}

static int rsa_pss_encode(icum_hash_algorithm hash,
                          const unsigned char *message,
                          unsigned long message_length,
                          unsigned long salt_length,
                          unsigned int modulus_bits,
                          unsigned char *encoded,
                          unsigned long timeout)
{
    unsigned char mhash[64], salt[64], hash_input[144], h[64], mask[ICUM_RSA_MAX_BYTES];
    unsigned long hlen, em_bits, em_length, db_length, i;
    unsigned int unused_bits;

    hlen=icum_hash_digest_size(hash);
    em_bits=modulus_bits-1u;
    em_length=(em_bits+7ul)/8ul;
    if ((hlen==0ul)||(salt_length>sizeof(salt))
        || (em_length<hlen+salt_length+2ul)
        || !icum_hash_compute(hash,message,message_length,mhash)
        || !rsa_random_bytes(salt,salt_length,timeout)) return 0;
    for(i=0ul;i<8ul;++i) hash_input[i]=0u;
    for(i=0ul;i<hlen;++i) hash_input[8ul+i]=mhash[i];
    for(i=0ul;i<salt_length;++i) hash_input[8ul+hlen+i]=salt[i];
    if(!icum_hash_compute(hash,hash_input,8ul+hlen+salt_length,h)) return 0;
    db_length=em_length-hlen-1ul;
    for(i=0ul;i<db_length-salt_length-1ul;++i) encoded[i]=0u;
    encoded[db_length-salt_length-1ul]=1u;
    for(i=0ul;i<salt_length;++i) encoded[db_length-salt_length+i]=salt[i];
    if(!rsa_mgf1(hash,h,hlen,mask,db_length)) return 0;
    for(i=0ul;i<db_length;++i) encoded[i]^=mask[i];
    unused_bits=(unsigned int)(8ul*em_length-em_bits);
    encoded[0]&=(unsigned char)(0xffu>>unused_bits);
    for(i=0ul;i<hlen;++i) encoded[db_length+i]=h[i];
    encoded[em_length-1ul]=0xbc;
    return 1;
}

int icum_rsa_sign_pss(const icum_rsa_private_key *key,
                      icum_hash_algorithm hash,
                      const unsigned char *message,
                      unsigned long message_length,
                      unsigned long salt_length,
                      unsigned char *signature,
                      unsigned long timeout)
{
    unsigned char encoded[ICUM_RSA_MAX_BYTES], full[ICUM_RSA_MAX_BYTES];
    unsigned long em_length, key_length;
    unsigned int bits;

    if ((key==(const icum_rsa_private_key *)0)
        || !rsa_key_valid(key->modulus,key->modulus_length)
        || (signature==(unsigned char *)0)||(timeout==0ul)
        || ((message==(const unsigned char *)0)&&(message_length!=0ul))) return 0;
    key_length=key->modulus_length;
    bits=rsa_modulus_bits(key->modulus,key_length);
    em_length=((unsigned long)(bits-1u)+7ul)/8ul;
    if(!rsa_pss_encode(hash,message,message_length,salt_length,bits,encoded,timeout)) return 0;
    if(em_length<key_length) {
        unsigned long i;
        for(i=0ul;i<key_length-em_length;++i) full[i]=0u;
        for(i=0ul;i<em_length;++i) full[key_length-em_length+i]=encoded[i];
    } else {
        unsigned long i;
        for(i=0ul;i<em_length;++i) full[i]=encoded[i];
    }
    return rsa_private_operation(key,full,signature);
}

int icum_rsa_verify_pss(const icum_rsa_public_key *key,
                        icum_hash_algorithm hash,
                        const unsigned char *message,
                        unsigned long message_length,
                        unsigned long salt_length,
                        const unsigned char *signature)
{
    unsigned char full[ICUM_RSA_MAX_BYTES], db[ICUM_RSA_MAX_BYTES];
    unsigned char mask[ICUM_RSA_MAX_BYTES], mhash[64], input[144], expected[64];
    unsigned long key_length, hlen, em_length, db_length, i, separator;
    unsigned int bits, unused_bits, mismatch;

    if ((key==(const icum_rsa_public_key *)0)
        || !rsa_key_valid(key->modulus,key->modulus_length)
        || (signature==(const unsigned char *)0)
        || ((message==(const unsigned char *)0)&&(message_length!=0ul))) return 0;
    key_length=key->modulus_length;
    bits=rsa_modulus_bits(key->modulus,key_length);
    em_length=((unsigned long)(bits-1u)+7ul)/8ul;
    hlen=icum_hash_digest_size(hash);
    if ((hlen==0ul)||(salt_length>64ul)||(em_length<hlen+salt_length+2ul)
        || !rsa_public_operation(key,signature,full)) return 0;
    for(i=0ul;i<key_length-em_length;++i) if(full[i]!=0u) return 0;
    if(full[key_length-1ul]!=0xbcu) return 0;
    db_length=em_length-hlen-1ul;
    for(i=0ul;i<db_length;++i) db[i]=full[key_length-em_length+i];
    for(i=0ul;i<hlen;++i) expected[i]=full[key_length-em_length+db_length+i];
    unused_bits=(unsigned int)(8ul*em_length-(bits-1u));
    if((unused_bits != 0u)
        && ((db[0] & (unsigned char)(0xffu << (8u-unused_bits)))!=0u)) return 0;
    if(!rsa_mgf1(hash,expected,hlen,mask,db_length)) return 0;
    for(i=0ul;i<db_length;++i) db[i]^=mask[i];
    db[0]&=(unsigned char)(0xffu>>unused_bits);
    separator=db_length-salt_length-1ul;
    mismatch=0u;
    for(i=0ul;i<separator;++i) mismatch|=db[i];
    mismatch|=(db[separator]^1u);
    if(mismatch!=0u) return 0;
    if(!icum_hash_compute(hash,message,message_length,mhash)) return 0;
    for(i=0ul;i<8ul;++i) input[i]=0u;
    for(i=0ul;i<hlen;++i) input[8ul+i]=mhash[i];
    for(i=0ul;i<salt_length;++i) input[8ul+hlen+i]=db[separator+1ul+i];
    if(!icum_hash_compute(hash,input,8ul+hlen+salt_length,mask)) return 0;
    mismatch=0u;
    for(i=0ul;i<hlen;++i) mismatch|=mask[i]^expected[i];
    return mismatch==0u;
}
