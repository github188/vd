#include <json/json.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <fstream>

#include "dsp_config.h"
#include "dsp_config_xml.h"
#include "proc_result.h"
#include "xmlParser.h"
#include "xmlCreater.h"
#include "cmem.h"
#include "commonfuncs.h"
#include "mcfw/interfaces/link_api/videoAnalysisLink_parm.h"
#include "mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"
#include "sysctrl.h"
#include "ve/config/hide_param.h"
#include "ApproDrvMsg.h"
#include "logger/log.h"
#include "bitcom_model_global.h"

#define DSP_RESULT_CMEM_SIZE 	(100 * Ksize)

#define HIDE_CFG_SIZE (100000)
#define DSP_MSG_SIZE	(1024 * Ksize)

static void *CMEM_allocHEAP(size_t size);
static VDConfigData *init_dsp_cfg(void);
static int refresh_dsp_cfg(void);
static char *init_hidden_param(void);
static int read_hidden_param(char *cfg_buffer, const char *file_path);
static void *init_dsp_result_cmem(void);
static int send_dsp_result_cmem(void);

extern ARM_config g_arm_config;

/*******************************************************************************
 * 函数名: CMEM_allocHEAP
 * 功  能: 申请CMEM堆内存
 * 返回值: 成功，返回内存地址；失败，返回NULL
*******************************************************************************/
void *CMEM_allocHEAP(size_t size)
{
	/* First initialize the CMEM module */
	if (CMEM_init() == -1)
	{
		fprintf(stderr, "Failed to initialize CMEM\n");
		exit(EXIT_FAILURE);
	}

	CMEM_AllocParams params;
	params.type = CMEM_HEAP; // CMEM_HEAP or CMEM_POOL
	params.flags = CMEM_NONCACHED; //CMEM_CACHED or CMEM_NONCACHED
	params.alignment = 4; //only used for heap allocations, must be power of 2

	void *p_cmem = NULL;
	p_cmem = CMEM_alloc(size, &params);
	if (p_cmem == NULL)
	{
		printf("CMEM_alloc %d bytes failed !\n", (int)size);
		CMEM_exit();
		return NULL;
	}

	memset(p_cmem, 0, size);

	return p_cmem;
}


/*******************************************************************************
 * 函数名: init_dsp_cfg
 * 功  能: 初始化算法配置参数结构体
 * 返回值: 成功，返回参数指针；失败，返回NULL
*******************************************************************************/
VDConfigData *init_dsp_cfg(void)
{
	VDConfigData *p_dsp_cfg = NULL;
	p_dsp_cfg = (VDConfigData *) CMEM_allocHEAP(sizeof(VDConfigData));
	if (p_dsp_cfg == NULL)
	{
		printf("CMEM_alloc for dsp cfg failed !\n");
		return NULL;
	}

	return p_dsp_cfg;
}


/*******************************************************************************
 * 函数名: get_dsp_cfg_pointer
 * 功  能: 获取算法配置参数结构体指针
 * 返回值: 成功，返回指针；失败，返回NULL
*******************************************************************************/
VDConfigData *get_dsp_cfg_pointer(void)
{
	static VDConfigData  *p_dsp_cfg = NULL;

	if (p_dsp_cfg == NULL)
	{
		p_dsp_cfg = (VDConfigData  *)init_dsp_cfg();
	}

	return p_dsp_cfg;
}





/*******************************************************************************
 * 函数名: init_hidden_param
 * 功  能: 初始化算法隐形参数
 * 返回值: 成功，返回参数指针；失败，返回NULL
*******************************************************************************/
char *init_hidden_param(void)
{
	char *p_hidden_param = NULL;
	p_hidden_param = (char *) CMEM_allocHEAP(HIDE_CFG_SIZE);
	if (p_hidden_param == NULL)
	{
		printf("CMEM_alloc for hidden param failed !\n");
		return NULL;
	}

	return p_hidden_param;
}


