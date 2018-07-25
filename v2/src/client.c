#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "pandora/buffer.h"
#include "pandora/client.h"
#include "crypto.h"
#include "utils.h"

#define PANDORA_DEFAULT_HOST "https://nb-pipeline.qiniuapi.com"
#define PANDORA_URL_MAX_SIZE 256
#define PANDORA_C_USER_AGENT "pandora-c/1.0.1"

#define CLIENT_MAX_BODY_SIZE 2*1024*1024

#define DATA_BUFFER_SIZE 4096
#define AUTH_BUFFER_SIZE 256

s_pandora_client *pandora_client_init(s_client_params *params)
{
    if (!params) {
        fprintf(stderr, "null client params");
        return NULL;
    }

    s_pandora_client *client = malloc(sizeof(s_pandora_client));
    if (!client) {
        fprintf(stderr, "null pandora client");
        return NULL;
    }

    if (params->host)
        client->params.host = pandora_strdup(params->host);
    else
        client->params.host = pandora_strdup(PANDORA_DEFAULT_HOST);
    client->params.access_key = pandora_strdup(params->access_key);
    client->params.secret_key = pandora_strdup(params->secret_key);
    client->params.fail_retry = params->fail_retry;

    client->cache_control.initialized = FALSE;
    client->cache_control.policy = NO_CACHE;
    client->cache_control.threshold = 0;
    client->cache_control.cachedir = ".";
    client->cache_control.fileptr = NULL;
    client->cache_control.filesize = 0;
    client->cache_control.oldpf = NULL;
    client->cache_control.start = UINT_MAX;

    pthread_mutex_init(&client->mutex, NULL);

    return client;
}

void cache_control_do_flush(s_cache_control *ctl)
{
    if (!ctl)
        return;

    if (ctl->fileptr) {
        fflush(ctl->fileptr);
        fclose(ctl->fileptr);
        ctl->fileptr = NULL;
    }

    if (ctl->oldpf) {
        fflush(ctl->oldpf);
        fclose(ctl->oldpf);
        ctl->oldpf = NULL;
    }
}

void pandora_client_cleanup(s_pandora_client *client)
{
    if (client) {
        cache_control_do_flush(&client->cache_control);

        pthread_mutex_destroy(&client->mutex);

        free(client->params.host);
        free(client->params.access_key);
        free(client->params.secret_key);
        free(client);
    }
}

s_point_entry *point_entry_create()
{
    s_point_entry *pentry = malloc(sizeof(s_point_entry));
    if (pentry)
        pentry->fields = NULL;
    return pentry;
}

void point_entry_clear(s_point_entry *pentry)
{
    if (pentry) {
        if (pentry->fields) {
            curl_slist_free_all(pentry->fields);
            pentry->fields = NULL;
        }
    }
}

void point_entry_destroy(s_point_entry *pentry)
{
    if (pentry) {
        if (pentry->fields) {
            curl_slist_free_all(pentry->fields);
            pentry->fields = NULL;
        }
        free(pentry);
    }
}

#define POINT_ENTRY_APPEND_FIELD(pentry, key, value, format, valuelen) \
    size_t bytes = strlen(key) + valuelen + 2; \
    char *field = malloc(bytes); \
    if (!field) \
        return PANDORAE_OUT_OF_MEMORY; \
    snprintf(field, bytes, format, key, value); \
    pentry->fields = curl_slist_append(pentry->fields, field); \
    free(field); \
    return PANDORAE_OK

pandora_error_t point_entry_append_boolean(s_point_entry *pentry, const char *key, int value)
{
    POINT_ENTRY_APPEND_FIELD(pentry, key, value ? "true" : "false", "%s=%s", 4);
}

pandora_error_t point_entry_append_int32(s_point_entry *pentry, const char *key, long value)
{
    POINT_ENTRY_APPEND_FIELD(pentry, key, value, "%s=%ld", 11);
}

