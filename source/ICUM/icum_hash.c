#include "icum_hash.h"
#include "device.h"

typedef unsigned long long icum_u64;

#define SHA1_ROL(x, n) (((x) << (n)) | ((x) >> (32u - (n))))
#define SHA256_ROTR(x, n) (((x) >> (n)) | ((x) << (32u - (n))))
#define SHA512_ROTR(x, n) (((x) >> (n)) | ((x) << (64u - (n))))

static const uint32 sha256_k[64] =
{
    0x428a2f98ul,0x71374491ul,0xb5c0fbcful,0xe9b5dba5ul,
    0x3956c25bul,0x59f111f1ul,0x923f82a4ul,0xab1c5ed5ul,
    0xd807aa98ul,0x12835b01ul,0x243185beul,0x550c7dc3ul,
    0x72be5d74ul,0x80deb1feul,0x9bdc06a7ul,0xc19bf174ul,
    0xe49b69c1ul,0xefbe4786ul,0x0fc19dc6ul,0x240ca1ccul,
    0x2de92c6ful,0x4a7484aaul,0x5cb0a9dcul,0x76f988daul,
    0x983e5152ul,0xa831c66dul,0xb00327c8ul,0xbf597fc7ul,
    0xc6e00bf3ul,0xd5a79147ul,0x06ca6351ul,0x14292967ul,
    0x27b70a85ul,0x2e1b2138ul,0x4d2c6dfcul,0x53380d13ul,
    0x650a7354ul,0x766a0abbul,0x81c2c92eul,0x92722c85ul,
    0xa2bfe8a1ul,0xa81a664bul,0xc24b8b70ul,0xc76c51a3ul,
    0xd192e819ul,0xd6990624ul,0xf40e3585ul,0x106aa070ul,
    0x19a4c116ul,0x1e376c08ul,0x2748774cul,0x34b0bcb5ul,
    0x391c0cb3ul,0x4ed8aa4aul,0x5b9cca4ful,0x682e6ff3ul,
    0x748f82eeul,0x78a5636ful,0x84c87814ul,0x8cc70208ul,
    0x90befffaul,0xa4506cebul,0xbef9a3f7ul,0xc67178f2ul
};

static const icum_u64 sha512_k[80] =
{
    0x428a2f98d728ae22ull,0x7137449123ef65cdull,0xb5c0fbcfec4d3b2full,0xe9b5dba58189dbbcull,
    0x3956c25bf348b538ull,0x59f111f1b605d019ull,0x923f82a4af194f9bull,0xab1c5ed5da6d8118ull,
    0xd807aa98a3030242ull,0x12835b0145706fbeull,0x243185be4ee4b28cull,0x550c7dc3d5ffb4e2ull,
    0x72be5d74f27b896full,0x80deb1fe3b1696b1ull,0x9bdc06a725c71235ull,0xc19bf174cf692694ull,
    0xe49b69c19ef14ad2ull,0xefbe4786384f25e3ull,0x0fc19dc68b8cd5b5ull,0x240ca1cc77ac9c65ull,
    0x2de92c6f592b0275ull,0x4a7484aa6ea6e483ull,0x5cb0a9dcbd41fbd4ull,0x76f988da831153b5ull,
    0x983e5152ee66dfabull,0xa831c66d2db43210ull,0xb00327c898fb213full,0xbf597fc7beef0ee4ull,
    0xc6e00bf33da88fc2ull,0xd5a79147930aa725ull,0x06ca6351e003826full,0x142929670a0e6e70ull,
    0x27b70a8546d22ffCull,0x2e1b21385c26c926ull,0x4d2c6dfc5ac42aedull,0x53380d139d95b3dfull,
    0x650a73548baf63deull,0x766a0abb3c77b2a8ull,0x81c2c92e47edaee6ull,0x92722c851482353bull,
    0xa2bfe8a14cf10364ull,0xa81a664bbc423001ull,0xc24b8b70d0f89791ull,0xc76c51a30654be30ull,
    0xd192e819d6ef5218ull,0xd69906245565a910ull,0xf40e35855771202aull,0x106aa07032bbd1b8ull,
    0x19a4c116b8d2d0c8ull,0x1e376c085141ab53ull,0x2748774cdf8eeb99ull,0x34b0bcb5e19b48a8ull,
    0x391c0cb3c5c95a63ull,0x4ed8aa4ae3418acbull,0x5b9cca4f7763e373ull,0x682e6ff3d6b2b8a3ull,
    0x748f82ee5defb2fcull,0x78a5636f43172f60ull,0x84c87814a1f0ab72ull,0x8cc702081a6439ecull,
    0x90befffa23631e28ull,0xa4506cebde82bde9ull,0xbef9a3f7b2c67915ull,0xc67178f2e372532bull,
    0xca273eceea26619cull,0xd186b8c721c0c207ull,0xeada7dd6cde0eb1eull,0xf57d4f7fee6ed178ull,
    0x06f067aa72176fbaull,0x0a637dc5a2c898a6ull,0x113f9804bef90daeull,0x1b710b35131c471bull,
    0x28db77f523047d84ull,0x32caab7b40c72493ull,0x3c9ebe0a15c9bebcull,0x431d67c49c100d4cull,
    0x4cc5d4becb3e42b6ull,0x597f299cfc657e2aull,0x5fcb6fab3ad6faecull,0x6c44198c4a475817ull
};

