/*
 * CLASS_FTP.cpp
 *
 *  Created on: 2013-3-15
 *      Author: shanhongwei
 */

#include <pthread.h>
#include <sys/sendfile.h>

#include "ftp.h"
#include "mutex_guard.h"
#include "ep_type.h"
#include "arm_config.h"

extern ARM_config g_arm_config; //arm参数结构体全局变量
extern SET_NET_PARAM g_set_net_param;

CLASS_FTP *g_ftp_chanel[FTP_CHANNEL_COUNT]; //ftp全部通道;

CLASS_FTP *g_ftp_illegal;
CLASS_FTP *g_ftp_pass_car;
CLASS_FTP *g_ftp_h264;
CLASS_FTP *g_ftp_conf;
CLASS_FTP *g_ftp_illegal_resuming;
CLASS_FTP *g_ftp_pass_car_resuming;
CLASS_FTP *g_ftp_h264_resuming;

static bool bInitialized = false;

//namespace FTP
//{

CLASS_FTP::CLASS_FTP() :
    server_is_full(false), thread_stop(false), pCallerParam(NULL)
{
    strcpy(descript, "");
    server_is_full = false;
    set_status(STA_FTP_FAIL, true);
    alarmFunc = NULL;
    pthread_mutex_init(&mutex, NULL);
}

CLASS_FTP::CLASS_FTP(const char *des) :
    server_is_full(false), thread_stop(false), pCallerParam(NULL)
{
    strcpy(descript, des);
    server_is_full = false;
    set_status(STA_FTP_FAIL, true);
    alarmFunc = NULL;
    pthread_mutex_init(&mutex, NULL);
}

CLASS_FTP::CLASS_FTP(void * pCallerParam) :
    server_is_full(false), thread_stop(false), pCallerParam(pCallerParam)
{
    strcpy(descript, "");
    server_is_full = false;
    set_status(STA_FTP_FAIL, true);
    alarmFunc = NULL;
    pthread_mutex_init(&mutex, NULL);
}

CLASS_FTP::~CLASS_FTP()
{
    if (started)
    {
        stop();
    }
}

/*
 *    功能: 打开ftp服务器,登录,检查空间是否满 .
 *    参数: 无
 *  返回值: FAILURE--失败; SUCCESS--成功.
 */
int CLASS_FTP::open_server()
{
    int sta;
    char text[LEN_MAX_MSG_LOG];

    debug_ftp("%s %s.....\n", descript, __func__);

    sta = ftp_mk_ctl_socket();
    if (sta != SUCCESS)
    {
        sprintf(text, "%s mk_ctl_socket() fail!\n", descript);
        debug_ftp((char*)text);
        log_error_ftp(text);
        set_status(STA_FTP_FAIL, true);

        return FAILURE;
    }
    debug_ftp("%s mk_ctl_socket() success!\n", descript);
    log_send(LOG_LEVEL_STATUS,0,"VD:","%s mk_ctl_socket() success!\n", descript);

    sta = ftp_login();
    if (sta != SUCCESS)
    {
        sprintf(text, "%s login_ftp() fail! return:%d\n", descript, sta);
        debug_ftp(text);
        log_error_ftp(text);
        set_status(STA_FTP_EXP, true);

        return FAILURE;
    }
    set_status(STA_FTP_OK);
    debug_ftp("%s login_ftp() success!\n", descript);
    log_send(LOG_LEVEL_STATUS,0,"VD:","%s login_ftp() success!\n", descript);

    //check full
    int ret = ftp_check_server_full();
    switch (ret)
    {
        case 0:
            break;
        case 1: //full
            sprintf(text, "%s ftp is full!\n", descript);
            log_send(LOG_LEVEL_STATUS,0,"VD:","%s login_ftp() success!\n", descript);
            debug_ftp(text)
                ;
            log_warn_ftp(text)
                ;
            break;
        case SOCKET_ERR:
            sprintf(text, "%s ftp_check_server_full() fail!\n", descript);
            log_send(LOG_LEVEL_STATUS,0,"VD:","%s ftp_check_server_full() fail!\n", descript);
            debug_ftp(text)
                ;
            log_error_ftp(text)
                ;
            break;
    }

    return SUCCESS;
}

/**
 *    功能: 关闭ftp服务器,登出,关闭连接.
 *    参数: 无
 *  返回值: 无.
 */
void CLASS_FTP::close_server()
{
    char text[LEN_MAX_MSG_LOG];

    //ftp_logout();
    set_status(STA_FTP_FAIL, true);
    ftp_close_ctl_socket();

    sprintf(text, "%s server close!\n", descript);
    debug_ftp(text);
    log_state_ftp(text);
}

int CLASS_FTP::ftp_check_ftp_sta(void)
{
    int send_bytes;
    char cmd[48] = "NOOP\r\n";
    char ftp_rcv_buf[200];

    mutex_guard guard(mutex);
    memset(ftp_rcv_buf, 0, sizeof(ftp_rcv_buf));

    send_bytes = ftp_send_cmd(cmd, strlen(cmd));
    if (send_bytes != (int) (strlen(cmd)))
    {
        debug_ftp("error when send NOOP.\n");
        ftp_status = STA_FTP_FAIL;
        return STA_FTP_FAIL;
    }

    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        return STA_FTP_FAIL;
    }
    else
    {
        if (strncasecmp(ftp_rcv_buf, "200", 3) != 0)
        {
            debug_ftp(
                    "%s check_ftp_sta() recv [NOOP] fail.. server>>:[%s]\n",
                    descript, ftp_rcv_buf);
            ftp_close_ctl_socket();
            ftp_status = STA_FTP_FAIL;

            return STA_FTP_FAIL;
        }
    }

    set_status(STA_FTP_OK);
    return STA_FTP_OK;
}


/**
 *    功能: 上传内存数据到ftp服务器,保存为文件.
 *    参数:
 *      dir: ftp服务器上的路径
 *      name: 文件名
 *      buf: 数据地址
 *      len: 数据字节数
 *      buf2: 第二段数据地址
 *      len2: 第二段数据字节数
 *  返回值: FAILURE -- 失败; 0-- 成功; SOCKET_ERR --发送错误
 */
int CLASS_FTP::ftp_put_data(char dir[][80], char * filename, char *buf,
        long len, char *buf2, long len2)
{
    char ftp_rcv_buf[200];
    int i, ret;
    char cmd_dir[256] = "/\0";

    if (strlen(dir[0]) == 0 || strlen(filename) == 0 || buf == NULL || len == 0)
    {
        debug_ftp("%s %s param error!\n", descript, __func__);
        TRACE_LOG_SYSTEM("[FTP_SEND]: 11111111111111111111111");
        return FAILURE;
    }
    if (ftp_status != STA_FTP_OK)
    {
        debug_ftp("%s ftp_put_file() 连接异常！\n", descript);
        TRACE_LOG_SYSTEM("[FTP_SEND]: 222222222222222222222222");		
        return FAILURE;
    }

    debug_ftp("%s %s start ....\n", descript, __func__);

    clear_socket_recvbuf(this->fd_ctl_socket);

    //###########/创建目录/#################//
    if (SUCCESS != ftp_mk_dir(dir))
    {
        TRACE_LOG_SYSTEM("[FTP_SEND]: 33333333333333333333333333");
        return FAILURE;
    }

    //##### 进入被动传输模式 ########//
    if (SUCCESS != ftp_set_trans_pasv_mode())
    {
        TRACE_LOG_SYSTEM("[FTP_SEND]: 444444444444444444444444444");
        return FAILURE;
    }

    //####### 创建数据连接端口 ########//
    if (SOCKET_ERR == ftp_mk_data_socket())
    {
        TRACE_LOG_SYSTEM("[FTP_SEND]: 555555555555555555555555555");
        return FAILURE;
    }

    debug_ftp("%s create ftp data  socket ok! \n", descript);

    //############/存储数据/###############//
    for (i = 0; i < MAX_DIR_LEVEL; i++)
    {
        if (strlen(dir[i]) != 0)
        {
            strcat(cmd_dir, dir[i]);
            strcat(cmd_dir, "/");
        }
        else
        {
            break;
        }
    }

    if (SUCCESS != ftp_store(cmd_dir, filename))
    {
        debug_ftp("%s store cmd false! \n", descript);
        ftp_close_data_socket();

        TRACE_LOG_SYSTEM("[FTP_SEND]: 6666666666666666666666666");

        return FAILURE;
    }

    //开始传送数据
    mutex_guard guard(mutex);
    ret = ftp_send_data(buf, len);
    if (buf2 != NULL)
    {
        ret |= ftp_send_data(buf2, len2);
    }
    ftp_close_data_socket();
    if (SUCCESS != ret)
    {
        TRACE_LOG_SYSTEM("[FTP_SEND]: 77777777777777777777777");
        return FAILURE;
    }

    //数据发送完毕正常
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("close data socket, and recv failed\n");
        TRACE_LOG_SYSTEM("[FTP_SEND]: 888888888888888888888888888888");
        return FAILURE;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_rcv_buf);

    if (strncasecmp(ftp_rcv_buf, "226", 3) == 0) //收到226表示成功;
    {
        debug_ftp("%s ftp send file success!	\n", descript);
    }
    else if (!strncasecmp(ftp_rcv_buf, "452", 3)) //452	磁盘空间不足
    {
        debug_ftp("%s ftp disk full!\n", descript);
        server_is_full = true;

        TRACE_LOG_SYSTEM("[FTP_SEND]: 999999999999999999999999999");
        return FAILURE;
    }
    else
    {
        debug_ftp("%s send buffer to ftp, failed..ftp server abnormal!\n", descript);
        ftp_status = STA_FTP_EXP;

        TRACE_LOG_SYSTEM("[FTP_SEND]: 000000000000000000000000000");
        return FAILURE;
    }

    return SUCCESS;
}


