#ifndef PANDORA_C_BUFFER_H
#define PANDORA_C_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef enum {
    BUFFER_OWNS_SELF    = 1, /* buffer struct will be freed by buffer_destroy() */
    BUFFER_OWNS_DATA    = 2, /* buffer data will be freed by buffer_destroy() */
    BUFFER_GROWABLE     = 4  /* buffer can grow dynamically to accommodate new data */
} buffer_flags_t;

typedef struct {
    int    flags;
    char   *data;
    size_t capacity;
    size_t written;
    size_t read;
} buffer_t;

#define BUFFER_RESET(b)             ((b)->written = 0, (b)->read = 0)
#define BUFFER_APPEND(b, byte)      ((b)->data[(b)->written++] = (byte))
#define BUFFER_GET(b)               ((b)->data[(b)->read++])
#define BUFFER_CAPACITY(b)          ((b)->capacity)
#define BUFFER_SIZE(b)              ((b)->written)
#define BUFFER_REMAIN(b)            ((b)->capacity - (b)->written)
#define BUFFER_IS_EMPTY(b)          ((b)->written == 0)
#define BUFFER_IS_FULL(b)           ((b)->written == (b)->capacity)
#define BUFFER_TELL(b)              ((b)->read)
#define BUFFER_EOF(b)               ((b)->read == (b)->written)
#define BUFFER_REWIND(b)            ((b)->read = 0)
#define BUFFER_SEEK(b, offset)      ((b)->read = (offset))

/**
 * Initialize an existing buffer with an existing data array of given length
 */
void buffer_init_static(buffer_t *buffer, char *data, size_t size);

/**
 * Initial an existing buffer with dynamically allocated data array of a known size
 * implies BUFFER_OWNS_DATA
 */
int buffer_init(buffer_t *buffer, size_t size, int flags);

/**
 * Allocate a new buffer with a given initial size and flags
 * implies BUFFER_OWNS_SELF | BUFFER_OWNS_DATA
 */
buffer_t* buffer_create(size_t size, int flags);

/**
 * Destroy buffer - possible free()'s both data and buffer struct, depending
 * on buffer->flags
 */
void buffer_destroy(buffer_t *buffer);

/**
 * Remove all data and reset read head to start
 */
void buffer_reset(buffer_t *buffer);

/**
 * Append a single byte to buffer, returns 1 on success, 0 on failure
 */
int buffer_append(buffer_t *buffer, char byte);

/*
 * Write len bytes from data to buffer. all or nothing operation; returns 0
 * if remaining capacity of buffer is < len, or 1 if write was successful
 */
int buffer_write(buffer_t *buffer, const char *data, size_t len);

/**
 * Reads a single character buffer from the buffer
 */
char buffer_get(buffer_t *buffer);

/*
 * Read maxlen bytes from the buffer to dest. read head is advanced by maxlen.
 * returns number of bytes read
 */
int buffer_read(buffer_t *buffer, size_t maxlen, void *dest);

/*
 * Returns the buffer's capacity (i.e. max number of bytes it can contain)
 */
size_t buffer_capacity(buffer_t *buffer);

/*
 * Returns the number of bytes which have been written to the buffer
 */
size_t buffer_size(buffer_t *buffer);

/*
 * Returns number of bytes that can still be written to this buffer
 */
size_t buffer_remain(buffer_t *buffer);

/*
 * 1 = buffer empty, 0 = not empty
 */
int buffer_is_empty(buffer_t *buffer);

/*
 * 1 = buffer full, 0 = not full
 */
int buffer_is_full(buffer_t *buffer);

/*
 * Get position of read head
 */
size_t buffer_tell(buffer_t *buffer);

/*
 * Is read head at end of buffer?
 */
int buffer_eof(buffer_t *buffer);

/*
 * Rewind read head to beginning of buffer. data in buffer is unaffected.
 */
void buffer_rewind(buffer_t *buffer);

/*
 * Seek read head to arbitrary point in buffer. data is unaffected
 */
void buffer_seek(buffer_t *buffer, size_t offset);

#ifdef __cplusplus
}
#endif

#endif //PANDORA_C_BUFFER_H
