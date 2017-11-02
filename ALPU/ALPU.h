#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <assert.h>



#define I2C_RETRIES     0x0701
#define I2C_TIMEOUT     0x0702
#define I2C_RDWR        0x0707


static const char i2c [] = "/dev/i2c-2" ;



unsigned char iic_write_fun(unsigned char , unsigned char , unsigned char *, int );
int iic_read_fun(unsigned char , unsigned char , unsigned char *, int );
void  ALPU(void);

struct i2c_msg
{
  __u16 addr; 
  __u16 flags;
  __u16 len;
  #define I2C_M_RD        0x01
  __u8 *buf; 
 };
 
 struct i2c_rdwr_ioctl_data
{
  struct i2c_msg *msgs; // i2c_msg[]÷∏’Î 
  int nmsgs; // i2c_msg ˝¡ø 
};

