#ifndef PANDORA_C_CLIENT_H
#define PANDORA_C_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PANDORA_C_EXPORTS  
#define PANDORA_C_API __declspec(dllexport)  
#else  
#define PANDORA_C_API
#endif  

#include <curl/curl.h>
#include <time.h>
#include <limits.h>

#include "buffer.h"
#include "error.h"

#define TRUE 1
#define FALSE 0

typedef struct PANDORA_C_API client_params {
    char *pipeline_host;
    char *insight_host;
    char *access_key;
    char *secret_key;
    int fail_retry;
} s_client_params;

typedef struct PANDORA_C_API pandora_client {
    s_client_params params;
} s_pandora_client;

/**
 * Initialize a pandora client with given parameters
 */
PANDORA_C_API s_pandora_client *pandora_client_init(s_client_params *params);

/**
 * Free resources used by a client
 */
PANDORA_C_API void pandora_client_cleanup(s_pandora_client *client);

typedef struct PANDORA_C_API  point_entry {
    struct curl_slist *fields;
} s_point_entry;

PANDORA_C_API s_point_entry *point_entry_create();
PANDORA_C_API void point_entry_clear(s_point_entry *pentry);
PANDORA_C_API void point_entry_destroy(s_point_entry *pentry);

PANDORA_C_API pandora_error_t point_entry_append_boolean(s_point_entry *pentry, const char *key, int value);
PANDORA_C_API pandora_error_t point_entry_append_int32(s_point_entry *pentry, const char *key, long value);
PANDORA_C_API pandora_error_t point_entry_append_int64(s_point_entry *pentry, const char *key, long long value);
PANDORA_C_API pandora_error_t point_entry_append_float32(s_point_entry *pentry, const char *key, float value);
PANDORA_C_API pandora_error_t point_entry_append_float64(s_point_entry *pentry, const char *key, double value);
PANDORA_C_API pandora_error_t point_entry_append_string(s_point_entry *pentry, const char *key, const char *value);

typedef struct PANDORA_C_API data_points {
    buffer_t *buf;
    int point_count;
} s_data_points;

PANDORA_C_API s_data_points *data_points_create();
PANDORA_C_API void data_points_clear(s_data_points *data);
PANDORA_C_API void data_points_destroy(s_data_points *data);

PANDORA_C_API pandora_error_t data_points_append(s_data_points *data, s_point_entry *pentry);

/**
 * Write data points to a given pandora repo
 */
PANDORA_C_API pandora_error_t pandora_client_write(s_pandora_client *client, const char *repo, s_data_points *data);

typedef struct PANDORA_C_API search_params {
    char *query;
    char *sort;
    char *fields;
    int size;
    int from;
} s_search_params;

/**
 * Search logs from pandora insight with given parameters.
 */
PANDORA_C_API pandora_error_t pandora_client_insight_search(s_pandora_client *client, const char *repo, s_search_params *params, char **result);

#ifdef __cplusplus
}
#endif

#endif //PANDORA_C_CLIENT_H
