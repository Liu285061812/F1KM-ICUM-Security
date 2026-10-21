#ifndef ICUM_RSA_H
#define ICUM_RSA_H

#include "icum_hash.h"

#define ICUM_RSA_MIN_BYTES 128ul
#define ICUM_RSA_MAX_BYTES 512ul

#define ICUM_RSA_OK 1
#define ICUM_RSA_ERROR 0

/* Key integers are unsigned big-endian byte strings; n is 1024..4096 bits. */
typedef struct
{
    const unsigned char *modulus;
    unsigned long modulus_length;
    unsigned long public_exponent;
} icum_rsa_public_key;

typedef struct
{
    const unsigned char *modulus;
    const unsigned char *private_exponent;
    unsigned long modulus_length;
} icum_rsa_private_key;

int icum_rsa_encrypt_pkcs1_v15(const icum_rsa_public_key *key,
                               const unsigned char *message,
                               unsigned long message_length,
                               unsigned char *ciphertext,
                               unsigned long timeout);
int icum_rsa_decrypt_pkcs1_v15(const icum_rsa_private_key *key,
                               const unsigned char *ciphertext,
                               unsigned long ciphertext_length,
                               unsigned char *message,
                               unsigned long message_capacity,
                               unsigned long *message_length);
int icum_rsa_sign_pkcs1_v15(const icum_rsa_private_key *key,
                            icum_hash_algorithm hash,
                            const unsigned char *message,
                            unsigned long message_length,
                            unsigned char *signature);
int icum_rsa_verify_pkcs1_v15(const icum_rsa_public_key *key,
                              icum_hash_algorithm hash,
                              const unsigned char *message,
                              unsigned long message_length,
                              const unsigned char *signature);
int icum_rsa_sign_pss(const icum_rsa_private_key *key,
                      icum_hash_algorithm hash,
                      const unsigned char *message,
                      unsigned long message_length,
                      unsigned long salt_length,
                      unsigned char *signature,
                      unsigned long timeout);
int icum_rsa_verify_pss(const icum_rsa_public_key *key,
                        icum_hash_algorithm hash,
                        const unsigned char *message,
                        unsigned long message_length,
                        unsigned long salt_length,
                        const unsigned char *signature);
#endif