/**
 *    功能: 上传内存数据到ftp服务器,保存为文件.
 *    参数:
 *      dir: ftp服务器上的路径
 *      name: 文件名
 *      buf: 数据地址
 *      len: 数据字节数
 *      buf2: 第二段数据地址
 *      len2: 第二段数据字节数
 *  返回值: FAILURE -- 失败; 0-- 成功; SOCKET_ERR --发送错误
 */
int CLASS_FTP::ftp_put_h264(char dir[][80], char * filename, EP_VidInfo *pEP_vidinfo)
{
    char ftp_rcv_buf[200];
    int i;
    int ret = -1;
    char cmd_dir[256] = "/\0";

    if (strlen(dir[0]) == 0 || strlen(filename) == 0 || pEP_vidinfo == NULL)// || len == 0)
    {
        debug_ftp("%s %s param error!\n", descript, __func__);
        return FAILURE;
    }
    if (ftp_status != STA_FTP_OK)
    {
        debug_ftp("%s ftp_put_file() 连接异常！\n", descript);
        return FAILURE;
    }

    debug_ftp("%s %s start ....\n", descript, __func__);

    clear_socket_recvbuf(this->fd_ctl_socket);

    //###########/创建目录/#################//
    if (SUCCESS != ftp_mk_dir(dir))
    {
        return FAILURE;
    }

    //##### 进入被动传输模式 ########//
    if (SUCCESS != ftp_set_trans_pasv_mode())
    {
        return FAILURE;
    }

    //####### 创建数据连接端口 ########//
    if (SOCKET_ERR == ftp_mk_data_socket())
    {
        return FAILURE;
    }

    debug_ftp("%s create ftp data  socket ok! \n", descript);

    //############/存储数据/###############//
    for (i = 0; i < MAX_DIR_LEVEL; i++)
    {
        if (strlen(dir[i]) != 0)
        {
            strcat(cmd_dir, dir[i]);
            strcat(cmd_dir, "/");
        }
        else
        {
            break;
        }
    }

    if (SUCCESS != ftp_store(cmd_dir, filename))
    {
        debug_ftp("%s store cmd false! \n", descript);
        ftp_close_data_socket();

        return FAILURE;
    }

    //开始传送数据
    mutex_guard guard(mutex);
    //	ret = ftp_send_data(buf, len);
    //	if (buf2 != NULL)
    //	{
    //		ret |= ftp_send_data(buf2, len2);
    //	}
    for(int i=0;i<pEP_vidinfo->buf_num;i++)
    {
        ret = ftp_send_data(pEP_vidinfo->buf[i], pEP_vidinfo->size[i]);
    }

    ftp_close_data_socket();
    if (SUCCESS != ret)
    {
        return FAILURE;
    }

    //数据发送完毕正常
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("close data socket, and recv failed\n");
        return FAILURE;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_rcv_buf);

    if (strncasecmp(ftp_rcv_buf, "226", 3) == 0) //收到226表示成功;
    {
        debug_ftp("%s ftp send file success!	\n", descript);
    }
    else if (!strncasecmp(ftp_rcv_buf, "452", 3)) //452	磁盘空间不足
    {
        debug_ftp("%s ftp disk full!\n", descript);
        server_is_full = true;

        return FAILURE;
    }
    else
    {
        debug_ftp("%s send buffer to ftp, failed..ftp server abnormal!\n", descript);
        ftp_status = STA_FTP_EXP;

        return FAILURE;
    }

    return SUCCESS;
}


/**
 *    功能: 上传内存数据到ftp服务器,保存为文件.
 *    参数:
 *      dir: ftp服务器上的路径
 *      name: 文件名
 *      buf: 数据地址
 *      size: 数据字节数
 *  返回值: FAILURE -- 失败; 0-- 成功; SOCKET_ERR --发送错误
 */
int CLASS_FTP::ftp_put_data(const char dir[][80], const char *filename,
        char *buf, long size)
{
    char ftp_rcv_buf[200];
    int i, ret;
    char cmd_dir[256] = "/\0";

    {
        static int count = 0;
        debug("have upload %d files.\n", ++count);
    }

    if (strlen(filename) == 0 || buf == NULL || size == 0)
    {
        debug_ftp("%s %s param error!\n", descript, __func__);
        return FAILURE;
    }
    if (ftp_status != STA_FTP_OK)
    {
        debug_ftp("%s ftp_put_file() 连接异常！\n", descript);
        return FAILURE;
    }

    debug_ftp("%s %s start ....\n", descript, __func__);
    clear_socket_recvbuf(this->fd_ctl_socket);
    //###########/创建目录/#################//
    if (SUCCESS != ftp_mk_dir(dir))
    {
        return FAILURE;
    }

    //##### 进入被动传输模式 ########//
    if (SUCCESS != ftp_set_trans_pasv_mode())
    {
        return FAILURE;
    }

    //####### 创建数据连接端口 ########//
    if (SOCKET_ERR == ftp_mk_data_socket())
    {
        log_error_ftp("[%s] create data socket fail !\n", descript);
        return FAILURE;
    }

    //############/存储数据/###############//
    for (i = 0; i < MAX_DIR_LEVEL; i++)
    {
        if (strlen(dir[i]) != 0)
        {
            strcat(cmd_dir, dir[i]);
            strcat(cmd_dir, "/");
        }
        else
        {
            break;
        }
    }

    if (SUCCESS != ftp_store(cmd_dir, filename))
    {
        debug_ftp("%s store cmd false! \n", descript);
        ftp_close_data_socket();

        return FAILURE;
    }

    //开始传送数据
    mutex_guard guard(mutex);
    ret = ftp_send_data(buf, size);
    ftp_close_data_socket();
    if (SUCCESS != ret)
    {
        return FAILURE;
    }

    //数据发送完毕正常
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("close data socket, and recv failed\n");
        return FAILURE;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_rcv_buf);

    if (strncasecmp(ftp_rcv_buf, "226", 3) == 0) //收到226表示成功;
    {
        debug_ftp("%s ftp send file success!	\n", descript);
    }
    else if (!strncasecmp(ftp_rcv_buf, "452", 3)) //452	磁盘空间不足
    {
        debug_ftp("%s ftp disk full!\n", descript);
        server_is_full = true;

        return FAILURE;
    }
    else
    {
        debug_ftp("%s send buffer to ftp, failed..ftp server abnormal!\n", descript);
        ftp_status = STA_FTP_EXP;

        return FAILURE;
    }

    return SUCCESS;
}


/*******************************************************************************
 * 函数名: ftp_put_data
 * 功  能: 上传内存数据到FTP服务器，并保存为文件
 * 参  数: dest_path，上传到FTP的目标路径
 *         dest_name，上传到FTP的目标文件名
 *         buf_ptr，数据指针
 *         buf_len，数据长度
 * 返回值: 成功，返回SUCCESS；出错，返回FAILURE
 *******************************************************************************/
