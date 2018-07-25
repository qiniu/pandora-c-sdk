#ifndef PANDORA_C_CLIENT_H
#define PANDORA_C_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>

#include "buffer.h"
#include "error.h"

#define TRUE 1
#define FALSE 0

typedef struct client_params {
    char *host;
    char *access_key;
    char *secret_key;
    int fail_retry;
} s_client_params;

typedef enum {
    NO_CACHE,
    CACHE_BY_SIZE,
    CACHE_BY_TIME,
} e_cache_policy;

typedef struct cache_control {
    int initialized;

    e_cache_policy policy;
    int threshold;

    char *cachedir;
    FILE *fileptr;
    int filesize;
    char filename[FILENAME_MAX];
    FILE *oldpf;
    char oldfn[FILENAME_MAX];

    unsigned int start;
} s_cache_control;

typedef struct pandora_client {
    pthread_mutex_t mutex;
    s_client_params params;
    s_cache_control cache_control;
} s_pandora_client;

/**
 * Initialize a pandora client with given parameters
 */
s_pandora_client *pandora_client_init(s_client_params *params);

/**
 * Set cache policy for a pandora client (use NO_CACHE as default cache policy)
 */
pandora_error_t pandora_client_set_cache_policy(s_pandora_client *client, e_cache_policy policy, int threshold, char *cachedir);

/**
 * Free resources used by a client
 */
void pandora_client_cleanup(s_pandora_client *client);

typedef struct point_entry
{
    struct curl_slist *fields;
} s_point_entry;

s_point_entry *point_entry_create();
void point_entry_clear(s_point_entry *pentry);
void point_entry_destroy(s_point_entry *pentry);

pandora_error_t point_entry_append_boolean(s_point_entry *pentry, const char *key, int value);
pandora_error_t point_entry_append_int32(s_point_entry *pentry, const char *key, long value);
pandora_error_t point_entry_append_int64(s_point_entry *pentry, const char *key, long long value);
pandora_error_t point_entry_append_float32(s_point_entry *pentry, const char *key, float value);
pandora_error_t point_entry_append_float64(s_point_entry *pentry, const char *key, double value);
pandora_error_t point_entry_append_string(s_point_entry *pentry, const char *key, const char *value);

typedef struct data_points
{
    buffer_t *buf;
    int point_count;
} s_data_points;

s_data_points *data_points_create();
void data_points_clear(s_data_points *data);
void data_points_destroy(s_data_points *data);

pandora_error_t data_points_append(s_data_points *data, s_point_entry *pentry);

/**
 * Write data points to a given pandora repo
 */
pandora_error_t pandora_client_write(s_pandora_client *client, const char *repo, s_data_points *data);

/**
 * Write data points from all cache files under given cache directory
 */
pandora_error_t pandora_client_write_cached(s_pandora_client *client, const char *repo, const char *cachedir);

#ifdef __cplusplus
}
#endif

#endif //PANDORA_C_CLIENT_H
