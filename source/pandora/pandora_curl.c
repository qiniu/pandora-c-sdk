#include "pandora_curl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>

static char* g_domain="http://pipeline.qiniu.com";

#define SDK_VERSION "0.0.2"

static const char* g_UA = "PANDORA-C/"SDK_VERSION;

#define BUFFER_SIZE 32*1024

PANDORA_Points* pandora_points_new(){
    PANDORA_Points* p = (PANDORA_Points*)malloc(sizeof(PANDORA_Points));
    if (p == NULL) {
        return NULL;
    }
    memset(p, 0, sizeof(PANDORA_Points));
    p->buffer = (char*)malloc(BUFFER_SIZE);
    if (p->buffer == NULL) {
        free(p);
        return NULL;
    }
    p->buffer_size = BUFFER_SIZE;
    return p;
}

void pandora_points_delete(PANDORA_Points* points){
    if (points == NULL) {
        return;
    }
    if (points->buffer != NULL) {
        free(points->buffer);
    }
    free(points);
}

static PANDORA_CURL_CODE check_and_enlarge(PANDORA_Points* points, const char* key, int size){
    int l = strlen(key);
    if (points->buffer_size - points->offset < size+l+2) {
        void* x = malloc(points->buffer_size * 2);
        if (x == NULL) {
            return PANDORA_CURL_NO_MEMORY;
        }
        memset(x, 0, points->buffer_size * 2);
        memcpy(x, points->buffer, points->offset);
        free(points->buffer);
        points->buffer = x;
    }
    return PANDORA_CURL_OK;
}

static void add_end(PANDORA_Points* points, int l){
    points->offset += l+1;
    points->buffer[points->offset-1] = '\t';
}

PANDORA_CURL_CODE pandora_points_add_long(PANDORA_Points* points, const char* key, long long value){
    int code = check_and_enlarge(points, key, 20);
    if (code != PANDORA_CURL_OK) {
        return code;
    }
    int l = snprintf(points->buffer + points->offset, points->buffer_size - points->offset, "%s=%lld", key, value);
    add_end(points, l);
    return PANDORA_CURL_OK;
}

PANDORA_CURL_CODE pandora_points_add_string(PANDORA_Points* points, const char* key, const char* value){
    int sl = strlen(value);
    int code = check_and_enlarge(points, key, sl);
    if (code != PANDORA_CURL_OK) {
        return code;
    }
    int l = snprintf(points->buffer + points->offset, points->buffer_size - points->offset, "%s=%s", key, value);
    add_end(points, l);
    return PANDORA_CURL_OK;
}

PANDORA_CURL_CODE pandora_points_add_float(PANDORA_Points* points, const char* key, double value){
    int code = check_and_enlarge(points, key, 30);
    if (code != PANDORA_CURL_OK) {
        return code;
    }
    int l = snprintf(points->buffer + points->offset, points->buffer_size - points->offset, "%s=%lf", key, value);
    add_end(points, l);
    return PANDORA_CURL_OK;
}

PANDORA_CURL_CODE pandora_points_add_boolean(PANDORA_Points* points, const char* key, int value){
    int code = check_and_enlarge(points, key, 6);
    if (code != PANDORA_CURL_OK) {
        return code;
    }
    char* b = "true";
    if (value == 0) {
        b = "false";
    }
    int l = snprintf(points->buffer + points->offset, points->buffer_size - points->offset, "%s=%s", key, b);
    add_end(points, l);
    return PANDORA_CURL_OK;
}

PANDORA_CURL_CODE pandora_points_add_time(PANDORA_Points* points, const char* key, long long time){
    int code = check_and_enlarge(points, key, 25);
    if (code != PANDORA_CURL_OK) {
        return code;
    }
    time_t t = (time_t) time;
    struct tm *stm = gmtime(&t);
    struct tm tm2 = *stm;

    int l = snprintf(points->buffer + points->offset, points->buffer_size - points->offset, "%s=%04d-%02d-%02dT%02d:%02d:%02dZ",
     key, tm2.tm_year+1900, tm2.tm_mon+1, tm2.tm_mday, tm2.tm_hour, tm2.tm_min, tm2.tm_sec);
    add_end(points, l);
    return PANDORA_CURL_OK;
}

void pandora_points_newline(PANDORA_Points* points){
    points->buffer[points->offset-1] = '\n';
}

