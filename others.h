
#define PORT_DFWL 8008

typedef enum CFG_STATUS
{
	GENERA = 0,	
	ITE_NAME,		
	ITE_VALUE,
} CFG_STATUS;

int getime_for_dahua(char *nowtime);
void *DahuaThrFxn(void * arg);
void *NetPoseVehicleThrFxn(void * arg);
void *othersThrFxn(void *arg);