int CLASS_FTP::ftp_put_data(const char *dest_path, const char *dest_name,
        const void *buf_ptr, size_t buf_len)
{
    if ((dest_path == NULL) || (dest_name == NULL) || (buf_ptr == NULL))
    {
        log_error_ftp("%s parameters can't be NULL !\n", __func__);
        return FAILURE;
    }

    if ((strlen(dest_path) == 0) || (strlen(dest_name) == 0) || (buf_len == 0))
    {
        log_error_ftp("%s parameters can't be zero !\n", __func__);
        return FAILURE;
    }

    if (ftp_status != STA_FTP_OK)
    {
        log_debug_ftp("%s %s 连接异常！\n", descript, __func__);
        return FAILURE;
    }

    debug_ftp("%s %s start ...\n", descript, __func__);

    if (SUCCESS != ftp_mk_path(dest_path)) 	/* 创建目录 */
    {
        return FAILURE;
    }

    if (SUCCESS != ftp_set_trans_pasv_mode()) 	/* 进入被动传输模式 */
    {
        return FAILURE;
    }

    if (SOCKET_ERR == ftp_mk_data_socket()) 	/* 创建数据传输接口 */
    {
        log_error_ftp("[%s] create data socket fail !\n", descript);
        return FAILURE;
    }

    if (SUCCESS != ftp_store(dest_path, dest_name)) 	/* 存储数据 */
    {
        debug_ftp("%s store cmd false! \n", descript);
        ftp_close_data_socket();
        return FAILURE;
    }

    mutex_guard guard(mutex);
    int ret = ftp_send_data(buf_ptr, buf_len); 	/* 发送数据 */
    ftp_close_data_socket();
    if (SUCCESS != ret)
    {
        return FAILURE;
    }

    char ftp_rcv_buf[256];
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("close data socket, and recv failed\n");
        return FAILURE;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_rcv_buf);

    if (strncasecmp(ftp_rcv_buf, "226", 3) == 0) //收到226表示成功;
    {
        debug_ftp("%s ftp send file success!	\n", descript);
    }
    else if (!strncasecmp(ftp_rcv_buf, "452", 3)) //452	磁盘空间不足
    {
        debug_ftp("%s ftp disk full!\n", descript);
        server_is_full = true;

        return FAILURE;
    }
    else
    {
        debug_ftp("%s send buffer to FTP failed. Receive %s !\n",
                descript, ftp_rcv_buf);
        ftp_status = STA_FTP_EXP;

        return FAILURE;
    }

    return SUCCESS;
}


/**
 *    功能: 设置 传输类型为Binary.
 *    参数: 无
 *  返回值: FAILURE -- 失败; 0-- 成功; SOCKET_ERR --发送错误
 */
int CLASS_FTP::ftp_set_trans_type_I()
{
    char cmd[48] = "TYPE I\r\n";
    char ftp_rcv_buf[200];

    debug_ftp (">>> in(%s|%d)\n", __func__, __LINE__);

    mutex_guard guard(mutex);
    debug_ftp ("got mutex ok\n");

    if (ftp_send_cmd(cmd, strlen(cmd)) != (int) strlen(cmd))
    {
        return FAILURE;
    }

    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        return FAILURE;
    }
    debug_ftp ("Server return: %s\n", ftp_rcv_buf);

    //200 Switching to Binary mode.
    //500--无效指令   501--错误参数  502--没执行  421--服务关闭  530--未登录  504--无效命令参数
    if (0 != strncmp(ftp_rcv_buf, "200", 3))
    {
        return FAILURE;
    }

    return SUCCESS;
}

/**
 *    功能: 创建控制fd_socket,设置socket的属性, 并且连接服务器.
 *    参数: 无
 *  返回值: -1 -- 失败; 0-- 成功.
 */
int CLASS_FTP::ftp_mk_ctl_socket()
{
    struct sockaddr_in server_addr;
    char ftp_cmd_buf[200];
    struct timeval time_out;
    char tmp[20];
    time_out.tv_sec = 3;
    time_out.tv_usec = 0;

    fd_ctl_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ctl_socket < 0)
    {
        //debug_ftp("%s Not create network socket connection\n", descript);
        return SOCKET_ERR;
    }

    //################# 设置socket的属性 ########################//

    if (SOCKET_ERR == setsockopt(fd_ctl_socket, SOL_SOCKET, SO_SNDTIMEO,
                (char*) (&time_out), sizeof(struct timeval)))
    {
        debug_ftp("%s set ftp socket timeout setsockopt(SO_SNDTIMEO)  error!\n",
                descript);
        ftp_close_ctl_socket();
        return SOCKET_ERR;;
    }
    if (SOCKET_ERR == setsockopt(fd_ctl_socket, SOL_SOCKET, SO_RCVTIMEO,
                (char*) (&time_out), sizeof(struct timeval)))
    {
        debug_ftp("set ftp socket timeout setsockopt(SO_RCVTIMEO)  error!\n" );
        ftp_close_ctl_socket();
        return SOCKET_ERR;
    }

    if (SOCKET_ERR == fcntl(fd_ctl_socket, F_SETFL, O_APPEND)) //设置socket端口超时非阻塞
    {
        ftp_close_ctl_socket();
        return SOCKET_ERR;
    }

    ////////////////// 连接服务器   //////////////////////////
    sprintf(tmp, "%d.%d.%d.%d", config.ip[0], config.ip[1], config.ip[2],
            config.ip[3]);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.port);
    server_addr.sin_addr.s_addr = inet_addr(tmp);

    debug ("[%s] ip: %s, port:%d \n", descript, tmp, config.port);
    int connect_succ_data = connect(fd_ctl_socket,
            (struct sockaddr*) (&server_addr), sizeof(struct sockaddr_in));
    if (connect_succ_data < 0)
    {
        connect_succ_data = connect(fd_ctl_socket,
                (struct sockaddr*) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_succ_data < 0) {
            debug_ftp("[%s] ftp_socket=%d connect fail!\n", descript, fd_ctl_socket);
            ftp_close_ctl_socket();
            return SOCKET_ERR;
        }
    }

    if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf, sizeof(ftp_cmd_buf)))
    {
        ftp_close_ctl_socket();
        return SOCKET_ERR;
    }

    debug_ftp("%s return\"%s\" \n", descript, ftp_cmd_buf);
    return SUCCESS;
}

/**
 *   功能: 获取ftp当前状态.
 * 返回值: 返回当前ftp连接状态.
 */
int CLASS_FTP::get_status()
{
    return this->ftp_status;
}

void CLASS_FTP::set_status(int sta, bool set)
{
    if (set)
    {
        this->ftp_status |= sta;
    }
    else
    {
        this->ftp_status &= sta;
    }
}

void CLASS_FTP::set_status(int sta)
{
    this->ftp_status = sta;
}
/*
 *   功能: 获取设置的ftp参数(user,pass, ip, port)
 *   参数: 无
 * 返回值: 返回设置的参数指针.
 */
TYPE_FTP_CONFIG_PARAM *CLASS_FTP::get_config()
{
    return &config;
}

/*
 *   功能: 设置ftp参数(user,pass, ip, port)
 *   参数: @[in]param:设置的参数指针
 * 返回值: NULL-失败, 成功返回设置的参数指针.
 */
TYPE_FTP_CONFIG_PARAM* CLASS_FTP::set_config(TYPE_FTP_CONFIG_PARAM *param)
{

    if (param != NULL)
    {
        debug_ftp("set ftp ip: %d.%d.%d.%d, port:%d, user:%s\n",
                param->ip[0],param->ip[1],param->ip[2],param->ip[3],
                param->port,param->user);

        //比较， 除了匿名设置
        if (memcmp(&config, param, sizeof(config) - sizeof(BOOL)))
        {
            set_status(STA_FTP_SET_CHANGED);
        }

        memcpy((void*) &config, (const void*) param,
                sizeof(TYPE_FTP_CONFIG_PARAM));
        return &config;
    }
    return NULL;
}

/*
 *   功能: 登录ftp服务器
 *   参数: 无
 * 返回值: -1: socket error; -2 --fail, 0--success
 */
int CLASS_FTP::ftp_login()
{
    char cmd_buf[128];
    char mode[] = "MODE S\r\n"; 	//设置ftp传输模式为流模式
    char ftp_rcv_buf[200];
    int login_ok = 0;
    int cmd_len = 0;

    log_state_ftp("%s FTP login.\n", descript);

    do 	//do't remove the brace. because of mutex.
    {
        mutex_guard guard(mutex);

        cmd_len = snprintf(cmd_buf, sizeof(cmd_buf),
                "USER %s\r\n", config.user); 	//用户名
        if (ftp_send_cmd(cmd_buf, cmd_len) != cmd_len) 	//发送用户登录名称
        {
            log_warn_ftp("FTP login : send user name to FTP server failed !\n");
            return SOCKET_ERR;
        }

        if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
        {
            log_warn_ftp("FTP login : receive FTP server response failed !\n");
            return SOCKET_ERR;
        }

        if (strstr(ftp_rcv_buf, FTP_LOGGED_ON) != 0)
        {
            login_ok = 1;
        }
        else if (strstr(ftp_rcv_buf, FTP_PASSWORD_REQUIRED) == 0)//判断确认消息，如果无该用户则返回
        {
            debug("3");
            return FAILURE;
        }
        debug_ftp("%s return : %s\n", descript, ftp_rcv_buf);

        if (0 == login_ok)
        {
            sprintf(cmd_buf, "PASS %s\r\n", config.passwd); //密码
            if (ftp_send_cmd(cmd_buf, strlen(cmd_buf)) != (int)strlen(cmd_buf))
            {
                return SOCKET_ERR;
            }
            //memset(ftp_rcv_buf, 0, sizeof(ftp_rcv_buf));
            if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf,
                        sizeof(ftp_rcv_buf)))
            {
                return SOCKET_ERR;
            }
            if (strstr(ftp_rcv_buf, FTP_LOGGED_ON) == 0)//确认登录密码是否正确
            {
                return FAILURE;
            }
        }
    } 	//do not remove this brace.
    while (0);

    //TYPE I
    if (SUCCESS != ftp_set_trans_type_I())
    {
        return FAILURE;
    }

    //MODE
    if (ftp_send_cmd(mode, strlen(mode)) != (int)strlen(mode))
    {
        return SOCKET_ERR;
    }
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        return SOCKET_ERR;
    }
    debug_ftp("%s return\"%s\" \n", descript, ftp_rcv_buf);
    return SUCCESS;
}


