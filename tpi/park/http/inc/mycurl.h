#ifndef __MYCURL_H__
#define __MYCURL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define curl_printf printf

#define CURL_BUF_MAX_LEN  1024
#define CURL_NAME_MAX_LEN 128
#define CURL_URL_MAX_LEN  128

typedef void* (*http_resp_cb)(void*);

enum curl_method
{
    CURL_METHOD_GET  = 1,
    CURL_METHOD_POST = 2,
};

struct curl_http_args_st
{
    int   curl_method;                   // curl 方法命令,enum curl_method
    char* url;                           // URL
    char* post_data;                     // POST 表单数据
    int   post_lens;                     // POST 表单数据
    char* resp_data;                     // 返回的数据
    int   resp_lens;                     // 返回的数据长度
};

/*
   http get func
   */
int curl_http_get(struct curl_http_args_st *args);

/*
   http post func
   */
int curl_http_post(struct curl_http_args_st *args, int post_type,
		void* data = 0, http_resp_cb cb = 0);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MYCURL_H__ */
