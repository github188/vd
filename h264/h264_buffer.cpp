
/*********************************************
 *  h264_buffer.c
 *  time: 2013-5-16
 *  Author: zmh
 ********************************************/

 
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <memory.h>
#include <errno.h>   
#include <unistd.h>
#include <time.h>
#include <math.h>


#include <sys/un.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/shm.h>
#include <cmem.h>


#include "h264_buffer.h"
#include "debug.h"
#include "commonfuncs.h"

#include "logger/log.h"




//默认参数，构造函数初始化列表
H264Buffer::H264Buffer(int flag_create,H264_Create_Info *h264_create_info)
{
	printf("in H264Buffer : flag_create=%d\n", flag_create);

	physAddr_cmem=0;
	len_cmem=0;
	
	virtAddr_cmem=NULL;		
	flag_write_right=0;
	index_recv_h264=0;
	
	h264_buffer_head=NULL;
	index_h264=NULL;
	buf_h264=NULL;

	this->flag_create = flag_create;
	if(h264_create_info!=NULL)
	{
		memcpy(&(this->h264_create_info), h264_create_info, sizeof(H264_Create_Info));
	}	
	attach_shm(flag_create, h264_create_info);//可以屏蔽，模拟测试，在set_h264_buf或get_h264_for_record中调用
	
	printf("H264Buffer constructor\n");

}


//默认参数，构造函数初始化列表
H264Buffer::H264Buffer(unsigned int addr_phy_cmem,unsigned int len_cmem)
{
	printf("in H264Buffer : addr_phy_cmem=0x%x, len_cmem=%d\n", addr_phy_cmem, len_cmem );

	physAddr_cmem=addr_phy_cmem;
	this->len_cmem=len_cmem;
		
	//virtAddr_cmem=NULL;		
	flag_write_right=0;
	index_recv_h264=0;
	flag_create=0;
	
	h264_buffer_head=NULL;
	index_h264=NULL;
	buf_h264=NULL;

	if(addr_phy_cmem>0)	
	{
		if (CMEM_init() == -1)
		{
			printf("Failed to initialize CMEM\n");
			//return -1;
		}
		virtAddr_cmem=(char *)CMEM_registerAlloc((unsigned long)(physAddr_cmem));
		h264_buffer_head=(H264_Buffer_Head *)virtAddr_cmem;

		index_recv_h264=h264_buffer_head->index_recv_h264;
		index_h264=(H264_Index *)(virtAddr_cmem+sizeof(H264_Buffer_Head));
		buf_h264=((unsigned char *)index_h264)+sizeof(H264_Index)*h264_buffer_head->max_index_h264;
		//memset(index_h264,0,sizeof(H264_Index)*h264_buffer_head->max_index_h264);
	}
	else
	{
		virtAddr_cmem=NULL;	
		h264_buffer_head=NULL;
	}

	if(virtAddr_cmem == NULL)
	{
		printf("in H264Buffer : virtAddr_cmem is NULL\n");		
	}
	
	printf("H264Buffer constructor\n");

}


H264Buffer::~H264Buffer()
{	
	deattach_shm();
	printf("~H264Buffer\n");

}	

//创建或映射共享内存
//flag_create:   0，读权限；1，创建者；2，写权限
//创建: 根据输入的帧率，需要保护的时间，计算出帧索引的最大值。
//真实的存放h264数据的缓冲的大小，根据码率，保护时间计算出。
int H264Buffer::attach_shm(int flag_create,H264_Create_Info *h264_create_info)
{	

//	int len_cmem;
	int max_index_h264=0;
	int len_buf_h264=0;

	int key=SYS_SHM_KEY;


	if((physAddr_cmem!=0)&&(index_h264!=NULL))//避免重复创建、关联
	{
		printf("attach_shm: cmem has attached before. do nothing. \n");
		return 0;
	}


	if(flag_create==1)
	{
		if(h264_create_info==NULL)
		{
			printf("to create cmem,but h264_create_info is NULL\n");
			return -1;
		}
		//计算帧索引的最大值。	//乘以10倍，是为了尽量保证h264数据缓冲能充分的使用。
		//例: 60s * 15fps *10 =9000 ,  帧索引缓冲大小: 80*9000=720kByte
		max_index_h264=h264_create_info->protect_time * h264_create_info->camera_frame_rate * 10;
		//计算h264数据的缓冲大小	//例: 60s * 8Mbps/8=60MByte
		len_buf_h264=h264_create_info->protect_time * h264_create_info->rate * 1024*1024/8;

		printf("h264_create_info->protect_time=%d,camera_frame_rate=%d,rate=%d\n",h264_create_info->protect_time,h264_create_info->camera_frame_rate,h264_create_info->rate);
		printf("max_index_h264=%d,len_buf_h264=%d\n",max_index_h264,len_buf_h264);
		
		len_cmem   = sizeof(H264_Buffer_Head);
		len_cmem += sizeof(H264_Index)*max_index_h264;
		len_cmem += len_buf_h264;
		
		
		printf("mem get: size=%d\n",len_cmem);	

		//申请cmem
		CMEM_AllocParams  prm;

		prm.type      = CMEM_HEAP;
		prm.flags     = CMEM_NONCACHED;
		prm.alignment = 128;

		if (CMEM_init() == -1)
		{
			printf("Failed to initialize CMEM\n");
			return -1;
		}
		virtAddr_cmem = (char *)CMEM_alloc(len_cmem,&prm);
		physAddr_cmem = CMEM_getPhys(virtAddr_cmem);
		printf("mem get: physAddr_cmem=0x%x\n",	physAddr_cmem);	
		
	}
	else
	{
		printf("don't need get mem\n");				

	}

	
	if(virtAddr_cmem!=NULL)	
	{			

		{				
			printf("cmem alloc ok\n");					

			//获取共享内存信息，并计算索引buf，h264数据buf的位置
			h264_buffer_head=(H264_Buffer_Head *)virtAddr_cmem;

			if(flag_create==1)//创建者，初始化缓冲头信息
			{
				flag_write_right=1;
				h264_buffer_head->id_shmem=key;
				h264_buffer_head->num_writer=1;
				h264_buffer_head->max_index_h264=max_index_h264;
				h264_buffer_head->len_buf_h264=len_buf_h264;
				h264_buffer_head->index_recv_h264=0;
				snprintf(h264_buffer_head->version,sizeof(h264_buffer_head->version),"%s",h264_create_info->version);
				
			}
			else if(flag_create==2)//写权限
			{
				flag_write_right=1;	
				if(h264_buffer_head->id_shmem!=key)//要求所使用的键值与创建者写入的键值相等
				{
					printf("get shmem ok, but the key is not matching.\n");
					return -1;
				}
				if(h264_buffer_head->num_writer>0)//不能同时存在两个拥有写权限的操作者
				{
					printf("get shmem ok, but 2 writers is fobid.\n");
					return -1;					
				}
				h264_buffer_head->num_writer++;
			}
			else//读权限
			{
				flag_write_right=0;
				if(h264_buffer_head->id_shmem !=key)
				{
					printf("get shmem ok, but the key is not matching.\n");
					return -1;
				}
			}
			
			if(flag_write_right==1)//只有写入者才有权限修改共享缓冲内容。			
			{					
		
			}				

			index_recv_h264=h264_buffer_head->index_recv_h264;
			index_h264=(H264_Index *)(virtAddr_cmem+sizeof(H264_Buffer_Head));
			buf_h264=((unsigned char *)index_h264)+sizeof(H264_Index)*h264_buffer_head->max_index_h264;
			if(flag_create==1)//创建者，初始化索引信息
			{
				memset(index_h264,0,sizeof(H264_Index)*h264_buffer_head->max_index_h264);
			}
			return 0;//成功		
		}	
	}	

	
	return -1;	
	
}


