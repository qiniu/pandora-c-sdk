#ifndef PANDORA_CURL_H
#define PANDORA_CURL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
返回码
*/
typedef enum {
  PANDORA_CURL_OK = 0,            /* 正常 */
  PANDORA_CURL_NOT_INIT = -1,     /* 没有调用 init 函数 */
  PANDORA_CURL_INVALID_DATA = -2, /* 传入错误的数据*/
  PANDORA_CURL_NO_MEMORY = -3     /* 内存不足 */
} PANDORA_CURL_CODE;

typedef struct PANDORA_Points {
  char *buffer;
  int buffer_size;
  int offset;
} PANDORA_Points;

/*
生成打点的数据
*/

PANDORA_Points *pandora_points_new();

void pandora_points_delete(PANDORA_Points *points);

PANDORA_CURL_CODE pandora_points_add_long(PANDORA_Points *points,
                                          const char *key, long long value);

PANDORA_CURL_CODE pandora_points_add_string(PANDORA_Points *points,
                                            const char *key, const char *str);

PANDORA_CURL_CODE pandora_points_add_float(PANDORA_Points *points,
                                           const char *key, double val);

PANDORA_CURL_CODE pandora_points_add_boolean(PANDORA_Points *points,
                                             const char *key, int val);

PANDORA_CURL_CODE pandora_points_add_time(PANDORA_Points *points,
                                          const char *key, long long time);

void pandora_points_newline(PANDORA_Points *points);

/*
发送数据到服务端
repo: repo名称, 不能为NULL
token: 上报的token, 不能为NULL
points: 要打的点, 不能为NULL

return: PANDORA_CURL_CODE 和 CURL error code
*/

PANDORA_CURL_CODE PANDORA_curl_send(const char *repo, const char *token,
                                    PANDORA_Points *points);

#ifdef __cplusplus
}
#endif

#endif
