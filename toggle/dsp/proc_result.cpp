
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

#include "commonfuncs.h"
#include "../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"
#include "proc_result.h"
#include "Msg_Def.h"
#include "dsp_config.h"
#include "traffic_flow_process.h"
#include "violation_records_process.h"
#include "h264_buffer.h"
#include "traffic_records_process.h"
#include "park_records_process.h"
#include "cmem.h"
#include "../../../ipnc_mcfw/mcfw/interfaces/link_api/videoAnalysisLink_parm.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>

#include "ve/platform/bitcom/thread.h"
#include "ve/platform/bitcom/record.h"
#include "logger/log.h"

#define MIN_VIDEO_INTERVAL 5  //Υ����Ƶ���ʱ�䣬��λ����

#define resume_print(msg...) \
	do{\
		printf("[resume](%s|%s|%d) ", __FILE__, __func__, __LINE__);\
		printf(msg);\
	} while (0);


MSGDATACMEM g_h264_cmem;
extern int gi_filllight_smart;
int proc_result(MSG_BUF *p_msg);
void *result_recv_task(void *argv);
//int process_vehicular_result(MSGDATACMEM *msg_vehicular_result);
int process_alg_result(MSGDATACMEM *msg_vehicular_result);
int process_alg_result_info(MSGDATACMEM *msg_alg_result_info);

int get_PD_h264(H264_Record_Info * ph264_record_info, IllegalInfoPer * pillegalInfoPer);

static pthread_t result_thread;		//�����㷨��Ϣ�߳�
static pthread_t pic_handle_thread;		//ͼƬ�����߳�
static pthread_t info_handle_thread;		//��Ϣ�����߳�


//����Υ����¼��h264��ʱ���
//��������Ƶ��ʱ�����2s֮�ڣ���Ϊ1��������Ƶ��ͬʱ������¼����Ӧ��Ϣ��
int adjust_PD_h264_time(IllegalInfoPer * pillegalInfoPer)
{

	int i=0;
	//	int ret=0;
	int time_end_old=0;
	int time_start_old=0;
	int index_old=0;
	int start, end;

	if(pillegalInfoPer==NULL)
	{
		printf("in %s: pillegalInfoPer is NULL \n",__func__);
		return -1;
	}

	//���ڴ���ͼƬƴ�ӣ�����ͼƬ������h264�Ķ�����һ����
//	for(i=0; i<pillegalInfoPer->picNum; i++)//MAX_CAPTURE_NUM
	for(i=0; i<MAX_CAPTURE_NUM; i++)
	{

		//��i��ͼ����Ϣ ����start

		VD_Time * pVD_Time;
		pVD_Time=&pillegalInfoPer->illegalPics[i].videoTime[0];//��1��ͼ���Ӧ����Ƶ�ε���ʼʱ��


		debug("video %d start time:  %d-%d-%d %d:%d:%d.  \n",
				i, pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec)
			;

		start = mk_time(pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec);

		//��i��ͼ����Ϣ ����end

		pVD_Time=&pillegalInfoPer->illegalPics[i].videoTime[1];//��1��ͼ���Ӧ����Ƶ�εĽ���ʱ��
		debug("video %d end time:  %d-%d-%d %d:%d:%d.  \n",
				i, pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec)
			;
		end = mk_time(pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec);

		//	start -= 8*60*60;//��8������8Сʱ
		//	end -= 8*60*60;

		debug("----------start:%d, end:%d ------\n", start, end)
			;


		if( (i>=1) && ((start-time_end_old<2)&&(start>=time_start_old)))//��������Ƶ��ʱ�����2s֮�ڣ�����Ϊ1��������Ƶ
		{
			//index_old--;
			printf("in %s: i=%d,start=%d,time_end_old=%d\n",__func__,i,start,time_end_old);

			memcpy( &(pillegalInfoPer->illegalPics[index_old].videoTime[1]),
					&(pillegalInfoPer->illegalPics[i].videoTime[1]),
					sizeof(VD_Time));

			memset(&(pillegalInfoPer->illegalPics[i].videoTime[0]),0,sizeof(VD_Time));
			memset(&(pillegalInfoPer->illegalPics[i].videoTime[1]),0,sizeof(VD_Time));

		}
		else
		{
			index_old=i;
		}

		time_start_old=start;
		time_end_old=end;


	}

	return 0;

}


