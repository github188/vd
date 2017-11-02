#include <stdio.h>
#include <sem_util.h>
#include <osa_i2c.h>
#include <lib_i2c.h>
#include <sys_env_type.h>
#include "i2c_sem_util.h"
#include "file_msg_drv.h"

#define SET_REGISTER_ENABLE_TIME	5000
static OSA_I2cHndl i2cHndl;
//device address 0x2f
static __u8 DevAddr[I2C_TRANSFER_SIZE_MAX][I2C_TRANSFER_SIZE_MAX] = {"0X2f"};

// function bank0Preset register address and value
 __u8 CheckregAddr1[I2C_TRANSFER_SIZE_MAX][I2C_TRANSFER_SIZE_MAX] = {"0xf1"};
 __u8 CheckregValue1[I2C_TRANSFER_SIZE_MAX][I2C_TRANSFER_SIZE_MAX] = {"0x00", "0x02", "0x01"};

/*	Following add by zdy, 2013-06-08	*/
 __u8 FocusIrisregAddr[I2C_TRANSFER_SIZE_MAX][I2C_TRANSFER_SIZE_MAX] = {"0xf1", "0xb8", "0xb7", "0xb6", "0xb5", "0xb9"};
 __u8 FocusIrisregValue[I2C_TRANSFER_SIZE_MAX][I2C_TRANSFER_SIZE_MAX] = {"0x02" ,"0x00", "0x01", "0x0b" /*0x06*/, "0x0c" /*0x07*/}; //modified by dsl,


static sem_t *sem;
static void enterCriticalSection()
{
	if (sem == NULL)
	{
		I2CSemInit();
  		sem = sem_open(SEM_NAME, OPEN_FLAG, OPEN_MODE, INIT_V);	
	}
	I2CSemWait(sem);
}

static void leaveCriticalSection()
{
	I2CSemRelease(sem);
}
static int GetBankVal()
{
	__u8 devAddr;
	__u8 regAddr[I2C_TRANSFER_SIZE_MAX], regValue8[I2C_TRANSFER_SIZE_MAX];	
	int status;
	int bankval;

	devAddr = (__u8)xstrtoi((char *)DevAddr[0]);
	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK) 
 	{
  		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		return OSA_EFAIL;
	}
	
	regAddr[0] = (__u8)xstrtoi((char *)CheckregAddr1[0]);
	status = OSA_i2cRead8(&i2cHndl, devAddr, regAddr, regValue8, 1);

	if (status != OSA_SOK)
	{
		fprintf(stderr, "bank0Preset Read ERROR!\n");
		OSA_i2cClose(&i2cHndl);
		return OSA_EFAIL;
	}
	OSA_i2cClose(&i2cHndl);

	usleep(SET_REGISTER_ENABLE_TIME);

	bankval = regValue8[0];

	return bankval;
}

static int SetBankVal(int value)
{
	__u8 devAddr;
	__u8 regAddr[I2C_TRANSFER_SIZE_MAX], regValue8[I2C_TRANSFER_SIZE_MAX];	
	int status;

	devAddr = xstrtoi((char *)DevAddr[0]);
	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK) 
 	{
  		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		return OSA_EFAIL;
	}
	
	regAddr[0] = xstrtoi((char *)CheckregAddr1[0]);
	regValue8[0] = value;

	status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, 1);

	if (status != OSA_SOK)
	{
		fprintf(stderr, "bank0Preset Write ERROR!\n");
		OSA_i2cClose(&i2cHndl);
		return OSA_EFAIL;
	}
	OSA_i2cClose(&i2cHndl);

	usleep(SET_REGISTER_ENABLE_TIME);
	return OSA_SOK;
}

#if 0
static int bank0Preset()
{
	__u8 devAddr;
	__u8 regAddr[I2C_TRANSFER_SIZE_MAX], regValue8[I2C_TRANSFER_SIZE_MAX];	
	int status;

	devAddr = xstrtoi((char *)DevAddr[0]);
	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK) 
 	{
  		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		return OSA_EFAIL;
	}
	
	regAddr[0] = xstrtoi((char *)CheckregAddr1[0]);
	regValue8[0] = xstrtoi((char *)CheckregValue1[0]);

	status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, 1);
	
	if (status != OSA_SOK)
	{
		fprintf(stderr, "bank0Preset Write ERROR!\n");
		OSA_i2cClose(&i2cHndl);
		return OSA_EFAIL;
	}
	usleep(SET_REGISTER_ENABLE_TIME);
	status = OSA_i2cClose(&i2cHndl);
	return OSA_SOK;
}
#endif

