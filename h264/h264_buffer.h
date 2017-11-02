
/*********************************************
 *  h264_buffer.h
 *  time: 2013-5-16
 *  Author: zmh
 ********************************************/
 
#ifndef _H264_BUFFER_H
#define _H264_BUFFER_H


//#define SHM_SIZE (4096*4)
//#define SHM_MODE 0600
#define SYS_SHM_KEY 0x7964
#define SIZE_VERSION 64


#define MAX_CAPTURE_NUM   4
#define MAX_H264_SEG_NUM  (MAX_CAPTURE_NUM+1)
#define MIN_VIDEO_INTERVAL 5  //违法视频最短时间，单位：秒


//#define DWORD unsigned int
//#define WORD unsigned short

//h264的帧索引信息
typedef struct H264_Index///add new point
{
	int buf_h264_index; //  在h264 循环缓冲中的索引号，范围在 0－500之间   60s*8fps
	int buf_h264_index_max; // 上一次填充的最大索引号  //由于每帧大小不一致，每次填充的最大索引号都会变化
	int h264_Position; //  大缓冲中偏移地址
	int h264_Len; //  h264长度

	int msecond;//arm接收时间：  毫秒
	int seconds;//arm接收时间： 总共秒数
	unsigned int timestamp;//时间戳

	int is_I_frame;//是否I帧
	int nextFlag;//后面是否还有视频数据？   缓冲尾,没有了，取0；其他，还有，取1

	int h264_id;	//无限增长id;

	int to_use[10];

} H264_Index;


//一帧h264的nal 信息
typedef struct Nal_Info
{
	char * nal;//指向nal流数据
	int nal_size;//nal流的长度
	int nal_IDC_type;//nal单元类型码  // 5，IDR图像的编码条带;	7，序列参数集;	8，图像参数集;
	long long timestamp;
} Nal_Info;

//h264的一段ps流信息
typedef struct H264_Seg_Info
{
	int h264_seg;//h264流的分段数，通常为1段，或者2段，0表示无效
	unsigned char * h264_buf_seg[2];//指向某一段h264流的起始地址
	int h264_buf_seg_size[2];//某一段h264流的长度
//	int index_start_record;//起始索引
//	int index_end_record;//结束索引
//	int buf_h264_index_max ;//上次h264存储的最大索引号
	int seconds_record_ret;//最终的违法记录对应的h264视频时间长度
} H264_Seg_Info;

//h264的ps流总的信息
typedef struct H264_Record_Info
{
	int h264_seg;//h264流的分段数，通常为1段，或者2段，0表示无效
	unsigned char * h264_buf_seg[MAX_H264_SEG_NUM];//指向某一段h264流的起始地址
	int h264_buf_seg_size[MAX_H264_SEG_NUM];//某一段h264流的长度
//	int index_start_record;//起始索引
//	int index_end_record;//结束索引
//	int buf_h264_index_max ;//上次h264存储的最大索引号
	int seconds_record_ret;//最终的违法记录对应的h264视频时间长度
} H264_Record_Info;


//h264缓冲创建所需信息
typedef struct H264_Create_Info
{
	int protect_time;//需要保护的时间长度，以秒为单位
	int camera_frame_rate;//相机的帧率，单位为fps
	int rate;//h264的生成码率，以M为单位
	char version[SIZE_VERSION];

} H264_Create_Info;

//根据输入的帧率，需要保护的时间，计算出帧索引的最大值。
//真实的存放h264数据的缓冲的大小，根据码率，保护时间计算出。









//#########################################//#########################################//

//h264的buffer信息, 实际对应一个共享内存。共享内存的大小，能动态设置。
//实际的h264存储帧数，是动态变化的。


//根据输入的帧率，需要保护的时间，计算出帧索引的最大值。
//真实的存放h264数据的缓冲的大小，根据码率，保护时间计算出。
//再加上共享内存的一个头结构体，组成一个共享缓冲。

//h264共享缓冲，只有拥有写权限的才能进行写操作。
//拥有写权限的用户，创建者，在同一时间内，只能有一个。
//是否拥有写权限，在类里面判断。

typedef struct H264_Buffer_Head
{
	int id_shmem;//第一次创建时赋值，其值即该共享内存的ID，用于读取者判断读到的共享内存是否正确。
	int num_writer;//写用户的数目，不允许大于1

	int max_index_h264;//H264_Index结构体的数目。由此数目可以计算出后面的h264数据缓冲的起始地址		
	int len_buf_h264;//实际的存放h264数据的缓冲大小

	int index_recv_h264;//当前正在进行写操作的帧索引号
	
	char version[SIZE_VERSION];//程序版本信息	
//	H264_Index index_h264[max_index_h264];//示意，在缓冲头之后紧接着存放索引信息
//	unsigned char buf_h264[len_buf_h264];//示意，在索引信息之后紧接着存放h264数据

	
} H264_Buffer_Head;






//假定，只有一个写，可以有多个读，不会存在访问冲突
//代码中保证: 当前写的区域，不能进行读。

class H264Buffer
{	
//public:		
protected://子类也需要访问数据成员
//	int shmid;		//共享内存ID	
//	char * shmptr;	//共享内存指针
	char * virtAddr_cmem ;
	unsigned int physAddr_cmem ;
	unsigned int len_cmem ;
	
	int flag_write_right;	//是否有写权限
	int index_recv_h264;	//当前接收索引号	
	int flag_create;
	H264_Create_Info h264_create_info;
	
	H264_Buffer_Head * h264_buffer_head;//指向共享内存中的h264 buffer头
	H264_Index * index_h264;//h264索引信息
	unsigned char *buf_h264;//h264数据

//	pthread_mutex_t mutex_h264_buffer;	

//private:
	int attach_shm(int flag_create=0,H264_Create_Info *h264_create_info=NULL);
	int deattach_shm();
	
public:		
	H264Buffer(int flag_create=0,H264_Create_Info *h264_create_info=NULL);	
	H264Buffer(unsigned int addr_phy_cmem,unsigned int len_cmem);
	~H264Buffer();		

	//#########################################//#########################################//
	int get_h264_for_record(H264_Seg_Info * h264_record_info,int sec_start,int sec_end,int illegal_video_seconds);
	int get_sec_at_index( int index);
	int get_frame_at_sec_forward( int& index_record, int sec_record,int flag_flushed);
	int get_frame_at_sec_backward( int& index_record, int sec_record,int flag_flushed);
	int get_frame_at_sec( int& index_start_record, int& index_end_record,int sec_start_record,int sec_end_record,int flag_flushed);
	int get_I_frame_by_index( int& index_start_record,int flag_flushed);
	int get_index_info(H264_Index *h264_index_ret, int last_cy, int index);
	int get_version(char * str_version,int len);
//	int get_shmid();
	char * get_cmem_addr_virt();
	unsigned int get_cmem_addr_phy();
	unsigned int get_cmem_len();

	int test_get_ill_record_h264();//just for test
//#########################################//#########################################//



};


int get_version_h264_pack(char *version_h264_pack, int len);
int h264_ps_adjust_timestamp(char *buf, int buf_len, long long  timestamp_start, long long * timestamp_end );
	
#endif

