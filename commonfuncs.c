/*
 * commonfuncs.c
 *
 *  Created on: 2013-4-8
 *      Author: shanhongwei
 */

#include <stdarg.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <time.h>
#include <pthread.h>
#include <iconv.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>

#include "commonfuncs.h"
#include "global.h"
#include "dsp_config.h"
#include "storage/data_process.h"
#include "logger/log.h"
#include "sys/time_util.h"
#include "ctype.h"

/*************************************
* 函数名:get_ftp_path
* 说明:  根据设置, 获取ftp上传的路径
*
 *************************************/
int get_ftp_path(FTP_URL_Level param, int year, int month, int day, int hour,
                 char* event_name, char *filepath)
{
	int len = 0;
	char dir[56];

	int i;
	for (i = 0; i < param.levelNum; i++)
	{
		switch (param.urlLevel[i])
		{
		case SPORT_ID:// = 0, //地点编号
			sprintf(dir, "%s", EP_POINT_ID);
			break;
		case DEV_ID:// = 1, //设备编号
			sprintf(dir, "%s", EP_DEV_ID);
			break;
		case YEAR_MONTH:// = 2, //年/月
			sprintf(dir, "%04d%02d", year, month);
			break;
		case DAY://= 4, //日
			sprintf(dir, "%02d", day);
			break;
		case EVENT_NAME:// = 5, //事件类型
			sprintf(dir, "%s", event_name);
			break;
		case HOUR:// = 6, //时
			sprintf(dir, "%02d", hour);
			break;
		case FACTORY_NAME:// = 7, //厂商名称
			sprintf(dir, "%s", EP_MANUFACTURER);//"bitcom"  "IPNC"
			break;
		default:
			break;
		}
		len += sprintf(filepath + len, "%s/", dir);
	}

	return 0;
}


/*******************************************************************************
 * 功能： 计算校验和;
 * 参数：
 *     data:校验数据指针;
 * 		len:校验数据字节长度;
 * 返回：校验和
*******************************************************************************/
u32 check_sum(u8 *data, int len)
{
	u32 check_sum = 0;
	int i;

	for (i = 0; i < len; i++)
	{
		check_sum += *data++;
	}

	return check_sum;
}

struct timeval g_time_start, g_time_end;


/*******************************************************************************
 * 功能：	测试代码耗时函数,单位ms;
 *
 * 参数：
 *
 * 返回：
 *
 *  											2011-10-31 Author：小寿
 *******************************************************************************/
void spend_ms_start(void)
{
	//获得系统时间
	gettimeofday(&g_time_start, NULL);

}

void spend_ms_end(const char *descp)
{
	int spend_ms;

	gettimeofday(&g_time_end, NULL);

	spend_ms = (g_time_end.tv_sec - g_time_start.tv_sec) * 1000
	           + (g_time_end.tv_usec - g_time_start.tv_usec) / 1000;
	DEBUG("  %s__spend_ms=%d ms\n", descp, spend_ms);
}

/*
 * 功能：在“/”下创建多级目录
 * 参数：
 * 		path：以“/”开始的目录结构
 * 返回：
 **/
int create_mul_dir(const char *path)
{
	char tmp_path[256];
	int len_path;
	int i;

	memset(tmp_path, 0, sizeof(tmp_path));
	strcpy(tmp_path, path);

	len_path = strlen(tmp_path);

	for (i = 1; i < len_path; i++) //跳过第一个根 /
	{
		if (tmp_path[i] == '/')
		{
			tmp_path[i] = 0;
			if (access(tmp_path, F_OK) != 0)
			{
				if (mkdir(tmp_path, 0755) == -1)
				{
					DEBUG("mkdir  {%s} error\n",tmp_path);
					return FAILURE;
				}
			}
			tmp_path[i] = '/';
		}
	}

	return SUCCESS;
}

/*******************************************************************************
 * 功能：保存数据到二进制文件;
 *
 * 参数：
 *
 * 返回：
 *
 *  											2011-8-30 Author：小寿
 *******************************************************************************/
int save_file(const char *path_file, const char *buff, int len)
{
	FILE * pfile = NULL;

	if (len <= 0)
	{
		return FAILURE;
	}

	if (create_mul_dir(path_file) != SUCCESS)
	{
		DEBUG("创建目录失败！\n");
		return FAILURE;
	}

	pfile = fopen(path_file, "wb");
	if (!pfile)
	{
		DEBUG("save file <%s>  is fail!", path_file);
		return FAILURE;
	}

	fwrite(buff, 1, len, pfile);
	fclose(pfile);
	return SUCCESS;
}