int H264Buffer::deattach_shm()
{
	if(virtAddr_cmem!=NULL)	
	{
		if(flag_write_right==1)//只有写入者才有权限修改共享缓冲内容。			
		{					
			h264_buffer_head->num_writer--;
		}		
		
		//释放cmem

		//int CMEM_unregister(void *ptr, CMEM_AllocParams *params)

		CMEM_AllocParams  prm;

		prm.type      = CMEM_HEAP;
		prm.flags     = 0;
		prm.alignment = 0;

		CMEM_free(virtAddr_cmem,&prm);
				
	}
	
	virtAddr_cmem=NULL;
	
	return 0;	
}


//根据索引号，取时间，再取相应h264流 -- 接口函数，会在其它线程调用，需要考虑异常处理，多线程访问安全性
int H264Buffer::get_h264_for_record(H264_Seg_Info * h264_record_info,int sec_start,int sec_end,int illegal_video_seconds)
{
	int sec_start_record,sec_end_record;//违法的起始时间，终止时间
	int index_start_record,index_end_record;//违法在h264缓冲中对应的起止索引
//	int index_h264_start,index_h264_end;//h264的可访问的起始索引，终止索引
	static int flag_flushed=0;//缓冲是否填满过：0:未填满，1：已经填满		//若断开重连，需要清零吗？
	int ret=-1;
	int seconds_record = illegal_video_seconds;

//	printf("in get_h264_for_record\n");

	if(virtAddr_cmem==NULL)		
	{
		ret=attach_shm();//关联共享内存－－只有读取 权限
		if(ret<0)
		{
			printf("h264_buffer::get_h264_for_record : attach_shm failed \n");
			return -1;
		}
	}
	
	//添加对输出参数的异常处理
	if(h264_record_info==NULL)
	{
		printf("h264_err: h264_record_info is NULL\n");
		return -1;
	}

	//输入参数，起止时间异常//起止时间在同一秒也是可能的
	if((sec_end-sec_start)<0 || (sec_end-sec_start)>120)
	{
		printf("sec is err : sec_start=%d,sec_end=%d\n",sec_start,sec_end);
		return -1;
	}	
	printf("input sec is : sec_start=%d,sec_end=%d\n",sec_start,sec_end);

	//若指定违法记录时间长度异常      暂定异常：短于5s，长于2分钟  （通常为5-30s）
	if((seconds_record<5)||(seconds_record>120))
	{
		printf("h264_err: input  seconds_record is unormal  : %d\n",seconds_record);
		seconds_record = 5;//不是根本性错误，指定默认值，继续运行
	}

	index_recv_h264=h264_buffer_head->index_recv_h264;

	h264_record_info->h264_seg=0;//先设置为无效

	//根据索引号，取时间
	sec_start_record=sec_start;//若需精确对应，需要使用毫秒index_h264[index1].msecond
	sec_end_record=sec_end;
	if(sec_start_record>sec_end_record)
	{
		printf("h264_err: the input index is err: start time is %x; end time is %x\n",sec_start_record,sec_end_record);
		return -1;
	}

	int index_last = (index_recv_h264>0)?(index_recv_h264-1):(index_h264[0].buf_h264_index_max);//已经处理完成的最近的一帧
	if((index_last<0)||(index_last>=h264_buffer_head->max_index_h264))
	{
		printf("%s err: index_last=%d\n",__func__,index_last);
		return -1;
	}
	int sec_last = index_h264[index_last].seconds;
	if(sec_start_record>sec_last)
	{
		printf("h264_err: the needed video is not in h264 buf. sec_start_record=%d, sec_last=%d. \n",sec_start_record,sec_last);
		return -1;
	}


	//是否h264流已经填满了缓冲区，即缓冲写指针是否掉头过
	if(flag_flushed ==0 )//还未填满过一次
	{
		//index_recv_h264之前的索引号对应h264数据有效     第一次填充缓冲，未掉头

		//确定一个有效的索引号，进行判断  ,由于不知道实际填了多少个缓冲，尽量使用前面的缓冲进行判断。由于index_recv_h264是正在填充的，不要使用
		int index_valid=index_recv_h264>1?(index_recv_h264-1):(index_recv_h264+1);

		if((index_h264[index_valid].buf_h264_index == index_valid) && (index_h264[index_valid].h264_id != index_valid))
		{
			flag_flushed=1;//若某个缓冲已经填充过，且不是第一次填充，则整个h264缓冲已经填满过
		}
		else
		{
			flag_flushed=0;
			if((index_valid>index_recv_h264)&&(index_h264[index_valid].buf_h264_index == index_valid))
			{
				flag_flushed=1;//当前写缓冲之后的已经填充过，则整个h264缓冲已经填满过
			}
		}

	}

	//重新调整结束时间		
	//若设定的违法记录时间长度，比违法记录两个索引对应的时间间隔要大，就要扩大取违法录像的时间
	if(seconds_record > (sec_end_record-sec_start_record) )
	{
		if(sec_start_record+seconds_record > sec_last)//缓冲中数据不够指定时间长度
		{
			sec_end_record=sec_last;//取到最新的h264帧
		}
		else
		{
			sec_end_record = sec_start_record+seconds_record;//取到违法记录的指定时间长度
		}
	}


	//根据时间，取相应h264对应索引号
	ret=get_frame_at_sec(index_start_record,index_end_record,sec_start_record,sec_end_record,flag_flushed);
	if(ret<0)
	{
		printf("h264_err: get_frame_at_sec failed. \n");
		return -1;
	}

	//根据h264第一个索引号，向前取第一个遇到的I帧
	ret=get_I_frame_by_index(index_start_record,flag_flushed);
	if(ret<0)
	{
		printf("h264_err: get_I_frame_by_index failed.\n");
		return -1;
	}


	//根据违法记录，取到对应h264流的位置信息
	//返回一个结构体，避免进行多次的copy操作
//	h264_record_info->index_start_record=index_start_record;
//	h264_record_info->index_end_record=index_end_record;
//	h264_record_info->buf_h264_index_max=index_h264[0].buf_h264_index_max;
	h264_record_info->seconds_record_ret=sec_end_record - sec_start_record;

	if(index_start_record<=index_end_record)//如果在一个段内（没有遇到h264的缓冲尾）
	{
		int len_copy=0;

		for(int i=index_start_record;i<=index_end_record;i++)
		{
			len_copy+=index_h264[i].h264_Len;
		}
		h264_record_info->h264_seg=1;
		h264_record_info->h264_buf_seg[0]=buf_h264 + index_h264[index_start_record].h264_Position;
		h264_record_info->h264_buf_seg_size[0]=len_copy;
		
		//printf("before file_write: h264_buf_seg[0][3]=%d\n",*(buf_h264 + index_h264[index_start_record].h264_Position+3));
		for(int i=0;i<10;i++)
		{
		//	printf("before file_write: h264_buf_seg[0][%d]=%d\n",i, h264_record_info->h264_buf_seg[0][i]);
		}

	}
	else//如果分为两个段（跨过h264的缓冲尾）
	{
		int len_copy=0;
		for(int i=index_start_record;i<=index_h264[0].buf_h264_index_max;i++)
		{
			len_copy+=index_h264[i].h264_Len;
		}

		h264_record_info->h264_seg=2;
		h264_record_info->h264_buf_seg[0]=buf_h264 + index_h264[index_start_record].h264_Position;
		h264_record_info->h264_buf_seg_size[0]=len_copy;


		len_copy=0;
		for(int i=0;i<=index_end_record;i++)
		{
			len_copy+=index_h264[i].h264_Len;
		}

		h264_record_info->h264_buf_seg[1]=buf_h264 + index_h264[0].h264_Position;
		h264_record_info->h264_buf_seg_size[1]=len_copy;

	}

//	printf("\n\n\n\nin get_h264_for_record: h264_seg=%d,index_start_record=%d,index_end_record=%d,buf_h264_index_max=%d\n",
//			h264_record_info->h264_seg,
//			h264_record_info->index_start_record,
//			h264_record_info->index_end_record,
//			h264_record_info->buf_h264_index_max
//	);
//	printf("start is i:%d, sec_start=%x,sec_end=%x\n\n\n\n",
//			index_h264[ h264_record_info->index_start_record ].is_I_frame,
//			index_h264[ h264_record_info->index_start_record ].seconds,
//			index_h264[ h264_record_info->index_end_record ].seconds
//			);

	return 0;

}

