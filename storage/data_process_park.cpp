
#include "data_process.h"
#include "database_mng.h"
#include "park_records_process.h"
#include "disk_mng.h"
#include "disk_func.h"
#include "storage_common.h"
#include "commonfuncs.h"
#include "upload.h"
#include "h264/debug.h"
#include "ftp.h"
#include "oss_interface.h"

#include "data_process_park.h"
#include "logger/log.h"

int flag_upload_complete_park = 0;

EP_PicInfo aliyun_pic_info[PARK_PIC_NUM];


/*******************************************************************************
 * ������: aliyun_record_reupload
 * ��  ��: ����ftp����
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int aliyun_record_reupload(DB_ParkRecord db_park_record, EP_PicInfo *pic_info, str_aliyun_image astr_aliyun_image)
{
	int li_i = 0;

	for(li_i=0; li_i<PARK_PIC_NUM; li_i++)
	{
		//�����жϵĲ��ϸ� �ɹ�һ�ξ���Ϊ�ɹ�
		if(oss_put_file(astr_aliyun_image.name[li_i], (char *)astr_aliyun_image.buf[li_i], astr_aliyun_image.size[li_i]) == 0)
		{

			return 0x000000fb;

			TRACE_LOG_PACK_PLATFORM("%s", "aliyun reupload successful!");
		}
		else
		{
			TRACE_LOG_PACK_PLATFORM("%s, name=%s, size=%d", "aliyun reupload failed!", astr_aliyun_image.name[li_i], astr_aliyun_image.size[li_i]);
		}
	}

	//���͵�����mq��Ϣ
	if((db_park_record.flag_send&0x04) == 0)
	{
		// 1:ʻ��
		if(db_park_record.objectState == 1)
		{
			//park_enter_process_fun(db_park_record);
		}
		//0:ʻ��
		else if(db_park_record.objectState == 0)
		{
			//park_leave_process_fun(db_park_record);
		}
	}

	return 0;
}


/*******************************************************************************
 * ������: ftp_record_reupload
 * ��  ��: ����ftp����
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int ftp_record_reupload(DB_ParkRecord db_park_record, EP_PicInfo *pic_info, DB_File db_file)
{
	str_aliyun_image lstr_aliyun_image;
	int ftp_status 	= ftp_get_status_traffic_record();
	int mq_status 	= mq_get_status_park_record();

    //MQ��FTP��ͨ�� �Ž�������
	if ((ftp_status != 0) || (mq_status != 0))
	{
        if(ftp_status != 0)
        {
        	DEBUG("%s", "Reupload FTP is not OK!");
        }

        if(mq_status != 0)
        {
            DEBUG("%s", "Reupload MQ is not OK!");
        }

        return -1;
	}

	do /*�ϴ�ͼƬ*/
	{
		//status==0(�ɹ�)��status==-2(û���ҵ�ͼƬ)
		ftp_status = send_park_records_image_file(&db_park_record, pic_info, &lstr_aliyun_image);
		if (ftp_status == 0)
		{
			CLASS_FTP * ftp;
			ftp = get_ftp_chanel(FTP_CHANNEL_PASSCAR_RESUMING);
			if (NULL == ftp)
			{
				return -1;
			}
			TYPE_FTP_CONFIG_PARAM *config = ftp->get_config();
			if (config == NULL)
			{
				return -1;
			}
			sprintf(db_park_record.ftp_user, config->user);
			sprintf(db_park_record.ftp_passwd, config->passwd);
			sprintf(db_park_record.ftp_ip, "%d.%d.%d.%d", config->ip[0], config->ip[1], config->ip[2], config->ip[3]);
			db_park_record.ftp_port = config->port;
			db_park_record.flag_send &= 0xfd;

			TRACE_LOG_PACK_PLATFORM("%s", "ftp reupload successful!");
		}
		else if (ftp_status == -2)
		{
			if (db_delete_park_records(db_file.record_path, &db_park_record, &mutex_db_records_park) < 0)
			{
				TRACE_LOG_PACK_PLATFORM("%s", "Not found reupload picture!");
			}

			return 0xfd;
		}
		else
		{
			TRACE_LOG_PACK_PLATFORM("%s", "ftp reupload failed!");

			return -1;
		}
	}
	while (0);

	do /* ͼƬ�ϴ��ɹ������ϴ���Ϣ */
	{
		if (((db_park_record.flag_send&0x02) == 0) && (mq_status == 0))
		{
			int ret = send_park_record_info(&db_park_record);
			if(ret < 0)
			{
				TRACE_LOG_PACK_PLATFORM("%s", "MQ topic reupload failed!");

				return -1;
			}
		}
	}
	while (0);

	return 0;
}


