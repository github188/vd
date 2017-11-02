#include "ALPU.h"

extern unsigned char alpuc_process(unsigned char *, unsigned char *);
unsigned char _alpu_rand(void);
void _alpu_delay_ms(unsigned int i);

unsigned char _i2c_read(unsigned char , unsigned char , unsigned char *, int );
unsigned char _i2c_write(unsigned char , unsigned char , unsigned char *, int );
//��ʱ���� ��������ʱ�ĵ�λΪMS
void _alpu_delay_ms(unsigned int i)
{
    int tmp;
    tmp = i*1000;
    usleep(tmp);
}

struct timeval now_time;  
unsigned char _alpu_rand(void)   //Modify this fuction using RTC. But you should not change the function name.
{
    int temp;
    gettimeofday(&now_time,NULL);//gettimeofday ���ڲ�����ǰʱ���������΢����
    srand(now_time.tv_usec);  //ȡ΢����(int��)��Ϊseed
    temp = rand();
    return (unsigned char)temp;  //��int��tempǿ��ת����char��
}


/*
   ��ע��
   ������IIC��д���� �ɹ�����0 ʧ�ܷ��ط���

device_addr :�豸��ַ������ͨ�Ų����豸��ַ ��һ�µģ�����Ĭ�ϵ�ֵ��7A ��������������ò�ͬ�����Բ�ʹ��������ݣ�
sub_addr���ӵ�ַ
buff���洢���ݵ�buff
ByteNo:���ݵĳ���

 **/
unsigned char _i2c_read(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    iic_read_fun(device_addr, sub_addr, buff, ByteNo);          //your IIC read funtion
    return 0;
}
unsigned char _i2c_write(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    iic_write_fun(device_addr,  sub_addr, buff, ByteNo);                 //your IIC write funtion
    return 0;
}


//For Encryption mode, you are supposed to call the _alpum_process() function in ALPU Check fuction or main source.
//The return value would be "0"(zero) if the library file is work well. otherwise, the return value would be error_code value.
void ALPU(void)
{
    int i;
    unsigned char error_code;
    unsigned char dx_data[8];		 // ��������ݣ� ���� ��ȷ�Ļ�Ӧ�ú�tx_data���
    unsigned char tx_data[8];		//	����� �� ����ϵͳ������


    for(i=0; i<8; i++)
    {	
        tx_data[i] = _alpu_rand();
        dx_data[i] = 0;
    }

    error_code = alpuc_process(tx_data,dx_data);
    if(error_code){
        printf("\r\nAlpu-M Encryption Test Fail!!!\r\n");
        printf("\r\nError Code : %d",error_code);
        printf("\n\r========================ALPU-M IC Encryption========================");
        printf("\r\n Tx Data : "); for (i=0; i<8; i++) printf("0x%2x ", tx_data[i]); printf("\r\n");
        printf(" Dx Data : "); for (i=0; i<8; i++) printf("0x%2x ", dx_data[i]); printf("\r\n");

        printf("\n\r====================================================================");
        exit(-1);
    }

    else{

        printf("\r\nAlpu-M Encryption Test Success!!!\r\n");
        printf("\r\n\nError_code : %d", error_code);
        printf("\n\r========================ALPU-M IC Encryption========================");
        printf("\r\n Tx Data : "); for (i=0; i<8; i++) printf("0x%2x ", tx_data[i]); printf("\r\n");
        printf(" Dx Data : "); for (i=0; i<8; i++) printf("0x%2x ", dx_data[i]); printf("\r\n");

        printf("\n\r====================================================================");
        printf("\r\n");
    }

    //other functions..
}




/* EOF */
