/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/param.h>
#include <getopt.h>
#include <curl/curl.h>
#include <openssl/crypto.h>

#include "logger/log.h"
#include "mycurl.h"
#include "http_common.h"

extern const char* jwt_encode(const char* space_code, const char* subject);

/* we have this global to let the callback get easy access to it */
static pthread_mutex_t *lockarray;

static void lock_callback(int mode, int type, const char *file, int line)
{
    (void)file;
    (void)line;
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(lockarray[type]));
    }
    else {
        pthread_mutex_unlock(&(lockarray[type]));
    }
}

static unsigned long thread_id(void)
{
    unsigned long ret;
    ret = (unsigned long)pthread_self();
    return ret;
}

void init_locks(void)
{
    int i;
    lockarray = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() *
            sizeof(pthread_mutex_t));
    for (i=0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_init(&(lockarray[i]), NULL);
    }

    CRYPTO_set_id_callback((unsigned long (*)())thread_id);
    CRYPTO_set_locking_callback(lock_callback);
}

void kill_locks(void)
{
    int i;

    CRYPTO_set_locking_callback(NULL);
    for (i=0; i< CRYPTO_num_locks(); i++)
        pthread_mutex_destroy(&(lockarray[i]));

    OPENSSL_free(lockarray);
}


size_t curl_write_data_cb(void *buffer, size_t size, size_t nmemb, void *stream)
{
	size_t realsize = size * nmemb;
    struct curl_http_args_st *args = (struct curl_http_args_st*)stream;

	char* p = (char*)realloc(args->resp_data, args->resp_lens + realsize + 1);
	if (p == NULL) {
		ERROR("not enough memory (realloc returned NULL)");
		return 0;
	}
	args->resp_data = p;

	memcpy(&(args->resp_data[args->resp_lens]), buffer, realsize);
	args->resp_lens += realsize;
	args->resp_data[args->resp_lens] = 0;
	return realsize;
}

/*
   http get func
   */
int curl_http_get(struct curl_http_args_st *args)
{
    //创建curl对象
    CURL *curl;
    CURLcode return_code;
    int ret = -1;

    //curl初始化
    curl = curl_easy_init();
    if (!curl)
    {
        curl_printf("%s[%d]: curl easy init failed\n", __FUNCTION__, __LINE__);
        return ret;;
    }

    if (strncmp(args->url, "https://", 8) == 0)
    {
#if 1
        // 方法1, 设定为不验证证书和HOST
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#else
        // 方法2, 设定一个SSL判别证书, 未测试
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        // TODO: 设置一个证书文件
        curl_easy_setopt(curl,CURLOPT_CAINFO,"ca-cert.pem");
#endif
    }

    //设置httpheader 解析, 不需要将HTTP头写传入回调函数
    curl_easy_setopt(curl,CURLOPT_HEADER, 0);
    //设置远端地址
    curl_easy_setopt(curl, CURLOPT_URL,args->url);
    // TODO: 打开调试信息
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    //设置允许302  跳转
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
    //执行写入文件流操作 的回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);
    // 设置回调函数的第4 个参数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, args);
    //设置为ipv4类型
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    //设置连接超时，单位s, CURLOPT_CONNECTTIMEOUT_MS 毫秒
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    // 整个CURL 执行的时间, 单位秒, CURLOPT_TIMEOUT_MS毫秒
    // curl_easy_setopt(curl,CURLOPT_TIMEOUT, 5);
    //linux多线程情况应注意的设置(防止curl被alarm信号干扰)
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    return_code = curl_easy_perform(curl);
    if (CURLE_OK != return_code) {
        curl_printf("curl_easy_perform() failed: %s\n",
                curl_easy_strerror(return_code));
        ret  = 0;
    }

    curl_easy_cleanup(curl);

    return ret;
}


/*
   http post func
   */