/*
 *   功能: 登出ftp服务器, server关闭连接
 *   参数: 无
 * 返回值: -1: socket error; -2 --fail, 0--success
 */
int CLASS_FTP::ftp_logout()
{
    char cmd_buf[128];
    const char *logout_ok = "221";
    char ftp_rcv_buf[200];

    sprintf(ftp_rcv_buf, "[%s] logout.\n", descript);
    log_state_ftp(ftp_rcv_buf);

    mutex_guard guard(mutex);

    sprintf(cmd_buf, "QUIT\r\n");
    if (ftp_send_cmd(cmd_buf, strlen(cmd_buf)) != (int)strlen(cmd_buf)) //发送用户登录名称
    {
        return SOCKET_ERR;
    }

    //221 Goodbye.
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("error when recv.\n");
        set_status(STA_FTP_FAIL, true);

        return SOCKET_ERR;
    }
    if (strstr(ftp_rcv_buf, logout_ok) != 0)
    {
        debug_ftp("logout not ok.\n");
        set_status(STA_FTP_FAIL, true);

        return FAILURE;
    }
    debug_ftp("%s return\"%s\" \n", descript, ftp_rcv_buf);

    debug_ftp("logout ok.\n");
    set_status(STA_FTP_FAIL, true);

    return SUCCESS;
}

/**
 * 检测服务器是否容量满 (容量少于5G,认为满)
 *
 * 返回值: 0 -- 未满, 1-- 满, -1 -- socket error
 */
int CLASS_FTP::ftp_check_server_full()
{
    char ALLO[] = "ALLO 5000000\r\n";
    char ftp_rcv_buf[200];

    mutex_guard guard(mutex);
    if (ftp_send_cmd(ALLO, strlen(ALLO)) != (int)strlen(ALLO)) //判断ftp可使用空间，如果小于5G，则认为满
    {
        debug_ftp("send error!\n");
        return SOCKET_ERR;
    }
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("recv error!\n");
        return SOCKET_ERR;
    }
    if (!strncasecmp(ftp_rcv_buf, "200", 3) || !strncasecmp(ftp_rcv_buf, "202",
                3))
    {
        debug_ftp("not full!\n");
        set_server_full(false);
        return 0;
    }

    debug_ftp("full!\n");
    //如果ftp满，则返回 1
    set_server_full(true);
    return 1;
}


/*******************************************************************************
 * 函数名: ftp_send_cmd
 * 功  能: 向FTP服务器发送指令
 * 参  数: cmd，指令；len，长度
 * 返回值: 成功，返回SUCCESS；失败，返回FAILURE
 *******************************************************************************/
int CLASS_FTP::ftp_send_cmd(char *cmd, int len)
{
    if (fd_ctl_socket <= 0)
    {
        log_state_ftp("%s ctl_socket is closed !\n", descript);
        return -1;
    }

    clear_socket_recvbuf(fd_ctl_socket);

    debug_ftp("Send command: %s\n", cmd);

    int send_bytes = send(fd_ctl_socket, cmd, len, 0);
    if (send_bytes != len)
    {
        debug_ftp("error when send %s, send %d bytes, %m\n", cmd, send_bytes);
        set_status(STA_FTP_FAIL, true);
    }

    return send_bytes;
}


/**
 *    功能: 从ftp服务器下载文件.
 *    参数:
 *      ftp_path_file: ftp服务器上全路径文件
 *      save_path_file:本地全路径文件名
 *  返回值: FAILURE -- 失败; SUCCESS-- 成功;
 */
int CLASS_FTP::ftp_get_file(char *ftp_path_file, const char *save_path_file)
{
    int rev_bytes;
    char *p_buffer;
    FILE * file;
    int file_size = 0;
    char ftp_rcv_buf[200];

    debug_ftp("%s  >>>in %s ....\n", descript, __func__);
    //	if (get_status() != STA_FTP_OK)
    //	{
    //		return FAILURE;
    //	}

    //##### 进入被动传输模式 ########//
    if (SUCCESS != ftp_set_trans_pasv_mode())
    {
        return FAILURE;
    }

    //#########/创建数据连接端口/#######################//
    if (SOCKET_ERR == ftp_mk_data_socket())
    {
        return FAILURE;
    }
    debug_ftp("%s create ftp data socket success! port:%d\n", descript, data_port);

    //###########/   下载数据   /##############//
    file = fopen(save_path_file, "wb");
    if (file == NULL)
    {
        debug_ftp("%s open file %s false!\n", descript, save_path_file);
        return FAILURE;
    }
    else
    {
        debug_ftp("%s open file %s success!\n", descript, save_path_file);
    }

    //send RETR command.
    if (SUCCESS != ftp_retr(ftp_path_file))
    {
        fclose(file);
        ftp_close_data_socket();

        debug_ftp("%s retr cmd false!\n", descript);
        return FAILURE;
    }

    mutex_guard guard(mutex);
    //memset(ftp_cmd_buf, 0, sizeof(ftp_cmd_buf));
    //	if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf, sizeof(ftp_cmd_buf)))
    //	{
    //		debug_ftp("the file to ftp is failed:%s\n", ftp_cmd_buf);
    //
    //		fclose(file);
    //		ftp_close_data_socket();
    //		return FAILURE;
    //	}
    //	debug_ftp("%s server>>:%s \n", descript, ftp_cmd_buf);

    //开始传送数据
    debug_ftp("%s start receive file ...\n", descript);
    //#########################################//#########################################//
    p_buffer = (char *) malloc(MAX_SIZE_SEND);
    if (NULL == p_buffer)
    {
        debug_ftp("malloc failed!\n");

        fclose(file);
        ftp_close_data_socket();
        return FAILURE;
    }

    do
    {
        rev_bytes = recv(fd_data_socket, p_buffer, MAX_SIZE_SEND, 0);
        if (rev_bytes <= 0)
        {
            break;
        }
        debug_ftp("%s receive data packet:%d,  size:%d ...\n", descript, ++index, rev_bytes);

        fwrite(p_buffer, rev_bytes, 1, file);
        fflush(file);
        file_size += rev_bytes;
        usleep(100);
    }
    while (rev_bytes != 0);

    free(p_buffer);
    fclose(file);
    ftp_close_data_socket(); //close socket first , then recv "226 File sent ok"

    debug_ftp("%s download [%s] success, file size:%d \n", descript, save_path_file,
            file_size);

    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("recv failed\n");
        ftp_status = STA_FTP_FAIL;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_rcv_buf);

    return SUCCESS;
}

/**
 * 设置服务器PASV模式
 *
 * 返回值: SUCCESS -- 成功, FAILURE-- 失败, -1 -- socket error
 */
int CLASS_FTP::ftp_set_trans_pasv_mode()
{
    char cmd[48] = "PASV\r\n";
    char ftp_rcv_buf[200];
    int port_hi, port_lo; //数据端口高地位

    mutex_guard guard(mutex);
    if (ftp_send_cmd(cmd, strlen(cmd)) == -1)
    {
        return SOCKET_ERR;
    }

    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        return SOCKET_ERR;
    }
    //debug_ftp ("Server return: %s\n", ftp_rcv_buf);

    //227 进入被动模式(IP地址、端口)
    //227 Entering Passive Mode (172,16,5,146,11,153)
    //回应: 500--无效指令   501--错误参数  502--没执行  421--服务关闭  530--未登录
    if (0 != strncmp(ftp_rcv_buf, "227", 3))
    {
        return FAILURE;
    }

    sscanf(ftp_rcv_buf, "%*[^(](%*d,%*d,%*d,%*d,%d,%d%*[^)]", &port_hi,
            &port_lo);
    data_port = port_hi * 256 + port_lo;

    return SUCCESS;
}

/**
 * 设置数据传输socket,并连接
 *
 * 返回值: 成功 返回socket,  失败返回 -1
 */
