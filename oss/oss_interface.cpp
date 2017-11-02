#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "oss_auth.h"
#include "oss_util.h"
#include "oss_api.h"

#include "arm_config.h"
#include "logger/log.h"

#include "oss_interface.h"

extern ARM_config g_arm_config;
static const OSS_ALIYUN_PARAM* p = &(g_arm_config.basic_param.oss_aliyun_param);
static const char* OSS_ENDPOINT = p->url;
static const char* ACCESS_KEY_ID = p->username;
static const char* ACCESS_KEY_SECRET = p->passwd;
static const char* BUCKET_NAME = p->bucket_name;

void init_sample_request_options(oss_request_options_t *options, int is_cname);

int oss_init(void) {
    if (aos_http_io_initialize(NULL, 0) != AOSE_OK) {
        ERROR("oss init failed.");
        return -1;
    }
    return 0;
}

void oss_deinit(void)
{
    aos_http_io_deinitialize();
}

void init_sample_request_options(oss_request_options_t *options, int is_cname)
{
    options->config = oss_config_create(options->pool);
    aos_str_set(&options->config->endpoint, OSS_ENDPOINT);
    aos_str_set(&options->config->access_key_id, ACCESS_KEY_ID);
    aos_str_set(&options->config->access_key_secret, ACCESS_KEY_SECRET);
    options->config->is_cname = is_cname;

    options->ctl = aos_http_controller_create(options->pool, 0);
}

int oss_put_file(const char* file_name, const char* buf, const unsigned int len)
{
    if (NULL == file_name) {
        ERROR("file_name is null.");
        return -1;
    }

    if (NULL == buf) {
        ERROR("buf is null");
        return -1;
    }

    if (len <= 0) {
        ERROR("lenght <= 0.");
        return -1;
    }

    aos_pool_t *p = NULL;
    aos_string_t bucket;
    aos_string_t object;
    int is_cname = 0;
    aos_table_t *headers = NULL;
    aos_table_t *resp_headers = NULL;
    oss_request_options_t *options = NULL;
    aos_list_t buffer;
    aos_buf_t *content = NULL;
    aos_status_t *s = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_sample_request_options(options, is_cname);
    headers = aos_table_make(p, 1);
    apr_table_set(headers, "x-oss-meta-author", "oss");

    aos_str_set(&bucket, BUCKET_NAME);
    aos_str_set(&object, file_name);

    aos_list_init(&buffer);
    content = aos_buf_pack(options->pool, buf, len);
    aos_list_add_tail(&content->node, &buffer);

    s = oss_put_object_from_buffer(options, &bucket, &object,
                   &buffer, headers, &resp_headers);

    INFO("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    if (aos_status_is_ok(s)) {
        DEBUG("put object from buffer succeeded\n");
        aos_pool_destroy(p);
        return 0;
    } else {
        ERROR("put object from buffer failed\n");
        aos_pool_destroy(p);
        return -1;
    }
}

#if 0
void put_object_from_file()
{
    aos_pool_t *p = NULL;
    aos_string_t bucket;
    aos_string_t object;
    int is_cname = 0;
    aos_table_t *headers = NULL;
    aos_table_t *resp_headers = NULL;
    oss_request_options_t *options = NULL;
    char *filename = "SouthIsland_NewZealand.jpg";
    aos_status_t *s = NULL;
    aos_string_t file;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_sample_request_options(options, is_cname);
    headers = aos_table_make(options->pool, 1);
    apr_table_set(headers, OSS_CONTENT_TYPE, "image/jpeg");
    aos_str_set(&bucket, BUCKET_NAME);
    aos_str_set(&object, OBJECT_NAME);
    aos_str_set(&file, filename);

    s = oss_put_object_from_file(options, &bucket, &object, &file,
                                 headers, &resp_headers);

    if (aos_status_is_ok(s)) {
        printf("put object from file succeeded\n");
    } else {
        printf("put object from file failed, code:%d, error_code:%s, error_msg:%s, request_id:%s\n",
            s->code, s->error_code, s->error_msg, s->req_id);
    }

    aos_pool_destroy(p);
}

void get_object_to_buffer()
{
    aos_pool_t *p = NULL;
    aos_string_t bucket;
    aos_string_t object;
    int is_cname = 0;
    oss_request_options_t *options = NULL;
    aos_table_t *headers = NULL;
    aos_table_t *params = NULL;
    aos_table_t *resp_headers = NULL;
    aos_status_t *s = NULL;
    aos_list_t buffer;
    aos_buf_t *content = NULL;
    char *buf = NULL;
    int64_t len = 0;
    int64_t size = 0;
    int64_t pos = 0;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_sample_request_options(options, is_cname);
    aos_str_set(&bucket, BUCKET_NAME);
    aos_str_set(&object, OBJECT_NAME);
    aos_list_init(&buffer);

    s = oss_get_object_to_buffer(options, &bucket, &object,
                                 headers, params, &buffer, &resp_headers);

    if (aos_status_is_ok(s)) {
        printf("get object to buffer succeeded\n");
    }
    else {
        printf("get object to buffer failed\n");
    }

    //get buffer len
    aos_list_for_each_entry(aos_buf_t, content, &buffer, node) {
        len += aos_buf_size(content);
    }

    buf = aos_pcalloc(p, (apr_size_t)(len + 1));
    buf[len] = '\0';

    //copy buffer content to memory
    aos_list_for_each_entry(aos_buf_t, content, &buffer, node) {
        size = aos_buf_size(content);
        memcpy(buf + pos, content->pos, (size_t)size);
        pos += size;
    }

    aos_pool_destroy(p);
}

void get_object_to_file()
{
    aos_pool_t *p = NULL;
    aos_string_t bucket;
    char *download_filename = "SouthIsland_NewZealand_New.jpg";
    aos_string_t object;
    int is_cname = 0;
    oss_request_options_t *options = NULL;
    aos_table_t *headers = NULL;
    aos_table_t *params = NULL;
    aos_table_t *resp_headers = NULL;
    aos_status_t *s = NULL;
    aos_string_t file;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_sample_request_options(options, is_cname);
    aos_str_set(&bucket, BUCKET_NAME);
    aos_str_set(&object, OBJECT_NAME);
    headers = aos_table_make(p, 0);
    aos_str_set(&file, download_filename);

    s = oss_get_object_to_file(options, &bucket, &object, headers,
                               params, &file, &resp_headers);

    if (aos_status_is_ok(s)) {
        printf("get object to local file succeeded\n");
    } else {
        printf("get object to local file failed\n");
    }

    aos_pool_destroy(p);
}

void delete_object()
{
    aos_pool_t *p = NULL;
    aos_string_t bucket;
    aos_string_t object;
    int is_cname = 0;
    oss_request_options_t *options = NULL;
    aos_table_t *resp_headers = NULL;
    aos_status_t *s = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_sample_request_options(options, is_cname);
    aos_str_set(&bucket, BUCKET_NAME);
    aos_str_set(&object, OBJECT_NAME);

    s = oss_delete_object(options, &bucket, &object, &resp_headers);

    if (aos_status_is_ok(s)) {
        printf("delete object succeed\n");
    } else {
        printf("delete object failed\n");
    }

    aos_pool_destroy(p);
}
#endif