/*
发送数据到服务端
repo: repo名称, 不能为NULL
token: 上报的token, 不能为NULL
points: 要打的点, 不能为NULL

return: PANDORA_CURL_CODE 和 CURL error code
*/

PANDORA_CURL_CODE PANDORA_curl_send(const char *repo,
                                        const char *token,
                                        PANDORA_Points* points) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *list = NULL;
    char url_buff[512];
    char token_buff[512];
    long retcode = PANDORA_CURL_CODE_DEFAULT;

    if (repo == NULL || token == NULL || points == NULL) {
        return PANDORA_CURL_INVALID_DATA;
    }

    /* get a curl handle */ 
    curl = curl_easy_init();
    if (curl == NULL ) {
        return PANDORA_CURL_NO_MEMORY;
    }

    memset(url_buff, 0, sizeof(url_buff));

    snprintf(url_buff, sizeof(url_buff), "%s/v2/repos/%s/data", g_domain, repo);

    curl_easy_setopt(curl, CURLOPT_URL, url_buff);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, g_UA);

    list = curl_slist_append(list, "Content-Type: text/plain");
    snprintf(token_buff, sizeof(token_buff), "Authorization: %s", token);
    list = curl_slist_append(list, token_buff);
 
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    pandora_points_newline(points);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, points->offset);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, points->buffer);

    res = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);

    curl_easy_cleanup(curl);

    curl_slist_free_all(list);
 
    if(res != CURLE_OK) {
        return (PANDORA_CURL_CODE)res;
    }
    if (retcode != 200) {
        return retcode;
    }

    return PANDORA_CURL_OK;
}

static PANDORA_CURL_CODE read_file(const char* file_path, char** buffer, int* buffer_len) {
    FILE *fp;
    int flen;
    char *p;

    if((fp = fopen(file_path,"r"))==NULL) {
        return PANDORA_CURL_NO_FILE;
    }
    fseek(fp, 0L, SEEK_END);
    flen=ftell(fp);
    p=(char *)malloc(flen);
    if(p == NULL) {
        fclose(fp);
        return PANDORA_CURL_NO_MEMORY;
    }
    fseek(fp, 0L, SEEK_SET);
    fread(p, flen, 1, fp);
    fclose(fp);
    *buffer = p;
    *buffer_len = flen;
    return PANDORA_CURL_OK;
}

/*
发送数据到服务端
repo: repo名称, 不能为NULL
token: 上报的token, 不能为NULL
file_path: 要上传的小文件，不能大于2M

return: PANDORA_CURL_CODE 和 CURL error code
*/

PANDORA_CURL_CODE PANDORA_curl_send_file(const char *repo,
                                        const char *token,
                                        const char* file_path) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *list = NULL;
    char url_buff[512];
    char token_buff[512];
    PANDORA_CURL_CODE file_status = 0;
    long retcode = PANDORA_CURL_CODE_DEFAULT;

    char* file_buffer = NULL;
    int buffer_len = 0;

    if (repo == NULL || token == NULL || file_path == NULL) {
        return PANDORA_CURL_INVALID_DATA;
    }

    /* get a curl handle */ 
    curl = curl_easy_init();
    if (curl == NULL ) {
        return PANDORA_CURL_NO_MEMORY;
    }


    file_status = read_file(file_path, &file_buffer, &buffer_len);
    if (file_status != PANDORA_CURL_OK) {
        curl_easy_cleanup(curl);
        return file_status;
    }

    memset(url_buff, 0, sizeof(url_buff));

    snprintf(url_buff, sizeof(url_buff), "%s/v2/repos/%s/data", g_domain, repo);

    curl_easy_setopt(curl, CURLOPT_URL, url_buff);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, g_UA);

    list = curl_slist_append(list, "Content-Type: text/plain");
    snprintf(token_buff, sizeof(token_buff), "Authorization: %s", token);
    list = curl_slist_append(list, token_buff);
 
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, buffer_len);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, file_buffer);

    res = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);

    curl_easy_cleanup(curl);

    curl_slist_free_all(list);
    free(file_buffer);

    if(res != CURLE_OK) {
        return (PANDORA_CURL_CODE)res;
    }

    if (retcode != 200) {
        return retcode;
    }

    return PANDORA_CURL_OK;
}