int CLASS_FTP::ftp_mk_data_socket()
{
    struct sockaddr_in server_addr;
    char tmp[20];
    struct timeval time_out =
    { 5, 0 };
    int i;
    int sta;

    //debug_ftp("in >>> %s\n", __func__);

    fd_data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_data_socket < 0)
    {
        debug_ftp("Create network socket connection error\n");
        return fd_data_socket;
    }
    setsockopt(fd_data_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &time_out,
            sizeof(struct timeval));
    setsockopt(fd_data_socket, SOL_SOCKET, SO_SNDTIMEO, (char *) &time_out,
            sizeof(struct timeval));

    sprintf(tmp, "%d.%d.%d.%d", config.ip[0], config.ip[1], config.ip[2],
            config.ip[3]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr.s_addr = inet_addr(tmp);

    for (i = 0; i < 3; i++) //尝试3次;
    {
        sta = connect(fd_data_socket, (struct sockaddr*) &server_addr,
                sizeof(struct sockaddr_in));

        if (sta >= 0)
        {
            break;
        }
        else
        {
            debug_ftp("%s connect data socket fail!\n", descript);
            usleep(10000);
        }
    }

    if (sta < 0) //连接失败;
    {
        ftp_close_data_socket();
        return SOCKET_ERR;
    }

    //debug_ftp("out <<< %s, data socket port %d\n", __func__, data_port);
    return fd_data_socket;
}

/**
 * 发送RETR指令
 * 参数:
 * 		filename --- 文件路径和名称
 * 返回值: 成功 返回SUCCESS,  失败返回FAIL
 */
int CLASS_FTP::ftp_retr(char* filename)
{
    char cmd[256];
    char ftp_cmd_buf[256];
    int send_bytes;

    debug_ftp("in >>> %s\n", __func__);

    mutex_guard guard(mutex);

    sprintf(cmd, "RETR %s\r\n", filename);

    //#ifndef USE_UTF8
    send_bytes = ftp_send_cmd(cmd, strlen(cmd));
    if (send_bytes != (int) (strlen(cmd)))
    {
        //return send_bytes;
    }
    //#else
    //	memset(ftp_cmd_buf, 0, sizeof(ftp_cmd_buf));
    //	convert_enc((char*) "GBK", (char*) "UTF-8", cmd, strlen(cmd), ftp_cmd_buf,
    //			sizeof(ftp_cmd_buf));
    //
    //	send_bytes = ftp_send_cmd(ftp_cmd_buf, strlen(ftp_cmd_buf));
    //	if (send_bytes != (int) (strlen(ftp_cmd_buf)))
    //	{
    //		//return send_bytes;
    //	}
    //#endif

    if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf, sizeof(ftp_cmd_buf)))
    {
        return FAILURE;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_cmd_buf);

    if ((strncmp(ftp_cmd_buf, "125", 3) && strncmp(ftp_cmd_buf, "150", 3)
                && strncmp(ftp_cmd_buf, "110", 3)))
    {
        return FAILURE;
    }

    debug_ftp("out >>> %s\n", __func__);
    return SUCCESS;
}

/**
 * 发送DELE指令
 * 参数:
 * 		filename --- 文件路径和名称
 * 返回值: 成功 返回SUCCESS,  失败返回FAIL
 */
int CLASS_FTP::ftp_dele(char* filename)
{
    char cmd[256];
    char ftp_cmd_buf[256];
    int send_bytes;

    debug_ftp("in >>> %s\n", __func__);

    mutex_guard guard(mutex);
    sprintf(cmd, "DELE %s\r\n", filename);
    //#ifndef USE_UTF8
    send_bytes = ftp_send_cmd(cmd, strlen(cmd));
    if (send_bytes != (int) (strlen(cmd)))
    {
        return FAILURE;
    }
    //#else
    //	memset(ftp_cmd_buf, 0, sizeof(ftp_cmd_buf));
    //	convert_enc((char*) "GBK", (char*) "UTF-8", cmd, strlen(cmd), ftp_cmd_buf,
    //			sizeof(ftp_cmd_buf));
    //
    //	send_bytes = ftp_send_cmd(ftp_cmd_buf, strlen(ftp_cmd_buf));
    //	if (send_bytes != (int) (strlen(ftp_cmd_buf)))
    //	{
    //		//return send_bytes;
    //	}
    //#endif

    if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf, sizeof(ftp_cmd_buf)))
    {
        return FAILURE;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_cmd_buf);

    if ((strncmp(ftp_cmd_buf, "125", 3) && strncmp(ftp_cmd_buf, "150", 3)
                && strncmp(ftp_cmd_buf, "110", 3)))
    {
        return FAILURE;
    }

    debug_ftp("out >>> %s\n", __func__);
    return SUCCESS;
}

/**
 * 发送STOR指令
 * 参数:
 * 		dir --- 以根路径为开始的绝对路径
 * 		filename --- 文件名称
 * 返回值: 成功 返回SUCCESS,  失败返回FAIL
 */
int CLASS_FTP::ftp_store(const char* dir, const char* filename)
{
    char cmd[256];
    char ftp_cmd_buf[200];
    int send_bytes;

    if (SUCCESS != ftp_cd_dir(dir))
    {
        debug_ftp("fail to enter dir %s\n", dir);
        return FAILURE;
    }

    mutex_guard guard(mutex);

    sprintf(cmd, "STOR %s\r\n", filename);
    //#ifndef USE_UTF8
    send_bytes = ftp_send_cmd(cmd, strlen(cmd));
    if (send_bytes != (int) (strlen(cmd)))
    {
        debug_ftp("error when send :(%s), return:%d \n", cmd, send_bytes);
        return FAILURE;
    }
    //#else
    //	memset(ftp_cmd_buf, 0, sizeof(ftp_cmd_buf));
    //	convert_enc((char*) "GBK", (char*) "UTF-8", cmd, strlen(cmd), ftp_cmd_buf,
    //			sizeof(ftp_cmd_buf));
    //
    //	send_bytes = ftp_send_cmd(ftp_cmd_buf, strlen(ftp_cmd_buf));
    //	if (send_bytes != (int) (strlen(ftp_cmd_buf)))
    //	{
    //		//return send_bytes;
    //	}
    //#endif

    if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf, sizeof(ftp_cmd_buf)))
    {
        return FAILURE;
    }
    //debug_ftp("%s server>>:%s \n", descript, ftp_cmd_buf);
    //	if ((!strncasecmp(ftp_cmd_buf, "125", 3)) || (!strncasecmp(ftp_cmd_buf, "150", 3)))


    if ((strncmp(ftp_cmd_buf, "125", 3) && strncmp(ftp_cmd_buf, "150", 3)
                && strncmp(ftp_cmd_buf, "110", 3)))
    {
        return FAILURE;
    }

    return SUCCESS;
}
/**
 * 显示当前工作目录 PWD
 * 参数:
 *
 * 返回值: 成功 返回SUCCESS,  失败返回FAIL
 */
int CLASS_FTP::ftp_pwd()
{
    char cmd[256];
    char ftp_cmd_buf[200];
    int send_bytes;

    mutex_guard guard(mutex);

    sprintf(cmd, "PWD\r\n");
    send_bytes = ftp_send_cmd(cmd, strlen(cmd));
    if (send_bytes != (int) (strlen(cmd)))
    {
        debug_ftp("error when send :(%s), return:%d \n", cmd, send_bytes);
        return FAILURE;
    }

    if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf, sizeof(ftp_cmd_buf)))
    {
        return FAILURE;
    }
    //debug_ftp("%s:%s\n", descript, ftp_cmd_buf);
    //	if (strncmp(ftp_cmd_buf, "250", 3))
    //	{
    //		return FAILURE;
    //	}

    return SUCCESS;
}

/**
 * 改变当前工作路径 CWD
 * 参数:
 * 		dir --- 以根路径为开始的绝对路径
 * 返回值: 成功 返回SUCCESS,  失败返回FAIL
 */