//根据索引号取时间
int H264Buffer::get_sec_at_index( int index)
{
	if(virtAddr_cmem==NULL)		
	{
		printf("get_sec_at_index : shm err \n");
		return -1;
	}
	return index_h264[index].seconds;
}



//在h264缓冲中查找对应秒的帧，一秒对应多帧，取最靠前的一帧
int H264Buffer::get_frame_at_sec_forward( int& index_record, int sec_record,int flag_flushed)
{
	//有时，可能会出现没有尾标志的情况。例如：上次缓冲400帧，而这次由于每帧都比较小，可以缓冲500帧，在写到450帧时，既覆盖了原来的尾标志，又没有置新的尾标志。－－保存上次的最大帧数
	//在写缓冲的位置之外，都可以访问。考虑到取h264的耗时，写指针之后的2s区域，应纳入保护区域中--避免同时进行读写访问。
	//按时间的搜索，转换成按索引的搜索。可以使用2分法，比较快。
	//由于帧率可以预估，也可以直接跳转至较近的位置，然后查找－－更快。
	if(virtAddr_cmem==NULL)
	{
		printf("get_frame_at_sec, virtAddr_cmem=NULL\n");
		return -1;
	}


	index_record=-1;

	if(1)
	{
		//从最旧的帧到最新的帧，顺序寻找。统一处理，自动掉头。
		int flag_buf_continuous=((flag_flushed ==0 )||(index_recv_h264 >= index_h264[0].buf_h264_index_max));//buf连续标志。连续，即最旧的帧为0，最新的帧为最大帧。
		int index_tmp=0;
		int i=0;
		int index_max=(flag_buf_continuous==1) ? index_recv_h264 : index_h264[0].buf_h264_index_max;//缓冲中实际存储的最大帧数目
		printf("\nforward: flag_buf_continuous=%d, index_recv_h264=%d, buf_h264_index_max=%d\n\n", flag_buf_continuous, index_recv_h264, index_h264[0].buf_h264_index_max);
		while(i<index_max)//循环次数为最大值减1 ，当前操作帧不可用
		{
			index_tmp=(flag_buf_continuous==1) ? i : ((index_recv_h264+1+i)%(index_h264[0].buf_h264_index_max+1));
			i++;

			if((index_record!=-1))//找到匹配帧后，跳出循环
			{
				break;
			}

//			printf("get_frame_at_sec: sec_start_record=%d, index_h264[%d].seconds=%d\n", sec_record, index_tmp, index_h264[index_tmp].seconds);

			//查找对应秒的第一帧
			if( (index_h264[index_tmp].seconds==sec_record)&&
				(index_record==-1))
			{
				index_record=index_tmp;
				printf("get index_record=%d\n",index_record);
			}

		}
		printf("get_frame_at_sec_forward: i=%d index_record=%d\n",i,index_record);

	}

	if((index_record==-1))
	{
		//失败，没有找到有效的h264帧
		printf("h264_err: not found valid h264 frame for record \n");//断开相机一会，会导致进入此错误
		return -1;
	}

	return 0;

}

