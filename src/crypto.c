#include <string.h>

#include "crypto.h"

/* a bit faster & bigger, if defined */
#define UNROLL_LOOPS

/* NIST's proposed modification to SHA */
#define USE_MODIFIED_SHA

/* SHA f()-functions */
#define f1(x,y,z)   ((x & y) | (~x & z))
#define f2(x,y,z)   (x ^ y ^ z)
#define f3(x,y,z)   ((x & y) | (x & z) | (y & z))
#define f4(x,y,z)   (x ^ y ^ z)

/* SHA constants */
#define CONST1      0x5a827999L
#define CONST2      0x6ed9eba1L
#define CONST3      0x8f1bbcdcL
#define CONST4      0xca62c1d6L

/* 32-bit rotate */

#define ROT32(x,n)  ((x << n) | (x >> (32 - n)))

#define FUNC(n,i)                       \
    temp = ROT32(A,5) + f##n(B,C,D) + E + W[i] + CONST##n;  \
    E = D; D = C; C = ROT32(B,30); B = A; A = temp

/** size of the SHA1 DIGEST */
#define SHA1_DIGESTSIZE 20

#define SHA_BLOCKSIZE   64

typedef struct sha1_ctx_t sha1_ctx_t;

/**
 * SHA1 context structure
 */
struct sha1_ctx_t {
    /** message digest */
    unsigned int digest[5];
    /** 64-bit bit counts */
    unsigned int count_lo, count_hi;
    /** SHA data buffer */
    unsigned int data[16];
    /** unprocessed amount in data */
    int local;
};

/* initialize the SHA digest */
void sha1_init(sha1_ctx_t *sha_info)
{
    sha_info->digest[0] = 0x67452301L;
    sha_info->digest[1] = 0xefcdab89L;
    sha_info->digest[2] = 0x98badcfeL;
    sha_info->digest[3] = 0x10325476L;
    sha_info->digest[4] = 0xc3d2e1f0L;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
}

union endianTest {
    long Long;
    char Char[sizeof(long)];
};

static char isLittleEndian(void)
{
    static union endianTest u;
    u.Long = 1;
    return (u.Char[0] == 1);
}

static void maybe_byte_reverse(unsigned int *buffer, int count)
{
    int i;
    unsigned char ct[4], *cp;

    if (isLittleEndian()) { /* do the swap only if it is little endian */
        count /= sizeof(unsigned int);
        cp = (unsigned char *) buffer;
        for (i = 0; i < count; ++i) {
            ct[0] = cp[0];
            ct[1] = cp[1];
            ct[2] = cp[2];
            ct[3] = cp[3];
            cp[0] = ct[3];
            cp[1] = ct[2];
            cp[2] = ct[1];
            cp[3] = ct[0];
            cp += sizeof(unsigned int);
        }
    }
}

/* do SHA transformation */
static void sha_transform(sha1_ctx_t *sha_info)
{
    int i;
    unsigned int temp, A, B, C, D, E, W[80];

    for (i = 0; i < 16; ++i) {
        W[i] = sha_info->data[i];
    }
    for (i = 16; i < 80; ++i) {
        W[i] = W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16];
#ifdef USE_MODIFIED_SHA
        W[i] = ROT32(W[i], 1);
#endif /* USE_MODIFIED_SHA */
    }
    A = sha_info->digest[0];
    B = sha_info->digest[1];
    C = sha_info->digest[2];
    D = sha_info->digest[3];
    E = sha_info->digest[4];
#ifdef UNROLL_LOOPS
FUNC(1, 0);  FUNC(1, 1);  FUNC(1, 2);  FUNC(1, 3);  FUNC(1, 4);
    FUNC(1, 5);  FUNC(1, 6);  FUNC(1, 7);  FUNC(1, 8);  FUNC(1, 9);
    FUNC(1,10);  FUNC(1,11);  FUNC(1,12);  FUNC(1,13);  FUNC(1,14);
    FUNC(1,15);  FUNC(1,16);  FUNC(1,17);  FUNC(1,18);  FUNC(1,19);

    FUNC(2,20);  FUNC(2,21);  FUNC(2,22);  FUNC(2,23);  FUNC(2,24);
    FUNC(2,25);  FUNC(2,26);  FUNC(2,27);  FUNC(2,28);  FUNC(2,29);
    FUNC(2,30);  FUNC(2,31);  FUNC(2,32);  FUNC(2,33);  FUNC(2,34);
    FUNC(2,35);  FUNC(2,36);  FUNC(2,37);  FUNC(2,38);  FUNC(2,39);

    FUNC(3,40);  FUNC(3,41);  FUNC(3,42);  FUNC(3,43);  FUNC(3,44);
    FUNC(3,45);  FUNC(3,46);  FUNC(3,47);  FUNC(3,48);  FUNC(3,49);
    FUNC(3,50);  FUNC(3,51);  FUNC(3,52);  FUNC(3,53);  FUNC(3,54);
    FUNC(3,55);  FUNC(3,56);  FUNC(3,57);  FUNC(3,58);  FUNC(3,59);

    FUNC(4,60);  FUNC(4,61);  FUNC(4,62);  FUNC(4,63);  FUNC(4,64);
    FUNC(4,65);  FUNC(4,66);  FUNC(4,67);  FUNC(4,68);  FUNC(4,69);
    FUNC(4,70);  FUNC(4,71);  FUNC(4,72);  FUNC(4,73);  FUNC(4,74);
    FUNC(4,75);  FUNC(4,76);  FUNC(4,77);  FUNC(4,78);  FUNC(4,79);