pandora_error_t point_entry_append_int64(s_point_entry *pentry, const char *key, long long value)
{
    POINT_ENTRY_APPEND_FIELD(pentry, key, value, "%s=%lld", 20);
}

pandora_error_t point_entry_append_float32(s_point_entry *pentry, const char *key, float value)
{
    POINT_ENTRY_APPEND_FIELD(pentry, key, value, "%s=%f", 32);
}

pandora_error_t point_entry_append_float64(s_point_entry *pentry, const char *key, double value)
{
    POINT_ENTRY_APPEND_FIELD(pentry, key, value, "%s=%lf", 48);
}

pandora_error_t point_entry_append_string(s_point_entry *pentry, const char *key, const char *value)
{
    POINT_ENTRY_APPEND_FIELD(pentry, key, value, "%s=%s", strlen(value));
}

s_data_points *data_points_create()
{
    buffer_t *buf;
    s_data_points *data;

    buf = buffer_create(DATA_BUFFER_SIZE, BUFFER_OWNS_SELF | BUFFER_OWNS_DATA | BUFFER_GROWABLE);
    if (!buf)
        return NULL;

    data = malloc(sizeof(s_data_points));
    if (!data) {
        buffer_destroy(buf);
        return NULL;
    }
    data->buf = buf;
    data->point_count = 0;

    return data;
}

void data_points_clear(s_data_points *data)
{
    if (!data || !data->buf)
        return;

    buffer_reset(data->buf);
    data->point_count = 0;
}

void data_points_destroy(s_data_points *data)
{
    if (data) {
        if (data->buf) {
            buffer_destroy(data->buf);
            data->buf = NULL;
            data->point_count = 0;
        }
        free(data);
    }
}

pandora_error_t data_points_append(s_data_points *data, s_point_entry *pentry)
{
    if (!data || !data->buf || !pentry || !pentry->fields)
        return PANDORAE_INVALID_ARGUMENT;

    struct curl_slist *item = pentry->fields;
    while (item) {
        buffer_write(data->buf, item->data, strlen(item->data));
        item = item->next;
        if (item)
            buffer_append(data->buf, '\t');
    }
    buffer_append(data->buf, '\n');

    data->point_count++;

    return PANDORAE_OK;
}

pandora_error_t data_points_append_string(s_data_points *data, const char *str)
{
    if (!data || !data->buf || !str)
        return PANDORAE_INVALID_ARGUMENT;

    buffer_write(data->buf, str, strlen(str));

    data->point_count++;

    return PANDORAE_OK;
}

char *data_points_to_string(s_data_points *data)
{
    if (!data)
        return NULL;

    return data->buf->data;
}

size_t data_points_length(s_data_points *data)
{
    if (!data || !data->buf)
        return 0;

    return data->buf->written;
}

int data_points_count(s_data_points *data)
{
    if (!data || !data->buf)
        return 0;

    return data->point_count;
}

int pandora_client_curl(const char *url, struct curl_slist *headers, s_data_points *data)
{
    CURLcode c;
    CURL *handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    size_t data_len = data_points_length(data);
    if (data_len > 0) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, data_points_length(data));
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data_points_to_string(data));
    }

    c = curl_easy_perform(handle);
    if (c == CURLE_OK) {
        long status_code = 0;
        if (curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status_code) == CURLE_OK)
            c = status_code;
    }

    curl_easy_cleanup(handle);

    return c;
}