int CLASS_FTP::ftp_cd_dir(const char* dir)
{
    char cmd[256];
    char ftp_cmd_buf[256];
    int send_bytes;

    //ftp_pwd();

    mutex_guard guard(mutex);

    sprintf(cmd, "CWD %s\r\n", dir);
#ifndef USE_UTF8
    send_bytes = ftp_send_cmd(cmd, strlen(cmd));
    if (send_bytes != (int) (strlen(cmd)))
    {
        debug("send cmd %s , but send %d bytes, less than %d\n",
                cmd, send_bytes, strlen(cmd));
        return FAILURE;
    }
#else
    memset(ftp_cmd_buf, 0, sizeof(ftp_cmd_buf));
    convert_enc("GBK", "UTF-8", cmd, strlen(cmd),
            ftp_cmd_buf, sizeof(ftp_cmd_buf));

    send_bytes = ftp_send_cmd(ftp_cmd_buf, strlen(ftp_cmd_buf));
    if (send_bytes != (int) (strlen(ftp_cmd_buf)))
    {
        debug("send cmd %s , but send %d bytes, less than %d\n",
                ftp_cmd_buf, send_bytes, strlen(ftp_cmd_buf));
        return FAILURE;
    }
#endif

    if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf, sizeof(ftp_cmd_buf)))
    {
        debug("error when recv.\n");
        return FAILURE;
    }

    if (strncmp(ftp_cmd_buf, "250", 3) != 0)
    {
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * 在Home'~'建立路径, 最多5层
 *
 * 返回值: 成功 返回SUCCESS,  失败返回FAIL
 */
int CLASS_FTP::ftp_mk_dir(const char dir[][80])
{
    char cmd[256];
    int send_bytes, i;
    char cmd_dir[256];

    char ftp_cmd_buf[1024];

    //debug_ftp("%s in %s ....\n", descript, __func__);

    memset(cmd_dir, 0, sizeof(cmd_dir));

    cmd_dir[0] = '/';
    this->ftp_cd_dir("/");

    mutex_guard guard(mutex);

    for (i = 0; i < MAX_DIR_LEVEL; i++)
    {
        if (strlen(dir[i]) != 0)
        {
            strcat(cmd_dir, dir[i]);
            strcat(cmd_dir, "/");

            sprintf(cmd, "MKD %s\r\n", cmd_dir);

            //debug_ftp("%s client>>: %s \n", descript, cmd);
#ifndef USE_UTF8
            send_bytes = ftp_send_cmd(cmd, strlen(cmd));
            if (send_bytes != (int) (strlen(cmd)))
            {
                return FAILURE;
            }
#else
            memset(ftp_cmd_buf, 0, sizeof(ftp_cmd_buf));
            convert_enc((char*) "GBK", (char*) "UTF-8", cmd, strlen(cmd),
                    ftp_cmd_buf, sizeof(ftp_cmd_buf));

            send_bytes = ftp_send_cmd(ftp_cmd_buf, strlen(ftp_cmd_buf));
            if (send_bytes != (int) (strlen(ftp_cmd_buf)))
            {
                debug_ftp("[%s] error when send cmd : %s , len=%d\n", descript, cmd, send_bytes);
                return FAILURE;
            }
#endif

            if (SUCCESS != ftp_recv_cmd_response(ftp_cmd_buf,
                        sizeof(ftp_cmd_buf)))
            {
                return FAILURE;
            }
            //debug_ftp("%s server>>: [[ %s ]] \n", descript, ftp_cmd_buf);
        }
        else
        {
            break;
        }
    }

    return SUCCESS;
}


/*******************************************************************************
 * 函数名: ftp_mk_path
 * 功  能: 在FTP服务器创建目录
 * 参  数: path，目录路径
 * 返回值: 成功，返回SUCCESS；出错，返回FAILURE
 *******************************************************************************/
int CLASS_FTP::ftp_mk_path(const char *path)
{
    if (path == NULL)
    {
        log_warn_ftp("%s path can not be NULL !\n", __func__);
        return FAILURE;
    }

    size_t path_len = strlen(path);

    char *tmp_path = (char *)malloc(path_len+1);
    if (tmp_path == NULL)
    {
        log_error_ftp("Duplicate string %s failed !\n", path);
        return FAILURE;
    }

    memcpy(tmp_path, path, path_len);
    tmp_path[path_len] = '\0';

    unsigned int i = path_len;
    while (i > 0)
    {
        if (ftp_cd_dir(tmp_path) == SUCCESS)
        {
            break;
        }

        while (i > 0)
        {
            if (tmp_path[i] == '/')
            {
                tmp_path[i] = '\0';
                break;
            }
            i--;
        }
    }

    if (i == 0)
    {
        if ((ftp_mkd(tmp_path) == FAILURE) || (ftp_cd_dir(tmp_path) == FAILURE))
        {
            free(tmp_path);
            return FAILURE;
        }
    }

    while (i < path_len-1)
    {
        if (tmp_path[i] == '\0')
        {
            if ((ftp_mkd(tmp_path+i+1) == FAILURE) ||
                    (ftp_cd_dir(tmp_path+i+1) == FAILURE))
            {
                free(tmp_path);
                return FAILURE;
            }
        }
        i++;
    }

    free(tmp_path);

    return SUCCESS;
}


/*******************************************************************************
 * 函数名: ftp_mkd
 * 功  能: MKD 在FTP服务器创建单级目录
 * 参  数: dir，目录名称
 * 返回值: 成功，返回0；出错，返回-1
 *******************************************************************************/
int CLASS_FTP::ftp_mkd(const char *dir)
{
    int ret = FAILURE;

    size_t dir_len = strlen(dir);
    if (dir_len == 0)
    {
        log_warn_ftp("%s dir strlen is 0 !", __func__);
        return ret;
    }

    char *dir_cmd = NULL;
    size_t buf_len = 0;
    char *buf = NULL;

#ifndef USE_UTF8
    dir_cmd = (char *)dir;
    buf_len = dir_len + 64;
#else
    dir_cmd = (char *)malloc(dir_len*2);
    if (dir_cmd == NULL)
    {
        log_error_ftp("%s malloc for dir_cmd failed !\n", __func__);
        return ret;
    }

    convert_enc("GBK", "UTF-8", dir, dir_len, dir_cmd, dir_len*2);

    buf_len = dir_len * 2 + 64;
#endif

    do
    {
        buf = (char *)malloc(buf_len);
        if (buf == NULL)
        {
            log_error_ftp("%s malloc for buf failed !\n", __func__);
            break;
        }

        int cmd_len = sprintf(buf, "MKD %s\r\n", dir_cmd);

        int send_bytes = ftp_send_cmd(buf, cmd_len);
        if (send_bytes != cmd_len)
        {
            log_warn_ftp("%s send %s, return %d bytes.\n",
                    __func__, buf, send_bytes);
            break;
        }

        if (ftp_recv_cmd_response(buf, buf_len) != SUCCESS)
        {
            log_warn_ftp("%s receive response failed !\n", __func__);
            break;
        }

        if (strncmp(buf, "257", 3) != 0)
        {
            log_warn_ftp("Reeive response: %s\n", buf);
            break;
        }

        ret = SUCCESS;
    }
    while (0);

#ifdef USE_UTF8
    free(dir_cmd);
#endif

    free(buf);

    return ret;
}


void CLASS_FTP::ftp_close_data_socket()
{
    debug_ftp("close data socket.\n");
    close(fd_data_socket);
}

void CLASS_FTP::ftp_close_ctl_socket()
{
    debug_ftp("close ctl socket.\n");
    close(fd_ctl_socket);
    fd_ctl_socket = -1;
}


int CLASS_FTP::ftp_recv_cmd_response(char *buf, int len)
{
    int ret = FAILURE;

    char *response = (char *)malloc(len);
    if (response == NULL)
    {
        log_error_ftp("%s malloc %d bytes for response failed !\n",
                __func__, len);
        return ret;
    }

    do
    {
        memset(response, 0, len);

        ssize_t recv_bytes = recv(fd_ctl_socket, response, len, 0);
        if (recv_bytes <= 0)
        {
            debug_ftp("recv error!\n");
            set_status(STA_FTP_FAIL, true);
            break;
        }

#ifndef USE_UTF8
        memcpy(buf, response, recv_bytes);
        buf[len] = '\0';
#else
        convert_enc("UTF-8", "GBK", response, recv_bytes, buf, len);
#endif

        ret = SUCCESS;
        log_debug_ftp("%s %s server>>: %s recv_bytes: %d.\n",
                __func__, descript, buf, recv_bytes);
    }
    while (0);

    free(response);

    return ret;
}


bool CLASS_FTP::getServer_is_full() const
{
    return server_is_full;
}

void CLASS_FTP::set_server_full(bool server_is_full)
{
    this->server_is_full = server_is_full;
}

bool CLASS_FTP::getThread_stop() const
{
    return thread_stop;
}

void CLASS_FTP::setThread_stop(bool thread_stop)
{
    this->thread_stop = thread_stop;
}

/*
 * 功能: 获取ftp实例描述
 */
int CLASS_FTP::getDescript(char *buf)
{
    if (NULL == buf)
    {
        return FAILURE;
    }
    strcpy(buf, descript);
    return SUCCESS;
}

/*
 * 功能:设置ftp实例描述
 */
void CLASS_FTP::setDescript(char* descript)
{
    if (NULL != descript)
    {
        strcpy(this->descript, descript);
    }
}

/***
 * 功能: 设置告警回调函数
 * 参数:
 *     alarmFunc
 */
void CLASS_FTP::setAlarmFxn(lpFun alarmFunc)
{
    this->alarmFunc = alarmFunc;
}

/**
 * 功能:获取告警回调函数
 */
lpFun CLASS_FTP::getAlarmFxn() const
{
    return alarmFunc;
}

/*
 * 功能: 获取回调函数第一个参数
 */
void *CLASS_FTP::getCallerParam() const
{
    return pCallerParam;
}

/**
 * 功能:设置回调函数第一个参数
 */
void CLASS_FTP::setCallerParam(void *pCallerParam)
{
    this->pCallerParam = pCallerParam;
}

/**
 * 向数据socket发送数据
 * 参数:
 * 		buf --- 数据指针
 *      size--- 数据字节数
 * 返回值: 成功 返回SUCCESS,  失败返回FAILURE
 */
int CLASS_FTP::ftp_send_data(const void *buf, long size)
{
    int send_bytes, byte_end;
    char* p_buffer = (char *)buf;
    long i;

    //######## 传送数据，分多次传送  ###############//

    for (i = 0; i < (size / MAX_SIZE_SEND); i++)
    {
        debug_ftp("%s send packet: %ld .... \n", descript, i);

        send_bytes =
            send(fd_data_socket, p_buffer + i * MAX_SIZE_SEND, MAX_SIZE_SEND, 0);

        if (send_bytes != MAX_SIZE_SEND)
        {
            debug_ftp("%s send packet: %ld fail! \n", descript, i);

            return FAILURE;
        }
        usleep(10);
    }

    byte_end = size % MAX_SIZE_SEND;

    send_bytes
        = send(fd_data_socket, p_buffer + i * MAX_SIZE_SEND, byte_end, 0);

    if (send_bytes != byte_end)
    {
        debug_ftp("%s send the last packet fail! \n", descript);

        return FAILURE;
    }

    //debug_ftp("out <<< %s !\n", __func__);
    return SUCCESS;
}


int CLASS_FTP::ftp_send_file(const char* file_path)
{
    int ret = FAILURE;

    struct stat dstat;

    if (stat(file_path, &dstat) < 0)
    {
        log_warn_ftp("stat %s error : %m\n", file_path);
        return ret;
    }

    if (S_ISREG(dstat.st_mode) != 1)
    {
        log_warn_ftp("%s is not a regular file\n", file_path);
        return ret;
    }

    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        log_warn_ftp("open %s failed : %m\n", file_path);
        return ret;
    }

    if (sendfile(fd_data_socket, fd, NULL, dstat.st_size) != dstat.st_size)
    {
        log_warn_ftp("sendfile %s failed : %m !\n", file_path);
    }
    else
    {
        ret = SUCCESS;
    }

    close(fd);

    return ret;
}

