#ifndef __HTTP_DESCEND_H__
#define __HTTP_DESCEND_H___

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Name:        recv_msg_park_down
 * Description: the callback handler for http resp handler
 * Paramaters:  const char *raw_message: Message
 * Return:      void
 */
void http_recv_msg_park_down(const char *raw_message);

#ifdef __cplusplus
}
#endif
#endif