/*******************************************************************************
 * 函数名: get_hidden_param_pointer
 * 功  能: 获取算法隐形参数结构体指针
 * 返回值: 成功，返回指针；失败，返回NULL
*******************************************************************************/
char *get_hidden_param_pointer(void)
{
	static char *p_hidden_param = NULL;

	if (p_hidden_param == NULL)
	{
		p_hidden_param = init_hidden_param();
	}

	return p_hidden_param;
}



/*******************************************************************************
 * 函数名: init_dsp_msg_param
 * 功  能: 初始化dsp消息的cmem缓冲
 * 返回值: 成功，返回参数指针；失败，返回NULL
*******************************************************************************/
char *init_dsp_msg_param(void)
{
	char *p = NULL;
	p = (char *) CMEM_allocHEAP(DSP_MSG_SIZE);
	if (p == NULL)
	{
		printf("CMEM_alloc for dsp_msg failed !\n");
		return NULL;
	}

	return p;
}


/*******************************************************************************
 * 函数名: get_dsp_msg_pointer
 * 功  能: 获取dsp消息的cmem缓冲地址
 * 返回值: 成功，返回指针；失败，返回NULL
*******************************************************************************/
char *get_dsp_msg_pointer(void)
{
	static char *p = NULL;

	if (p == NULL)
	{
		p = init_dsp_msg_param();
	}

	return p;
}


/*******************************************************************************
 * 函数名: init_dsp_result_cmem
 * 功  能: 初始化算法隐形参数
 * 返回值: 成功，返回参数指针；失败，返回NULL
*******************************************************************************/
void *init_dsp_result_cmem(void)
{
	void *p_dsp_result = NULL;
	p_dsp_result = CMEM_allocHEAP(DSP_RESULT_CMEM_SIZE);
	if (p_dsp_result == NULL)
	{
		printf("CMEM_alloc for DSP result failed !\n");
		return NULL;
	}

	return p_dsp_result;
}

/*******************************************************************************
 * 函数名: get_dsp_result_cmem_pointer
 * 功  能: 获取算法输出结果CMEM虚拟地址指针
 * 返回值: 成功，返回指针；失败，返回NULL
*******************************************************************************/
void *get_dsp_result_cmem_pointer(void)
{
	static void *p_dsp_result = NULL;

	if (p_dsp_result == NULL)
	{
		p_dsp_result = init_dsp_result_cmem();
	}

	return p_dsp_result;
}

/*******************************************************************************
 * 函数名: read_hidden_param
 * 功  能: 读取隐形参数
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int read_hidden_param(char *cfg_buffer, const char *file_path)
{
	int ret = -1;
	int flen = 0;

	FILE *param_file = fopen(file_path, "r");
	if (NULL == param_file)
	{
		printf("open file %s fail : %m.\n", file_path);
		return ret;
	}

	fseek(param_file,0,SEEK_END); /* 定位到文件末尾 */
	flen=ftell(param_file); /* 得到文件大小 */
	fseek(param_file,0,SEEK_SET); /* 定位到文件开头 */
	fread(cfg_buffer,flen,1,param_file); /* 一次性读取全部文件内容 */

	fclose(param_file);
	ret=flen;

	return ret;
}

