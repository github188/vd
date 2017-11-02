
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
 * 函数名: aliyun_record_reupload
 * 功  能: 泊车ftp续传
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int aliyun_record_reupload(DB_ParkRecord db_park_record, EP_PicInfo *pic_info, str_aliyun_image astr_aliyun_image)
{
	int li_i = 0;

	for(li_i=0; li_i<PARK_PIC_NUM; li_i++)
	{
		//现在判断的不严格， 成功一次就认为成功
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

	//发送第三方mq消息
	if((db_park_record.flag_send&0x04) == 0)
	{
		// 1:驶入
		if(db_park_record.objectState == 1)
		{
			//park_enter_process_fun(db_park_record);
		}
		//0:驶出
		else if(db_park_record.objectState == 0)
		{
			//park_leave_process_fun(db_park_record);
		}
	}

	return 0;
}


/*******************************************************************************
 * 函数名: ftp_record_reupload
 * 功  能: 泊车ftp续传
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int ftp_record_reupload(DB_ParkRecord db_park_record, EP_PicInfo *pic_info, DB_File db_file)
{
	str_aliyun_image lstr_aliyun_image;
	int ftp_status 	= ftp_get_status_traffic_record();
	int mq_status 	= mq_get_status_park_record();

    //MQ跟FTP都通了 才进行续传
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

	do /*上传图片*/
	{
		//status==0(成功)，status==-2(没有找到图片)
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

	do /* 图片上传成功才能上传信息 */
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
 * 函数名: park_record_reupload
 * 功  能: 泊车记录续传
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int park_record_reupload(void)
{
	if (!EP_REUPLOAD_CFG.is_resume_passcar)           //泊车暂时使用了卡口的续传标识，以后可以进行修改
		return -1;

	int ret;
	static DB_File db_file;//逐条记录后移，依赖于ID
	EP_PicInfo pic_info[PARK_PIC_NUM];
	str_aliyun_image lstr_aliyun_image;
	static DB_ParkRecord db_park_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//由于需要续传完成一个通行记录的数据库，才更新，所以需要存储旧的ID.
	static int ID_record_last = 0;//读取记录时判断条件中的ID，若续传失败，不可读取下一条记录
	int li_i = 0;
	int li_tmp_sendflag = 0;

	memset(pic_info, 0, 2 * sizeof(EP_PicInfo));

	//通行描述: 区分续传与实时上传
	sprintf(db_park_record.description, "续传");

	//读取索引数据库
	sprintf(sql_cond, "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;", ID_DB_file_last);
	ret = db_read_DB_file(DB_NAME_PARK_RECORD, &db_file, &mutex_db_files_park, sql_cond);
	if (ret == -1)
	{
		return -1;
	}

	DEBUG("db_file.record_path=%s, ID_record_last=%d", db_file.record_path, ID_record_last);

	//按索引数据库读取数据记录
	sprintf(sql_cond, "SELECT * FROM park_records WHERE ID>%d AND flag_send>0 limit 1;", ID_record_last);
	ret = db_read_park_record(db_file.record_path, &db_park_record, &mutex_db_records_park, sql_cond);
	if (ret == -1)
	{
		DEBUG("%s", "Read record database is failed!");

		//索引数据库和记录数据库游标更新
		db_park_record.ID = 0;
		ID_record_last = 0;
		ID_DB_file_last = db_file.ID;

		//删除(续传)或者更新(实时存储)索引数据库的记录
		ret = db_delete_DB_file((char*)DB_NAME_PARK_RECORD, &db_file, &mutex_db_files_park);
		if (ret != 0)
		{
			TRACE_LOG_PACK_PLATFORM("flag_store=%d(0-delete reupload，1-update timingstore) park_records dosometing is failed!", db_file.flag_store);
		}

		ID_DB_file_last = 0;

		return -1;
	}

	//分析续传图片路径
	analysis_park_file_path(&db_park_record, pic_info);

	//aliyun信息上传
	aliyun_reupload_info(&db_park_record, pic_info, &lstr_aliyun_image);
	li_tmp_sendflag = db_park_record.flag_send;
	DEBUG("flag_send=%d(4-aliyun reupload，2-ftp reupload, 6-aliyun&&ftp reupload)!", db_park_record.flag_send);


	//ftp和bitcom topic续传
	if((db_park_record.flag_send& 0x02) != 0)
	{
		db_park_record.flag_send &= ftp_record_reupload(db_park_record, pic_info, db_file);
	}

	//aliyun和songli queue续传
	if((db_park_record.flag_send& 0x04) != 0)
	{
		db_park_record.flag_send &= aliyun_record_reupload(db_park_record, pic_info, lstr_aliyun_image);
	}

	//aliyun&&ftp续传成功 删除记录
	if(((db_park_record.flag_send&0x02)==0) && ((db_park_record.flag_send&0x04)==0))
	{
    	if (db_park_record.flag_store == 0) //如果不是实时存储，转移文件
    	{
    		for(li_i=0; li_i<PARK_PIC_NUM; li_i++)
    		{
    			//删除数据库记录
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

		//更新flag_send 标识
		sprintf(sql_cond, "update park_records set flag_send=%d where ID=%d;", db_park_record.flag_send, db_park_record.ID);

		db_update_records(db_file.record_path, sql_cond, &mutex_db_records_park);
	}

	return 0;
}

/*************************************************
*函数名: analysis_park_file_path
*功能: 解析数据库中的路径到EP_PicInfo结构体
*************************************************/
int analysis_park_file_path(DB_ParkRecord* db_records,
                               EP_PicInfo pic_info[])
{
	int pos = 0;
	char file_name_pic[256];

	//解析过车图片文件名至字符数组中
    for (int num = 0; num < PARK_PIC_NUM; num++)
    {
    	char *str = db_records->image_path[num];
		char *str1 = str;
    	sprintf(file_name_pic, "%s/%s/%s", db_records->partition_path, DISK_RECORD_PATH, db_records->image_path[num]);

    	//解析ftp 路径
    	for (int i = 0; i <  STORAGE_PATH_DEPTH; i++)
    	{

    		str1=strchr(str,'/');//寻找'/'
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

    	//最后一部分为文件名
    	memcpy(pic_info[num].name, str, strlen(str));
    	pic_info[num].name[strlen(str)] = 0;

    	resume_print("traffic pic name=%s\n", pic_info[num].name);
    }
	return 0;
}