/*******************************************************************************
 * 功能： 由于vport无法传输 0x00 和 0xFF
 *
 * 参数：
 *
 * 返回：
 *
 *  											2011-10-12 Author：小寿
 *******************************************************************************/
static void char_256_to_254(char *src, char *dest)
{
	char value_src = *src;
	char value_high = 0; // 高位,存储大于等于254的值;
	char value_low = 0; // 低位,存储[0 ~ 253]的值;

	value_high = value_src / 254 + 1;
	value_low = value_src % 254 + 1;

	*dest = value_low;
	*(dest + 1) = value_high;

	//	DEBUG("char_256_to_254 : address = 0x%x, value_high = %d ,value_low = %d \n",src,*dest,*(dest+1));

}

/*******************************************************************************
 * 功能：
 *
 * 参数：
 *
 * 返回：
 *
 *  											2012-2-6 Author：小寿
 *******************************************************************************/
void covert_256_to_254(char *src, int len, char *dest)
{
	int i;
	char *p_src; 
	char *p_dest;

	//	DEBUG("covert_256_to_254: src_address = 0x%x \n",src);
	for (i = 0; i < len; i++)
	{
		p_src = src + i;
		p_dest = dest + i * 2;

		char_256_to_254(p_src, p_dest);

		//		DEBUG("p_dest[0x%x]=%d,p_dest+1[0x%x]=%d \n",p_dest,*p_dest,p_dest+1,*(p_dest+1));

	}

}

/*******************************************************************************
 * 功能：
 *
 * 参数：
 *
 * 返回：
 *
 *  											2012-2-6 Author：小寿
 *******************************************************************************/
static int char_254_to_256(char *src, char *dest)
{
	char value = 0; //转换结果;
	char value_high = *(src + 1); // 高位,存储大于等于254的值;
	char value_low = *src; // 低位,存储[0 ~ 253]的值;

	if (value_high == 0 || value_high > 2 || value_low == 0 || value_low == 255)
	{
		DEBUG("char_254_to_256(), 入口参数 出错....!\n ");
		return FAILURE;
	}
	else
	{
		value = (value_high - 1) * 254 + (value_low - 1);
		*dest = value;
	}

	return SUCCESS;
}

/*******************************************************************************
 * 功能：
 *
 * 参数：
 *
 * 返回：
 *
 *  											2012-2-6 Author：小寿
 *******************************************************************************/
int covert_254_to_256(char *src, int len, char *dest)
{
	int i;
	char *p_src, *p_dest;

	if (len % 2 != 0)
	{
		DEBUG("covert_254_to_256(), 入口参数 len[=%d]%%2 != 0 出错....!\n ", len);
		return FAILURE;

	}

	for (i = 0; i < len / 2; i++)
	{
		p_src = src + i * 2;
		p_dest = dest + i;

		if (char_254_to_256(p_src, p_dest) < 0)
		{
			return FAILURE;
		}

	}

	return SUCCESS;
}

int vpot_int_256_to_254(int a) //要求a，小于254*254*254*254
{
	int b; //
	b = a % 254 + 1; //低8位
	a = a / 254;
	b = b + ((a % 254 + 1) << 8); //低16位
	a = a / 254;
	b = b + ((a % 254 + 1) << 16); //低24位
	a = a / 254;
	b = b + ((a % 254 + 1) << 24); //全部32 位
	return b;
}

/*******************************************************************************
 * 功能：
 *
 * 参数：
 *
 * 返回：
 *
 *  											2011-10-12 Author：小寿
 *******************************************************************************/
int vpot_int_254_to_256(int b) //要求a，小于254*254*254*254
{
	int a; //
	a = b % 256 - 1; //低8位
	b = b >> 8;
	a += (b % 256 - 1) * 254; //低16位
	b = b >> 8;
	a += (b % 256 - 1) * 254 * 254; //低24位
	b = b >> 8;
	a += (b % 256 - 1) * 254 * 254 * 254; //全部32 位
	return a;
}


/*******************************************************************************
 * 功能：
 *
 * 参数：
 *
 * 返回：
 *
 *  											2012-4-9 Author：小寿
 *******************************************************************************/