//在h264缓冲中查找对应秒的帧，一秒对应多帧，取最靠后的一帧
int H264Buffer::get_frame_at_sec_backward( int& index_record, int sec_record,int flag_flushed)
{
	//有时，可能会出现没有尾标志的情况。例如：上次缓冲400帧，而这次由于每帧都比较小，可以缓冲500帧，在写到450帧时，既覆盖了原来的尾标志，又没有置新的尾标志。－－保存上次的最大帧数
	//在写缓冲的位置之外，都可以访问。考虑到取h264的耗时，写指针之后的2s区域，应纳入保护区域中--避免同时进行读写访问。
	//按时间的搜索，转换成按索引的搜索。可以使用2分法，比较快。
	//由于帧率可以预估，也可以直接跳转至较近的位置，然后查找－－更快。
	if(virtAddr_cmem==NULL)
	{
		printf("get_frame_at_sec, virtAddr_cmem=NULL\n");
		return -1;
	}


	index_record=-1;

	if(1)
	{
		//从最旧的帧到最新的帧，顺序寻找。统一处理，自动掉头。
		int flag_buf_continuous=((flag_flushed ==0 )||(index_recv_h264 >= index_h264[0].buf_h264_index_max));//buf连续标志。连续，即最旧的帧为0，最新的帧为最大帧。
		int index_tmp=0;
		int i=0;
		int index_max=(flag_buf_continuous==1) ? index_recv_h264 : index_h264[0].buf_h264_index_max;//缓冲中实际存储的最大帧数目
		printf("\nbackward: flag_buf_continuous=%d, index_recv_h264=%d, buf_h264_index_max=%d\n\n", flag_buf_continuous, index_recv_h264, index_h264[0].buf_h264_index_max);
		while(i<index_max)//循环次数为最大值减1 ，当前操作帧不可用
		{
			index_tmp=(flag_buf_continuous==1) ? i : ((index_recv_h264+1+i)%(index_h264[0].buf_h264_index_max+1));
			i++;

			if((index_record!=-1))//找到匹配秒的索引后，调整秒对应的索引号至最靠后
			{
				printf("find end sec ,index_tmp=%d\n",index_tmp);
				if(index_h264[index_tmp].seconds==sec_record)
				{
					index_record=index_tmp;
					continue;//在已经找到首尾索引后，不再进入后面的判断
				}
				else
				{
					break;//遇到第一个不对应当前秒的索引，跳出。
				}
			}

//			printf("get_frame_at_sec: sec_end_record=%d, index_h264[%d].seconds=%d\n", sec_record, index_tmp, index_h264[index_tmp].seconds);
			//查找对应秒的第一帧
			if( (index_h264[index_tmp].seconds==sec_record)&&
				(index_record==-1))
			{
				index_record=index_tmp;
				printf("get index_record=%d\n",index_record);
			}

		}
		printf("get_frame_at_sec_backward: i=%d index_record=%d\n",i,index_record);

	}

	if((index_record==-1))
	{
		//失败，没有找到有效的h264帧
		printf("h264_err: not found valid h264 frame for record \n");//断开相机一会，会导致进入此错误
		return -1;
	}

	return 0;

}


//根据时间，取相应h264对应索引号
//希望取到的视频，尽量包含完整时间段，起止点尽量与要求的时间匹配
int H264Buffer::get_frame_at_sec( int& index_start_record, int& index_end_record,int sec_start_record,int sec_end_record,int flag_flushed)
{

	//有时，可能会出现没有尾标志的情况。例如：上次缓冲400帧，而这次由于每帧都比较小，可以缓冲500帧，在写到450帧时，既覆盖了原来的尾标志，又没有置新的尾标志。－－保存上次的最大帧数
	//在写缓冲的位置之外，都可以访问。考虑到取h264的耗时，写指针之后的2s区域，应纳入保护区域中--避免同时进行读写访问。
	//按时间的搜索，转换成按索引的搜索。可以使用2分法，比较快。
	//由于帧率可以预估，也可以直接跳转至较近的位置，然后查找－－更快。
	if(virtAddr_cmem==NULL)		
	{
		printf("get_frame_at_sec, virtAddr_cmem=NULL\n");
		return -1;
	}
	index_start_record=-1;
	index_end_record=-1;

	if(1)
	{
		//搜索起始秒对应的帧
		//h264分段录像，每次开始时需要寻找I帧，可能有所滞后，所以优先向后搜索
		get_frame_at_sec_forward(index_start_record,sec_start_record,flag_flushed);//搜索当前秒
		if((index_start_record==-1))
		{
			get_frame_at_sec_forward(index_start_record,sec_start_record-1,flag_flushed);//搜索后一秒
			if((index_start_record==-1))
			{
				get_frame_at_sec_forward(index_start_record,sec_start_record-2,flag_flushed);//搜索后两秒
				if((index_start_record==-1))
				{
					get_frame_at_sec_forward(index_start_record,sec_start_record+1,flag_flushed);//搜索前一秒
					if((index_start_record==-1))
					{
						get_frame_at_sec_forward(index_start_record,sec_start_record+2,flag_flushed);//搜索前两秒
					}
				}
			}
		}

		//搜索停止秒对应的帧
		//尽量时间准确，靠近当前秒
		get_frame_at_sec_backward(index_end_record,sec_end_record,flag_flushed);//搜索当前秒
		if(index_end_record==-1)
		{
			get_frame_at_sec_backward(index_end_record,sec_end_record-1,flag_flushed);//搜索后一秒
			if((index_end_record==-1))
			{
				get_frame_at_sec_backward(index_end_record,sec_end_record+1,flag_flushed);//搜索后两秒
				if((index_end_record==-1))
				{
					get_frame_at_sec_backward(index_end_record,sec_end_record-2,flag_flushed);//搜索前一秒
					if((index_end_record==-1))
					{
						get_frame_at_sec_backward(index_end_record,sec_end_record+2,flag_flushed);//搜索前两秒
					}
				}
			}
		}

	}
	else
	{
		if(1)
		{
			//只搜索当前秒对应的帧
			get_frame_at_sec_forward(index_start_record,sec_start_record,flag_flushed);

			get_frame_at_sec_backward(index_end_record,sec_end_record,flag_flushed);
		}
		else
		{
			//尽量扩大范围，保证截取的h264的完整  //适合于h264录像连续完整的情况
			//搜索起始秒对应的帧
			get_frame_at_sec_forward(index_start_record,sec_start_record-2,flag_flushed);
			if((index_start_record==-1))
			{
				get_frame_at_sec_forward(index_start_record,sec_start_record-1,flag_flushed);
				if((index_start_record==-1))
				{
					get_frame_at_sec_forward(index_start_record,sec_start_record,flag_flushed);
					if((index_start_record==-1))
					{
						get_frame_at_sec_forward(index_start_record,sec_start_record+1,flag_flushed);
						if((index_start_record==-1))
						{
							get_frame_at_sec_forward(index_start_record,sec_start_record+2,flag_flushed);
						}
					}
				}
			}

			//搜索停止秒对应的帧
			get_frame_at_sec_backward(index_end_record,sec_end_record+2,flag_flushed);
			if(index_end_record==-1)
			{
				get_frame_at_sec_backward(index_end_record,sec_end_record+1,flag_flushed);
				if((index_end_record==-1))
				{
					get_frame_at_sec_backward(index_end_record,sec_end_record,flag_flushed);
					if((index_end_record==-1))
					{
						get_frame_at_sec_backward(index_end_record,sec_end_record-1,flag_flushed);
						if((index_end_record==-1))
						{
							get_frame_at_sec_backward(index_end_record,sec_end_record-2,flag_flushed);
						}
					}
				}
			}

		}
	}
	printf("get_frame_at_sec ,index_start_record=%d,index_end_record=%d\n",index_start_record,index_end_record);

	if((index_start_record==-1)||(index_end_record==-1))
	{
		//失败，没有找到有效的h264帧
		printf("h264_err: not found valid h264 frame for record \n");//断开相机一会，会导致进入此错误
		return -1;
	}

	return 0;
	
}