/*******************************************************************************
 * 函数名: refresh_hidden_param
 * 功  能: 更新算法隐形参数
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int refresh_hidden_param(void)
{
	char *p_hidden_param = get_hidden_param_pointer();

	return read_hidden_param(p_hidden_param, HIDE_PARAM_FILE_PATH);
}

/*******************************************************************************
 * 函数名: refresh_dsp_cfg
 * 功  能: 更新算法配置参数
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int refresh_dsp_cfg(void)
{
//	VDCS_config
	VDConfigData *p_dsp_cfg = (VDConfigData  *)get_dsp_cfg_pointer();
	if (p_dsp_cfg == NULL)
	{
		printf("Get DSP config pointer failed !\n");
		return -1;
	}

	if (check_file_correct(DSP_PARAM_FILE_PATH) < 0)
	{
		int len = sizeof(DEFAULT_DSP_CONFIG_XML);
		char *tempBuf = (char *)malloc(256*1024);
		if(tempBuf != NULL)
		{
			convert_enc("GBK", "UTF-8",
			            (char*) DEFAULT_DSP_CONFIG_XML, len,
			            tempBuf, 256*1024);
			save_file(DSP_PARAM_FILE_PATH, tempBuf, 256*1024);
			free(tempBuf);
		}
		else
		{
			debug("cannot malloc enough space!\n");
		}
	}

	parse_xml_doc(DSP_PARAM_FILE_PATH, p_dsp_cfg, NULL, NULL);

	printf("解析dsp_config.xml 完成!\n");

	return 0;
}


//给dsp发送一个消息，采用cmem传递数据
//比较通用，可以传递道闸控制消息，或者车牌库文件等
//msg_type : 消息类型，1，算法配置参数；2，隐形配置参数；3，车牌库；4，道闸控制输入参数；5，电子车牌触发参数；6，重启前记录回传；7，arm参数
//msg_type : 消息类型, 8, 抓拍请求； 11，出入口白名单
int send_dsp_msg(void * buf, int size, int msg_type)
{

	printf("vd %s\n",__func__);
	if(buf==NULL)
	{
		printf("input buf is NULL\n");
		return -1;
	}
	if((size<=0)||(size >DSP_MSG_SIZE))
	{
		printf("input buf size=%d is invalid\n",size);
		return -1;
	}


	//为了避免多个线程访问cmem导致冲突，添加互斥锁进行保护
	static pthread_mutex_t mutex;
	static int flag_first=1;
	if(flag_first==1)
	{
		flag_first=0;
		pthread_mutex_init(&mutex, NULL);
	}

	pthread_mutex_lock(&mutex);


	//拷贝到cmem中
	char *p_msg = get_dsp_msg_pointer();
	memcpy(p_msg,buf,size);

	//赋值到MSGDATACMEM中
	MSGDATACMEM msg_cmem;
	msg_cmem.addr_phy=CMEM_getPhys(p_msg);
	msg_cmem.off_set=0;
	msg_cmem.size=size;
	msg_cmem.flag_changed=0;
	msg_cmem.msg_type=msg_type;

	log_state("vd", "%s: msg_cmem.addr_phy=0x%x,size=%d,msg_type=%d\n",__func__,
			msg_cmem.addr_phy,
			msg_cmem.size,
			msg_cmem.msg_type);

	//向mcfw.out传递
	int ret=SendToDspMsg(&msg_cmem,sizeof(MSGDATACMEM));


	pthread_mutex_unlock(&mutex);

	return ret;
}

/*******************************************************************************
 * 函数名: send_dsp_cfg
 * 功  能: 发送配置给算法
 * 参  数: cfg_changed，配置是否更改；hidden_param_changed，隐形参数是否更改
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_dsp_cfg(char cfg_changed, char hidden_param_changed)
{
	MSGConfigALG msg_dsp_cfg;

	memset(&msg_dsp_cfg,0,sizeof(msg_dsp_cfg));

	if (cfg_changed == 1)
	{
		if (refresh_dsp_cfg() < 0)
		{
			printf("Refresh DSP config failed !\n");
			return -1;
		}
	}

	VDConfigData  *p_dsp_cfg = (VDConfigData  *)get_dsp_cfg_pointer();
	msg_dsp_cfg.msg_config_alg.addr_phy 	= CMEM_getPhys(p_dsp_cfg);
	msg_dsp_cfg.msg_config_alg.size 		= sizeof(VDConfigData);
	msg_dsp_cfg.msg_config_alg.flag_changed = cfg_changed;
	msg_dsp_cfg.msg_config_alg.msg_type 	= 1;

	if (hidden_param_changed == 1)
	{
		if (refresh_hidden_param() < 0)
		{
			printf("Refresh hidden param failed !\n");
			return -1;
		}
	}

	char *p_hidden_param = get_hidden_param_pointer();
	msg_dsp_cfg.msg_hide_parm_alg.addr_phy 		= CMEM_getPhys(p_hidden_param);
	msg_dsp_cfg.msg_hide_parm_alg.size 			= HIDE_CFG_SIZE;
	msg_dsp_cfg.msg_hide_parm_alg.flag_changed 	= hidden_param_changed;
	msg_dsp_cfg.msg_hide_parm_alg.msg_type 		= 2;


	p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.x = p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.left;
	p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.y = p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.top;
	p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.width = p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.right-p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.x+1;
	p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.height = p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.bottom-p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.y+1;



	DEBUG("detect_area.left=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.left);
	DEBUG("detect_area.right=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.right);
	DEBUG("detect_area.top=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.top);
	DEBUG("detect_area.botom=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.bottom);
	DEBUG("detect_area.x=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.x);
	DEBUG("detect_area.y=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.y);
	DEBUG("detect_area.width=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.width);
	DEBUG("detect_area.heigh=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.detect_area.height);


	DEBUG("leftLineInfo.startpoint.x=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.x);
	DEBUG("leftLineInfo.startpoint.y=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.y);

	DEBUG("leftLineInfo.endpoint.x=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.x);
	DEBUG("leftLineInfo.endpoint.y=%d", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.y);

	DEBUG("leftLineInfo.line.a=%f", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.leftLineInfo.line.a);
	DEBUG("leftLineInfo.line.b=%f", p_dsp_cfg->vdConfig.vdCS_config.detectarea_info.leftLineInfo.line.b);




	printf("解析dsp_config.xml  hide_param 完成!\n");
	if (ControlSystemData(SFIELD_SET_DSP_CFG,
	                      &msg_dsp_cfg, sizeof(MSGConfigALG)) < 0)
	{
		printf("Send dsp config failed !\n");
		return -1;
	}

	char buf_whiteblacklist[100*1024];//假定黑白名单的文件长度小于100k
	printf("to read file: %s\n",WHITE_BLACK_LIST_FILE_PATH);
	int flen= read_hidden_param(buf_whiteblacklist, WHITE_BLACK_LIST_FILE_PATH);
	if(flen>=0)
	{
		habitcom_read_new();
		INFO("to send_dsp_msg: whiteblacklist:%s\n",buf_whiteblacklist);
		INFO("to send_dsp_msg: whiteblacklist,flen=%d\n",flen);
		send_dsp_msg(buf_whiteblacklist,flen,3);//消息类型，1，算法配置参数；2，隐形配置参数；3，车牌库；4，道闸控制输入参数；5，电子车牌触发参数；6，重启前记录回传；
	}
	else
	{
		printf("to read file: %s failed\n",WHITE_BLACK_LIST_FILE_PATH);
	}

	return 0;
}

/*******************************************************************************
 * 函数名: send_arm_cfg
 * 功  能: 发送arm配置给算法
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_arm_cfg()
{

	parse_xml_doc(ARM_PARAM_FILE_PATH, NULL, NULL, &g_arm_config);
	//TRACE_LOG_SYSTEM(" ====== %s 4: exp_device_id  = %s\n", __func__,
	//		g_arm_config.basic_param.exp_device_id
	//		);
	TRACE_LOG_SYSTEM(" ====== %s 5: sizeof(ARM_config)  = %d, sizeof(VDCS_config)=%d, ill=%d\n",
			__func__,
			sizeof(ARM_config),
			sizeof(VDCS_config),
			sizeof(VD_IllegalParkingConfig)
				);
	send_dsp_msg(&g_arm_config,sizeof(ARM_config),7);


	return 0;
}

/*******************************************************************************
 * 函数名: send_park_cfg
 * 功  能: 发送arm配置给算法
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_park_cfg(void)
{
    const char* park_config_path = "/mnt/nand/park.conf";
    int fd = open(park_config_path, O_RDWR);
    if (fd < 0) {
        if (errno == ENOENT) { /* No such file or directory */
            Json::Value root;
            Json::Value global;

            /* save the file use utf-8 */
            const char* utf8_province = "山东";
            const char* utf8_city = "青岛";

            global["province"] = utf8_province;
            global["city"] = utf8_city;
            root["global"] = global;

            Json::StyledWriter writer;
            std::string strWrite = writer.write(root);
            std::ofstream ofs;
            ofs.open(park_config_path);
            ofs << strWrite;
            ofs.close();
        }
        else {
            ERROR("open file %s failed. errno = %d", park_config_path, errno);
            return -1;
        }
    }

    /* /mnt/nand/park.conf exist, load it and send to dsp */
    /* get file size*/
    off_t size = lseek(fd, 0, SEEK_END);
    char *buf = (char*)malloc(sizeof(char) * size + 1);
    if (NULL == buf) {
        ERROR("malloc buf failed.");
        return -1;
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, buf, size);

    /* convert utf-8 to gbk & send to dsp */
    char* buf_gbk = (char*)malloc(sizeof(char) * size);
    if (buf_gbk == NULL) {
        ERROR("malloc buf_gbk failed.");
        return -1;
    }

    ssize_t length = convert_enc_s("UTF-8", "GBK", buf, size, buf_gbk, size);

    send_dsp_msg(buf_gbk, length + 1, 12);

    free(buf);
    buf = NULL;

    free(buf_gbk);
    buf_gbk = NULL;

    close(fd);
	return 0;
}