//����Υ����¼��ƴ�ӵļ���h264��ps����ʱ�����ʹ֮����������������
int change_h264_timestamp(H264_Record_Info * ph264_record_info)
{

	int i=0;
	int ret=0;
	int seg_num;
	long long timestamp_start, timestamp_end;

	if(ph264_record_info==NULL)
	{
		ERROR("in %s: ph264_record_info is NULL \n",__func__);
		return -1;
	}

	timestamp_start=-1;
	timestamp_end=0;
	seg_num=ph264_record_info->h264_seg;
	for(i=0;i<seg_num;i++)
	{
		DEBUG("seg_num=%d,before h264_ps_adjust_timestamp\n",seg_num);
		ret=h264_ps_adjust_timestamp(
				(char *)ph264_record_info->h264_buf_seg[i],
				ph264_record_info->h264_buf_seg_size[i],
				timestamp_start,
				&timestamp_end );
		if(ret<0)
		{
			ERROR("h264_ps_adjust_timestamp return failed, h264 seg %d\n",i);
			return -1;
		}
		timestamp_start=timestamp_end;//+INTERVAL_TIMESTAMP;
		DEBUG("seg_num=%d,after h264_ps_adjust_timestamp\n",seg_num);
	}

	return 0;

}



/******************************************************
 *������:get_PD_h264
 *����:
 *����:  ��ȡΥ����Ƶ
 *����ֵ:�ɹ�����0��ʧ�ܷ���-1
 *******************************************************/
int get_PD_h264(H264_Record_Info * ph264_record_info, IllegalInfoPer * pillegalInfoPer)
{
	//log_state("VD:","In get_PD_h264\n");
	DEBUG("In get_PD_h264\n");
	int has_h264=0;
	int ret=-1;
	int start, end;
	static H264Buffer *h264Buffer;
	H264_Seg_Info h264_seg_info;
	static int flag_create_h264Buffer=0;

	if(ph264_record_info==NULL)
	{
		ERROR("ph264_record_info is NULL\n");
		return -1;
	}

	//zmh���20150624����������Ƶ��ʱ�����2s֮�ڣ���Ϊ1��������Ƶ������Υ����Ƶ��ĳһ������������
	ret=adjust_PD_h264_time(pillegalInfoPer);
	if(ret==-1)
	{
		INFO("adjust_PD_h264_time failed\n");
	}

	int i=0;
	int index=0;

	if(flag_create_h264Buffer==0)
	{
		flag_create_h264Buffer=1;
		h264Buffer = new H264Buffer(g_h264_cmem.addr_phy, g_h264_cmem.size);//��ι��죬���ͷŻ������ڴ�й¶��
	}
	INFO("g_h264_cmem.addr_phy=0x%x, g_h264_cmem.size=%d\n",g_h264_cmem.addr_phy, g_h264_cmem.size);

	memset(ph264_record_info,0,sizeof(H264_Record_Info));
	for(i=0; i<MAX_CAPTURE_NUM; i++)
	{


		//��i��ͼ����Ϣ ����start
		VD_Time * pVD_Time;
		pVD_Time=&pillegalInfoPer->illegalPics[i].videoTime[0];//��1��ͼ���Ӧ����Ƶ�ε���ʼʱ��

		DEBUG("video %d start time:  %d-%d-%d %d:%d:%d.  \n",
				i, pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec)
			;

		start = mk_time(pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec);

		//��i��ͼ����Ϣ ����end
		pVD_Time=&pillegalInfoPer->illegalPics[i].videoTime[1];//��1��ͼ���Ӧ����Ƶ�εĽ���ʱ��
		DEBUG("video %d end time:  %d-%d-%d %d:%d:%d.  \n",
				i, pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec)
			;
		end = mk_time(pVD_Time->tm_year, pVD_Time->tm_mon, pVD_Time->tm_mday,
				pVD_Time->tm_hour, pVD_Time->tm_min, pVD_Time->tm_sec);

		//	start -= 8*60*60;//��8������8Сʱ
		//	end -= 8*60*60;

		DEBUG("----------start:%d, end:%d ------\n", start, end)
			;


		if (h264Buffer)
		{
			DEBUG("start h264 getting. \n");
			ret = h264Buffer->get_h264_for_record(&h264_seg_info,
					start, end, MIN_VIDEO_INTERVAL);
			DEBUG("h264 got.  ret=%d\n",ret);
			if (ret == 0)
			{
				//����segԽ��
				int num_h264_seg=ph264_record_info->h264_seg + h264_seg_info.h264_seg;
				if( num_h264_seg > MAX_H264_SEG_NUM)
				{
					log_error("pd_proc", "num_h264_seg =%d > %d\n", num_h264_seg, MAX_H264_SEG_NUM);
					break;
				}

				has_h264 = 1;
				ph264_record_info->h264_seg = num_h264_seg;
				ph264_record_info->seconds_record_ret += h264_seg_info.seconds_record_ret;

				ph264_record_info->h264_buf_seg[index] = h264_seg_info.h264_buf_seg[0];
				ph264_record_info->h264_buf_seg_size[index] = h264_seg_info.h264_buf_seg_size[0];
				index++;
				if(h264_seg_info.h264_seg==2)
				{
					ph264_record_info->h264_buf_seg[index] = h264_seg_info.h264_buf_seg[1];
					ph264_record_info->h264_buf_seg_size[index] = h264_seg_info.h264_buf_seg_size[1];
					index++;
				}
			}
			else
			{
				//has_h264 = 0;
			}
		}
		else
		{
			ERROR("h264Buffer is NULL\n");
		}

	}

	if(has_h264!=1)
	{
		ERROR("get h264 failed\n");
		return -1;
	}
	//zmh���20150623�����µ���h264��ps����ʱ�����ʹ��ƴ�ӵ�ps���������Ĳ���
	ret=change_h264_timestamp(ph264_record_info);
	if(ret!=0)
	{
		ERROR("change_h264_timestamp failed\n");
		return -1;
	}
	DEBUG("get_PD_h264 complete\n");
	return 0;

}

