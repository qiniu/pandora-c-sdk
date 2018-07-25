#include <pandora/buffer.h>

#include <string.h>
#include <stdlib.h>

static int buffer_grow(buffer_t *buffer, size_t min_capacity)
{
    size_t new_capacity = buffer->capacity;
    while (new_capacity < min_capacity)
        new_capacity <<= 2;
    void *new_data = realloc(buffer->data, new_capacity);
    if (!new_data) {
        return 0;
    } else {
        buffer->capacity = new_capacity;
        buffer->data = new_data;
        return 1;
    }
}

void buffer_init_static(buffer_t *buffer, char *data, size_t size)
{
    buffer->flags = 0;
    buffer->data = data;
    buffer->capacity = size;
    buffer->written = 0;
    buffer->read = 0;
}

int buffer_init(buffer_t *buffer, size_t size, int flags)
{
    buffer->flags = flags | BUFFER_OWNS_DATA;
    buffer->data = malloc(size);
    if (!buffer->data)
        return 0;
    buffer->capacity = size;
    buffer->written = 0;
    buffer->read = 0;
    return 1;
}

buffer_t* buffer_create(size_t size, int flags)
{
    buffer_t *buffer = malloc(sizeof(buffer_t));
    if (!buffer)
        return NULL;
    if (!buffer_init(buffer, size, flags | BUFFER_OWNS_SELF)) {
        free(buffer);
    }
    return buffer;
}

void buffer_destroy(buffer_t *buffer)
{
    if (buffer->flags & BUFFER_OWNS_DATA)
        free(buffer->data);
    if (buffer->flags & BUFFER_OWNS_SELF)
        free(buffer);
}

void buffer_reset(buffer_t *buffer)
{
    BUFFER_RESET(buffer);
}

int buffer_append(buffer_t *buffer, char byte)
{
    if (buffer->capacity - buffer->written == 0) {
        if (buffer->flags & BUFFER_GROWABLE) {
            if (!buffer_grow(buffer, buffer->capacity + 1)) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    buffer->data[buffer->written++] = byte;
    return 1;
}

int buffer_write(buffer_t *buffer, const char *data, size_t len)
{
    if (buffer->capacity - buffer->written < len) {
        if (buffer->flags & BUFFER_GROWABLE) {
            if (!buffer_grow(buffer, buffer->capacity + len)) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    memcpy(buffer->data + buffer->written, data, len);
    buffer->written += len;
    return 1;
}

char buffer_get(buffer_t *buffer)
{
    if (buffer->read >= buffer->written)
        return 0;
    return BUFFER_GET(buffer);
}

int buffer_read(buffer_t *buffer, size_t maxlen, void *dest)
{
    size_t remain = buffer->written - buffer->read;
    if (remain < maxlen)
        maxlen = remain;
    memcpy(dest, buffer->data + buffer->read, maxlen);
    buffer->read += maxlen;
    return maxlen;
}

size_t buffer_capacity(buffer_t *buffer) { return BUFFER_CAPACITY(buffer); }
size_t buffer_size(buffer_t *buffer) { return BUFFER_SIZE(buffer); }
size_t buffer_remain(buffer_t *buffer) { return BUFFER_REMAIN(buffer); }
int buffer_is_empty(buffer_t *buffer) { return BUFFER_IS_EMPTY(buffer); }
int buffer_is_full(buffer_t *buffer) { return BUFFER_IS_FULL(buffer); }
size_t buffer_tell(buffer_t *buffer) { return BUFFER_TELL(buffer); }
int buffer_eof(buffer_t *buffer) { return BUFFER_EOF(buffer); }
void buffer_rewind(buffer_t *buffer) { BUFFER_REWIND(buffer); }
void buffer_seek(buffer_t *buffer, size_t offset) { BUFFER_SEEK(buffer, offset); }