//根据h264第一个索引号，向前取第一个遇到的I 帧
int H264Buffer::get_I_frame_by_index( int& index_start_record,int flag_flushed)
{
	int flag_get_I_frame=0;

	if(virtAddr_cmem==NULL)		
	{
		printf("get_I_frame_by_index, virtAddr_cmem=NULL\n");
		return -1;
	}
		
	if((flag_flushed ==0 )||(index_recv_h264 >= index_h264[0].buf_h264_index_max))//0-index_recv_h264
	{
		int i=index_start_record;
		while(i>=0)//往前查找，寻找范围：index_start_record-0
		{
			printf("get_I_frame_by_index:i=%d,index_h264[i].is_I_frame=%d\n",i,index_h264[i].is_I_frame);
			if(index_h264[i].is_I_frame == 1)//找到I 帧
			{
				index_start_record=i;
				flag_get_I_frame=1;
				break;
			}
			i--;
		}

	}
	else 
	{
		//需要分两段处理
		//先判断写指针之后的部分,再判断写指针之前的----也可以统一处理，加上自动掉头判断即可。
		int i=index_start_record;
		if(i<index_recv_h264)
		{
			while(i>=0)//(i<=index_h264[0].buf_h264_index_max)//寻找范围：index_start_record-0
			{
				printf("get_I_frame_by_index:i=%d,index_h264[i].is_I_frame=%d\n",i,index_h264[i].is_I_frame);
				if(index_h264[i].is_I_frame == 1)//找到I 帧
				{
					index_start_record=i;
					flag_get_I_frame=1;
					break;
				}
				i--;
			}
			if(flag_get_I_frame!=1)
			{
				i=index_h264[0].buf_h264_index_max;
			}
		}
		else
		{
			i=index_start_record;
		}

		if(flag_get_I_frame!=1)
		{
			while(i>index_recv_h264)//寻找范围：尾部或者index_start_record，到index_recv_h264
			{
				printf("get_I_frame_by_index:i=%d,index_h264[i].is_I_frame=%d\n",i,index_h264[i].is_I_frame);
				if(index_h264[i].is_I_frame == 1)//找到I 帧
				{
					index_start_record=i;
					flag_get_I_frame=1;
					break;
				}
				i--;
			}
		}

	}

	if(flag_get_I_frame!=1)
	{
		printf("h264_err: find I frame failed:  flag_get_I_frame=%d\n",flag_get_I_frame);
		return -1;
	}

	return 0;

}

int H264Buffer::get_index_info(H264_Index *h264_index_ret, int last_cy, int index)
{

	if(h264_index_ret==NULL)
	{
		printf("get_index_info: h264_index_ret is NULL\n");
		return -1;
	}

	if(virtAddr_cmem==NULL)		
	{
		printf("get_index_info, virtAddr_cmem=NULL\n");
		return -1;
	}

	
	if(last_cy==1)
	{
		//寻找已经处理完成的最新的一帧的索引信息
		int index_temp=0;		
		if(index_recv_h264==0)
		{			
			index_temp=index_h264[0].buf_h264_index_max;
		}
		else
		{
			index_temp=index_recv_h264-1;
		}

		memcpy(h264_index_ret, index_h264+index_temp, sizeof(H264_Index));
	
	}
	else
	{
		//寻找指定帧的索引信息
		int index_valid=index_recv_h264==0?0:(index_recv_h264-1);
		int max_index;//=max(index_h264[index_valid].buf_h264_index_max, index_recv_h264-1);
		if(index_h264[index_valid].buf_h264_index_max > index_recv_h264-1)
		{
			max_index = index_h264[index_valid].buf_h264_index_max;
		}
		else
		{
			max_index = index_recv_h264-1;
		}
			
		if((index<0) || (index>max_index))
		{
			printf("input index=%d,is out of range\n",index);
			return -1;
		}
		if(index==index_recv_h264)
		{
			printf("input index=%d,is just the recv index, cannot use!\n",index);
			return -1;	
		}
		memcpy(h264_index_ret, index_h264+index, sizeof(H264_Index));

	}
	
	return 0;

}
	
//获取版本信息
int H264Buffer::get_version(char * str_version,int len)
{
	if(virtAddr_cmem==NULL)		
	{
		printf("get_version : shm err \n");
		return -1;
	}
	if(str_version==NULL)		
	{
		printf("get_version : str_version is NULL \n");
		return -1;
	}
	if(len<0)
	{
		printf("get_version : len=%d <0  failed \n",len);
		return -1;
	}
	
	int ret=snprintf(str_version, len, "%s", h264_buffer_head->version);

	return ret;
}

//获取CMEM 虚拟地址
char * H264Buffer::get_cmem_addr_virt()
{
	return virtAddr_cmem;
}

//获取CMEM 物理地址
unsigned int H264Buffer::get_cmem_addr_phy()
{
	return physAddr_cmem;
}

//获取CMEM 长度
unsigned int H264Buffer::get_cmem_len()
{
	return len_cmem;
}

