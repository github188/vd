#ifndef __OSS_INTERFACE__
#define __OSS_INTERFACE__

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ossc/client.h"
#endif

int oss_init();
void oss_deinit();
int oss_put_file(const char* file_name, const char* buf, const unsigned int len);
#if 0
int oss_delete_object(char * full_file_name_oss);

int oss_put_local_file(char * full_file_name_oss, const char *local_file);
int oss_get_object_to_file(char * full_file_name_oss, const char *local_file);

int oss_test_sync_upload(char * dir_to_send);
int oss_generate_presigned_url_with_method(char * full_file_name_oss, char * url);
int oss_get_bucket_list();
#endif

#ifdef __cplusplus
}
#endif
#endif
