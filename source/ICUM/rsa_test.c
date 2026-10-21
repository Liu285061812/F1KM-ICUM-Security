#include "rsa_test.h"
#include "icum_rsa.h"

static int rsa_hex_decode(const char *hex,
                          unsigned char *output,
                          unsigned long output_length)
{
    unsigned long i;
    unsigned int high, low;
    char c;

    for (i = 0ul; i < output_length; ++i) {
        c = hex[i * 2ul];
        high = (c >= '0' && c <= '9') ? (unsigned int)(c - '0')
             : (c >= 'a' && c <= 'f') ? (unsigned int)(c - 'a' + 10)
             : (c >= 'A' && c <= 'F') ? (unsigned int)(c - 'A' + 10) : 16u;
        c = hex[i * 2ul + 1ul];
        low = (c >= '0' && c <= '9') ? (unsigned int)(c - '0')
            : (c >= 'a' && c <= 'f') ? (unsigned int)(c - 'a' + 10)
            : (c >= 'A' && c <= 'F') ? (unsigned int)(c - 'A' + 10) : 16u;
        if ((high > 15u) || (low > 15u)) {
            return 0;
        }
        output[i] = (unsigned char)((high << 4) | low);
    }
    return hex[output_length * 2ul] == '\0';
}

static int rsa_bytes_equal(const unsigned char *left,
                           const unsigned char *right,
                           unsigned long length)
{
    unsigned long i;
    unsigned int difference;
    difference = 0u;
    for (i = 0ul; i < length; ++i) difference |= left[i] ^ right[i];
    return difference == 0u;
}

int icum_rsa_self_test(unsigned long timeout)
{
    /* This key is a public test vector and must never protect application data. */
    static const char n_hex[] =
        "858e9a74981ac06b56620bfec30819d45b6877c3876dd4c39e28ba561e6e68fa"
        "e38baa093803aa0877c23c4b1e0bb43a83a7609f13269866264d831452d41497"
        "67b8a3493172424ced21c360e108822865eee75bab0858f8492ca49082b8c7f"
        "6926f913cf99988a316227f4864a84e5298f6486d44ccd1ce28ec734048423987";
    static const char d_hex[] =
        "215742aeb5a4040227bbed43a99b759b424077996bcf1accc4c8651007a3c778"
        "4e62b8cd795accd135585b65e337c7c3cdc6f48371abf26299d042d74980212d"
        "78873fed78241296186b483ba4116889dd3097926e91bf659c8a0ea915221e"
        "836a3c63a311631a9a29a2a72426e80e6d3ad2c4ac6090a7b666c8b5bf8456796d";
    static const char pkcs_signature_hex[] =
        "5159ae0d610d501ac8ef42fde6868fa966a563d959dbaadeb03dee7ac90319ff"
        "07aabc6b9da4aecab446bc9d8c59c09ff3a5f0821f92ecaeb75dc45c04fff45"
        "75373ed0196e771997115b2616cf39274cf30c4842284bc73908246462ffd883"
        "d33d2e586fead47aaa0ded3f41433495a7a97ce02c5551d4d3ad63617c11c79d6";
    static const char pss_signature_hex[] =
        "56e07f951e94a9f12b527a33e5c1dd556c6d10a55390242198111f925cd32343"
        "ccade819c7b3fec92126e20508ca98521feedc5f37d7124e1566b6e28ddd642"
        "05bbea3e49064cd8d64326e42d1ac7a989869cce11dbce0aef16be25a710133"
        "64607c4a33ca797325e44aed78b9adde10948c0057a1c0f652853fe28eed81268e";
    static const unsigned char test_message[] = "icum rsa self test";
    static const unsigned char changed_message[] = "icum rsa self tesu";
    unsigned char modulus[128], private_exponent[128];
    unsigned char pkcs_signature[128], pss_signature[128], signature[128];
    unsigned char ciphertext[128], plaintext[128];
    unsigned long plaintext_length;
    icum_rsa_public_key public_key;
    icum_rsa_private_key private_key;

    if (!rsa_hex_decode(n_hex, modulus, sizeof(modulus))) {
        return 0;
    }
    if (!rsa_hex_decode(d_hex, private_exponent, sizeof(private_exponent))) {
        return 0;
    }
    if (!rsa_hex_decode(pkcs_signature_hex, pkcs_signature, sizeof(pkcs_signature))) {
        return 0;
    }
    if (!rsa_hex_decode(pss_signature_hex, pss_signature, sizeof(pss_signature))) {
        return 0;
    }

    public_key.modulus = modulus;
    public_key.modulus_length = sizeof(modulus);
    public_key.public_exponent = 65537ul;
    private_key.modulus = modulus;
    private_key.private_exponent = private_exponent;
    private_key.modulus_length = sizeof(modulus);

    if (!icum_rsa_verify_pkcs1_v15(&public_key, ICUM_HASH_SHA256,
                                   test_message, sizeof(test_message) - 1ul,
                                   pkcs_signature)) {
        return 0;
    }
    if (icum_rsa_verify_pkcs1_v15(&public_key, ICUM_HASH_SHA256,
                                  changed_message, sizeof(changed_message) - 1ul,
                                  pkcs_signature)) {
        return 0;
    }
    if (!icum_rsa_sign_pkcs1_v15(&private_key, ICUM_HASH_SHA256,
                                 test_message, sizeof(test_message) - 1ul,
                                 signature)) {
        return 0;
    }
    if (!rsa_bytes_equal(signature, pkcs_signature, sizeof(signature))) {
        return 0;
    }
    if (!icum_rsa_verify_pss(&public_key, ICUM_HASH_SHA256,
                             test_message, sizeof(test_message) - 1ul,
                             32ul, pss_signature)) {
        return 0;
    }
    if (icum_rsa_verify_pss(&public_key, ICUM_HASH_SHA256,
                            changed_message, sizeof(changed_message) - 1ul,
                            32ul, pss_signature)) {
        return 0;
    }
    if (!icum_rsa_sign_pss(&private_key, ICUM_HASH_SHA256,
                           test_message, sizeof(test_message) - 1ul,
                           32ul, signature, timeout)) {
        return 0;
    }
    if (!icum_rsa_verify_pss(&public_key, ICUM_HASH_SHA256,
                             test_message, sizeof(test_message) - 1ul,
                             32ul, signature)) {
        return 0;
    }
    if (!icum_rsa_encrypt_pkcs1_v15(&public_key,
                                    test_message, sizeof(test_message) - 1ul,
                                    ciphertext, timeout)) {
        return 0;
    }
    if (!icum_rsa_decrypt_pkcs1_v15(&private_key, ciphertext,
                                    sizeof(ciphertext), plaintext,
                                    sizeof(plaintext), &plaintext_length)) {
        return 0;
    }
    if (plaintext_length != sizeof(test_message) - 1ul) {
        return 0;
    }
    if (!rsa_bytes_equal(plaintext, test_message, plaintext_length)) {
        return 0;
    }

    return 1;
}