void add_request_headers(s_pandora_client *client, const char *repo, struct curl_slist **headers)
{
    char date[36];
    char signstr[128];

    char *pmt = current_gmt();
    snprintf(date, 36, "Date: %s", pmt);
    snprintf(signstr, 128, "POST\n\ntext/plain\n%s\n/v2/repos/%s/data", pmt, repo);
    free(pmt);

    unsigned char hmac[20];
    char b64[((20 + 1) * 4) / 3 +1];
    char auth[AUTH_BUFFER_SIZE];

    hmac_sha1(hmac, (unsigned char *)client->params.secret_key, strlen(client->params.secret_key),
              (unsigned char *)signstr, strlen(signstr));
    base64_encode(hmac, 20, b64);
    snprintf(auth, AUTH_BUFFER_SIZE, "Authorization: Pandora %s:%s", client->params.access_key, b64);

    *headers = curl_slist_append(*headers, "Content-Type: text/plain");
    *headers = curl_slist_append(*headers, date);
    *headers = curl_slist_append(*headers, "User-Agent: " PANDORA_C_USER_AGENT);
    *headers = curl_slist_append(*headers, auth);
    *headers = curl_slist_append(*headers, "Expect:");
    *headers = curl_slist_append(*headers, "Accept-Encoding: gzip");
}