#else /* !UNROLL_LOOPS */
    for (i = 0; i < 20; ++i) {
        FUNC(1,i);
    }
    for (i = 20; i < 40; ++i) {
        FUNC(2,i);
    }
    for (i = 40; i < 60; ++i) {
        FUNC(3,i);
    }
    for (i = 60; i < 80; ++i) {
        FUNC(4,i);
    }
#endif /* !UNROLL_LOOPS */
    sha_info->digest[0] += A;
    sha_info->digest[1] += B;
    sha_info->digest[2] += C;
    sha_info->digest[3] += D;
    sha_info->digest[4] += E;
}

/* update the SHA digest */
void sha1_update(sha1_ctx_t *sha_info, const char *buffer, unsigned int count)
{
    unsigned int i;

    if ((sha_info->count_lo + ((unsigned int) count << 3)) < sha_info->count_lo) {
        ++sha_info->count_hi;
    }
    sha_info->count_lo += (unsigned int) count << 3;
    sha_info->count_hi += (unsigned int) count >> 29;
    if (sha_info->local) {
        i = SHA_BLOCKSIZE - sha_info->local;
        if (i > count) {
            i = count;
        }
        memcpy(((unsigned char *) sha_info->data) + sha_info->local, buffer, i);
        count -= i;
        buffer += i;
        sha_info->local += i;
        if (sha_info->local == SHA_BLOCKSIZE) {
            maybe_byte_reverse(sha_info->data, SHA_BLOCKSIZE);
            sha_transform(sha_info);
        }
        else {
            return;
        }
    }
    while (count >= SHA_BLOCKSIZE) {
        memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
        maybe_byte_reverse(sha_info->data, SHA_BLOCKSIZE);
        sha_transform(sha_info);
    }
    memcpy(sha_info->data, buffer, count);
    sha_info->local = count;
}

/* finish computing the SHA digest */
void sha1_final(unsigned char digest[SHA1_DIGESTSIZE], sha1_ctx_t *sha_info)
{
    int count, i, j;
    unsigned int lo_bit_count, hi_bit_count, k;

    lo_bit_count = sha_info->count_lo;
    hi_bit_count = sha_info->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x3f);
    ((unsigned char *) sha_info->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 8) {
        memset(((unsigned char *) sha_info->data) + count, 0, SHA_BLOCKSIZE - count);
        maybe_byte_reverse(sha_info->data, SHA_BLOCKSIZE);
        sha_transform(sha_info);
        memset((unsigned char *) sha_info->data, 0, SHA_BLOCKSIZE - 8);
    }
    else {
        memset(((unsigned char *) sha_info->data) + count, 0,
               SHA_BLOCKSIZE - 8 - count);
    }
    maybe_byte_reverse(sha_info->data, SHA_BLOCKSIZE);
    sha_info->data[14] = hi_bit_count;
    sha_info->data[15] = lo_bit_count;
    sha_transform(sha_info);

    for (i = 0, j = 0; j < SHA1_DIGESTSIZE; i++) {
        k = sha_info->digest[i];
        digest[j++] = (unsigned char) ((k >> 24) & 0xff);
        digest[j++] = (unsigned char) ((k >> 16) & 0xff);
        digest[j++] = (unsigned char) ((k >> 8) & 0xff);
        digest[j++] = (unsigned char) (k & 0xff);
    }
}

void hmac_sha1(unsigned char hmac[20], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len)
{
    unsigned char kopad[64], kipad[64];
    int i;
    unsigned char digest[SHA1_DIGESTSIZE];
    sha1_ctx_t context;

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

    sha1_init(&context);
    sha1_update(&context, (const char *)kipad, 64);
    sha1_update(&context, (const char *)message, (unsigned int)message_len);
    sha1_final(digest, &context);

    sha1_init(&context);
    sha1_update(&context, (const char *)kopad, 64);
    sha1_update(&context, (const char *)digest, 20);
    sha1_final(hmac, &context);
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