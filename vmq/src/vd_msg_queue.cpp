#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <errno.h>
#include "logger/log.h"
#include "bitcom_model_global.h"

#include "vd_msg_queue.h"

C_CODE_BEGIN

int gi_msg_platform_info_id = -1;
int gi_msg_alleyway_id = -1;
int gi_msg_vm_id = -1;
int gi_msg_vp_id = -1;
int gi_msg_park_id = -1;

int vd_msgueue_init()
{
#if(5 == DEV_TYPE)
	//Create alleyway message queue
	gi_msg_alleyway_id = msgget(3000, IPC_EXCL);
	if(gi_msg_alleyway_id < 0)
	{
		gi_msg_alleyway_id = msgget(3000, IPC_CREAT|0666);
		if(gi_msg_alleyway_id < 0)
		{
			return -1;
		}
	}
#elif (2 == DEV_TYPE)
	//Create vm message queue
	gi_msg_vm_id = msgget(3001, IPC_EXCL);
	if(gi_msg_vm_id < 0)
	{
		gi_msg_vm_id = msgget(3001, IPC_CREAT|0666);
		if(gi_msg_vm_id < 0)
		{
			return -1;
		}
	}
#elif (4 == DEV_TYPE)
	//Create vp message queue
	gi_msg_vp_id = msgget(3002, IPC_EXCL);
	if(gi_msg_vp_id < 0)
	{
		gi_msg_vp_id = msgget(3002, IPC_CREAT|0666);
		if(gi_msg_vp_id < 0)
		{
			return -1;
		}
	}
#elif (1 == DEV_TYPE)
	//Create park message queue
	gi_msg_park_id = msgget(3003, IPC_EXCL);
	if(gi_msg_park_id < 0)
	{
		gi_msg_park_id = msgget(3003, IPC_CREAT|0666);
		if(gi_msg_park_id < 0)
		{
			return -1;
		}
	}
#endif
	gi_msg_platform_info_id = msgget(3004, IPC_EXCL);
	if(gi_msg_platform_info_id < 0)
	{
		gi_msg_platform_info_id = msgget(3004, IPC_CREAT|0666);
		if(gi_msg_platform_info_id < 0)
		{
			return -1;
		}
	}

	return 0;
}

static __inline__ int32_t msg_del(int32_t msgid)
{
	return msgctl(msgid,IPC_RMID,0);
}

int32_t ve_msg_del(void)
{
	return msg_del(MSG_ALLEYWAY_ID);
}



/****************************alleyway message queue*******************************/
int vmq_sendto_tabitcom(str_alleyway_msg msg, char ac_func)
{
	msg.type = MSG_ALLEYWAY_BITCOM_TYPE;
	msg.bitcom.func = ac_func;

	return msgsnd(MSG_ALLEYWAY_ID, &msg, sizeof(msg) - sizeof(long), 0);
}



/****************************park message queue*******************************/
int vmq_sendto_tpzehin(str_park_msg as_park_msg)
{
	as_park_msg.type = MSG_PARK_ZEHIN_TYPE;

    errno = 0;
	int ret = msgsnd(MSG_PARK_ID, &as_park_msg, sizeof(str_park_msg) - sizeof(long), IPC_NOWAIT);
    if (ret < 0) {
        DEBUG("send msg failed. errno = %d", errno);
        if (errno == EAGAIN) {
            DEBUG("send msg failed. errno = EAGAIN");
        }
    }

	return 0;
}