/*******************************************************************************
 * ������: process_alg_result_info
 * ��  ��: �����㷨������
 * ��  ��: msg_vehicular_result
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
 *******************************************************************************/
int process_alg_result_info(MSGDATACMEM *msg_alg_result_info)
{
	log_state("VD:","In process_alg_result_info!\n");

	void *result_addr_vir = get_dsp_result_cmem_pointer();

	if (result_addr_vir == NULL)
	{
		printf("Alg result info virtual address is NULL\n");
		return -1;
	}

	DEBUG("########msg_alg_result_info->msg_type = %d########\n",msg_alg_result_info->msg_type);

	if(MSG_TYPE_PARK_ALARM_INFO == msg_alg_result_info->msg_type)//����������Ϣ
	{
		process_park_alarm(msg_alg_result_info);
	}
	else if(MSG_TYPE_PARK_LIGHT_CONTROL == msg_alg_result_info->msg_type)//����ָʾ�ƿ���
	{
		process_park_light(msg_alg_result_info);
	}
	else if(MSG_TYPE_ENTRANCE_CTRL_OUTPUT == msg_alg_result_info->msg_type)//����ڵ�բ����
	{
		process_entrance_control(msg_alg_result_info);
	}
    else if(MSG_TYPE_DAY_NIGHT_INFO == msg_alg_result_info->msg_type)//��ҹģʽ�л���Ϣ
	{
		process_fillinlight_smart_control(msg_alg_result_info);
	}
	else if (MSG_TYPE_VEHICLE_INFO == msg_alg_result_info->msg_type)//������Ϣ�������ʹ�ã�
	{
#if (5 == DEV_TYPE)
		process_rg_mq(msg_alg_result_info);
#endif
	}
	else if(MSG_TYPE_PARK_LEAVE_INFO == msg_alg_result_info->msg_type)//��������ʻ��info
	{
		process_park_leave_info(msg_alg_result_info);
	}
	else if(MSG_TYPE_PARK_STATUS == msg_alg_result_info->msg_type)//����״̬
	{
		process_park_status_info(msg_alg_result_info);
	}
	else if(MSG_TYPE_SNAP_RETURN == msg_alg_result_info->msg_type)//ץ�ķ�����Ϣ
	{
		process_snap_return_info(msg_alg_result_info);
	}
	else //����δ����
	{
		printf("alg_result_info.type = %d is undefined!\n",msg_alg_result_info->msg_type);
	}

	return 0;
}
/*******************************************************************************
 * ������: process_alg_result
 * ��  ��: �����㷨������
 * ��  ��: msg_vehicular_result
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
 ******************************************************************************/