//测试函数: 取违法记录对应的h264流，并写文件。
int H264Buffer::test_get_ill_record_h264()
{
	int ret=0;
	static int cy_first=1;

	//需要赋值:
//	int num_jpeg_max;
//	int video_index_head;
//	int video_index_tail;

	if(virtAddr_cmem==NULL)		
	{
		printf("test_get_ill_record_h264 : virtAddr_cmem=NULL \n");
		return -1;
	}


	//当前进行写操作的帧，应避免访问。
	//注意：buf_index为正在操作的索引，其对应值不可访问！  应访问其前面已经写好的值，例如： (buf_index+num_jpeg_max-1)%num_jpeg_max
	if(((cy_first>=1) && (index_recv_h264==160))||(flag_write_right==0))
	{
		//只有第二次进入才能写文件，模拟跨尾情况//读模式不用考虑
		cy_first++;
		if(cy_first<3)//测试
		{
			printf("cy_first=%d\n",cy_first);
			if(flag_write_right!=0)//写模式，跨过第一次，避免缓冲未写满的情况－－特别对于跨缓冲尾的情况
				return 0;
		}
		//cy_first=0;

		printf("to write h264 record: index_recv_h264=%d\n",index_recv_h264);

		static H264_Seg_Info * h264_record_info=NULL;
		if(h264_record_info==NULL)
		{
			h264_record_info=new (H264_Seg_Info);
			if(h264_record_info==NULL)
			{
				printf("malloc h264_record_info failed\n");
				return -1;
			}
			else
			{
				memset(h264_record_info, 0, sizeof(H264_Seg_Info));
			}
		}
		//模拟给出起止index，取相应时间对应的h264
		int index1,index3;//h264的索引号，
		index1=50;//不跨缓冲尾
		index3=160-30;
		////index1=800;//跨缓冲尾  //只在特定场景可用: 设置缓冲15s，存约1200帧
		//index1=index_h264[0].buf_h264_index_max-50;//跨缓冲尾 ，可以动态使用
		//index3=30;

		if((index1<=index_recv_h264)&&(index3>=index_recv_h264))
		{
			printf("test_get_ill_record_h264: index not in area:  index_recv_h264=%d,index1=%d,index3=%d\n",index_recv_h264,index1,index3);
			if(h264_record_info!=NULL)
			{
				delete( h264_record_info);
				h264_record_info=NULL;
			}
			return -1;
		}

		printf("test_get_ill_record_h264: index1=%d,index3=%d\n",index1,index3);
		int sec_start=index_h264[index1].seconds;
		int sec_end=index_h264[index3].seconds;
		
		int ret_get=get_h264_for_record( h264_record_info, sec_start, sec_end,5);

		if(ret_get==0)
		{
			char filename[50];
			struct tm *p_tm;
			time_t timep;
		    struct tm time_log;

		    time(&timep);
		    p_tm= localtime_r(&timep,&time_log);
		    sprintf(filename, "%04d%02d%02d-%02d%02d%02d.avi", (1900 + p_tm->tm_year), (1 + p_tm->tm_mon), p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);

		    printf("test_get_ill_record_h264: to write h264 file: %s , h264_seg=%d\n",filename,h264_record_info->h264_seg);
		    printf("buf_h264 addr=0x%x\n",(unsigned int)buf_h264);
		    if(h264_record_info->h264_seg >= 1)
		    {
			printf("test_get_ill_record_h264: 1: buf: 0x%x , size=%d\n",(unsigned int)h264_record_info->h264_buf_seg[0],h264_record_info->h264_buf_seg_size[0]);
			for(int i=0;i<10;i++)
			{
			//	printf("before file_write: h264_buf_seg[0][%d]=%d\n",i, h264_record_info->h264_buf_seg[0][i]);
			}
		    	file_write(filename,(char *)h264_record_info->h264_buf_seg[0],h264_record_info->h264_buf_seg_size[0]);//写nal流数据，查看，解析测试用
		    }

		    if(h264_record_info->h264_seg >= 2)
		    {
   		    	printf("test_get_ill_record_h264: 2: buf: 0x%x , size=%d\n",(unsigned int)h264_record_info->h264_buf_seg[1],h264_record_info->h264_buf_seg_size[1]);
		    	file_write(filename,(char *)h264_record_info->h264_buf_seg[1],h264_record_info->h264_buf_seg_size[1]);//写nal流数据，查看，解析测试用
		    }


		}
		else
		{
			printf("h264_err: get_h264_for_record return failed\n");
			ret = -1;
		}


	    //打印记录对应每一帧的信息，查看是否正确！
	    if(h264_record_info->h264_seg == 1)
	    {
	    	//for(int i=h264_record_info->index_start_record;i<=h264_record_info->index_end_record;i++)
	    	//{
	    	//	printf("i=%d,buf_h264_index=%d,",i,index_h264[i].buf_h264_index);
		//		printf("h264_id=%d,seconds=%d ,timestamp=0x%x \n",index_h264[i].h264_id,index_h264[i].seconds,index_h264[i].timestamp);
		//		printf("h264_Len=%d,h264_Position=%d, is_I_frame=%d\n",index_h264[i].h264_Len,index_h264[i].h264_Position,index_h264[i].is_I_frame);
	    	//}
	    }

	    else if(h264_record_info->h264_seg == 2)
	    {
	    	//for(int i=h264_record_info->index_start_record;i<=h264_record_info->buf_h264_index_max;i++)
	    	//{
	    	//	printf("i=%d,buf_h264_index=%d,",i,index_h264[i].buf_h264_index);
		//		printf("h264_id=%d,seconds=%d ,timestamp=0x%x \n",index_h264[i].h264_id,index_h264[i].seconds,index_h264[i].timestamp);
		//		printf("h264_Len=%d,h264_Position=%d, is_I_frame=%d\n",index_h264[i].h264_Len,index_h264[i].h264_Position,index_h264[i].is_I_frame);
	    	//}
	    	//for(int i=0;i<=h264_record_info->index_end_record;i++)
	    	//{
	    	//	printf("i=%d,buf_h264_index=%d,",i,index_h264[i].buf_h264_index);
		//		printf("h264_id=%d,seconds=%d ,timestamp=0x%x \n",index_h264[i].h264_id,index_h264[i].seconds,index_h264[i].timestamp);
		//		printf("h264_Len=%d,h264_Position=%d, is_I_frame=%d\n",index_h264[i].h264_Len,index_h264[i].h264_Position,index_h264[i].is_I_frame);
	    	//}
	    }
	    else
	    {
	    	printf("h264_err: get h264 err: h264_seg=%d\n",h264_record_info->h264_seg);

	    }

		delete( h264_record_info);
		h264_record_info=NULL;

	}

	return ret;

}


