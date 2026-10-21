#ifndef ICUM_HASH_H
#define ICUM_HASH_H

typedef enum
{
    ICUM_HASH_SHA1 = 1,
    ICUM_HASH_SHA256 = 2,
    ICUM_HASH_SHA384 = 3,
    ICUM_HASH_SHA512 = 4
} icum_hash_algorithm;

unsigned long icum_hash_digest_size(icum_hash_algorithm algorithm);
int icum_hash_compute(icum_hash_algorithm algorithm,
                      const unsigned char *message,
                      unsigned long message_length,
                      unsigned char *digest);
#endif