static uint32 get_be32(const unsigned char *p)
{
    return ((uint32)p[0] << 24) | ((uint32)p[1] << 16)
         | ((uint32)p[2] << 8) | (uint32)p[3];
}

static void put_be32(unsigned char *p, uint32 value)
{
    p[0] = (unsigned char)(value >> 24);
    p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8);
    p[3] = (unsigned char)value;
}

static icum_u64 get_be64(const unsigned char *p)
{
    return ((icum_u64)get_be32(p) << 32) | get_be32(p + 4);
}

static void put_be64(unsigned char *p, icum_u64 value)
{
    put_be32(p, (uint32)(value >> 32));
    put_be32(p + 4, (uint32)value);
}

static void sha1_block(uint32 state[5], const unsigned char block[64])
{
    uint32 w[80];
    uint32 a, b, c, d, e, f, k, temp;
    unsigned int i;

    for (i = 0u; i < 16u; ++i) {
        w[i] = get_be32(block + 4u * i);
    }
    for (i = 16u; i < 80u; ++i) {
        w[i] = SHA1_ROL(w[i - 3u] ^ w[i - 8u] ^ w[i - 14u] ^ w[i - 16u], 1u);
    }
    a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];
    for (i = 0u; i < 80u; ++i) {
        if (i < 20u) {
            f = (b & c) | ((~b) & d); k = 0x5a827999ul;
        } else if (i < 40u) {
            f = b ^ c ^ d; k = 0x6ed9eba1ul;
        } else if (i < 60u) {
            f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdcul;
        } else {
            f = b ^ c ^ d; k = 0xca62c1d6ul;
        }
        temp = SHA1_ROL(a, 5u) + f + e + k + w[i];
        e = d; d = c; c = SHA1_ROL(b, 30u); b = a; a = temp;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

static void sha256_block(uint32 state[8], const unsigned char block[64])
{
    uint32 w[64];
    uint32 a, b, c, d, e, f, g, h, s0, s1, ch, maj, t1, t2;
    unsigned int i;

    for (i = 0u; i < 16u; ++i) w[i] = get_be32(block + 4u * i);
    for (i = 16u; i < 64u; ++i) {
        s0 = SHA256_ROTR(w[i - 15u], 7u) ^ SHA256_ROTR(w[i - 15u], 18u) ^ (w[i - 15u] >> 3);
        s1 = SHA256_ROTR(w[i - 2u], 17u) ^ SHA256_ROTR(w[i - 2u], 19u) ^ (w[i - 2u] >> 10);
        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }
    a=state[0]; b=state[1]; c=state[2]; d=state[3];
    e=state[4]; f=state[5]; g=state[6]; h=state[7];
    for (i = 0u; i < 64u; ++i) {
        s1 = SHA256_ROTR(e, 6u) ^ SHA256_ROTR(e, 11u) ^ SHA256_ROTR(e, 25u);
        ch = (e & f) ^ ((~e) & g);
        t1 = h + s1 + ch + sha256_k[i] + w[i];
        s0 = SHA256_ROTR(a, 2u) ^ SHA256_ROTR(a, 13u) ^ SHA256_ROTR(a, 22u);
        maj = (a & b) ^ (a & c) ^ (b & c);
        t2 = s0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

static void sha512_block(icum_u64 state[8], const unsigned char block[128])
{
    icum_u64 w[80];
    icum_u64 a,b,c,d,e,f,g,h,s0,s1,ch,maj,t1,t2;
    unsigned int i;

    for (i = 0u; i < 16u; ++i) w[i] = get_be64(block + 8u * i);
    for (i = 16u; i < 80u; ++i) {
        s0 = SHA512_ROTR(w[i-15u],1u) ^ SHA512_ROTR(w[i-15u],8u) ^ (w[i-15u] >> 7);
        s1 = SHA512_ROTR(w[i-2u],19u) ^ SHA512_ROTR(w[i-2u],61u) ^ (w[i-2u] >> 6);
        w[i] = w[i-16u] + s0 + w[i-7u] + s1;
    }
    a=state[0]; b=state[1]; c=state[2]; d=state[3];
    e=state[4]; f=state[5]; g=state[6]; h=state[7];
    for (i = 0u; i < 80u; ++i) {
        s1 = SHA512_ROTR(e,14u) ^ SHA512_ROTR(e,18u) ^ SHA512_ROTR(e,41u);
        ch = (e & f) ^ ((~e) & g);
        t1 = h + s1 + ch + sha512_k[i] + w[i];
        s0 = SHA512_ROTR(a,28u) ^ SHA512_ROTR(a,34u) ^ SHA512_ROTR(a,39u);
        maj = (a & b) ^ (a & c) ^ (b & c);
        t2 = s0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

unsigned long icum_hash_digest_size(icum_hash_algorithm algorithm)
{
    switch (algorithm) {
    case ICUM_HASH_SHA1: return 20ul;
    case ICUM_HASH_SHA256: return 32ul;
    case ICUM_HASH_SHA384: return 48ul;
    case ICUM_HASH_SHA512: return 64ul;
    default: return 0ul;
    }
}

int icum_hash_compute(icum_hash_algorithm algorithm,
                      const unsigned char *message,
                      unsigned long message_length,
                      unsigned char *digest)
{
    unsigned long offset;
    unsigned int remain, i, out_words;
    unsigned char final_block[256];
    uint32 s32[8];
    icum_u64 s64[8];
    unsigned long long bit_length;
    unsigned long digest_size;

    digest_size = icum_hash_digest_size(algorithm);
    if ((digest_size == 0ul) || (digest == (unsigned char *)0)
        || ((message == (const unsigned char *)0) && (message_length != 0ul))) {
        return 0;
    }
    offset = 0ul;
    if ((algorithm == ICUM_HASH_SHA1) || (algorithm == ICUM_HASH_SHA256)) {
        if (algorithm == ICUM_HASH_SHA1) {
            s32[0]=0x67452301ul; s32[1]=0xefcdab89ul; s32[2]=0x98badcfeul;
            s32[3]=0x10325476ul; s32[4]=0xc3d2e1f0ul; out_words=5u;
        } else {
            s32[0]=0x6a09e667ul; s32[1]=0xbb67ae85ul; s32[2]=0x3c6ef372ul; s32[3]=0xa54ff53aul;
            s32[4]=0x510e527ful; s32[5]=0x9b05688cul; s32[6]=0x1f83d9abul; s32[7]=0x5be0cd19ul; out_words=8u;
        }
        while (message_length - offset >= 64ul) {
            if (algorithm == ICUM_HASH_SHA1) sha1_block(s32, message + offset);
            else sha256_block(s32, message + offset);
            offset += 64ul;
        }
        remain = (unsigned int)(message_length - offset);
        for (i=0u;i<remain;++i) final_block[i]=message[offset+i];
        final_block[remain]=0x80u;
        if (remain >= 56u) {
            for (i=remain+1u;i<64u;++i) final_block[i]=0u;
            if (algorithm == ICUM_HASH_SHA1) sha1_block(s32, final_block);
            else sha256_block(s32, final_block);
            final_block[0]=0u;
            remain=0u;
        }
        for (i=remain+1u;i<56u;++i) final_block[i]=0u;
        bit_length = (unsigned long long)message_length * 8ull;
        for (i=0u;i<8u;++i) final_block[63u-i]=(unsigned char)(bit_length>>(8u*i));
        if (algorithm == ICUM_HASH_SHA1) sha1_block(s32, final_block);
        else sha256_block(s32, final_block);
        for (i=0u;i<out_words;++i) put_be32(digest+4u*i,s32[i]);
    } else {
        if (algorithm == ICUM_HASH_SHA384) {
            s64[0]=0xcbbb9d5dc1059ed8ull; s64[1]=0x629a292a367cd507ull;
            s64[2]=0x9159015a3070dd17ull; s64[3]=0x152fecd8f70e5939ull;
            s64[4]=0x67332667ffc00b31ull; s64[5]=0x8eb44a8768581511ull;
            s64[6]=0xdb0c2e0d64f98fa7ull; s64[7]=0x47b5481dbefa4fa4ull;
        } else {
            s64[0]=0x6a09e667f3bcc908ull; s64[1]=0xbb67ae8584caa73bull;
            s64[2]=0x3c6ef372fe94f82bull; s64[3]=0xa54ff53a5f1d36f1ull;
            s64[4]=0x510e527fade682d1ull; s64[5]=0x9b05688c2b3e6c1full;
            s64[6]=0x1f83d9abfb41bd6bull; s64[7]=0x5be0cd19137e2179ull;
        }
        while (message_length - offset >= 128ul) {
            sha512_block(s64,message+offset);
            offset+=128ul;
        }
        remain=(unsigned int)(message_length-offset);
        for(i=0u;i<remain;++i) final_block[i]=message[offset+i];
        final_block[remain]=0x80u;
        if(remain>=112u) {
            for(i=remain+1u;i<128u;++i) final_block[i]=0u;
            sha512_block(s64,final_block);
            final_block[0]=0u;
            remain=0u;
        }
        for(i=remain+1u;i<120u;++i) final_block[i]=0u;
        bit_length=(unsigned long long)message_length*8ull;
        for(i=0u;i<8u;++i) final_block[127u-i]=(unsigned char)(bit_length>>(8u*i));
        sha512_block(s64,final_block);
        out_words=(algorithm==ICUM_HASH_SHA384)?6u:8u;
        for(i=0u;i<out_words;++i) put_be64(digest+8u*i,s64[i]);
    }
    return 1;
}
