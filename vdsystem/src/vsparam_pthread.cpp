#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <json/json.h>
#include <sstream>
#include <errno.h>

#include "interface.h"
#include "vd_msg_queue.h"
#include "Msg_Def.h"
#include "vsparam_parse.h"
#include "commonfuncs.h"
#include "logger/log.h"
#include "dsp_config.h"

using namespace std;

static int send_platform_info(const char* msg, const size_t msg_len)
{
	if ((msg == NULL) || (0 == msg_len)) {
		ERROR("bad param");
		return -EINVAL;
	}

	str_platform_info_msg m = {0};

	m.type = 10; // FIXME dummy data, change it to MSG_PLATFORM_CONFIG.
	m.info.data_lens = msg_len;
	memcpy(m.info.data, msg, msg_len);
	size_t msgsz = sizeof(str_platform_info_msg) - sizeof(long);

	int ret = msgsnd(MSG_PLATFORM_INFO_ID, &m, msgsz, IPC_NOWAIT);
	if(-1 == ret) {
		ERROR("msgsend, errno = %d %s", errno, strerror(errno));
		return -errno;
	}
	return 0;
}

//vd recv message from web
int  vd_recvfrom_web(int mq_key)
{
	int ret = 0;
	str_intelligent_param msg;

	memset(&msg, 0x00, sizeof(str_intelligent_param));

	//recv intellignet param
	size_t msgsz = sizeof(msg) - sizeof(long);
	ret = msgrcv(mq_key, &msg, msgsz, INTELLIGENT_PARAM, 0);
	if (ret <= 0) {
		ERROR("vd recv from boa failed. %s", strerror(errno));
		return -1;
	}

	INFO("vd recv from boa: size = %d  %s", strlen(msg.buf), msg.buf);

	Json::Reader reader;
	Json::Value read_root;
	if (!reader.parse(msg.buf, read_root)) {
		ERROR("parse json %s failed.", msg.buf);
		return -1;
	}

	const string &cmd = read_root["cmd"].asString();
	if ((cmd == "set_platform_info") || (cmd == "get_platform_info")) {
		send_platform_info(msg.buf, strlen(msg.buf));
	} else if (cmd == "set_hide_param") {
		send_hidden_param_to_dsp();
	} else {
		const string &method = read_root["method"].asString();

		if (method == "set") {
			intelligent_jsdecode(msg.buf);
		} else if (method == "get") {
			intelligent_jsparam_encode(msg.buf);
		}
	}
	return ret;
}

//vd system recv param
void *vsparam_pthread(void * arg)
{
	int webvd_key = 0;

    set_thread("vsparam");
	//init web&&vd message queue!
	webvd_key = msgget(WEB_VD_MSG_KEY, IPC_CREAT);

	while(1)
	{
		vd_recvfrom_web(webvd_key);
	}

	return NULL;
}