int process_alg_result(MSGDATACMEM *msg_alg_result)
{
	if (CMEM_init() == -1) {
		ERROR("Failed to initialize CMEM");
		return -1;
	}

	static void *result_addr = NULL;
	static bool flag_first = true;

	if(flag_first) {
		DEBUG("msg_vehicular_result->addr_phy=0x%x",msg_alg_result->addr_phy);
		result_addr = CMEM_registerAlloc(msg_alg_result->addr_phy);

		if (NULL == result_addr) {
			ERROR("Register vehicular result pointer failed !");
			CMEM_exit();
			return -1;
		}
		flag_first = false;
	}

	SystemVpss_SimcopJpegInfo *jpeg_info =
		(SystemVpss_SimcopJpegInfo *)
		((char *)result_addr +
		 msg_alg_result->off_set * SIZE_JPEG_BUFFER );//JPEG_INFO_BLK_SIZE

	switch (jpeg_info->algResultInfo.type)
	{
		case 1://����(������)
		{
#if (5 == DEV_TYPE)
			process_rg_records_motor_vehicle(jpeg_info);
#else
			process_vm_records_motor_vehicle(jpeg_info);
#endif
			break;
		}
		case 2://Υͣ
		{
			IllegalInfoPer illegalInfoPer;
			memcpy(&illegalInfoPer,
					(char *)&(jpeg_info->algResultInfo.AlgResultInfo),
					sizeof(IllegalInfoPer));

			H264_Record_Info h264_record_info;
			get_PD_h264(&h264_record_info, &illegalInfoPer);

			process_vp_records((void *)jpeg_info, (void *)&h264_record_info);
			break;
		}
		case 3://
		{

		}
		case 4: //����
		{
			process_park_records(jpeg_info);
			break;
		}
		case 5://����(����)
		{
			process_vm_records_others(jpeg_info);
			break;
		}
		default:
		{
			ERROR("VD:","algResultInfo.type is wrong!\n");
			break;
		}
	}
	return 0;
}

/*******************************************************************************
 * ������: info_handle
 * ��  ��: ��Ϣ����
 * ��  ��: argv������Ҫ��NULL����
 *******************************************************************************/
void *info_handle(void *argv)
{
    set_thread("info_handle");
	int msg_size = 0;
	int algorithm_qid = Msg_Init(1000);
	int msg_count = 0;
	MSG_BUF msgbuf;
	MSGDATACMEM msg_alg_result_info;

	while(1)
	{
		msg_size = msgrcv(algorithm_qid, &msgbuf, sizeof(msgbuf) - sizeof(long), 200, 0);
		if(msg_size > 0)
		{
			msg_count++;
			DEBUG("Message count  %d, info handle",  msg_count);
			memcpy(&msg_alg_result_info, &(msgbuf.mem_info),sizeof(msg_alg_result_info));
			process_alg_result_info(&msg_alg_result_info);
		}
		else
		{
			ERROR("Message count  %d, info handle error",  msg_count);
			usleep(10000);
		}
	}

	pthread_exit(NULL);
}



/*******************************************************************************
 * ������: pic_handle
 * ��  ��: ͼƬ����
 * ��  ��: argv������Ҫ��NULL����
 *******************************************************************************/
