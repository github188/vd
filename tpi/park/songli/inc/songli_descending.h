#ifndef __SONGLI_DESCEND_H__
#define __SONGLI_DESCEND_H___

#include "mq_module.h"

/*
 * Name:        recv_msg_park_down
 * Description: the callback handler for class_mq_consumer
 *              the portal fun to handler MQ received messages.
 * Paramaters:  const Message *raw_message: Message
 * Return:      void
 */
void recv_msg_park_down(const Message *raw_message);

#endif
