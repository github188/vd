
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




//Ĭ�ϲ��������캯����ʼ���б�
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
	attach_shm(flag_create, h264_create_info);//�������Σ�ģ����ԣ���set_h264_buf��get_h264_for_record�е���
	
	printf("H264Buffer constructor\n");

}


//Ĭ�ϲ��������캯����ʼ���б�
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

//������ӳ�乲���ڴ�
//flag_create:   0����Ȩ�ޣ�1�������ߣ�2��дȨ��
//����: ���������֡�ʣ���Ҫ������ʱ�䣬�����֡���������ֵ��
//��ʵ�Ĵ��h264���ݵĻ���Ĵ�С���������ʣ�����ʱ��������
int H264Buffer::attach_shm(int flag_create,H264_Create_Info *h264_create_info)
{	

//	int len_cmem;
	int max_index_h264=0;
	int len_buf_h264=0;

	int key=SYS_SHM_KEY;


	if((physAddr_cmem!=0)&&(index_h264!=NULL))//�����ظ�����������
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
		//����֡���������ֵ��	//����10������Ϊ�˾�����֤h264���ݻ����ܳ�ֵ�ʹ�á�
		//��: 60s * 15fps *10 =9000 ,  ֡���������С: 80*9000=720kByte
		max_index_h264=h264_create_info->protect_time * h264_create_info->camera_frame_rate * 10;
		//����h264���ݵĻ����С	//��: 60s * 8Mbps/8=60MByte
		len_buf_h264=h264_create_info->protect_time * h264_create_info->rate * 1024*1024/8;

		printf("h264_create_info->protect_time=%d,camera_frame_rate=%d,rate=%d\n",h264_create_info->protect_time,h264_create_info->camera_frame_rate,h264_create_info->rate);
		printf("max_index_h264=%d,len_buf_h264=%d\n",max_index_h264,len_buf_h264);
		
		len_cmem   = sizeof(H264_Buffer_Head);
		len_cmem += sizeof(H264_Index)*max_index_h264;
		len_cmem += len_buf_h264;
		
		
		printf("mem get: size=%d\n",len_cmem);	

		//����cmem
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

			//��ȡ�����ڴ���Ϣ������������buf��h264����buf��λ��
			h264_buffer_head=(H264_Buffer_Head *)virtAddr_cmem;

			if(flag_create==1)//�����ߣ���ʼ������ͷ��Ϣ
			{
				flag_write_right=1;
				h264_buffer_head->id_shmem=key;
				h264_buffer_head->num_writer=1;
				h264_buffer_head->max_index_h264=max_index_h264;
				h264_buffer_head->len_buf_h264=len_buf_h264;
				h264_buffer_head->index_recv_h264=0;
				snprintf(h264_buffer_head->version,sizeof(h264_buffer_head->version),"%s",h264_create_info->version);
				
			}
			else if(flag_create==2)//дȨ��
			{
				flag_write_right=1;	
				if(h264_buffer_head->id_shmem!=key)//Ҫ����ʹ�õļ�ֵ�봴����д��ļ�ֵ���
				{
					printf("get shmem ok, but the key is not matching.\n");
					return -1;
				}
				if(h264_buffer_head->num_writer>0)//����ͬʱ��������ӵ��дȨ�޵Ĳ�����
				{
					printf("get shmem ok, but 2 writers is fobid.\n");
					return -1;					
				}
				h264_buffer_head->num_writer++;
			}
			else//��Ȩ��
			{
				flag_write_right=0;
				if(h264_buffer_head->id_shmem !=key)
				{
					printf("get shmem ok, but the key is not matching.\n");
					return -1;
				}
			}
			
			if(flag_write_right==1)//ֻ��д���߲���Ȩ���޸Ĺ��������ݡ�			
			{					
		
			}				

			index_recv_h264=h264_buffer_head->index_recv_h264;
			index_h264=(H264_Index *)(virtAddr_cmem+sizeof(H264_Buffer_Head));
			buf_h264=((unsigned char *)index_h264)+sizeof(H264_Index)*h264_buffer_head->max_index_h264;
			if(flag_create==1)//�����ߣ���ʼ��������Ϣ
			{
				memset(index_h264,0,sizeof(H264_Index)*h264_buffer_head->max_index_h264);
			}
			return 0;//�ɹ�		
		}	
	}	

	
	return -1;	
	
}