//获取h264_pack 程序的版本号
//输入参数: version_h264_pack 用于存放输出的版本信息；len用于指定version_h264_pack 缓冲的最大长度
//返回值: 小于0，表示获取版本信息失败，大于0，表示版本信息长度
int get_version_h264_pack(char *version_h264_pack, int len)
{

	if(version_h264_pack==NULL)
	{
		printf("get_version_h264_pack: version_h264_pack is NULL\n");
		return -1;
	}

	if(len<=0)
	{
		printf("get_version_h264_pack: len=%d, <0 ,failed.\n", len);
		return -1;
	}


	H264Buffer *h264_buffer = new H264Buffer();	//只有读取 权限
	if(h264_buffer == NULL)
	{
		printf("h264_buffer new failed \n");
		return -1;
	}

	int ret=h264_buffer->get_version(version_h264_pack, len);

	delete h264_buffer ;
	h264_buffer=NULL;

	return ret;
}




//在缓冲buf 中寻找字符串str
int find_str_in_buf(char * buf_jpeg,int len_jpeg,char * str,int len_str)
{
	int pos_buf=0;
	char * buf_tmp=buf_jpeg;
	int i,num;
	//printf("len_jpeg=%d,len_str=%d\n",len_jpeg,len_str);
	while(pos_buf+len_str<=len_jpeg)
	{
		num=0;
		for(i=0;i<len_str;i++)
		{
			//printf("buf_tmp[%d]=%c\n",i,buf_tmp[i]);
			if(buf_tmp[i]==str[i])
			{
				num++;
			}
			else
			{
				break;
			}
		}
		if(num==len_str)
		{
			return pos_buf;
		}
		pos_buf++;
		buf_tmp++;
	}
	return -1;

}

long long h264_get_timestamp_packheader(char *buf,int buf_len)
{

	//时间戳数据分布：3，1，15，1，15，1，（有效位：3+15+15）
	long long time_stamp=0;
	time_stamp = (buf[0]&0x38)>>3;

	time_stamp = time_stamp<<2;
	time_stamp += buf[0]&0x03;

	time_stamp = time_stamp<<8;
	time_stamp += buf[1]&0xff;


	time_stamp = time_stamp<<5;
	time_stamp += (buf[2]&0xf8)>>3;

	time_stamp = time_stamp<<2;
	time_stamp += buf[2]&0x03;

	time_stamp = time_stamp<<8;
	time_stamp += buf[3]&0xff;

	time_stamp = time_stamp<<5;
	time_stamp += (buf[4]&0xf8)>>3;

	return time_stamp;

}

int h264_change_timestamp_packheader(char *buf,int buf_len,long long timestamp_set)
{
		//时间戳数据分布：3，1，15，1，15，1，（有效位：3+15+15）
		//pack header: 存放在5个字节中，时间戳有关位置1： 3b ff fb ff f8 (共35位，中间有两位与时间戳没有关系)

	if(buf==NULL)
	{
		printf("in %s buf is null\n",__func__);
		return -1;
	}

	//将时间戳存放到5个字节中：
	//0-4位：
	buf[4]=(buf[4]&0x07)+((timestamp_set&0x1f)<<3);
	timestamp_set>>=5;
	//5-12位：
	buf[3]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//13-14位：
	buf[2]=(buf[2]&0xfc)+(timestamp_set&0x03);
	timestamp_set>>=2;
	//15-19位：
	buf[2]=(buf[2]&0x07)+((timestamp_set&0x1f)<<3);
	timestamp_set>>=5;
	//20-27位：
	buf[1]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//28-29位：
	buf[0]=(buf[0]&0xfc)+(timestamp_set&0x03);
	timestamp_set>>=2;
	//30-32位：
	buf[0]=(buf[0]&0xc7)+((timestamp_set&0x07)<<3);
	timestamp_set>>=3;


	return 0;

}


long long h264_get_timestamp_PESheader(char *buf,int buf_len)
{
	//时间戳数据分布：3，1，15，1，15，1，（有效位：3+15+15）
	long long time_stamp=0;
	time_stamp = (buf[0]&0x0E)>>1;

	time_stamp = time_stamp<<8;
	time_stamp += buf[1]&0xff;

	time_stamp = time_stamp<<7;
	time_stamp += (buf[2]&0xfe)>>1;

	time_stamp = time_stamp<<8;
	time_stamp += buf[3]&0xff;

	time_stamp = time_stamp<<7;
	time_stamp += (buf[4]&0xfe)>>1;

	return time_stamp;

}

