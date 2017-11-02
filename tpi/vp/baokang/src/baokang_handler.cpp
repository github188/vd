#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

#include "logger/log.h"
#include "baokang_handler.h"
#include "baokang_timer.h"

#ifdef MODEL_SENSOR_IMGS_SONY_IMX225
#define IMAGE_WIDTH     (1280u)  // 3200u 3264u  	1920
#define IMAGE_HEIGHT    (960u) // 2400u 2464u		1080
#define IMAGE_HEIGHT_ENCODE 960 //1072
#else
#define IMAGE_WIDTH     (1920u)  // 3200u 3264u  	1920
#define IMAGE_HEIGHT    (1080u) // 2400u 2464u		1080
#define IMAGE_HEIGHT_ENCODE 1072 //1072
#endif

extern int g_cmd_fd;
extern int g_pic_fd;
extern timer_t g_hb_timer;

enum BAOKANG_CMD
{
    BAOKANG_CMD_GET_STATE = 0x70,
    BAOKANG_CMD_SET_TIME = 0x18,
    BAOKANG_CMD_GET_RESOLUTION = 0x31
};

typedef struct _baokang_cmd
{
    unsigned char cmd;
    unsigned char reply_flag;
    int length;
    unsigned char data[];
}baokang_cmd_s;

/*******************************************************************************
 * Function   : baokang_get_state
 * Description: baokang get state cmd handler
 * Input      : pcmd -- struct of baokang_cmd_s
 *              resp -- a null pointer reference used to store response
 *                      memory malloced in function, should be freed at caller.
 * Output     : length -- the length of response.
 ******************************************************************************/
int baokang_get_state(baokang_cmd_s *pcmd, unsigned char* &resp)
{
    if(NULL == pcmd)
    {
        ERROR("bad paramater.\n");
        return -1;
    }

    if(pcmd->cmd != BAOKANG_CMD_GET_STATE)
    {
        ERROR("bad cmd 0x%x\n", pcmd->cmd);
        return -1;
    }

	struct tm *timenow;
    struct timeval now;

    gettimeofday(&now, NULL);
    timenow = localtime(&now.tv_sec);

    resp = new unsigned char[28]();
    resp[0] = 0x18;
    resp[16] = 0x04;
    resp[20] = timenow->tm_year % 100;
    resp[21] = timenow->tm_mon + 1;
    resp[22] = timenow->tm_mday;
    resp[23] = timenow->tm_wday;
    resp[24] = timenow->tm_hour;
    resp[25] = timenow->tm_min;
    resp[26] = timenow->tm_sec;
    resp[27] = now.tv_usec / 1000;

    return 28;
}

/*******************************************************************************
 * Function   : baokang_set_time
 * Description: baokang set time, it will set the hardware time. 
 * Input      : pcmd -- struct of baokang_cmd_s
 *              resp -- no use
 * Output     : -1 fail; 0 success
 ******************************************************************************/
int baokang_set_time(baokang_cmd_s *pcmd, unsigned char* &resp)
{
    if(NULL == pcmd)
    {
        ERROR("bad paramater.\n");
        return -1;
    }

    if(pcmd->cmd != BAOKANG_CMD_SET_TIME)
    {
        ERROR("bad cmd 0x%x\n", pcmd->cmd);
        return -1;
    }

    unsigned char* time_str = pcmd->data;

	struct tm timenow;
    timenow.tm_year = time_str[0] + (2000 - 1900); 
    timenow.tm_mon = time_str[1] - 1;
    timenow.tm_mday = time_str[2];
    timenow.tm_wday = time_str[3];
    timenow.tm_hour = time_str[4];
    timenow.tm_min = time_str[5];
    timenow.tm_sec = time_str[6];

    time_t timep = mktime(&timenow);
    struct timeval tv;
    tv.tv_sec = timep;
    tv.tv_usec = 0;
    if(settimeofday(&tv, (struct timezone *)0) < 0)
        ERROR("settimeofday");
    system("hwclock -uw");

	struct tm *gettime;
    struct timeval getnow;

    gettimeofday(&getnow, NULL);
    gettime = localtime(&getnow.tv_sec);
    return 0;
}

/*******************************************************************************
 * Function   : baokang_get_resolution
 * Description: baokang get resolution
 * Input      : pcmd -- struct of baokang_cmd_s
 *              resp -- a null pointer reference used to store response
 *                      memory malloced in function, should be freed at caller.
 * Output     : length -- the length of response.
 ******************************************************************************/
int baokang_get_resolution(baokang_cmd_s *pcmd, unsigned char* &resp)
{
    if(NULL == pcmd)
    {
        ERROR("bad paramater.\n");
        return -1;
    }

    if(pcmd->cmd != BAOKANG_CMD_GET_RESOLUTION)
    {
        ERROR("bad cmd 0x%x\n", pcmd->cmd);
        return -1;
    }

    unsigned image_width = IMAGE_WIDTH;
    unsigned image_height = IMAGE_HEIGHT;

    resp = new unsigned char[16]();
    resp[0]  = 0x0c;

    resp[4]  = (image_width >> 24) & 0xff;
    resp[5]  = (image_width >> 16) & 0xff;
    resp[6]  = (image_width >>  8) & 0xff;
    resp[7]  = (image_width      ) & 0xff;

    resp[8]  = (image_height >> 24) & 0xff;
    resp[9]  = (image_height >> 16) & 0xff;
    resp[10] = (image_height >>  8) & 0xff;
    resp[11] = (image_height      ) & 0xff;
    
    return 16;
}