int H264Buffer::deattach_shm()
{
	if(virtAddr_cmem!=NULL)	
	{
		if(flag_write_right==1)//ֻ��д���߲���Ȩ���޸Ĺ��������ݡ�			
		{					
			h264_buffer_head->num_writer--;
		}		
		
		//�ͷ�cmem

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


//���������ţ�ȡʱ�䣬��ȡ��Ӧh264�� -- �ӿں��������������̵߳��ã���Ҫ�����쳣�������̷߳��ʰ�ȫ��
int H264Buffer::get_h264_for_record(H264_Seg_Info * h264_record_info,int sec_start,int sec_end,int illegal_video_seconds)
{
	int sec_start_record,sec_end_record;//Υ������ʼʱ�䣬��ֹʱ��
	int index_start_record,index_end_record;//Υ����h264�����ж�Ӧ����ֹ����
//	int index_h264_start,index_h264_end;//h264�Ŀɷ��ʵ���ʼ��������ֹ����
	static int flag_flushed=0;//�����Ƿ���������0:δ������1���Ѿ�����		//���Ͽ���������Ҫ������
	int ret=-1;
	int seconds_record = illegal_video_seconds;

//	printf("in get_h264_for_record\n");

	if(virtAddr_cmem==NULL)		
	{
		ret=attach_shm();//���������ڴ棭��ֻ�ж�ȡ Ȩ��
		if(ret<0)
		{
			printf("h264_buffer::get_h264_for_record : attach_shm failed \n");
			return -1;
		}
	}
	
	//��Ӷ�����������쳣����
	if(h264_record_info==NULL)
	{
		printf("h264_err: h264_record_info is NULL\n");
		return -1;
	}

	//�����������ֹʱ���쳣//��ֹʱ����ͬһ��Ҳ�ǿ��ܵ�
	if((sec_end-sec_start)<0 || (sec_end-sec_start)>120)
	{
		printf("sec is err : sec_start=%d,sec_end=%d\n",sec_start,sec_end);
		return -1;
	}	
	printf("input sec is : sec_start=%d,sec_end=%d\n",sec_start,sec_end);

	//��ָ��Υ����¼ʱ�䳤���쳣      �ݶ��쳣������5s������2����  ��ͨ��Ϊ5-30s��
	if((seconds_record<5)||(seconds_record>120))
	{
		printf("h264_err: input  seconds_record is unormal  : %d\n",seconds_record);
		seconds_record = 5;//���Ǹ����Դ���ָ��Ĭ��ֵ����������
	}

	index_recv_h264=h264_buffer_head->index_recv_h264;

	h264_record_info->h264_seg=0;//������Ϊ��Ч

	//���������ţ�ȡʱ��
	sec_start_record=sec_start;//���辫ȷ��Ӧ����Ҫʹ�ú���index_h264[index1].msecond
	sec_end_record=sec_end;
	if(sec_start_record>sec_end_record)
	{
		printf("h264_err: the input index is err: start time is %x; end time is %x\n",sec_start_record,sec_end_record);
		return -1;
	}

	int index_last = (index_recv_h264>0)?(index_recv_h264-1):(index_h264[0].buf_h264_index_max);//�Ѿ�������ɵ������һ֡
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


	//�Ƿ�h264���Ѿ������˻�������������дָ���Ƿ��ͷ��
	if(flag_flushed ==0 )//��δ������һ��
	{
		//index_recv_h264֮ǰ�������Ŷ�Ӧh264������Ч     ��һ����仺�壬δ��ͷ

		//ȷ��һ����Ч�������ţ������ж�  ,���ڲ�֪��ʵ�����˶��ٸ����壬����ʹ��ǰ��Ļ�������жϡ�����index_recv_h264���������ģ���Ҫʹ��
		int index_valid=index_recv_h264>1?(index_recv_h264-1):(index_recv_h264+1);

		if((index_h264[index_valid].buf_h264_index == index_valid) && (index_h264[index_valid].h264_id != index_valid))
		{
			flag_flushed=1;//��ĳ�������Ѿ��������Ҳ��ǵ�һ����䣬������h264�����Ѿ�������
		}
		else
		{
			flag_flushed=0;
			if((index_valid>index_recv_h264)&&(index_h264[index_valid].buf_h264_index == index_valid))
			{
				flag_flushed=1;//��ǰд����֮����Ѿ�������������h264�����Ѿ�������
			}
		}

	}

	//���µ�������ʱ��		
	//���趨��Υ����¼ʱ�䳤�ȣ���Υ����¼����������Ӧ��ʱ����Ҫ�󣬾�Ҫ����ȡΥ��¼���ʱ��
	if(seconds_record > (sec_end_record-sec_start_record) )
	{
		if(sec_start_record+seconds_record > sec_last)//���������ݲ���ָ��ʱ�䳤��
		{
			sec_end_record=sec_last;//ȡ�����µ�h264֡
		}
		else
		{
			sec_end_record = sec_start_record+seconds_record;//ȡ��Υ����¼��ָ��ʱ�䳤��
		}
	}


	//����ʱ�䣬ȡ��Ӧh264��Ӧ������
	ret=get_frame_at_sec(index_start_record,index_end_record,sec_start_record,sec_end_record,flag_flushed);
	if(ret<0)
	{
		printf("h264_err: get_frame_at_sec failed. \n");
		return -1;
	}

	//����h264��һ�������ţ���ǰȡ��һ��������I֡
	ret=get_I_frame_by_index(index_start_record,flag_flushed);
	if(ret<0)
	{
		printf("h264_err: get_I_frame_by_index failed.\n");
		return -1;
	}


	//����Υ����¼��ȡ����Ӧh264����λ����Ϣ
	//����һ���ṹ�壬������ж�ε�copy����
//	h264_record_info->index_start_record=index_start_record;
//	h264_record_info->index_end_record=index_end_record;
//	h264_record_info->buf_h264_index_max=index_h264[0].buf_h264_index_max;
	h264_record_info->seconds_record_ret=sec_end_record - sec_start_record;

	if(index_start_record<=index_end_record)//�����һ�����ڣ�û������h264�Ļ���β��
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
	else//�����Ϊ�����Σ����h264�Ļ���β��
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

//����������ȡʱ��
int H264Buffer::get_sec_at_index( int index)
{
	if(virtAddr_cmem==NULL)		
	{
		printf("get_sec_at_index : shm err \n");
		return -1;
	}
	return index_h264[index].seconds;
}



//��h264�����в��Ҷ�Ӧ���֡��һ���Ӧ��֡��ȡ�ǰ��һ֡
int H264Buffer::get_frame_at_sec_forward( int& index_record, int sec_record,int flag_flushed)
{
	//��ʱ�����ܻ����û��β��־����������磺�ϴλ���400֡�����������ÿ֡���Ƚ�С�����Ի���500֡����д��450֡ʱ���ȸ�����ԭ����β��־����û�����µ�β��־�����������ϴε����֡��
	//��д�����λ��֮�⣬�����Է��ʡ����ǵ�ȡh264�ĺ�ʱ��дָ��֮���2s����Ӧ���뱣��������--����ͬʱ���ж�д���ʡ�
	//��ʱ���������ת���ɰ�����������������ʹ��2�ַ����ȽϿ졣
	//����֡�ʿ���Ԥ����Ҳ����ֱ����ת���Ͻ���λ�ã�Ȼ����ң������졣
	if(virtAddr_cmem==NULL)
	{
		printf("get_frame_at_sec, virtAddr_cmem=NULL\n");
		return -1;
	}


	index_record=-1;

	if(1)
	{
		//����ɵ�֡�����µ�֡��˳��Ѱ�ҡ�ͳһ�����Զ���ͷ��
		int flag_buf_continuous=((flag_flushed ==0 )||(index_recv_h264 >= index_h264[0].buf_h264_index_max));//buf������־������������ɵ�֡Ϊ0�����µ�֡Ϊ���֡��
		int index_tmp=0;
		int i=0;
		int index_max=(flag_buf_continuous==1) ? index_recv_h264 : index_h264[0].buf_h264_index_max;//������ʵ�ʴ洢�����֡��Ŀ
		printf("\nforward: flag_buf_continuous=%d, index_recv_h264=%d, buf_h264_index_max=%d\n\n", flag_buf_continuous, index_recv_h264, index_h264[0].buf_h264_index_max);
		while(i<index_max)//ѭ������Ϊ���ֵ��1 ����ǰ����֡������
		{
			index_tmp=(flag_buf_continuous==1) ? i : ((index_recv_h264+1+i)%(index_h264[0].buf_h264_index_max+1));
			i++;

			if((index_record!=-1))//�ҵ�ƥ��֡������ѭ��
			{
				break;
			}

//			printf("get_frame_at_sec: sec_start_record=%d, index_h264[%d].seconds=%d\n", sec_record, index_tmp, index_h264[index_tmp].seconds);

			//���Ҷ�Ӧ��ĵ�һ֡
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
		//ʧ�ܣ�û���ҵ���Ч��h264֡
		printf("h264_err: not found valid h264 frame for record \n");//�Ͽ����һ�ᣬ�ᵼ�½���˴���
		return -1;
	}

	return 0;

}

//��h264�����в��Ҷ�Ӧ���֡��һ���Ӧ��֡��ȡ����һ֡
int H264Buffer::get_frame_at_sec_backward( int& index_record, int sec_record,int flag_flushed)
{
	//��ʱ�����ܻ����û��β��־����������磺�ϴλ���400֡�����������ÿ֡���Ƚ�С�����Ի���500֡����д��450֡ʱ���ȸ�����ԭ����β��־����û�����µ�β��־�����������ϴε����֡��
	//��д�����λ��֮�⣬�����Է��ʡ����ǵ�ȡh264�ĺ�ʱ��дָ��֮���2s����Ӧ���뱣��������--����ͬʱ���ж�д���ʡ�
	//��ʱ���������ת���ɰ�����������������ʹ��2�ַ����ȽϿ졣
	//����֡�ʿ���Ԥ����Ҳ����ֱ����ת���Ͻ���λ�ã�Ȼ����ң������졣
	if(virtAddr_cmem==NULL)
	{
		printf("get_frame_at_sec, virtAddr_cmem=NULL\n");
		return -1;
	}


	index_record=-1;

	if(1)
	{
		//����ɵ�֡�����µ�֡��˳��Ѱ�ҡ�ͳһ�����Զ���ͷ��
		int flag_buf_continuous=((flag_flushed ==0 )||(index_recv_h264 >= index_h264[0].buf_h264_index_max));//buf������־������������ɵ�֡Ϊ0�����µ�֡Ϊ���֡��
		int index_tmp=0;
		int i=0;
		int index_max=(flag_buf_continuous==1) ? index_recv_h264 : index_h264[0].buf_h264_index_max;//������ʵ�ʴ洢�����֡��Ŀ
		printf("\nbackward: flag_buf_continuous=%d, index_recv_h264=%d, buf_h264_index_max=%d\n\n", flag_buf_continuous, index_recv_h264, index_h264[0].buf_h264_index_max);
		while(i<index_max)//ѭ������Ϊ���ֵ��1 ����ǰ����֡������
		{
			index_tmp=(flag_buf_continuous==1) ? i : ((index_recv_h264+1+i)%(index_h264[0].buf_h264_index_max+1));
			i++;

			if((index_record!=-1))//�ҵ�ƥ����������󣬵������Ӧ�������������
			{
				printf("find end sec ,index_tmp=%d\n",index_tmp);
				if(index_h264[index_tmp].seconds==sec_record)
				{
					index_record=index_tmp;
					continue;//���Ѿ��ҵ���β�����󣬲��ٽ��������ж�
				}
				else
				{
					break;//������һ������Ӧ��ǰ���������������
				}
			}

//			printf("get_frame_at_sec: sec_end_record=%d, index_h264[%d].seconds=%d\n", sec_record, index_tmp, index_h264[index_tmp].seconds);
			//���Ҷ�Ӧ��ĵ�һ֡
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
		//ʧ�ܣ�û���ҵ���Ч��h264֡
		printf("h264_err: not found valid h264 frame for record \n");//�Ͽ����һ�ᣬ�ᵼ�½���˴���
		return -1;
	}

	return 0;

}


//����ʱ�䣬ȡ��Ӧh264��Ӧ������
//ϣ��ȡ������Ƶ��������������ʱ��Σ���ֹ�㾡����Ҫ���ʱ��ƥ��
int H264Buffer::get_frame_at_sec( int& index_start_record, int& index_end_record,int sec_start_record,int sec_end_record,int flag_flushed)
{

	//��ʱ�����ܻ����û��β��־����������磺�ϴλ���400֡�����������ÿ֡���Ƚ�С�����Ի���500֡����д��450֡ʱ���ȸ�����ԭ����β��־����û�����µ�β��־�����������ϴε����֡��
	//��д�����λ��֮�⣬�����Է��ʡ����ǵ�ȡh264�ĺ�ʱ��дָ��֮���2s����Ӧ���뱣��������--����ͬʱ���ж�д���ʡ�
	//��ʱ���������ת���ɰ�����������������ʹ��2�ַ����ȽϿ졣
	//����֡�ʿ���Ԥ����Ҳ����ֱ����ת���Ͻ���λ�ã�Ȼ����ң������졣
	if(virtAddr_cmem==NULL)		
	{
		printf("get_frame_at_sec, virtAddr_cmem=NULL\n");
		return -1;
	}
	index_start_record=-1;
	index_end_record=-1;

	if(1)
	{
		//������ʼ���Ӧ��֡
		//h264�ֶ�¼��ÿ�ο�ʼʱ��ҪѰ��I֡�����������ͺ����������������
		get_frame_at_sec_forward(index_start_record,sec_start_record,flag_flushed);//������ǰ��
		if((index_start_record==-1))
		{
			get_frame_at_sec_forward(index_start_record,sec_start_record-1,flag_flushed);//������һ��
			if((index_start_record==-1))
			{
				get_frame_at_sec_forward(index_start_record,sec_start_record-2,flag_flushed);//����������
				if((index_start_record==-1))
				{
					get_frame_at_sec_forward(index_start_record,sec_start_record+1,flag_flushed);//����ǰһ��
					if((index_start_record==-1))
					{
						get_frame_at_sec_forward(index_start_record,sec_start_record+2,flag_flushed);//����ǰ����
					}
				}
			}
		}

		//����ֹͣ���Ӧ��֡
		//����ʱ��׼ȷ��������ǰ��
		get_frame_at_sec_backward(index_end_record,sec_end_record,flag_flushed);//������ǰ��
		if(index_end_record==-1)
		{
			get_frame_at_sec_backward(index_end_record,sec_end_record-1,flag_flushed);//������һ��
			if((index_end_record==-1))
			{
				get_frame_at_sec_backward(index_end_record,sec_end_record+1,flag_flushed);//����������
				if((index_end_record==-1))
				{
					get_frame_at_sec_backward(index_end_record,sec_end_record-2,flag_flushed);//����ǰһ��
					if((index_end_record==-1))
					{
						get_frame_at_sec_backward(index_end_record,sec_end_record+2,flag_flushed);//����ǰ����
					}
				}
			}
		}

	}
	else
	{
		if(1)
		{
			//ֻ������ǰ���Ӧ��֡
			get_frame_at_sec_forward(index_start_record,sec_start_record,flag_flushed);

			get_frame_at_sec_backward(index_end_record,sec_end_record,flag_flushed);
		}
		else
		{
			//��������Χ����֤��ȡ��h264������  //�ʺ���h264¼���������������
			//������ʼ���Ӧ��֡
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

			//����ֹͣ���Ӧ��֡
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
		//ʧ�ܣ�û���ҵ���Ч��h264֡
		printf("h264_err: not found valid h264 frame for record \n");//�Ͽ����һ�ᣬ�ᵼ�½���˴���
		return -1;
	}

	return 0;
	
}


//����h264��һ�������ţ���ǰȡ��һ��������I ֡
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
		while(i>=0)//��ǰ���ң�Ѱ�ҷ�Χ��index_start_record-0
		{
			printf("get_I_frame_by_index:i=%d,index_h264[i].is_I_frame=%d\n",i,index_h264[i].is_I_frame);
			if(index_h264[i].is_I_frame == 1)//�ҵ�I ֡
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
		//��Ҫ�����δ���
		//���ж�дָ��֮��Ĳ���,���ж�дָ��֮ǰ��----Ҳ����ͳһ���������Զ���ͷ�жϼ��ɡ�
		int i=index_start_record;
		if(i<index_recv_h264)
		{
			while(i>=0)//(i<=index_h264[0].buf_h264_index_max)//Ѱ�ҷ�Χ��index_start_record-0
			{
				printf("get_I_frame_by_index:i=%d,index_h264[i].is_I_frame=%d\n",i,index_h264[i].is_I_frame);
				if(index_h264[i].is_I_frame == 1)//�ҵ�I ֡
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
			while(i>index_recv_h264)//Ѱ�ҷ�Χ��β������index_start_record����index_recv_h264
			{
				printf("get_I_frame_by_index:i=%d,index_h264[i].is_I_frame=%d\n",i,index_h264[i].is_I_frame);
				if(index_h264[i].is_I_frame == 1)//�ҵ�I ֡
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
		//Ѱ���Ѿ�������ɵ����µ�һ֡��������Ϣ
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
		//Ѱ��ָ��֡��������Ϣ
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
	
//��ȡ�汾��Ϣ
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

//��ȡCMEM �����ַ
char * H264Buffer::get_cmem_addr_virt()
{
	return virtAddr_cmem;
}

//��ȡCMEM �����ַ
unsigned int H264Buffer::get_cmem_addr_phy()
{
	return physAddr_cmem;
}

//��ȡCMEM ����
unsigned int H264Buffer::get_cmem_len()
{
	return len_cmem;
}

//���Ժ���: ȡΥ����¼��Ӧ��h264������д�ļ���
int H264Buffer::test_get_ill_record_h264()
{
	int ret=0;
	static int cy_first=1;

	//��Ҫ��ֵ:
//	int num_jpeg_max;
//	int video_index_head;
//	int video_index_tail;

	if(virtAddr_cmem==NULL)		
	{
		printf("test_get_ill_record_h264 : virtAddr_cmem=NULL \n");
		return -1;
	}


	//��ǰ����д������֡��Ӧ������ʡ�
	//ע�⣺buf_indexΪ���ڲ��������������Ӧֵ���ɷ��ʣ�  Ӧ������ǰ���Ѿ�д�õ�ֵ�����磺 (buf_index+num_jpeg_max-1)%num_jpeg_max
	if(((cy_first>=1) && (index_recv_h264==160))||(flag_write_right==0))
	{
		//ֻ�еڶ��ν������д�ļ���ģ���β���//��ģʽ���ÿ���
		cy_first++;
		if(cy_first<3)//����
		{
			printf("cy_first=%d\n",cy_first);
			if(flag_write_right!=0)//дģʽ�������һ�Σ����⻺��δд������������ر���ڿ绺��β�����
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
		//ģ�������ֹindex��ȡ��Ӧʱ���Ӧ��h264
		int index1,index3;//h264�������ţ�
		index1=50;//���绺��β
		index3=160-30;
		////index1=800;//�绺��β  //ֻ���ض���������: ���û���15s����Լ1200֡
		//index1=index_h264[0].buf_h264_index_max-50;//�绺��β �����Զ�̬ʹ��
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
		    	file_write(filename,(char *)h264_record_info->h264_buf_seg[0],h264_record_info->h264_buf_seg_size[0]);//дnal�����ݣ��鿴������������
		    }

		    if(h264_record_info->h264_seg >= 2)
		    {
   		    	printf("test_get_ill_record_h264: 2: buf: 0x%x , size=%d\n",(unsigned int)h264_record_info->h264_buf_seg[1],h264_record_info->h264_buf_seg_size[1]);
		    	file_write(filename,(char *)h264_record_info->h264_buf_seg[1],h264_record_info->h264_buf_seg_size[1]);//дnal�����ݣ��鿴������������
		    }


		}
		else
		{
			printf("h264_err: get_h264_for_record return failed\n");
			ret = -1;
		}


	    //��ӡ��¼��Ӧÿһ֡����Ϣ���鿴�Ƿ���ȷ��
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


//��ȡh264_pack ����İ汾��
//�������: version_h264_pack ���ڴ������İ汾��Ϣ��len����ָ��version_h264_pack �������󳤶�
//����ֵ: С��0����ʾ��ȡ�汾��Ϣʧ�ܣ�����0����ʾ�汾��Ϣ����
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


	H264Buffer *h264_buffer = new H264Buffer();	//ֻ�ж�ȡ Ȩ��
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




//�ڻ���buf ��Ѱ���ַ���str
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

	//ʱ������ݷֲ���3��1��15��1��15��1������Чλ��3+15+15��
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
		//ʱ������ݷֲ���3��1��15��1��15��1������Чλ��3+15+15��
		//pack header: �����5���ֽ��У�ʱ����й�λ��1�� 3b ff fb ff f8 (��35λ���м�����λ��ʱ���û�й�ϵ)

	if(buf==NULL)
	{
		printf("in %s buf is null\n",__func__);
		return -1;
	}

	//��ʱ�����ŵ�5���ֽ��У�
	//0-4λ��
	buf[4]=(buf[4]&0x07)+((timestamp_set&0x1f)<<3);
	timestamp_set>>=5;
	//5-12λ��
	buf[3]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//13-14λ��
	buf[2]=(buf[2]&0xfc)+(timestamp_set&0x03);
	timestamp_set>>=2;
	//15-19λ��
	buf[2]=(buf[2]&0x07)+((timestamp_set&0x1f)<<3);
	timestamp_set>>=5;
	//20-27λ��
	buf[1]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//28-29λ��
	buf[0]=(buf[0]&0xfc)+(timestamp_set&0x03);
	timestamp_set>>=2;
	//30-32λ��
	buf[0]=(buf[0]&0xc7)+((timestamp_set&0x07)<<3);
	timestamp_set>>=3;


	return 0;

}


long long h264_get_timestamp_PESheader(char *buf,int buf_len)
{
	//ʱ������ݷֲ���3��1��15��1��15��1������Чλ��3+15+15��
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
		//ʱ������ݷֲ���3��1��15��1��15��1������Чλ��3+15+15��
		//PES header: �����5���ֽ��У�ʱ����й�λ��1�� 0e ff fe ff fe (��35λ���м�����λ��ʱ���û�й�ϵ)

	if(buf==NULL)
	{
		printf("in %s buf is null\n",__func__);
		return -1;
	}
	long long timestamp_set=timestamp;

	//��ʱ�����ŵ�5���ֽ��У�
	//0-6λ��
	buf[4]=(buf[4]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//7-14λ��
	buf[3]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//15-22λ��
	buf[2]=(buf[2]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//23-30λ��
	buf[1]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//31-33λ��
	buf[0]=(buf[0]&0xf1)+((timestamp_set&0x07)<<1);
	timestamp_set>>=3;


	timestamp_set=timestamp;

	//��ʱ�����ŵ�5���ֽ��У�
	//0-6λ��
	buf[9]=(buf[9]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//7-14λ��
	buf[8]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//15-22λ��
	buf[7]=(buf[7]&0x01)+((timestamp_set&0x7f)<<1);
	timestamp_set>>=7;
	//23-30λ��
	buf[6]=(timestamp_set&0xff);
	timestamp_set>>=8;
	//31-33λ��
	buf[5]=(buf[5]&0xf1)+((timestamp_set&0x07)<<1);
	timestamp_set>>=3;


	return 0;

}

#define PACK_HEARDER 0
#define PES_HEARDER 1
#define INTERVAL_TIMESTAMP 3600

//��h264��ps��������ʱ���
int h264_ps_adjust_timestamp(char *buf, int buf_len, long long  timestamp_start, long long * p_timestamp_end )
{
	//��������ж�
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
	//��֤ps������Ч��



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
	int flag_timestamp_old=PES_HEARDER;//0,pack_header  1,PES_header   ȡֵΪpack_headerʱ������������

	while(1)
	{

		//Ѱ�������ʱ���
//		if(pos_PES_header>=pos_pack_header)
		if(pos_PES_total>=pos_pack_total)//�����β���ͬһ����ͷ//ʹ�þ���λ�ý����ж� //���λ�ã�����Щʱ�򲻺���
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



		//����ʱ���
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
			else if( ((flag_first_stamp==1) && (timestamp_start==-1)) //��һ�λ�ȡʱ�������������ֵ
						  || ((timestamp_diff>=0)&&(timestamp_diff<=INTERVAL_TIMESTAMP*2)) )//����ʱ�������
			{
				flag_first_stamp=0;
				timestamp_old=timestamp_pack;
			}
			else //ʱ����쳣
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
			else if( ((flag_first_stamp==1) && (timestamp_start==-1)) //��һ�λ�ȡʱ�������������ֵ
						  || ((timestamp_diff>=0)&&(timestamp_diff<=INTERVAL_TIMESTAMP*2)) )//����ʱ�������
			{
				flag_first_stamp=0;
				timestamp_old=timestamp_PES;
			}
			else //ʱ����쳣
			{
//				DEBUG("pos_buf=%d,timestamp_PES=%lld,time_diff=%lld\n",pos_buf,timestamp_PES,timestamp_diff);
				//�����һ��ʱ����ǰ�ͷ�ģ���ô����������pesͷ��ʱ�����Ӧ���Ǳ���һ�µģ���Ӧ���ٴ����һ��ʱ������
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