void *pic_handle(void *argv)
{
    set_thread("pic_handle");
	int msg_size = 0;
	int algorithm_qid = Msg_Init(1000);
	int msg_count = 0;
	MSG_BUF msgbuf;
	MSGDATACMEM msg_alg_result;

	while(1)
	{
		msg_size = msgrcv(algorithm_qid, &msgbuf, sizeof(msgbuf) - sizeof(long), 100, 0);
		if(msg_size > 0)
		{
			msg_count++;
			DEBUG("Message count  %d, picture handle",  msg_count);
			memcpy(&msg_alg_result, &(msgbuf.mem_info),sizeof(msg_alg_result));
			process_alg_result(&msg_alg_result);
		}
		else
		{
			ERROR("Message count  %d, picture handle error",  msg_count);
			usleep(10000);
		}
	}


	pthread_exit(NULL);
}


/*******************************************************************************
 * ������: result_recv_task
 * ��  ��: �㷨�����������߳�
 * ��  ��: argv������Ҫ��NULL����
 ******************************************************************************/
void *result_recv_task(void *argv)
{
    set_thread("result_recv");
	MSG_BUF msgbuf;
	int qid = Msg_Init(SYS_MSG_KEY);
	int alg_qid = Msg_Init(1000);
	int msg_size = 0;
	int ret = 0;
	static int msg_count = 0;

	//�����ƿ��߳�
	ret = pthread_create(&info_handle_thread, NULL, info_handle, NULL);
	if (ret != 0) {
		ERROR("[error]:create info_handle failed");
	}

	//������Ϣ�����߳�
	ret = pthread_create(&pic_handle_thread, NULL, pic_handle, NULL);
	if (ret != 0) {
		ERROR("[error]:create pic_handle failed");
	}

	//��ȡg_h264_cmem
	msgbuf.Des = MSG_TYPE_MSG21;
	msgbuf.cmd = SYS_MSG_GET_H264_CMEM_INFO;
	msgbuf.Src = MSG_TYPE_MSG17;
	Msg_Send(qid, &msgbuf, sizeof(msgbuf));

	while (1) {
		memset(&msgbuf, 0, sizeof(MSG_BUF));
		msg_size = Msg_Rsv(qid, MSG_TYPE_MSG17, &msgbuf, sizeof(msgbuf));

		msg_count++;
		DEBUG("VD recived message from SERVER:%d, cmd=%d, Src=%d",
			   msg_count, msgbuf.cmd, msgbuf.Src);

		if (msg_size < 0) {
			CRIT("VD recived from SERVER errno = %d : %d, cmd=%d, Src=%d",
					errno, msg_count, msgbuf.cmd, msgbuf.Src);
			usleep(10000);
			continue;
		} else if ((msgbuf.Src == MSG_TYPE_MSG17) || (msgbuf.Src < 0)) {
			DEBUG("Got Error message, src : %d\n", msgbuf.Src);
			usleep(10000);
			continue;
		}

		switch(msgbuf.cmd) {
		case SYS_MSG_RECV_ALG_RESULT:
			msgbuf.Des = 100;
			msgsnd(alg_qid, &msgbuf, sizeof(msgbuf) - sizeof(long), IPC_NOWAIT);
			break;

		case SYS_MSG_RECV_ALG_RESULT_INFO:
			msgbuf.Des = 200;
			msgsnd(alg_qid, &msgbuf, sizeof(msgbuf) - sizeof(long), IPC_NOWAIT);
			break;

		case SYS_MSG_SEND_H264_CMEM_INFO:
			DEBUG("Message count  %d, viedio handle",  msg_count);
			memcpy((char *)&g_h264_cmem, (char *)&msgbuf.mem_info,
					sizeof(MSGDATACMEM));//p_msg->mem_info
			break;
		}
	}

	pthread_exit(NULL);
}

int create_result_recv_task(void)
{
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int ret = pthread_create(&result_thread, &attr, &result_recv_task, NULL);

	pthread_attr_destroy(&attr);

	if (ret != 0) {
		ERROR("Create result_recv_task failed !!!\n");
	}

	return ret;
}