char * get_date_time(void)
{
	time_t now;
	static char str_m[100];
	struct tm *time_now;
	struct tm tm_read;

	memset(str_m, 0, sizeof(str_m));

	time(&now);

	time_now = localtime_r(&now, &tm_read);
	strftime(str_m, 20, "%m-%d %H:%M:%S", time_now);
	//	str = asctime(time_now);
	//	DEBUG("now time = %s\n",str_m);

	return str_m;
}

char * get_time(void)
{
	time_t now;
	static char str_m[40];
	struct tm *time_now;
	struct tm tm_read;

	memset(str_m, 0, sizeof(str_m));

	time(&now);

	time_now = localtime_r(&now, &tm_read);
	strftime(str_m, 20, "%Y-%m-%d %H:%M:%S", time_now);
	//	str = asctime(time_now);
	//	DEBUG("now time = %s\n",str_m);

	return str_m;
}


/*******************************************************************************
 * 功能：
 *
 * 参数：
 *
 * 返回：
 *
 *  											2012-6-6 Author：小寿
 *******************************************************************************/
int set_systime(int year, int mon, int day, int hour, int min, int sec)
{

	struct tm t;
	time_t time_sys;
	int sta;

	t.tm_year = year - 1900;
	t.tm_mon = mon - 1;
	t.tm_mday = day;
	t.tm_wday = 0;
	t.tm_hour = hour;
	t.tm_min = min;
	t.tm_sec = sec;
	t.tm_isdst = 0;

	time_sys = mktime(&t);
	sta = stime(&time_sys);
	DEBUG("set_systime sta = %d \n", sta);

	return SUCCESS;

}

/*******************************************************************************
 * 功能：
 *
 * 参数：
 *
 * 返回：
 *
 *  											2012-7-10 Author：小寿
 *******************************************************************************/
int mk_time(int year, int mon, int day, int hour, int min, int sec)
{
	struct tm tm;
	tm.tm_year = year - 1900;
	tm.tm_mon = mon - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;

	return mktime(&tm);

}

/*
 * 功能：检查jpeg图片是否完整,检查头部标志和尾端标志;
 * 参数：
 * 返回：
 */
int check_jpeg(const void *ptr_pic, size_t len)
{
	char head[2]; //头部标志;
	char tail[2]; //尾端标志;

	char flg_head_suc[2] = { 0xFF, 0xD8 }; 	//jpeg文件头;
	char flg_tail_suc[2] = { 0xFF, 0xD9 }; 	//jpeg文件尾;

	memcpy(head, ptr_pic, sizeof(head));
	memcpy(tail, ptr_pic + len - sizeof(tail), sizeof(tail));

	if ((memcmp(head, flg_head_suc, sizeof(head)) == 0 ) &&
	        (memcmp(tail, flg_tail_suc, sizeof(tail)) == 0))
	{
		return SUCCESS;
	}

	log_warn("[CHECK_JPEG]", "jpeg head: %02X %02X, tail: %02X %02X\n",
	         head[0], head[1], tail[0], tail[1]);

	return FAILURE;
}


/*******************************************************************************
 * 功能： 获得线程名字;
 *
 * 参数：
 *
 * 返回：
 *
 *******************************************************************************/
int get_thread_name(char *name)
{
	prctl(PR_GET_NAME, name);
	return SUCCESS;
}

/**
 * 功能：设置线程的名字;
 * 参数：thread_name:线程名称
 * 返回：无
 **/
void set_thread(const char *thread_name)
{
	prctl(PR_SET_NAME, thread_name);
}

/*
 * 转化编码
 */
int convert_enc(const char* fromcode, const char* tocode,
                const char *buf_in, int inlen, char *buf_out, int outlen)
{
	iconv_t cd = iconv_open(tocode, fromcode);
	if (-1 == (intptr_t)cd)
	{
		ERROR("iconv_open false: %s ==> %s, %m/n", fromcode, tocode);
		return -1;
	}

	memset(buf_out, 0, outlen);

	char *inbuf 	= (char *)buf_in;
	char *outbuf 	= buf_out;

	int ret = iconv(cd, &inbuf, (size_t*) &inlen, &outbuf, (size_t*) &outlen);
	if (ret == -1)
	{
		perror("error when iconv.");
	}

	iconv_close(cd);

	return ret;
}