int vmq_sendto_http(str_park_msg park_msg)
{
	park_msg.type = MSG_PARK_HTTP_TYPE;

    errno = 0;
	int ret = msgsnd(MSG_PARK_ID, &park_msg, sizeof(str_park_msg) - sizeof(long), IPC_NOWAIT);
    if (ret < 0) {
        DEBUG("send msg failed. errno = %d", errno);
        if (errno == EAGAIN) {
            DEBUG("send msg failed. errno = EAGAIN");
        }
    }

    struct msqid_ds msg_info;
    msgctl(MSG_PARK_ID, IPC_STAT, &msg_info);

    DEBUG("\n");
    DEBUG("current number of bytes on queue is %d", msg_info.msg_cbytes);
    DEBUG("number of messages in queue is      %d",msg_info.msg_qnum);
    DEBUG("max number of bytes on queue is     %d",msg_info.msg_qbytes);
    /* 每个消息队列的容量（字节数）都有限制MSGMNB，值的大小因系统而异。
     * 在创建新的消息队列时，//msg_qbytes的缺省值就是MSGMNB
     */
    DEBUG("pid of last msgsnd is %d",msg_info.msg_lspid);
    DEBUG("pid of last msgrcv is %d",msg_info.msg_lrpid);
    DEBUG("last msgsnd time is   %s", ctime(&(msg_info.msg_stime)));
    DEBUG("last msgrcv time is   %s", ctime(&(msg_info.msg_rtime)));
    DEBUG("last change time is   %s", ctime(&(msg_info.msg_ctime)));
    DEBUG("msg uid is            %d", msg_info.msg_perm.uid);
    DEBUG("msg gid is            %d", msg_info.msg_perm.gid);

	return 0;
}

int vmq_sendto_songli(str_park_msg park_msg)
{
	park_msg.type = MSG_PARK_SONGLI_TYPE;

    errno = 0;
	int ret = msgsnd(MSG_PARK_ID, &park_msg, sizeof(str_park_msg) - sizeof(long), IPC_NOWAIT);
    if (ret < 0) {
        DEBUG("send msg failed. errno = %d", errno);
        if (errno == EAGAIN) {
            DEBUG("send msg failed. errno = EAGAIN");
        }
    }

	return 0;
}

int vmq_sendto_ktetc(str_park_msg park_msg)
{
	park_msg.type = MSG_PARK_KTETC_TYPE;

    errno = 0;
	int ret = msgsnd(MSG_PARK_ID, &park_msg, sizeof(str_park_msg) - sizeof(long), IPC_NOWAIT);
    if (ret < 0) {
        DEBUG("send msg failed. errno = %d", errno);
        if (errno == EAGAIN) {
            DEBUG("send msg failed. errno = EAGAIN");
        }
    }

	return 0;
}

int vmq_sendto_tpbitcom(str_park_msg as_park_msg)
{
	as_park_msg.type = MSG_PARK_BITCOM_TYPE;

    errno = 0;
	int ret = msgsnd(MSG_PARK_ID, &as_park_msg, sizeof(str_park_msg) - sizeof(long), IPC_NOWAIT);
    if (ret < 0) {
        DEBUG("send msg failed. errno = %d", errno);
        if (errno == EAGAIN) {
            DEBUG("send msg failed. errno = EAGAIN");
        }
    }

	return 0;
}

/****************************vp message queue*******************************/
int vmq_sendto_tvpuniview(str_vp_msg as_vp_msg)
{
	as_vp_msg.type = MSG_VP_UNIVIEW_TYPE;

	msgsnd(MSG_VP_ID, &as_vp_msg, sizeof(str_vp_msg) - sizeof(long), IPC_NOWAIT);

	return 0;
}

int vmq_sendto_tvpbaokang(str_vp_msg as_vp_msg)
{
	as_vp_msg.type = MSG_VP_BAOKANG_TYPE;

	msgsnd(MSG_VP_ID, &as_vp_msg, sizeof(str_vp_msg) - sizeof(long), IPC_NOWAIT);

	return 0;
}

int vmq_sendto_tvpnetposa(str_vp_msg as_vp_msg)
{
	as_vp_msg.type = MSG_VP_NETPOSA_TYPE;

	msgsnd(MSG_VP_ID, &as_vp_msg, sizeof(str_vp_msg) - sizeof(long), IPC_NOWAIT);

	return 0;
}


C_CODE_END