int h264_change_timestamp_PESheader(char *buf,int buf_len,long long timestamp)
{
		//时间戳数据分布：3，1，15，1，15，1，（有效位：3+15+15）
		//PES header: 存放在5个字节中，时间戳有关位置1： 0e ff fe ff fe (共35位，中间有两位与时间戳没有关系)

	if(buf==NULL)
	{
		printf("in %s buf is null\n",__func__);
		return -1;
	}
	long long timestamp_set=timestamp;

	//将时间戳存放到5个字节中：
	//0-6位：
	buf[4]=(buf[4]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//7-14位：
	buf[3]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//15-22位：
	buf[2]=(buf[2]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//23-30位：
	buf[1]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//31-33位：
	buf[0]=(buf[0]&0xf1)+((timestamp_set&0x07)<<1);
	timestamp_set>>=3;


	timestamp_set=timestamp;

	//将时间戳存放到5个字节中：
	//0-6位：
	buf[9]=(buf[9]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//7-14位：
	buf[8]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//15-22位：
	buf[7]=(buf[7]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//23-30位：
	buf[6]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//31-33位：
	buf[5]=(buf[5]&0xf1)+((timestamp_set&0x07)<<1);
	timestamp_set>>=3;


	return 0;

}

#define PACK_HEARDER 0
#define PES_HEARDER 1
#define INTERVAL_TIMESTAMP 3600

//对h264的ps流，调整时间戳
int h264_ps_adjust_timestamp(char *buf, int buf_len, long long  timestamp_start, long long * p_timestamp_end )
{
	//输入参数判断
	if(buf==NULL)
	{
		ERROR("buf is NULL\n");
		return -1;
	}
	if(buf_len<=0)
	{
		ERROR("buf_len is %d ,too small\n",buf_len);
		return -1;
	}
	DEBUG("%s: buf_len is %d ,buf is 0x%x\n",__func__,buf_len,(int)buf);
	//验证ps流的有效性



	int pos_buf=0;
	int pos_pack_header=0;
	int pos_PES_header=0;
	int pos_pack_total=0;
	int pos_PES_total=0;
	char str_PackHeader[8];
	char str_PESHeader[8];
	long long timestamp_pack=0;
	long long timestamp_PES=0;
	long long timestamp_old=0;
	long long timestamp_set=0;
	long long timestamp_diff=0;
	int ret=-1;
	int flag_first_stamp=1; //0;//test

	memset (str_PackHeader,0,sizeof(str_PackHeader));
	str_PackHeader[0]=0x00;
	str_PackHeader[1]=0x00;
	str_PackHeader[2]=0x01;
	str_PackHeader[3]=0xBA;

	memset (str_PESHeader,0,sizeof(str_PESHeader));
	str_PESHeader[0]=0x00;
	str_PESHeader[1]=0x00;
	str_PESHeader[2]=0x01;
	str_PESHeader[3]=0xE0;

	int num_loop=0;
	if(timestamp_start!=-1)
	{
		timestamp_old=timestamp_start;
	}

	int flag_timestamp=PES_HEARDER;//0,pack_header  1,PES_header
	int flag_timestamp_old=PES_HEARDER;//0,pack_header  1,PES_header   取值为pack_header时，有特殊作用

	while(1)
	{

		//寻找最近的时间戳
//		if(pos_PES_header>=pos_pack_header)
		if(pos_PES_total>=pos_pack_total)//避免多次查找同一个包头//使用绝对位置进行判断 //相对位置，在有些时候不合适
		{			
			pos_pack_header=find_str_in_buf( buf+pos_buf,buf_len-pos_buf,str_PackHeader,4);
			//printf("find_str_in_buf,pos_buf=%d,pos_pack_header=%d\n",pos_buf,pos_pack_header);
			if(pos_pack_header<0)
			{
				DEBUG("find_str_in_buf failed,pos_buf=%d\n",pos_pack_header);
				//return -1;
			}
			else
			{
				pos_pack_total=pos_buf+pos_pack_header;
			}
		}

		pos_PES_header=find_str_in_buf( buf+pos_buf,buf_len-pos_buf,str_PESHeader,4);
//		printf_with_ms("find_str_in_buf,pos_buf=%d,pos_pack_header=%d,pos_PES_header=%d,num_loop=%d\n",
//					pos_buf,pos_pack_header,pos_PES_header,num_loop);
		if(pos_PES_header<0)
		{
			DEBUG("find_str_in_buf failed,pos_buf=%d\n",pos_PES_header);
			//return -1;
			if(pos_pack_header<0)
			{
				ERROR("can not find timestamp,break.\n");
				break;
			}
		}
		else
		{
			pos_PES_total=pos_buf+pos_PES_header;
		}

		//if((pos_PES_header>pos_pack_header)&&(pos_pack_header>=0))
		if((pos_PES_total>pos_pack_total)&&(pos_pack_header>=0))
		{
			flag_timestamp=PACK_HEARDER;
//			pos_buf+=pos_pack_header;
			pos_buf=pos_pack_total;
		}
		else
		{
			flag_timestamp=PES_HEARDER;
//			pos_buf+=pos_PES_header;
			pos_buf=pos_PES_total;
		}

//		printf_with_ms("after find timestamp: flag_timestamp=%d,pos_buf=%d\n",flag_timestamp,pos_buf);



		//解析时间戳
		if(flag_timestamp==PACK_HEARDER)//pack_header
		{
			pos_buf+=4;
			timestamp_pack=h264_get_timestamp_packheader(buf+pos_buf,buf_len-pos_buf);
			timestamp_diff=timestamp_pack-timestamp_old;
			//printf_with_ms("pos_buf=%d,timestamp_pack=%lld, time_diff=%lld\n",pos_buf,timestamp_pack,timestamp_diff);

			if(timestamp_pack==0)
			{
				DEBUG("timestamp is 0\n");
			}
			else if( ((flag_first_stamp==1) && (timestamp_start==-1)) //第一次获取时间戳，用作基础值
						  || ((timestamp_diff>=0)&&(timestamp_diff<=INTERVAL_TIMESTAMP*2)) )//或者时间戳正常
			{
				flag_first_stamp=0;
				timestamp_old=timestamp_pack;
			}
			else //时间戳异常
			{
				DEBUG("pos_buf=%d,timestamp_pack=%lld, time_diff=%lld\n",pos_buf,timestamp_pack,timestamp_diff);
				timestamp_set=timestamp_old+INTERVAL_TIMESTAMP;
				ret=h264_change_timestamp_packheader(buf+pos_buf,buf_len-pos_buf,timestamp_set);
				timestamp_old=timestamp_set;
			}
		}
		else//PES_header
		{
			pos_buf+=9;
			timestamp_PES=h264_get_timestamp_PESheader(buf+pos_buf,buf_len-pos_buf);
			timestamp_diff=timestamp_PES-timestamp_old;
			//printf("pos_buf=%d,timestamp_PES=%lld,time_diff=%lld\n",pos_buf,timestamp_PES,timestamp_diff);


			if(timestamp_PES==0)
			{
				DEBUG("timestamp is 0\n");
			}
			else if( ((flag_first_stamp==1) && (timestamp_start==-1)) //第一次获取时间戳，用作基础值
						  || ((timestamp_diff>=0)&&(timestamp_diff<=INTERVAL_TIMESTAMP*2)) )//或者时间戳正常
			{
				flag_first_stamp=0;
				timestamp_old=timestamp_PES;
			}
			else //时间戳异常
			{
//				DEBUG("pos_buf=%d,timestamp_PES=%lld,time_diff=%lld\n",pos_buf,timestamp_PES,timestamp_diff);
				//如果上一个时间戳是包头的，那么，接下来的pes头的时间戳，应该是保持一致的，不应该再次添加一个时间戳间隔
				if(flag_timestamp_old==PACK_HEARDER)
				{
					timestamp_set=timestamp_old;
				}
				else
				{
					timestamp_set=timestamp_old+INTERVAL_TIMESTAMP;
				}
				ret=h264_change_timestamp_PESheader(buf+pos_buf,buf_len-pos_buf,timestamp_set);
				timestamp_old=timestamp_set;
			}
		}

		flag_timestamp_old=flag_timestamp;
//		printf("timestamp_old=%lld\n",timestamp_old);

		num_loop++;


	}//end while(1)
	*p_timestamp_end=timestamp_old;

	return 0;

}