/**
 * convert_enc_s - string code convert
 *
 * @author cyj (2016/5/23)
 *
 * @param fromcode
 * @param tocode
 * @param inbuf
 * @param inlen
 * @param outbuf
 * @param outlen
 *
 * @return ssize_t if error return -1, otherwise return the
 *  	   length of string after convert
 */
ssize_t convert_enc_s(const char *fromcode, const char *tocode,
					  const char *inbuf, size_t inlen,
					  char *outbuf, size_t outlen)
{
	iconv_t cd = iconv_open(tocode, fromcode);
	if (((iconv_t)-1) == cd) {
		ERROR("iconv_open (%s ==> %s) failed:%m\n", fromcode, tocode);
		return -1;
	}

	char *in = (char *)inbuf;
	char *out = outbuf;
	size_t inleft = inlen;
	size_t outleft = outlen;

	size_t ret = iconv(cd, &in, &inleft, &out, &outleft);
	iconv_close(cd);

	if (ret == ((size_t)-1)) {
		ERROR("iconv failed:%m");
		return -1;
	}

	return outlen - outleft;
}

/*
 * 清除socket接收缓冲区.
 */
int clear_socket_recvbuf(int sockfd)
{
	struct timeval stWait;
	fd_set stFdSet;
	int lRet;
	char response[256];
	int num_recv=0;

	FD_ZERO(&stFdSet);
	FD_SET(sockfd, &stFdSet);

	stWait.tv_sec = 0;
	stWait.tv_usec = 0;
	printf("clear_socket_recvbuf: sockfd=%d\n",sockfd);
	for(;;)
	{
		lRet = select(sockfd + 1, &stFdSet, NULL, NULL, &stWait);
//		printf("clear_socket_recvbuf: after select, lRet=%d\n",lRet);
		switch (lRet)
		{
		case -1:
			log_warn("[COMMON]", "%s select : %m !\n", __func__);
			return -1;
		case 0:
			return 0;
		default:
			if (FD_ISSET(sockfd, &stFdSet))
			{
				lRet = recv(sockfd, response, sizeof(response), 0);
//				printf("clear_socket_recvbuf: after recv: lRet=%d, response is %s\n",lRet,response);
				if (lRet < 0)
					return -1;

				num_recv++;
				if(num_recv>=20)
				{
					printf("clear_socket_recvbuf: recv %d,return error\n",num_recv);
					return -1;
				}
			}
			else
			{
				log_error("[COMMON]", "%d is not in fdset\n", sockfd);
				return -1;
			}
			break;
		}
	}

	return 0;
}


int check_file_correct(const char * fpName)
{
	int ret = 0;

	if(access(fpName, F_OK) != 0)
	{
		DEBUG("cannot find file:%s\n ", fpName);
		ret = -1;
	}
	else if(get_file_size(fpName) <= 0)
	{
		DEBUG("file:%s size is wrong\n ", fpName);
		ret = -2;
	}

	return ret;
}


//解析一行，获取字段值
//等号之后的，双引号之内的，为字段内容(/mnt/nand/info.cfg)
ssize_t get_value(char *buf, char *val)// const char *key,
{
	char *ch, *tmp;
	ssize_t val_len;

	if((buf==NULL)||(val==NULL))
	{
		fprintf(stderr, "input param is invalid!");
		return -1;
	}

	/* 找等号 */
	ch = strchr(buf, '=');
	if(!ch){
		fprintf(stderr, "can not find '='!");
		return -1;
	}

	/* 跳过等号后的空格 */
	++ch;
	tmp = buf + sizeof(buf);
	while(isspace(*ch) && (ch++ < tmp));

	/*
	 * "xxx"格式去掉"", xxx格式直接拷贝
	 */
	if('\"' == *ch){
		/* 找后半引号 */
		tmp = strchr(ch + 1, '\"');
		if(!tmp){
			/* 没找到, 格式错误 */
			fprintf(stderr, "cmd format error!");
			return -1;
		}

		val_len = tmp - ch -1;
		strncpy(val, (ch + 1), val_len);
	}else{
		val_len = 0;
		tmp = val;
		while (!isspace(*ch) && (';' != *ch)) {
			*tmp++ = *ch++;
			++val_len;
		}
	}

	/* 末尾添加结束符 */
	val[val_len] = 0;
	return val_len ;

}