pandora_error_t cache_control_create_tmpfile(s_cache_control *ctl)
{
    if (!ctl)
        return PANDORAE_INVALID_ARGUMENT;

    if (ctl->fileptr) {
        fflush(ctl->fileptr);
        rewind(ctl->fileptr);

        ctl->oldpf = ctl->fileptr;
        memcpy(ctl->oldfn, ctl->filename, strlen(ctl->filename));
        ctl->fileptr = NULL;
    }

    time_t rawtime;
    struct tm *ptm;

gen_filename:
    time (&rawtime);
    ptm = gmtime (&rawtime);
    if (ctl->cachedir[strlen(ctl->cachedir) - 1] == '/') {
        snprintf(ctl->filename, FILENAME_MAX, "%scache.%02d%02d%02d%02d%02d", ctl->cachedir,
                 ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    } else {
        snprintf(ctl->filename, FILENAME_MAX, "%s/cache.%02d%02d%02d%02d%02d", ctl->cachedir,
                 ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    }
    if (strcmp(ctl->filename, ctl->oldfn) == 0) {
        sleep(1);
        goto gen_filename;
    }

    ctl->fileptr = fopen(ctl->filename, "w+");
    if (!ctl->fileptr) {
        perror("fopen");
        return PANDORAE_CREATE_CACHE;
    }
    ctl->filesize = 0;

    return PANDORAE_OK;
}

pandora_error_t cache_control_delete_tmpfile(s_cache_control *ctl)
{
    if (!ctl)
        return PANDORAE_INVALID_ARGUMENT;

    if (ctl->oldpf) {
        fclose(ctl->oldpf);
        ctl->oldpf = NULL;
        int ret = remove(ctl->oldfn);
        if (ret == -1) {
            fprintf(stderr, "remove cache file %s failed\n", ctl->oldfn);
            return PANDORAE_DELETE_CACHE;
        }
        fprintf(stdout, "remove cache file %s successfully\n", ctl->oldfn);
    }

    return PANDORAE_OK;
}

pandora_error_t pandora_client_set_cache_policy(s_pandora_client *client, e_cache_policy policy, int threshold, char *cachedir)
{
    if (!client)
        return PANDORAE_INVALID_CLIENT;

    if (client->cache_control.initialized)
        return PANDORAE_CACHE_POLICY_INIT;

    client->cache_control.policy = policy;
    client->cache_control.threshold = threshold;

    DIR *dirp = opendir(cachedir);
    if (!dirp) {
        if (mkdir(cachedir, 0777)) {
            fprintf(stderr, "cannot create cache directory %s automatically\n", cachedir);
            return PANDORAE_NO_CACHE_DIR;
        }
        fprintf(stdout, "create cache directory %s automatically\n", cachedir);
    }
    closedir(dirp);

    client->cache_control.cachedir = cachedir;

    if (policy == CACHE_BY_SIZE || policy == CACHE_BY_TIME) {
        client->cache_control.start = (unsigned int)time(NULL);

        pandora_error_t status = cache_control_create_tmpfile(&client->cache_control);
        if (status != PANDORAE_OK)
            return status;
    }

    client->cache_control.initialized = TRUE;

    return PANDORAE_OK;
}

int cache_control_need_flush(s_cache_control *ctl, size_t delta)
{
    unsigned int now, elapsed;

    switch (ctl->policy) {
        case CACHE_BY_SIZE:
            if (ctl->filesize+delta >= ctl->threshold) {
                fprintf(stderr, "need_flush(by_size): %lu\n", ctl->filesize+delta);
                return TRUE;
            }

            break;

        case CACHE_BY_TIME:
            now = (unsigned int)time(NULL);
            elapsed = now - ctl->start;
            if (elapsed >= ctl->threshold) {
                ctl->start = now;
                fprintf(stderr, "need_flush(by_time): %d\n", elapsed);
                return TRUE;
            }

            break;

        default:
            return FALSE;
    }

    return FALSE;
}

typedef struct write_context {
    const char *url;
    const char *repo;
    s_data_points *data;
} s_write_context;

int do_write_should_retry(CURLcode code)
{
    if (code >= (CURLcode)500) {
        return TRUE;
    }

    switch (code) {
        case CURLE_OPERATION_TIMEDOUT:
        case CURLE_WRITE_ERROR:
        case CURLE_COULDNT_CONNECT:
            return TRUE;
        default:
            return FALSE;
    }
}

pandora_error_t pandora_client_do_write(s_pandora_client *client, s_write_context *ctx)
{
    int retry = 0;
    struct curl_slist *headers = NULL;

do_write:
    add_request_headers(client, ctx->repo, &headers);
    int code = pandora_client_curl(ctx->url, headers, ctx->data);
    if (do_write_should_retry(code)) {
        curl_slist_free_all(headers);
        headers = NULL;

        if (++retry < client->params.fail_retry) {
            fprintf(stderr, "write failed after %d retry: %s", retry, curl_easy_strerror(code));
            sleep(2);
            goto do_write;
        } else {
            fprintf(stderr, "up to max fail retry %d: %s\n", client->params.fail_retry, curl_easy_strerror(code));
            return PANDORAE_WRITE_FAILED;
        }
    }

    curl_slist_free_all(headers);
    return code;
}

pandora_error_t pandora_client_do_cache(s_pandora_client *client, s_write_context *ctx)
{
    char *ptr;
    size_t bytes;

    if (!client->cache_control.fileptr)
        return PANDORAE_WRITE_CACHE;

    ptr = data_points_to_string(ctx->data);
    bytes = data_points_length(ctx->data);
    if (!ptr || bytes == 0)
        return PANDORAE_INVALID_ARGUMENT;

    fwrite(ptr, 1, bytes, client->cache_control.fileptr);
    client->cache_control.filesize += bytes;

    return PANDORAE_OK;
}

pandora_error_t pandora_client_write(s_pandora_client *client, const char *repo, s_data_points *data) {
    size_t data_len = data_points_length(data);
    if (data_len == 0)
        return PANDORAE_OK;

    int status;
    char url[PANDORA_URL_MAX_SIZE];
    snprintf(url, PANDORA_URL_MAX_SIZE, "%s/v2/repos/%s/data", client->params.host, repo);

    s_write_context ctx = {
        .url= url,
        .repo = repo,
        .data = data,
    };

    pthread_mutex_lock(&client->mutex);

    if (client->cache_control.policy == NO_CACHE)
        goto do_write;
    else if (cache_control_need_flush(&client->cache_control, data_len))
        goto do_flush;
    else
        goto do_cache;

do_write:
    status = pandora_client_do_write(client, &ctx);
    pthread_mutex_unlock(&client->mutex);
    return status;

do_flush:
    cache_control_create_tmpfile(&client->cache_control);

    status = pandora_client_do_cache(client, &ctx);
    if (status != PANDORAE_OK) {
        fprintf(stderr, "cache failed with status: %d\n", status);
    }

    char buf[128*1024];
    s_data_points *tmpdata = data_points_create();

    while (!feof(client->cache_control.oldpf)) {
        if (fgets(buf, 128*1024, client->cache_control.oldpf) == NULL)
            break;

        data_points_append_string(tmpdata, buf);
        if (data_points_length(tmpdata) >= CLIENT_MAX_BODY_SIZE) {
            ctx.data = tmpdata;
            pandora_client_do_write(client, &ctx);
            data_points_clear(tmpdata);
        }
    }

    if (data_points_count(tmpdata) > 0) {
        ctx.data = tmpdata;
        pandora_client_do_write(client, &ctx);
    }
    data_points_destroy(tmpdata);

    status = cache_control_delete_tmpfile(&client->cache_control);
    pthread_mutex_unlock(&client->mutex);
    return status;

do_cache:
    status = pandora_client_do_cache(client, &ctx);
    pthread_mutex_unlock(&client->mutex);
    return status;
}

pandora_error_t do_write_from_file(s_pandora_client *client, FILE *fp, s_write_context *ctx)
{
    if (!client || !fp)
        return PANDORAE_INVALID_ARGUMENT;

    char buf[128*1024];
    s_data_points *tmpdata = data_points_create();

    while (!feof(fp)) {
        if (fgets(buf, 128*1024, fp) == NULL)
            break;

        data_points_append_string(tmpdata, buf);
        if (data_points_length(tmpdata) >= CLIENT_MAX_BODY_SIZE) {
            ctx->data = tmpdata;
            pandora_client_do_write(client, ctx);
            data_points_clear(tmpdata);
        }
    }

    if (data_points_count(tmpdata) > 0) {
        ctx->data = tmpdata;
        pandora_client_do_write(client, ctx);
    }
    data_points_destroy(tmpdata);

    return PANDORAE_OK;
}

pandora_error_t pandora_client_write_cached(s_pandora_client *client, const char *repo, const char *cachedir)
{
    DIR *dirp = opendir(cachedir);
    if (!dirp) {
        fprintf(stderr, "cache directory %s not exist", cachedir);
        return PANDORAE_NO_CACHE_DIR;
    }

    int status;
    char url[PANDORA_URL_MAX_SIZE];
    snprintf(url, PANDORA_URL_MAX_SIZE, "%s/v2/repos/%s/data", client->params.host, repo);

    struct dirent *direntp;
    while ((direntp = readdir(dirp)) != NULL) {
        if (strcmp(direntp->d_name, ".") == 0 ||
            strcmp(direntp->d_name, "..") == 0)
            continue;

        char filepath[PATH_MAX];
        if (cachedir[strlen(cachedir) - 1] == '/') {
            snprintf(filepath, PATH_MAX, "%s%s", cachedir, direntp->d_name);
        } else {
            snprintf(filepath, PATH_MAX, "%s/%s", cachedir, direntp->d_name);
        }

        if (strcmp(filepath, client->cache_control.filename) == 0 ||
            strcmp(filepath, client->cache_control.oldfn) == 0)
            continue;

        FILE *fp = fopen(filepath, "r");
        if (!fp) {
            fprintf(stderr, "could not open cache file: %s", filepath);
            return PANDORAE_READ_CACHE;
        }
        fprintf(stdout, "begin to read from cache file %s...\n", filepath);

        s_write_context ctx = { .url = url, .repo = repo, .data = NULL };
        status = do_write_from_file(client, fp, &ctx);
        if (status != PANDORAE_OK) {
            fprintf(stderr, "write failed with status: %d", status);
            return PANDORAE_WRITE_FAILED;
        }

        fprintf(stdout, "cache file %s read done\n", filepath);
        fclose(fp);
        status = remove(filepath);
        if (status == -1) {
            fprintf(stderr, "could not delete cache file: %s", filepath);
            return PANDORAE_DELETE_CACHE;
        }
    }

    closedir(dirp);

    return PANDORAE_OK;
}