/*******************************************************************************
 * 函数名: get_dsp_result_addr_phy
 * 功  能: 获取算法输出结果CMEM物理地址
 * 返回值: 成功，返回物理地址；失败，返回-1
*******************************************************************************/
unsigned long get_dsp_result_addr_phy(void)
{
	static unsigned long addr_phy = -1;

	if (addr_phy == (unsigned long)-1)
	{
		void *addr_vir = get_dsp_result_cmem_pointer();
		if (addr_vir != NULL)
		{
			addr_phy = CMEM_getPhys(addr_vir);
		}
	}

	return addr_phy;
}

/*******************************************************************************
 * 函数名: send_dsp_result_cmem
 * 功  能: 发送算法输出结果CMEM信息给DSP
 * 返回值: 成功，返回0
*******************************************************************************/
int send_dsp_result_cmem(void)
{
	MSGDATACMEM msg_dsp_result_cmem;

	msg_dsp_result_cmem.addr_phy 		= get_dsp_result_addr_phy();
	msg_dsp_result_cmem.size 			= DSP_RESULT_CMEM_SIZE;
	msg_dsp_result_cmem.flag_changed 	= 1;
	msg_dsp_result_cmem.msg_type 		= 0;

	if (ControlSystemData(SFIELD_SET_CMEM_ALG_RESULT,
	                      &msg_dsp_result_cmem, sizeof(MSGDATACMEM)) < 0)
	{
		printf("Send cmem alg result failed !\n");
		return -1;
	}

	printf("Send cmem alg result done.\n");

	return 0;
}

/*******************************************************************************
 * 函数名: dsp_init
 * 功  能: DSP算法初始化
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int dsp_init(void)
{
	if ( (get_dsp_cfg_pointer() == NULL) ||
	        (get_hidden_param_pointer() == NULL) ||
	        (get_dsp_result_cmem_pointer() == NULL) )  //1VD开辟的CMEM空间，用于存放ArithmOutput 结构体，跟图片缓存CMEM没关系
	{
		printf("dsp init failed\n");
		return -1;
	}

	send_dsp_cfg(1, 1);
	send_arm_cfg();
	send_dsp_result_cmem();

#if 1 /* vd send park config to dsp */
#if (DEV_TYPE == 1)
    send_park_cfg();
#endif
#endif

	create_result_recv_task();

	return 0;
}

