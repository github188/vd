

#include "ALPU.h"
#include "logger/log.h"



/**************************************************
 *��������ALPU_write
 *����˵����ͨ������ ioctl ʵ����IIC�豸д����
 ****************************************************/
unsigned char iic_write_fun(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    int fd,ret,i;
    //��I2C�豸�ļ�	
    if((fd = open(i2c,O_RDWR)) == -1)
    {
        perror("Open I2C failed");
        return -1;
    }
    //����i2c_rdwr_ioctl_data�ṹ�岢�ڶ��϶���ṹ���Աmsgs
    struct i2c_rdwr_ioctl_data alpu_data;
    alpu_data.nmsgs = 1;
    alpu_data.msgs = (struct i2c_msg *)malloc(alpu_data.nmsgs * sizeof(struct i2c_msg));
    if(!alpu_data.msgs)
    {
        ERROR("Malloc for alpu data msg failed!");
        close(fd);
        printf("malloc error\n");
        return -1;
    }
    //���msgs�ṹ��
    (alpu_data.msgs[0]).len = ByteNo + 1;//ByteNo+1���ֽڣ���1���ֽ�װ�Ĵ�����ַ������8���ֽ�װ����
    (alpu_data.msgs[0]).addr = device_addr;
    (alpu_data.msgs[0]).flags = 0;//0:write  1:read
    (alpu_data.msgs[0]).buf = (unsigned char*)malloc(ByteNo + 1);//����ByteNo��unsigned  char�Ϳռ�
    if (!(alpu_data.msgs[0]).buf) {
        ERROR("Malloc for alpu data msg's buf failed!");
        free(alpu_data.msgs);
        close(fd);
        return -1;
    }
    (alpu_data.msgs[0]).buf[0] = sub_addr; //��һ���ֽ�װ�Ĵ�����ַ
    for(i = 1;i < ByteNo + 1;i++)   //ʣ��ByteNo��char������װҪд��ALPU������
    {
        (alpu_data.msgs[0]).buf[i] = buff[i - 1];
    }
    //ͨ��ioctlʵ����IIC�豸д����
    ret = ioctl(fd,I2C_RDWR,(unsigned long)&alpu_data);

    if(ret == -1)
    {
        perror("write----ioctl error");
        free((alpu_data.msgs[0]).buf);
        free(alpu_data.msgs);
        close(fd);
        return -1;
    }
#if 0
    printf("ByteNo:%d \n",ByteNo);
    printf("tx: ");

    for(i = 0;i < ByteNo + 1;i++)
    {
        printf("0X%x ",(alpu_data.msgs[0]).buf[i]);
    }
    putchar(10);
#endif

    free((alpu_data.msgs[0]).buf);
    free(alpu_data.msgs);
    close(fd);

    return 0;

}
/**************************************************
 *��������ALPU_read
 *����˵����ͨ������ ioctl ʵ����IIC�豸������
 ****************************************************/

int iic_read_fun(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    int fd,ret;
    //��I2C�豸�ļ�	
    if((fd = open(i2c,O_RDWR)) == -1)
    {
        perror ("Open I2C failed");
        return -1;
    }

    //����i2c_rdwr_ioctl_data�ṹ�岢�ڶ��϶���ṹ���Աmsgs
    struct i2c_rdwr_ioctl_data alpu_data;
    alpu_data.nmsgs = 2; //nmsgs=2 ��ζ�Ž�����2��start�ź�
    if( (alpu_data.msgs = (struct i2c_msg*)malloc(alpu_data.nmsgs*sizeof(struct i2c_msg))) == NULL)
    {
        printf("malloc error\n");
        close(fd);
        return -1;
    }
    //�����������msgs�ṹ��
    //��һ��msgs����ʵ�ַ���I2C_ADDR��reg_addr
    (alpu_data.msgs[0]).len = 1;
    (alpu_data.msgs[0]).addr = device_addr;
    (alpu_data.msgs[0]).flags = 0;
    (alpu_data.msgs[0]).buf =(unsigned char*)malloc(1); //0x80
    if((alpu_data.msgs[0]).buf == NULL)
    {
        printf("malloc error\n");
        free(alpu_data.msgs);
        close(fd);
        return -1;
    }
    (alpu_data.msgs[0]).buf[0] = sub_addr;

    //�ڶ���msgs���ڶ�ȡByteNo λ���� 
    (alpu_data.msgs[1]).len = ByteNo;
    (alpu_data.msgs[1]).addr = device_addr;
    (alpu_data.msgs[1]).flags = 1;  //��
    (alpu_data.msgs[1]).buf = buff;//��Ŷ���������


    ret = ioctl(fd,I2C_RDWR,(unsigned long)&alpu_data);
    if(ret == -1)
    {
        perror("ioctl error --read ");
    }
#if 0
    printf("ByteNo:%d \n",ByteNo);
    printf("rx: \n");
    for(i = 0;i < ByteNo;i++)
    {
        printf("0X%x ",buff[i]);
    }
    putchar(10);
#endif

    free((alpu_data.msgs[0]).buf);
    free(alpu_data.msgs);
    close(fd);
    return 0;

}