/*******************************************************************************
 * Function   : baokang_cmd_parse
 * Description: parse the cmd 
 * Input      : buf -- buf received from socket.
 *              length -- length of buf
 * Output     : struct of cmd
 ******************************************************************************/
baokang_cmd_s* baokang_cmd_parse(char* buf, int length)
{
    // Cmd ID, ReplyFlag, PLen1, PLen2, PLen3, PLen4, P1, P2, ... , Pn
    int paramater_count = length - 6;
    baokang_cmd_s* request = 
        (baokang_cmd_s*)malloc(sizeof(baokang_cmd_s) + paramater_count);
    if(NULL == request)
    {
        ERROR("malloc failed\n");
        return NULL;
    }

    request->cmd = buf[0];
    request->reply_flag = buf[1];
    request->length = (buf[5] << 24) + (buf[4] << 16) + (buf[3] << 8) + buf[2];
    memcpy(request->data, &buf[6], paramater_count * sizeof(char));

    return request;
}

/*******************************************************************************
 * Function   : baokang_cmd_handler
 * Description: receive cmd from socket, parse it and then send response.
 * Input      : fd -- the socket fd used to receive buf.
 * Output     : handler state
 ******************************************************************************/
int baokang_cmd_handler(int fd)
{
    // read fd;
    char read_buf[512] = {0};
    int length = sizeof(read_buf);

    errno = 0;
    length = recv(fd, read_buf, length, 0);
    if(length == -1)
    {
        if(errno == EAGAIN)
        {
            ERROR("recv timeout\n");
        }
        else
        {
            ERROR("recv error errno = %d\n", errno);
        }
        return -1;
    }
    else if(length == 0)
    {
        INFO("cmd connection shutdown fd = %d\n", fd);
        close(fd);
        g_cmd_fd = 0;
        return 0;
    }

    INFO("%s", "recv buffer: ");
#if 1
    for(int i = 0; i < length; i++)
    {
        INFO("%02x ", read_buf[i]);
    }
#endif

    baokang_cmd_s *pcmd = baokang_cmd_parse(read_buf, length);

    int resp_len = 0;
    unsigned char* resp = NULL;
    switch(pcmd->cmd)
    {
    case BAOKANG_CMD_GET_STATE:
        DEBUG("baokang_get_state\n");
        resp_len = baokang_get_state(pcmd, resp);
        break;
    case BAOKANG_CMD_SET_TIME:
        DEBUG("baokang_set_time\n");
        resp_len = baokang_set_time(pcmd, resp);
        break;
    case BAOKANG_CMD_GET_RESOLUTION:
        DEBUG("baokang_get_resolution\n");
        resp_len = baokang_get_resolution(pcmd, resp);
        break;
    default:
        break;
    }

    if(pcmd->reply_flag)
    {
        if((resp != NULL) && (resp_len != -1))
        {
            // send the response
            send(fd, (char*)resp, resp_len, 0);
        }
    }

    // pcmd malloced at baokang_cmd_parse, free it here.
    if(pcmd != NULL)
    {
        free(pcmd);
        pcmd = NULL;
    }

    // resp malloced at baokang_construct_response, free it here.
    if(resp != NULL)
    {
        //free(resp);
    	delete [] resp;
        resp = NULL;
    }
    return 0;
}

/*******************************************************************************
 * Function   : baokang_pic_handler
 * Description: pic sock receive 0xffff indicate to terminate connection.
 * Input      : fd -- the socket fd used to receive buf.
 * Output     : handler state
 ******************************************************************************/
int baokang_pic_handler(int fd)
{
    // read fd;
    char read_buf[4] = {0};
    int length = sizeof(read_buf);

    errno = 0;
    length = recv(fd, read_buf, length, 0);
    if(length == -1)
    {
        if(errno == EAGAIN)
        {
            ERROR("recv timeout\n");
        }
        else
        {
            ERROR("recv error errno = %d\n", errno);
        }
        return -1;
    }
    else if(length == 0)
    {
        INFO("pic connection shutdown fd = %d\n", fd);
        // we should stop heartbeat timer here.
        baokang_delete_timer(&g_hb_timer);
        close(fd);
        g_pic_fd = 0;
        return 0;
    }
 
    INFO("received from baokang pic sock.");
    if((read_buf[0] == 0xff) && (read_buf[1] == 0xff))
    {
        INFO("0xffff received from baokang pic sock, close the connection.");
        // we should stop heartbeat timer here.
        baokang_delete_timer(&g_hb_timer);
        close(fd);
        g_pic_fd = 0;
    }
    return 0;
}