int curl_http_post(struct curl_http_args_st *args, int post_type, void* data, http_resp_cb cb)
{
    //创建curl对象
    CURL *curl;
    CURLcode return_code;
    struct curl_httppost *formpost = NULL;  // POST 需要的参数
    struct curl_httppost *lastptr  = NULL;
    int ret = -1;

    //curl初始化
    curl = curl_easy_init();
    if (!curl)
    {
        ERROR("curl easy init failed.");
        return ret;;
    }

    if (strncmp(args->url, "https://", 8) == 0)
    {
#if 1
        // 方法1, 设定为不验证证书和HOST
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#else
        // 方法2, 设定一个SSL判别证书
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        // TODO: 设置一个证书文件
        curl_easy_setopt(curl,CURLOPT_CAINFO,"ca-cert.pem");
#endif
    }
    //设置httpheader 解析, 不需要将HTTP头写传入回调函数
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    //设置远端地址
    curl_easy_setopt(curl, CURLOPT_URL,args->url);
    // TODO: 打开调试信息
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    //设置允许302  跳转
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
    //执行写入文件流操作 的回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);
    // 设置回调函数的第4 个参数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, args);
    //设备为ipv4类型
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    //设置连接超时，单位s, CURLOPT_CONNECTTIMEOUT_MS 毫秒
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    // 整个CURL 执行的时间, 单位秒, CURLOPT_TIMEOUT_MS毫秒
    curl_easy_setopt(curl,CURLOPT_TIMEOUT, 60);
    //linux多线程情况应注意的设置(防止curl被alarm信号干扰)
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

#if 1 /* enable JWT */
    /* construct the JWT(Json Web Token) */
    const char* space_code = (const char*)data;
    /* NOTE jwt should be free */
    const char* jwt = jwt_encode(space_code, args->url);

    char* authorization_header =
        (char*)malloc(strlen(jwt) + 1 + strlen("Authorization: Bearer "));
    if (NULL == authorization_header)
        return -1;

    sprintf(authorization_header, "Authorization: Bearer %s", jwt);
    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, authorization_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
#endif

	struct curl_slist *c = NULL;

	args->resp_data = (char*)malloc(1);
	args->resp_lens = 0;

    /* set Authorization header */
    if (post_type == 1)
    {
        // 方法1, 普通的POST , application/x-www-form-urlencoded
        // 设置 为POST 方法
        curl_easy_setopt(curl,CURLOPT_POST, 1);

		const char *ct = "Content-Type: application/json;charset=UTF-8";
		c = curl_slist_append(c, ct);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, c);

        // POST 的数据内容
        curl_easy_setopt(curl,CURLOPT_POSTFIELDS, args->post_data);
        // POST的数据长度, 可以不要此选项
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(args->post_data));
    }
    else if (post_type == 2)
    {
        body_data_t *post_data = (body_data_t*)(args->post_data);
        INFO("%s %d", post_data->json_data, post_data->json_lens);
        curl_formadd(&formpost, &lastptr,
                     CURLFORM_PTRNAME, "json",
                     CURLFORM_CONTENTTYPE, "application/json;charset=UTF-8",
                     CURLFORM_PTRCONTENTS, post_data->json_data,
                     CURLFORM_CONTENTSLENGTH, post_data->json_lens,
                     CURLFORM_END);
        if (post_data->pic != NULL) {
            for (auto i = 0; i < 2; i++) {
                if ((strcmp(post_data->pic[i].name, "") == 0) ||
                    (post_data->pic[i].size <= 0) ||
					(post_data->pic[i].buf == NULL)){
                    continue;
                }

                DEBUG("pic[%d].name %s", i, post_data->pic[i].name);
                DEBUG("pic[%d].size %d", i, post_data->pic[i].size);
                curl_formadd(&formpost, &lastptr,
                             CURLFORM_PTRNAME, "file",
                             //CURLFORM_BUFFER, err_file,
                             CURLFORM_BUFFER, post_data->pic[i].name,
                             CURLFORM_CONTENTTYPE, "image/jpeg",
                             CURLFORM_BUFFERPTR, post_data->pic[i].buf,
                             CURLFORM_BUFFERLENGTH, post_data->pic[i].size,
                             CURLFORM_END);
            }
        }
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    }

    return_code = curl_easy_perform(curl);
    if (CURLE_OK != return_code)
    {
        ERROR("curl_easy_perform fail: %s\n", curl_easy_strerror(return_code));
        ret = -1;
    }

	long retcode = 0;
	return_code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode);
	if ((CURLE_OK == return_code) && (retcode == 200)) {
		long length = 0;
		return_code = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length);
        INFO("\n%s\n--------\n%s\n--------\n%d\n--------",
				args->url, args->resp_data, args->resp_lens);
		if (cb != NULL) {
			cb(args);
		}
        ret = 0;
	}

    curl_easy_cleanup(curl);

	if (post_type == 1) {
		curl_slist_free_all(c);
	}
    if (post_type == 2)   // 用这两种方法需要释放POST数据.
        curl_formfree(formpost);
	free(args->resp_data);

#if 1 /* enable JWT */
	if (jwt != NULL)
		free((void*)jwt);
	if (authorization_header != NULL)
		free(authorization_header);

	curl_slist_free_all(chunk);
#endif

    return ret;
}

///*
//   1, 从参数中传入操作选项.
//   2. 若在线程中要用到HTTPS , 请参看allexamples/threaded-ssl.c 文件使用
//   */
//int main(int argc, char **argv)
//{
//    struct curl_http_args_st curl_args;
//    memset(&curl_args, 0x00, sizeof(curl_args));
//
//    /* Must initialize libcurl before any threads are started */
//    curl_global_init(CURL_GLOBAL_ALL);
//    /* 多线程使用SSL时, 需要先初始化锁*/
//    init_locks();
//
//#if 1 // GET
//    curl_args.curl_method = CURL_METHOD_GET;
//    strncpy(curl_args.url, "http://teyiting.com/api/leaguer/queryParkConsultFee.do?parkCode=000001_000004", sizeof(curl_args.url)); // https test ok
//#endif
//
//#if 0 // POST
//    curl_args.curl_method = CURL_METHOD_POST;
//    strncpy(curl_args.url, "http://www.wx.com:8080/test.php", sizeof(curl_args.url));
//    strncpy(curl_args.file_name, "/tmp/curl/index.html", sizeof(curl_args.file_name));
//    strncpy(curl_args.post_data, "aa=111111111111111", sizeof(curl_args.post_data)); // 普通post 1 ok
//    // strncpy(curl_args.post_file, "./xx.mp4", sizeof(curl_args.post_file)); // POST 文件OK , 用方法3
//    strncpy(curl_args.post_file, "./post_file.txt", sizeof(curl_args.post_file)); // POST 文件OK
//#endif
//
//
//    switch(curl_args.curl_method)
//    {
//        case CURL_METHOD_GET:
//            {
//                curl_http_get(&curl_args);
//                break;
//            }
//        case CURL_METHOD_POST:
//            {
//                curl_http_post(&curl_args);
//                break;
//            }
//        default:
//            {
//                curl_printf("curl method error:%d\n", curl_args.curl_method);
//                break;
//            }
//    }
//    /* 退出时, 释放锁*/
//    kill_locks();
//    return 0;
//}
