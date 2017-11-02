

#include "ALPU.h"
#include "logger/log.h"



/**************************************************
 *函数名：ALPU_write
 *程序说明：通过调用 ioctl 实现向IIC设备写数据
 ****************************************************/
unsigned char iic_write_fun(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    int fd,ret,i;
    //打开I2C设备文件	
    if((fd = open(i2c,O_RDWR)) == -1)
    {
        perror("Open I2C failed");
        return -1;
    }
    //定义i2c_rdwr_ioctl_data结构体并在堆上定义结构体成员msgs
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
    //填充msgs结构体
    (alpu_data.msgs[0]).len = ByteNo + 1;//ByteNo+1个字节，第1个字节装寄存器地址，其余8个字节装数据
    (alpu_data.msgs[0]).addr = device_addr;
    (alpu_data.msgs[0]).flags = 0;//0:write  1:read
    (alpu_data.msgs[0]).buf = (unsigned char*)malloc(ByteNo + 1);//申请ByteNo个unsigned  char型空间
    if (!(alpu_data.msgs[0]).buf) {
        ERROR("Malloc for alpu data msg's buf failed!");
        free(alpu_data.msgs);
        close(fd);
        return -1;
    }
    (alpu_data.msgs[0]).buf[0] = sub_addr; //第一个字节装寄存器地址
    for(i = 1;i < ByteNo + 1;i++)   //剩下ByteNo个char型数据装要写入ALPU的数据
    {
        (alpu_data.msgs[0]).buf[i] = buff[i - 1];
    }
    //通过ioctl实现往IIC设备写数据
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
 *函数名：ALPU_read
 *程序说明：通过调用 ioctl 实现向IIC设备读数据
 ****************************************************/

int iic_read_fun(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    int fd,ret;
    //打开I2C设备文件	
    if((fd = open(i2c,O_RDWR)) == -1)
    {
        perror ("Open I2C failed");
        return -1;
    }

    //定义i2c_rdwr_ioctl_data结构体并在堆上定义结构体成员msgs
    struct i2c_rdwr_ioctl_data alpu_data;
    alpu_data.nmsgs = 2; //nmsgs=2 意味着将会有2个start信号
    if( (alpu_data.msgs = (struct i2c_msg*)malloc(alpu_data.nmsgs*sizeof(struct i2c_msg))) == NULL)
    {
        printf("malloc error\n");
        close(fd);
        return -1;
    }
    //第三步：填充msgs结构体
    //第一个msgs用于实现发送I2C_ADDR和reg_addr
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

    //第二个msgs用于读取ByteNo 位数据 
    (alpu_data.msgs[1]).len = ByteNo;
    (alpu_data.msgs[1]).addr = device_addr;
    (alpu_data.msgs[1]).flags = 1;  //读
    (alpu_data.msgs[1]).buf = buff;//存放读到的数据


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