/**
 *    功能: 上传文件到ftp服务器.
 *    参数:
 *    	src_file-name: 源文件名, 全路径
 *    	dst_dir      : 目的目录;
 *      dst_file_name: 目的文件名,不带路径.
 *  返回值: FAILURE--失败; SUCCESS--成功.
 */
int CLASS_FTP::ftp_put_file(const char *src_file_name, char dst_dir[][80],
        char *dst_file_name)
{
    char ftp_rcv_buf[200];
    int i, ret;
    char cmd_dir[256] = "/\0";

    if (strlen(dst_dir[0]) == 0 || strlen(src_file_name) == 0 || strlen(
                dst_file_name) == 0)
    {
        debug_ftp("%s ftp_put_file() param error! dst_dir[0]:%s, src_file-name:%s\n", descript, dst_dir[0],src_file_name);
        return FAILURE;
    }
    if (ftp_status != STA_FTP_OK)
    {
        debug_ftp("%s ftp_put_file() 连接异常！\n", descript);
        return FAILURE;
    }
    else{
        printf("ftp_status is OK \n");
    }

    if (access(src_file_name, F_OK) != 0)//若要传送的文件并不存在
    {
        debug_ftp("ftp_put_file() send file '%s' is not exist！\n", src_file_name);
        return FAILURE;
    }
    else{
        printf("access OK\n");
    }

    debug_ftp("%s ftp_put_file() start uploading %s....\n", descript, src_file_name);
    clear_socket_recvbuf(this->fd_ctl_socket);
    printf("ftp_put_file(): after clear_socket_recvbuf\n");
    //###########/创建目录/#################//
    if (SUCCESS != ftp_mk_dir(dst_dir))
    {
        return FAILURE;
    }

    //##### 进入被动传输模式 ########//
    if (SUCCESS != ftp_set_trans_pasv_mode())
    {
        return FAILURE;
    }

    //####### 创建数据连接端口 ########//
    if (SOCKET_ERR == ftp_mk_data_socket())
    {
        return FAILURE;
    }

    debug_ftp("%s create ftp data  socket ok! \n", descript);

    //############/存储数据/###############//
    for (i = 0; i < MAX_DIR_LEVEL; i++)
    {
        if (strlen(dst_dir[i]) != 0)
        {
            strcat(cmd_dir, dst_dir[i]);
            strcat(cmd_dir, "/");
        }
        else
        {
            break;
        }
    }

    if (SUCCESS != ftp_store(cmd_dir, dst_file_name))
    {
        debug_ftp("%s store cmd false! \n", descript);
        ftp_close_data_socket();

        return FAILURE;
    }

    //开始传送数据
    mutex_guard guard(mutex);
    ret = ftp_send_file(src_file_name);

    ftp_close_data_socket();

    if (SUCCESS != ret)
    {
        return FAILURE;
    }

    //数据发送完毕正常
    if (SUCCESS != ftp_recv_cmd_response(ftp_rcv_buf, sizeof(ftp_rcv_buf)))
    {
        debug_ftp("close data socket, and recv failed\n");

        return FAILURE;
    }
    debug_ftp("%s server>>:%s \n", descript, ftp_rcv_buf);

    if (strncasecmp(ftp_rcv_buf, "226", 3) == 0) //收到226表示成功;
    {
        debug_ftp("%s ftp send file success!	\n", descript);
    }
    else if (!strncasecmp(ftp_rcv_buf, "452", 3)) //452	磁盘空间不足
    {
        debug_ftp("%s ftp disk full!\n", descript);
        server_is_full = true;


        return FAILURE;
    }
    else
    {
        debug_ftp("%s send buffer to ftp, failed..ftp server abnormal!\n", descript);
        ftp_status = STA_FTP_EXP;

        return FAILURE;
    }

    return SUCCESS;
}

/*
 * 功能: 监控ftp通道连接状态,保持连接,调用告警回调函数
 */
void* CLASS_FTP::thread_ftp_run(void *arg)
{
    CLASS_FTP *me = (CLASS_FTP *) arg;
    int count = 0;
    int first_time = 1; //故障标志第一次发送;
    time_t tm;
    char text[LEN_MAX_MSG_LOG];

    debug_ftp(">>> in: (%s|%d) \n", __func__,__LINE__);
    //prctl(PR_SET_NAME, me->descript);

    //######### 设置线程取消属性 ############//
    if (0 != pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL))
    {
        debug_ftp("%s pthread_setcancelstate failed\n", me->descript);
    }
    if (0 != pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL))
    {
        debug_ftp("%s pthread_setcanceltype failed\n", me->descript);
    }
    //#########################################// 
    if (me->get_status() != STA_FTP_OK)
    {
        me->open_server();
    }

    while (!me->getThread_stop())
    {
        sleep(1);
        count++;
        if (count >= 10000)
        {
            count = 0;
        }

        if (count % 10 == 0)
        {
            time(&tm);
            //	alarm_record_real.m_tmTime = tm;

            switch (me->get_status())
            {
                case STA_FTP_OK:
                    //debug_ftp("%s link ok.  time:%ld\n", me->descript, tm)
                    //;
                    first_time = 1;
                    break;

                case STA_FTP_FAIL:
                    sprintf(text, "[%s] state: STA_FTP_FAILE\n", me->descript);
                    log_state_ftp(text)
                        ;
                    if (me->getAlarmFxn() != NULL)
                    {
                        if ((first_time == 1)
                                || (count % TIME_SEC_FAULT_ALARM == 0)) //十分钟 报一次故障信息;
                        {
                            first_time = 0;
                            me->getAlarmFxn()(me->getCallerParam(),
                                    (void *) &"connection socket error.\n");
                        }
                    }
                    break;

                case STA_FTP_EXP:

                    sprintf(text, "[%s] server response unnormal.\n", me->descript);
                    log_state_ftp(text)
                        ;

                    if (me->getAlarmFxn() != NULL)
                    {
                        //十分钟 报一次故障信息;
                        if ((first_time == 1)
                                || (count % TIME_SEC_FAULT_ALARM == 0))
                        {
                            first_time = 0;
                            me->getAlarmFxn()(me->getCallerParam(),
                                    (void *) &"server response error.");
                        }
                    }

                    break;

                default:
                    sprintf(text, "[%s] status=%d\n", me->descript,
                            me->get_status());
                    log_state_ftp(text)
                        ;
                    break;
            }

            if (me->getServer_is_full())
            {
                sprintf(text, "[%s] server is full!\n", me->descript);
                log_state_ftp(text);

                if (me->getAlarmFxn() != NULL)
                {
                    if (/*(first_time == 1) ||*/
                            (count % TIME_SEC_FAULT_ALARM == 0)) //十分钟 报一次故障信息;
                    {
                        //						first_time = 0;
                        me->getAlarmFxn()(me->getCallerParam(),
                                (void *) &"server space full.\n");
                    }
                }
            }

            if (me->get_status() != STA_FTP_OK)
            {
                //sleep(2); //ftp连接异常时，等待2s，然后重连

                debug("status: %d != 0 \n", me->get_status());
                me->close_server();
                me->open_server();
            }
            //			else
            //			{
            //				if (STA_FTP_OK != me->ftp_check_ftp_sta())
            //				{
            //
            //					sprintf(text, "%s ftp_check_ftp_sta() return false!\n",
            //							me->descript);
            //					debug_ftp(text);
            //					log_warn_ftp(text);
            //				}
            //			}
        }

        //#######// //#######//
        if (count % 30 == 0)
        {
            if (STA_FTP_OK != me->ftp_check_ftp_sta())
            {
                sprintf(text, "%s ftp_check_ftp_sta() return false!\n",
                        me->descript);
                debug_ftp(text);
                log_state_ftp(text);
            }
        }

        //#######//一个小时检测一次FTP是否满#######//
        if(count % 3600 == 0)
        {
            me->ftp_check_server_full();
        }
    }
    //	debug_ftp("%s %s thread quit!\n", me->descript, __func__);
    log_state_ftp("%s %s thread quit!\n", me->descript, __func__);
    return 0;
}