#if 0
// f1:01
static int bank1Preset()
{
	__u8 devAddr;
	__u8 regAddr[I2C_TRANSFER_SIZE_MAX], regValue8[I2C_TRANSFER_SIZE_MAX];	
	int status;

	devAddr = xstrtoi((char *)DevAddr[0]);
	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK) 
 	{
  		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		return OSA_EFAIL;
	}
	
	regAddr[0] = xstrtoi((char *)CheckregAddr1[0]);
	regValue8[0] = xstrtoi((char *)CheckregValue1[2]); //0x01

	status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, 1);
	if (status == OSA_SOK)
	{
		usleep(SET_REGISTER_ENABLE_TIME);
	}
	else
	{
		fprintf(stderr, "bank1Preset Write ERROR!\n");
		OSA_i2cClose(&i2cHndl);
		return OSA_EFAIL;
	}
	OSA_i2cClose(&i2cHndl);
	return OSA_SOK;
}
#endif

// 0xf1: 0x02
static int bank2Preset()
{
	__u8 devAddr;
	__u8 regAddr[I2C_TRANSFER_SIZE_MAX], regValue8[I2C_TRANSFER_SIZE_MAX];	
	int status;

	devAddr = xstrtoi((char *)DevAddr[0]);
	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK) 
 	{
  		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		return OSA_EFAIL;
	}
	
	regAddr[0] = xstrtoi((char *)CheckregAddr1[0]);	 //0xf1
	regValue8[0] = xstrtoi((char *)CheckregValue1[1]); //0x02

	status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, 1);
	if (status == OSA_SOK)
	{
		usleep(SET_REGISTER_ENABLE_TIME);
	}
	else
	{
		fprintf(stderr, "bank2Preset Write ERROR!\n");
		OSA_i2cClose(&i2cHndl);
		return OSA_EFAIL;
	}
	OSA_i2cClose(&i2cHndl);
	return OSA_SOK;
}

#if 0
static void GetValue(__u8 HiLow[], unsigned short value)
{
	HiLow[1] = value/0x100; //High
	HiLow[0] = value%0x100; //Low	 
}
#endif

int stopFocus()
{
	enterCriticalSection();
	int ret = OSA_SOK;
	__u8 devAddr;
	__u8 regValue8[I2C_TRANSFER_SIZE_MAX], regAddr[I2C_TRANSFER_SIZE_MAX];
	int numRegs, status;

	int bankval;
	bankval = GetBankVal();	
	if (bankval == OSA_EFAIL)
	{
		printf("Get bank value fail!\n");
		ret = OSA_EFAIL;
		goto exit;
	}

	devAddr = xstrtoi((char *)DevAddr[0]);
	numRegs = 2;
	printf("inside : %s\n", __func__);

	if (bank2Preset() == OSA_EFAIL)
	{
		fprintf(stderr, "ERROR: %s\n", __func__);
		ret = OSA_EFAIL;
		goto exit;
	}

	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK)
	{
		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		ret = OSA_EFAIL;
		goto exit;
	}

	regAddr[0] = xstrtoi((char *)FocusIrisregAddr[3]); //0xb6
	regAddr[1] = xstrtoi((char *)FocusIrisregAddr[4]); //0xb5 //deleted by dsl, 2013-11-25

	regValue8[0] = xstrtoi((char *)FocusIrisregValue[1]); //0x00
	regValue8[1] = xstrtoi((char *)FocusIrisregValue[1]); //0x00 //deleted by dsl, 2013-11-25

	status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, numRegs);
	if(status != OSA_SOK)
	{
		fprintf(stderr, "Stop Focus ERROR!\n");
		OSA_i2cClose(&i2cHndl);
		ret = OSA_EFAIL;
		goto exit;
	}
	usleep(SET_REGISTER_ENABLE_TIME);


	if (SetBankVal(bankval) != OSA_SOK)
	{
		printf("Set Bank Value Fail!\n");
		ret = OSA_EFAIL;
		goto exit;
	}
exit:
	leaveCriticalSection();
	return ret;;
}

