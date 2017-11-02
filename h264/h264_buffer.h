
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
#define MIN_VIDEO_INTERVAL 5  //Υ����Ƶ���ʱ�䣬��λ����


//#define DWORD unsigned int
//#define WORD unsigned short

//h264��֡������Ϣ
typedef struct H264_Index///add new point
{
	int buf_h264_index; //  ��h264 ѭ�������е������ţ���Χ�� 0��500֮��   60s*8fps
	int buf_h264_index_max; // ��һ���������������  //����ÿ֡��С��һ�£�ÿ��������������Ŷ���仯
	int h264_Position; //  �󻺳���ƫ�Ƶ�ַ
	int h264_Len; //  h264����

	int msecond;//arm����ʱ�䣺  ����
	int seconds;//arm����ʱ�䣺 �ܹ�����
	unsigned int timestamp;//ʱ���

	int is_I_frame;//�Ƿ�I֡
	int nextFlag;//�����Ƿ�����Ƶ���ݣ�   ����β,û���ˣ�ȡ0�����������У�ȡ1

	int h264_id;	//��������id;

	int to_use[10];

} H264_Index;


//һ֡h264��nal ��Ϣ
typedef struct Nal_Info
{
	char * nal;//ָ��nal������
	int nal_size;//nal���ĳ���
	int nal_IDC_type;//nal��Ԫ������  // 5��IDRͼ��ı�������;	7�����в�����;	8��ͼ�������;
	long long timestamp;
} Nal_Info;

//h264��һ��ps����Ϣ
typedef struct H264_Seg_Info
{
	int h264_seg;//h264���ķֶ�����ͨ��Ϊ1�Σ�����2�Σ�0��ʾ��Ч
	unsigned char * h264_buf_seg[2];//ָ��ĳһ��h264������ʼ��ַ
	int h264_buf_seg_size[2];//ĳһ��h264���ĳ���
//	int index_start_record;//��ʼ����
//	int index_end_record;//��������
//	int buf_h264_index_max ;//�ϴ�h264�洢�����������
	int seconds_record_ret;//���յ�Υ����¼��Ӧ��h264��Ƶʱ�䳤��
} H264_Seg_Info;

//h264��ps���ܵ���Ϣ
typedef struct H264_Record_Info
{
	int h264_seg;//h264���ķֶ�����ͨ��Ϊ1�Σ�����2�Σ�0��ʾ��Ч
	unsigned char * h264_buf_seg[MAX_H264_SEG_NUM];//ָ��ĳһ��h264������ʼ��ַ
	int h264_buf_seg_size[MAX_H264_SEG_NUM];//ĳһ��h264���ĳ���
//	int index_start_record;//��ʼ����
//	int index_end_record;//��������
//	int buf_h264_index_max ;//�ϴ�h264�洢�����������
	int seconds_record_ret;//���յ�Υ����¼��Ӧ��h264��Ƶʱ�䳤��
} H264_Record_Info;


//h264���崴��������Ϣ
typedef struct H264_Create_Info
{
	int protect_time;//��Ҫ������ʱ�䳤�ȣ�����Ϊ��λ
	int camera_frame_rate;//�����֡�ʣ���λΪfps
	int rate;//h264���������ʣ���MΪ��λ
	char version[SIZE_VERSION];

} H264_Create_Info;

//���������֡�ʣ���Ҫ������ʱ�䣬�����֡���������ֵ��
//��ʵ�Ĵ��h264���ݵĻ���Ĵ�С���������ʣ�����ʱ��������









//#########################################//#########################################//

//h264��buffer��Ϣ, ʵ�ʶ�Ӧһ�������ڴ档�����ڴ�Ĵ�С���ܶ�̬���á�
//ʵ�ʵ�h264�洢֡�����Ƕ�̬�仯�ġ�


//���������֡�ʣ���Ҫ������ʱ�䣬�����֡���������ֵ��
//��ʵ�Ĵ��h264���ݵĻ���Ĵ�С���������ʣ�����ʱ��������
//�ټ��Ϲ����ڴ��һ��ͷ�ṹ�壬���һ�������塣

//h264�����壬ֻ��ӵ��дȨ�޵Ĳ��ܽ���д������
//ӵ��дȨ�޵��û��������ߣ���ͬһʱ���ڣ�ֻ����һ����
//�Ƿ�ӵ��дȨ�ޣ����������жϡ�

typedef struct H264_Buffer_Head
{
	int id_shmem;//��һ�δ���ʱ��ֵ����ֵ���ù����ڴ��ID�����ڶ�ȡ���ж϶����Ĺ����ڴ��Ƿ���ȷ��
	int num_writer;//д�û�����Ŀ�����������1

	int max_index_h264;//H264_Index�ṹ�����Ŀ���ɴ���Ŀ���Լ���������h264���ݻ������ʼ��ַ		
	int len_buf_h264;//ʵ�ʵĴ��h264���ݵĻ����С

	int index_recv_h264;//��ǰ���ڽ���д������֡������
	
	char version[SIZE_VERSION];//����汾��Ϣ	
//	H264_Index index_h264[max_index_h264];//ʾ�⣬�ڻ���ͷ֮������Ŵ��������Ϣ
//	unsigned char buf_h264[len_buf_h264];//ʾ�⣬��������Ϣ֮������Ŵ��h264����

	
} H264_Buffer_Head;






//�ٶ���ֻ��һ��д�������ж������������ڷ��ʳ�ͻ
//�����б�֤: ��ǰд�����򣬲��ܽ��ж���

class H264Buffer
{	
//public:		
protected://����Ҳ��Ҫ�������ݳ�Ա
//	int shmid;		//�����ڴ�ID	
//	char * shmptr;	//�����ڴ�ָ��
	char * virtAddr_cmem ;
	unsigned int physAddr_cmem ;
	unsigned int len_cmem ;
	
	int flag_write_right;	//�Ƿ���дȨ��
	int index_recv_h264;	//��ǰ����������	
	int flag_create;
	H264_Create_Info h264_create_info;
	
	H264_Buffer_Head * h264_buffer_head;//ָ�����ڴ��е�h264 bufferͷ
	H264_Index * index_h264;//h264������Ϣ
	unsigned char *buf_h264;//h264����

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

