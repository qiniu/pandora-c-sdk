#ifndef PANDORA_C_CRYPTO_H
#define PANDORA_C_CRYPTO_H

void hmac_sha1(unsigned char hmac[20], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len);

int base64_encode(const unsigned char *in, int inLen, char *out);

#endif //PANDORA_C_CRYPTO_H