// Focus near
int focusNear(unsigned char value)
{
	enterCriticalSection();
	int ret = OSA_SOK;
	__u8 devAddr;
	__u8 regValue8[I2C_TRANSFER_SIZE_MAX], regAddr[I2C_TRANSFER_SIZE_MAX];
	int numRegs, status;
	int speed = -1;
	__u8 speedValue[3] = {0x20, 0x05, 0x1}; //FIXME: neet to modify, normal, sharp, fine focus

	int bankval;

	SysInfo *pSysInfo = GetSysInfo();

	if (pSysInfo == NULL) {
		ret = OSA_EFAIL;
		goto exit;
	}
	speed = pSysInfo->cam_set.nFocusSpeed;


	bankval = GetBankVal();	
	if (bankval == OSA_EFAIL)
	{
		printf("Get bank value fail!\n");
		ret = OSA_EFAIL;
		goto exit;
	}

	devAddr = xstrtoi((char *)DevAddr[0]);
	numRegs = 2;

	if (bank2Preset() == OSA_EFAIL)
	{
		fprintf(stderr, "ERROR: %s\n", __func__);
		ret = OSA_EFAIL;
		goto exit;
	}

	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK)
	{
		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		ret = OSA_EFAIL;
		goto exit;
	}

	switch (value)
	{
		case 0:
			stopFocus();
			break;
		case 1:
			regAddr[0] = xstrtoi((char *)FocusIrisregAddr[3]); //0xb6
			regAddr[1] = xstrtoi((char *)FocusIrisregAddr[4]); //0xb5

			regValue8[0] = xstrtoi((char *)FocusIrisregValue[4]); //0x0c
			regValue8[1] = speedValue[speed];		

			status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, numRegs);
			if(status != OSA_SOK)
			{
				fprintf(stderr, "Focus Near ERROR!\n");
				OSA_i2cClose(&i2cHndl);
				return OSA_EFAIL;
			}
			usleep(SET_REGISTER_ENABLE_TIME);
			break;
		default:
			break;
	}

	if (SetBankVal(bankval) != OSA_SOK)
	{
		printf("Set Bank Value Fail!\n");
		ret = OSA_EFAIL;
		goto exit;
	}
exit:
	leaveCriticalSection();	
	return ret;
}
// Focus far
int focusFar(unsigned char value)
{
	enterCriticalSection();
	int ret = OSA_SOK;
	__u8 devAddr;
	__u8 regValue8[I2C_TRANSFER_SIZE_MAX], regAddr[I2C_TRANSFER_SIZE_MAX];
	int numRegs, status;
	int speed = -1;
	__u8 speedValue[3] = {0x20, 0x05, 0x1}; //FIXME: need to modify, normal, sharp, fine focus
	int bankval;

	SysInfo *pSysInfo = GetSysInfo();

	if (pSysInfo == NULL) {
		ret = OSA_EFAIL;
		goto exit;
	}
	speed = pSysInfo->cam_set.nFocusSpeed;

	bankval = GetBankVal();	
	if (bankval == OSA_EFAIL)
	{
		printf("Get bank value fail!\n");
		ret = OSA_EFAIL;
		goto exit;
	}

	devAddr = xstrtoi((char *)DevAddr[0]);
	numRegs = 2;

	if (bank2Preset() == OSA_EFAIL)
	{
		fprintf(stderr, "ERROR: %s\n", __func__);
		ret = OSA_EFAIL;
		goto exit;
	}

	status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);
	if(status != OSA_SOK)
	{
		fprintf(stderr, "ERROR: OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
		ret = OSA_EFAIL;
		goto exit;
	}

	switch (value)
	{
		case 0:
			stopFocus();
			break;
		case 1:
			regAddr[0] = xstrtoi((char *)FocusIrisregAddr[3]); //0xb6
			regAddr[1] = xstrtoi((char *)FocusIrisregAddr[4]); //0xb5

			regValue8[0] = xstrtoi((char *)FocusIrisregValue[3]); //0x0b
			regValue8[1] = speedValue[speed];
			
			status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, numRegs);
			if(status != OSA_SOK)
			{
				fprintf(stderr, "Focus Near ERROR!\n");
				return OSA_EFAIL;
			}
			usleep(SET_REGISTER_ENABLE_TIME);
			break;
		default:
			break;
	}

	if (SetBankVal(bankval) != OSA_SOK)
	{
		printf("Set Bank Value Fail!\n");
		ret = OSA_EFAIL;
	}
exit:
	leaveCriticalSection();	
	return ret;
}