/**
 * 功能:启动ftp连接监视线程
 */
int CLASS_FTP::start()
{
    setThread_stop(false);

    if (0 != pthread_create(&thread_t_ftp, NULL, thread_ftp_run, this))
    {
        char msg[LEN_MAX_MSG_LOG];
        sprintf(msg, "%s create thread fail!\n", descript);
        debug_ftp(msg);
        log_error_ftp(msg);
        return FAILURE;
    }
    started = true;
    return SUCCESS;
}

/*
 * 功能: 停止ftp连接监视线程,回收socket.
 */
void CLASS_FTP::stop()
{
    void* state;

    setThread_stop(true);
    sleep(2); //wait thread quit.

    pthread_join(this->thread_t_ftp, &state);

    this->close_server();
    started = false;
}

//} //end of namespace FTP.


/*
 * 功能: 获取ftp模块是否已经初始化
 * 参数: 无
 * 返回: true -- 已经初始化, false---未初始化.
 */
bool ftp_module_initialized()
{
    return bInitialized;
}

/*******************************************************************************
 * 功能：初始化各个ftp通道,并启动
 * 参数：无
 * 返回：
 *******************************************************************************/
int ftp_module_init(void)
{
    TYPE_FTP_CONFIG_PARAM ftp_conf;

    /***************Ftp违法***************/
    g_ftp_illegal = new CLASS_FTP("[Ftp违法]");
    for (int i = 0; i < 4; i++)
    {
        ftp_conf.ip[i] = g_arm_config.basic_param.ftp_param_illegal.ip[i];
    }

    sprintf(ftp_conf.user, "%s",
            g_arm_config.basic_param.ftp_param_illegal.user);
    sprintf(ftp_conf.passwd, "%s",
            g_arm_config.basic_param.ftp_param_illegal.passwd);
    ftp_conf.port = g_arm_config.basic_param.ftp_param_illegal.port;
    g_ftp_illegal->set_config(&ftp_conf);
    g_ftp_illegal->start();

    /***************Ftp违法***************/
    g_ftp_illegal_resuming = new CLASS_FTP("[Ftp违法_续传]");
    for (int i = 0; i < 4; i++)
    {
        ftp_conf.ip[i] = g_arm_config.basic_param.ftp_param_illegal.ip[i];
    }

    sprintf(ftp_conf.user, "%s",
            g_arm_config.basic_param.ftp_param_illegal.user);
    sprintf(ftp_conf.passwd, "%s",
            g_arm_config.basic_param.ftp_param_illegal.passwd);
    ftp_conf.port = g_arm_config.basic_param.ftp_param_illegal.port;
    g_ftp_illegal_resuming->set_config(&ftp_conf);
    g_ftp_illegal_resuming->start();

    /***************Ftp过车***************/
    g_ftp_pass_car = new CLASS_FTP("[Ftp过车]");
    for (int i = 0; i < 4; i++)
    {
        ftp_conf.ip[i] = g_arm_config.basic_param.ftp_param_pass_car.ip[i];
    }
    sprintf(ftp_conf.user, "%s",
            g_arm_config.basic_param.ftp_param_pass_car.user);
    sprintf(ftp_conf.passwd, "%s",
            g_arm_config.basic_param.ftp_param_pass_car.passwd);
    ftp_conf.port = g_arm_config.basic_param.ftp_param_pass_car.port;
    g_ftp_pass_car->set_config(&ftp_conf);
    g_ftp_pass_car->start();

    /***************Ftp过车***************/
    g_ftp_pass_car_resuming = new CLASS_FTP("[Ftp过车_续传]");
    for (int i = 0; i < 4; i++)
    {
        ftp_conf.ip[i] = g_arm_config.basic_param.ftp_param_pass_car.ip[i];
    }
    sprintf(ftp_conf.user, "%s",
            g_arm_config.basic_param.ftp_param_pass_car.user);
    sprintf(ftp_conf.passwd, "%s",
            g_arm_config.basic_param.ftp_param_pass_car.passwd);
    ftp_conf.port = g_arm_config.basic_param.ftp_param_pass_car.port;
    g_ftp_pass_car_resuming->set_config(&ftp_conf);
    g_ftp_pass_car_resuming->start();

    /***************Ftp配置***************/
    g_ftp_conf = new CLASS_FTP("[Ftp配置]");
    for (int i = 0; i < 4; i++)
    {
        ftp_conf.ip[i] = g_set_net_param.m_NetParam.ftp_param_conf.ip[i];
    }
    sprintf(ftp_conf.user, "%s", g_set_net_param.m_NetParam.ftp_param_conf.user);
    sprintf(ftp_conf.passwd, "%s",
            g_set_net_param.m_NetParam.ftp_param_conf.passwd);
    ftp_conf.port = g_set_net_param.m_NetParam.ftp_param_conf.port;
    g_ftp_conf->set_config(&ftp_conf);
    g_ftp_conf->start();

    /***************Ftp-H264***************/
    g_ftp_h264 = new CLASS_FTP("[Ftp-H264]");
    for (int i = 0; i < 4; i++)
    {
        ftp_conf.ip[i] = g_arm_config.basic_param.ftp_param_h264.ip[i];
    }

    sprintf(ftp_conf.user, "%s", g_arm_config.basic_param.ftp_param_h264.user);
    sprintf(ftp_conf.passwd, "%s",
            g_arm_config.basic_param.ftp_param_h264.passwd);
    ftp_conf.port = g_arm_config.basic_param.ftp_param_h264.port;
    g_ftp_h264->set_config(&ftp_conf);
    g_ftp_h264->start();

    /***************Ftp-H264***************/
    g_ftp_h264_resuming = new CLASS_FTP("[Ftp-H264_续传]");
    for (int i = 0; i < 4; i++)
    {
        ftp_conf.ip[i] = g_arm_config.basic_param.ftp_param_h264.ip[i];
    }

    sprintf(ftp_conf.user, "%s", g_arm_config.basic_param.ftp_param_h264.user);
    sprintf(ftp_conf.passwd, "%s",
            g_arm_config.basic_param.ftp_param_h264.passwd);
    ftp_conf.port = g_arm_config.basic_param.ftp_param_h264.port;
    g_ftp_h264_resuming->set_config(&ftp_conf);
    g_ftp_h264_resuming->start();


    g_ftp_chanel[FTP_CHANNEL_CONFIG] = g_ftp_conf;
    g_ftp_chanel[FTP_CHANNEL_ILLEGAL] = g_ftp_illegal;
    g_ftp_chanel[FTP_CHANNEL_PASSCAR] = g_ftp_pass_car;
    g_ftp_chanel[FTP_CHANNEL_H264] = g_ftp_h264;

    g_ftp_chanel[FTP_CHANNEL_ILLEGAL_RESUMING] = g_ftp_illegal_resuming;
    g_ftp_chanel[FTP_CHANNEL_PASSCAR_RESUMING] = g_ftp_pass_car_resuming;
    g_ftp_chanel[FTP_CHANNEL_H264_RESUMING] = g_ftp_h264_resuming;
    bInitialized = true;
    return 0;
}

/*******************************************************************************
 * 功能：各个ftp通道退出.
 * 参数：无
 * 返回：
 *******************************************************************************/
void ftp_module_exit(void)
{
    CLASS_FTP* instance;

    for (int i = 0; i < FTP_CHANNEL_COUNT; i++)
    {
        instance = get_ftp_chanel(i);
        instance->stop();
    }
    bInitialized = false;
}
/*******************************************************************************
 * 功能：获得的ftp连接;
 * 参数：
 * 		int index:通道索引;
 *
 * 返回：成功返回ftp连接, 失败返回NULL
 *******************************************************************************/
CLASS_FTP* get_ftp_chanel(int index)
{
    if (index >= FTP_CHANNEL_COUNT)
    {
        return NULL;
    }

    return g_ftp_chanel[index];
}