/*******************************************************************************
 * ������: park_record_reupload
 * ��  ��: ������¼����
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int park_record_reupload(void)
{
	if (!EP_REUPLOAD_CFG.is_resume_passcar)           //������ʱʹ���˿��ڵ�������ʶ���Ժ���Խ����޸�
		return -1;

	int ret;
	static DB_File db_file;//������¼���ƣ�������ID
	EP_PicInfo pic_info[PARK_PIC_NUM];
	str_aliyun_image lstr_aliyun_image;
	static DB_ParkRecord db_park_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//������Ҫ�������һ��ͨ�м�¼�����ݿ⣬�Ÿ��£�������Ҫ�洢�ɵ�ID.
	static int ID_record_last = 0;//��ȡ��¼ʱ�ж������е�ID��������ʧ�ܣ����ɶ�ȡ��һ����¼
	int li_i = 0;
	int li_tmp_sendflag = 0;

	memset(pic_info, 0, 2 * sizeof(EP_PicInfo));

	//ͨ������: ����������ʵʱ�ϴ�
	sprintf(db_park_record.description, "����");

	//��ȡ�������ݿ�
	sprintf(sql_cond, "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;", ID_DB_file_last);
	ret = db_read_DB_file(DB_NAME_PARK_RECORD, &db_file, &mutex_db_files_park, sql_cond);
	if (ret == -1)
	{
		return -1;
	}

	DEBUG("db_file.record_path=%s, ID_record_last=%d", db_file.record_path, ID_record_last);

	//���������ݿ��ȡ���ݼ�¼
	sprintf(sql_cond, "SELECT * FROM park_records WHERE ID>%d AND flag_send>0 limit 1;", ID_record_last);
	ret = db_read_park_record(db_file.record_path, &db_park_record, &mutex_db_records_park, sql_cond);
	if (ret == -1)
	{
		DEBUG("%s", "Read record database is failed!");

		//�������ݿ�ͼ�¼���ݿ��α����
		db_park_record.ID = 0;
		ID_record_last = 0;
		ID_DB_file_last = db_file.ID;

		//ɾ��(����)���߸���(ʵʱ�洢)�������ݿ�ļ�¼
		ret = db_delete_DB_file((char*)DB_NAME_PARK_RECORD, &db_file, &mutex_db_files_park);
		if (ret != 0)
		{
			TRACE_LOG_PACK_PLATFORM("flag_store=%d(0-delete reupload��1-update timingstore) park_records dosometing is failed!", db_file.flag_store);
		}

		ID_DB_file_last = 0;

		return -1;
	}

	//��������ͼƬ·��
	analysis_park_file_path(&db_park_record, pic_info);

	//aliyun��Ϣ�ϴ�
	aliyun_reupload_info(&db_park_record, pic_info, &lstr_aliyun_image);
	li_tmp_sendflag = db_park_record.flag_send;
	DEBUG("flag_send=%d(4-aliyun reupload��2-ftp reupload, 6-aliyun&&ftp reupload)!", db_park_record.flag_send);


	//ftp��bitcom topic����
	if((db_park_record.flag_send& 0x02) != 0)
	{
		db_park_record.flag_send &= ftp_record_reupload(db_park_record, pic_info, db_file);
	}

	//aliyun��songli queue����
	if((db_park_record.flag_send& 0x04) != 0)
	{
		db_park_record.flag_send &= aliyun_record_reupload(db_park_record, pic_info, lstr_aliyun_image);
	}

	//aliyun&&ftp�����ɹ� ɾ����¼
	if(((db_park_record.flag_send&0x02)==0) && ((db_park_record.flag_send&0x04)==0))
	{
    	if (db_park_record.flag_store == 0) //�������ʵʱ�洢��ת���ļ�
    	{
    		for(li_i=0; li_i<PARK_PIC_NUM; li_i++)
    		{
    			//ɾ�����ݿ��¼
				ret = db_delete_park_records(db_file.record_path, &db_park_record, &mutex_db_records_park);

				if (ret != 0)
				{
					TRACE_LOG_PACK_PLATFORM("%s", "Delete records info failed!");
				}
				else
				{
					TRACE_LOG_PACK_PLATFORM("%s", "Delete records and move picture info successful!");

					move_record_to_trash(db_park_record.partition_path, db_park_record.image_path[li_i]);
				}
    		}
    	}
	}

	if(li_tmp_sendflag != db_park_record.flag_send)
	{

		//����flag_send ��ʶ
		sprintf(sql_cond, "update park_records set flag_send=%d where ID=%d;", db_park_record.flag_send, db_park_record.ID);

		db_update_records(db_file.record_path, sql_cond, &mutex_db_records_park);
	}

	return 0;
}

/*************************************************
*������: analysis_park_file_path
*����: �������ݿ��е�·����EP_PicInfo�ṹ��
*************************************************/
int analysis_park_file_path(DB_ParkRecord* db_records,
                               EP_PicInfo pic_info[])
{
	int pos = 0;
	char file_name_pic[256];

	//��������ͼƬ�ļ������ַ�������
    for (int num = 0; num < PARK_PIC_NUM; num++)
    {
    	char *str = db_records->image_path[num];
		char *str1 = str;
    	sprintf(file_name_pic, "%s/%s/%s", db_records->partition_path, DISK_RECORD_PATH, db_records->image_path[num]);

    	//����ftp ·��
    	for (int i = 0; i <  STORAGE_PATH_DEPTH; i++)
    	{

    		str1=strchr(str,'/');//Ѱ��'/'
    		if (str1 == 0)
    		{
    			break;
    		}
    		pos = (unsigned int) str1 - (unsigned int) str;
    		resume_print("Pos: %d",pos);

    		//if(pos!=0)
    		{
    			memcpy(pic_info[num].path[i], str, pos);
    			pic_info[num].path[i][pos] = 0;
    			//str+=pos;
    			resume_print("path[%d]:%s\n",i,pic_info[num].path[i]);
    			debug_print("dir[%d]=%s\n", i, pic_info[num].path[i]);
    		}
    		str = str1 + 1;

    	}

    	//���һ����Ϊ�ļ���
    	memcpy(pic_info[num].name, str, strlen(str));
    	pic_info[num].name[strlen(str)] = 0;

    	resume_print("traffic pic name=%s\n", pic_info[num].name);
    }
	return 0;
}
