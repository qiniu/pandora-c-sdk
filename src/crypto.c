#include "crypto.h"

#include <apr_md5.h>
#include <apr_sha1.h>

void hmac_sha1(unsigned char hmac[20], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len)
{
    unsigned char kopad[64], kipad[64];
    int i;
    unsigned char digest[APR_SHA1_DIGESTSIZE];
    apr_sha1_ctx_t context;

    if (key_len > 64) {
        key_len = 64;
    }

    for (i = 0; i < key_len; i++) {
        kopad[i] = key[i] ^ 0x5c;
        kipad[i] = key[i] ^ 0x36;
    }

    for ( ; i < 64; i++) {
        kopad[i] = 0 ^ 0x5c;
        kipad[i] = 0 ^ 0x36;
    }

    apr_sha1_init(&context);
    apr_sha1_update(&context, (const char *)kipad, 64);
    apr_sha1_update(&context, (const char *)message, (unsigned int)message_len);
    apr_sha1_final(digest, &context);

    apr_sha1_init(&context);
    apr_sha1_update(&context, (const char *)kopad, 64);
    apr_sha1_update(&context, (const char *)digest, 20);
    apr_sha1_final(hmac, &context);
}

int base64_encode(const unsigned char *in, int inLen, char *out)
{
    static const char *ENC = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    char *orig_out = out;

    while (inLen) {
        // first 6 bits of char 1
        *out++ = ENC[*in >> 2];
        if (!--inLen) {
            // last 2 bits of char 1, 4 bits of 0
            *out++ = ENC[(*in & 0x3) << 4];
            *out++ = '=';
            *out++ = '=';
            break;
        }
        // last 2 bits of char 1, first 4 bits of char 2
        *out++ = ENC[((*in & 0x3) << 4) | (*(in + 1) >> 4)];
        in++;
        if (!--inLen) {
            // last 4 bits of char 2, 2 bits of 0
            *out++ = ENC[(*in & 0xF) << 2];
            *out++ = '=';
            break;
        }
        // last 4 bits of char 2, first 2 bits of char 3
        *out++ = ENC[((*in & 0xF) << 2) | (*(in + 1) >> 6)];
        in++;
        // last 6 bits of char 3
        *out++ = ENC[*in & 0x3F];
        in++, inLen--;
    }
    *out = '\0';

    return (out - orig_out);
}