//解析一行，取第一个遇到的由空格分割的字符串
ssize_t get_str(char *buf, char *val)// const char *key,
{
	char *ch, *tmp;
	ssize_t val_len;

	if((buf==NULL)||(val==NULL))
	{
		fprintf(stderr, "input param is invalid!");
		return -1;
	}


	/* 跳过前面的空格 */
	ch = buf;
	tmp = buf + sizeof(buf);
	while(isspace(*ch) && (ch++ < tmp));

	/*
	 * "xxx"格式去掉"", xxx格式直接拷贝
	 */
	if('\"' == *ch){
		/* 找后半引号 */
		tmp = strchr(ch + 1, '\"');
		if(!tmp){
			/* 没找到, 格式错误 */
			fprintf(stderr, "cmd format error!");
			return -1;
		}

		val_len = tmp - ch -1;
		strncpy(val, (ch + 1), val_len);
	}else{
		val_len = 0;
		tmp = val;
		while (!isspace(*ch) && (';' != *ch)) {
			*tmp++ = *ch++;
			++val_len;
		}
	}

	/* 末尾添加结束符 */
	val[val_len] = 0;
	return val_len ;

}


//从状态日志文件中解析时间(本次重启前的有效运行时间)
int get_time_from_log(const char * fpName, time_t *rec_time_ret )
{
	FILE *f;

	//输入参数异常处理
	if( (fpName==NULL) || (rec_time_ret==NULL) )
	{
		printf("input fpName or rec_time_ret is NULL\n");
		return -1;
	}

	//打开文件
	f = fopen(fpName, "r");
	if (!f) {
		ERROR("Open %d failed!", fpName);
		return -1;
	}

	//读取文件，解析相关日志信息(log start,log number,time)
	char buf_line[256];
	int flag_exit=0;
	char * p_buf=NULL;
	char * ret_buf=NULL;

	int log_num=0;
	int log_num_ret=0;
	time_t rec_time=0;

	do
	{
		ret_buf=fgets(buf_line, 256-1, f);
		if(ret_buf==NULL)
		{
			flag_exit=1;
		}
		else
		{
			p_buf=strstr(buf_line,"log start" );
			if(p_buf!=NULL)
			{
				//重启信息很重要，可能没有新的 log number 信息，但是有 log start 信息
				//寻找本次重启前的最后一个日志时间
				log_num_ret=log_num;
				*rec_time_ret=rec_time;

				//print the time result
				struct tm *tm_rec=localtime(rec_time_ret);
				printf("before reboot : number=%d,rec_time=0x%x,",log_num_ret,(int)*rec_time_ret);
				printf("%d-%d-%d %d:%d:%d,\n",tm_rec->tm_year+1900,tm_rec->tm_mon+1,tm_rec->tm_mday,
						tm_rec->tm_hour,tm_rec->tm_min,tm_rec->tm_sec);

				continue;
			}
			else
			{
				//flag_reboot=0;
			}
			p_buf=strstr(buf_line,"log number" );
			if(NULL!=p_buf)//找到字段名
			{
				//单次写入记录头信息如下：
				//log number = 129   2015-10-30 15:23:02 -------------------------------
				//printf("analysis： %s",buf_line);

				//解析log number值与时间
				struct tm tm_log;
				int year;
				sscanf(buf_line,"log number = %d   %d-%d-%d %d:%d:%d ",&log_num,&year,&tm_log.tm_mon,&tm_log.tm_mday,
						&tm_log.tm_hour,&tm_log.tm_min,&tm_log.tm_sec);

				//tm is different, tm of dsp and tm of linux
				tm_log.tm_year=year-1900;//tm of dsp use 1900, tm of linux use 1990
				tm_log.tm_mon -= 1;//tm of dsp

				//转换成time_t
				rec_time=mktime(&tm_log);

			}
		}

	}while(flag_exit==0);

	//print the time result
	struct tm *tm_rec=localtime(rec_time_ret);
	printf("return : log_num=%d,rec_time=0x%x,",log_num_ret,(int)*rec_time_ret);
	printf("%d-%d-%d %d:%d:%d,\n",tm_rec->tm_year+1900,tm_rec->tm_mon+1,tm_rec->tm_mday,
			tm_rec->tm_hour,tm_rec->tm_min,tm_rec->tm_sec);


    fclose(f);
	return 0;
}










