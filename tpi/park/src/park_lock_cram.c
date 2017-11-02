/**
 * @file park_lock_cram.cpp
 * @brief this file include the park lock control functions for park lock
 *        from CrAM IoT Technology Co.,Ltd. including protocal analyze, park
 *        lock control etc.
 * @author Felix Du <durd07@gmail.com>
 * @version 0.0.1
 * @date 2017-05-18
 */

/*
 * Make sure the #define _GNU_SOURCE is before any of the your headers are
 * included. Macros are set up by <features.h>, which include various parts
 * of the GNU C library. If you've included other headers before you define
 * _GNU_SOURCE, <features.h> will have already been included and will have
 * not seen _GNU_SOURCE.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <pthread.h>

#include <sys/prctl.h>

#include "logger/log.h"

#include "park_lock_control.h"
#include "park_lock_cram.h"
#include "park_lock_cram_protocal.h"

#define CRAM_GW_MAC "001500FFFFFFFFFF"

static park_lock_cram cram_lock;

static void* park_lock_cram_resp_ack(void* args);
static void* park_lock_cram_resp_state(void* args);
static void* park_lock_cram_resp_refresh_begin(void* args);
static void* park_lock_cram_resp_refresh_finish(void* args);
static void* park_lock_cram_resp_refresh_error(void* args);
static void* park_lock_cram_resp_lock_list(void* args);

#if 0
static int park_lock_cram_start_watchdog(void);
static void park_lock_cram_watchdog_feed(void);
static void park_lock_cram_watchdog_reset(union sigval sig);
#endif

park_lock_cram_resp_cb resp_cb_table[PARK_LOCK_CRAM_RESP_MAX] = {
    [PARK_LOCK_CRAM_RESP_ACK]            = park_lock_cram_resp_ack,
    [PARK_LOCK_CRAM_RESP_STATE]          = park_lock_cram_resp_state,
    [PARK_LOCK_CRAM_RESP_REFRESH_BEGIN]  = park_lock_cram_resp_refresh_begin,
    [PARK_LOCK_CRAM_RESP_REFRESH_FINISH] = park_lock_cram_resp_refresh_finish,
    [PARK_LOCK_CRAM_RESP_REFRESH_ERROR]  = park_lock_cram_resp_refresh_error,
    [PARK_LOCK_CRAM_RESP_LOCK_LIST]      = park_lock_cram_resp_lock_list
};

static int park_lock_cram_lock(void);
static int park_lock_cram_unlock(void);
static int park_lock_cram_get_status(void);
static int park_lock_cram_ctrl(void*);

static void* cram_recv_thread(void* args);

/**
 * @brief connect cram gateway
 *
 * @param server ip/hostname
 * @param port default 8008
 *
 * @return sockfd
 */
static int park_lock_cram_gw_register(const char* server, const int port)
{
    if ((NULL == server) || (port == 0))
    {
        ERROR("bad parameter");
        return -EINVAL;
    }

    int sockfd = 0;
    struct sockaddr_in addr;
    struct hostent* phost;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERROR("create socket failed.");
        return -errno;
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(server);

    if (addr.sin_addr.s_addr == INADDR_NONE){ /* hostname */
        phost = (struct hostent*)gethostbyname(server);
        if (phost==NULL) {
            ERROR("Init socket s_addr error!");
            return -errno;
        }

        addr.sin_addr.s_addr =((struct in_addr*)phost->h_addr)->s_addr;
    }

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ERROR("Connect server fail! %d", errno);
        return -errno;
    } else {
        INFO("Connect cram gw success.");
        return sockfd;
    }
}

static int park_lock_cram_lock_register(const char* mac,
        park_lock_cram *lock)
{
    if ((NULL == mac) || (NULL == lock)) {
        ERROR("bad param");
        return -EINVAL;
    }

    memcpy(lock->mac, mac, sizeof(lock->mac));
    return 0;
}

static int park_lock_cram_lock_bind(park_lock_cram* lock, int sockfd)
{
    if ((NULL == lock) || (0 == sockfd)) {
        ERROR("bad param");
        return -EINVAL;
    }

    /* TODO verify lock mac is in this gateway: refresh and get lock list */

    lock->gw_fd = sockfd;
    return 0;
}

int park_lock_cram_init(const char* gateway_ip,
        const int gateway_port, const char* lock_mac)
{
    if ((NULL == gateway_ip) || (0 == gateway_port) || (NULL == lock_mac)) {
        ERROR("bad param");
        return -EINVAL;
    }

    /* register the callback functions for operation */
    extern park_lock_operation_t *g_park_lock_operation;

    cram_lock.operation.lock = park_lock_cram_lock;
    cram_lock.operation.unlock = park_lock_cram_unlock;
    cram_lock.operation.get_status = park_lock_cram_get_status;
    cram_lock.operation.ctrl = park_lock_cram_ctrl;

    g_park_lock_operation = &(cram_lock.operation);

#if 0
    int ret = 0;
    int sockfd = park_lock_cram_gw_register(gateway_ip, gateway_port);
    if (sockfd < 0) {
        return -1;
    }

    ret = park_lock_cram_lock_register(lock_mac, &cram_lock);
    if (ret < 0) {
        return -1;
    }

    park_lock_cram_lock_bind(&cram_lock, sockfd);

    /* after gateway connect success, start the message recv thread. */
    ret = pthread_create(&(cram_lock.recv_thread), NULL, cram_recv_thread, NULL);
    if (ret != 0) {
        ERROR("create cram recv thread failed.");
        return -1;
    }

    park_lock_cram_start_watchdog();
#endif

    /* check the park lock status. */
    park_lock_cram_get_status();
    return 0;
}

#if 0
static int park_lock_cram_start_watchdog(void)
{
    park_create_timer(&(cram_lock.watchdog_timer), park_lock_cram_watchdog_reset, (int)0);
    park_set_timer(&(cram_lock.watchdog_timer), 300000);
    return 0;
}

static void park_lock_cram_watchdog_feed(void)
{
    park_set_timer(&(cram_lock.watchdog_timer), 300000);
}

static void park_lock_cram_watchdog_reset(union sigval sig)
{
    INFO("CRAM_WATCHDOG time expired, reconnect the connection.");

    close(cram_lock.gw_fd);
    pthread_cancel(cram_lock.recv_thread);

    park_lock_init();

    //park_lock_cram_start_watchdog is started at park_lock_cram_init.
    //park_lock_cram_watchdog_feed();
}
#endif

static int park_lock_cram_send_msg(int sockfd, const char *buff);

int park_lock_cram_lock(void)
{
    park_lock_cram* cram = &cram_lock;

    const char* payload = "[01:01]";
    cram_lock_cmd_req *cmd = (cram_lock_cmd_req*)malloc(sizeof(cram_lock_cmd_req) + strlen(payload) + 1);
    if (NULL == cmd) {
        ERROR("malloc cmd failed.");
        return -1;
    }

    cmd->ver = 1;
    memcpy(cmd->mac, park_lock_cram_mac(), sizeof(cmd->mac));
    strcpy(cmd->code, "GET");
    strcpy(cmd->type, "CON");
    strcpy(cmd->payload, payload);

    char cmd_result[1024] = {0};
    park_lock_cram_construct_json(cmd, cmd_result, 1024);

    free(cmd);

    /* send command */
    return park_lock_cram_send_msg(cram->gw_fd, cmd_result);
}

int park_lock_cram_unlock(void)
{
    park_lock_cram* cram = &cram_lock;

    const char* payload = "[01:02]";
    cram_lock_cmd_req *cmd = (cram_lock_cmd_req*)malloc(sizeof(cram_lock_cmd_req) + strlen(payload) + 1);
    if (NULL == cmd) {
        ERROR("malloc cmd failed.");
        return -1;
    }

    cmd->ver = 1;
    memcpy(cmd->mac, park_lock_cram_mac(), sizeof(cmd->mac));
    strcpy(cmd->code, "GET");
    strcpy(cmd->type, "CON");
    strcpy(cmd->payload, payload);

    char cmd_result[1024] = {0};
    park_lock_cram_construct_json(cmd, cmd_result, 1024);

    free(cmd);

    /* send command */
    return park_lock_cram_send_msg(cram->gw_fd, cmd_result);
}

int park_lock_cram_get_status(void)
{
    park_lock_cram* cram = &cram_lock;

    const char* payload = "[04:01]";
    cram_lock_cmd_req *cmd = (cram_lock_cmd_req*)malloc(sizeof(cram_lock_cmd_req) + strlen(payload) + 1);
    if (NULL == cmd) {
        ERROR("malloc cmd failed.");
        return -1;
    }

    cmd->ver = 1;
    memcpy(cmd->mac, park_lock_cram_mac(), sizeof(cmd->mac));
    strcpy(cmd->code, "GET");
    strcpy(cmd->type, "CON");
    strcpy(cmd->payload, payload);

    char cmd_result[1024] = {0};
    park_lock_cram_construct_json(cmd, cmd_result, 1024);

    free(cmd);

    /* send command */
    return park_lock_cram_send_msg(cram->gw_fd, cmd_result);
}

int park_lock_cram_ctrl(void* args)
{
    cram_ctrl_t *cram_ctrl = (cram_ctrl_t *)args;
    if (NULL == cram_ctrl) {
        ERROR("NULL is cram_ctrl");
        return -1;
    }

    switch (cram_ctrl->type) {
        case 1: /* beep */ {
            int beep_time = cram_ctrl->beep_time;

            park_lock_cram* cram = &cram_lock;

            char payload[8] = {0};
            sprintf(payload, "[0A:%02d]", beep_time);
            cram_lock_cmd_req *cmd = (cram_lock_cmd_req*)malloc(sizeof(cram_lock_cmd_req) + strlen(payload) + 1);
            if (NULL == cmd) {
                ERROR("malloc cmd failed.");
                return -1;
            }

            cmd->ver = 1;
            memcpy(cmd->mac, park_lock_cram_mac(), sizeof(cmd->mac));
            strcpy(cmd->code, "GET");
            strcpy(cmd->type, "CON");
            strcpy(cmd->payload, payload);

            char cmd_result[1024] = {0};
            park_lock_cram_construct_json(cmd, cmd_result, 1024);

            free(cmd);

            /* send command */
            return park_lock_cram_send_msg(cram->gw_fd, cmd_result);
        }
        case 2: {
            park_lock_cram* cram = &cram_lock;

            const char* payload = "[F0:01]";
            cram_lock_cmd_req *cmd = (cram_lock_cmd_req*)malloc(sizeof(cram_lock_cmd_req) + strlen(payload) + 1);
            if (NULL == cmd) {
                ERROR("malloc cmd failed.");
                return -1;
            }

            cmd->ver = 1;
            memcpy(cmd->mac, CRAM_GW_MAC, sizeof(cmd->mac));
            strcpy(cmd->code, "GET");
            strcpy(cmd->type, "CON");
            strcpy(cmd->payload, payload);

            char cmd_result[1024] = {0};
            park_lock_cram_construct_json(cmd, cmd_result, 1024);

            free(cmd);

            /* send command */
            return park_lock_cram_send_msg(cram->gw_fd, cmd_result);
        }

        case 3: {
            park_lock_cram* cram = &cram_lock;

            const char* payload = "[F1:01]";
            cram_lock_cmd_req *cmd = (cram_lock_cmd_req*)malloc(sizeof(cram_lock_cmd_req) + strlen(payload) + 1);
            if (NULL == cmd) {
                ERROR("malloc cmd failed.");
                return -1;
            }

            cmd->ver = 1;
            memcpy(cmd->mac, CRAM_GW_MAC, sizeof(cmd->mac));
            strcpy(cmd->code, "GET");
            strcpy(cmd->type, "CON");
            strcpy(cmd->payload, payload);

            char cmd_result[1024] = {0};
            park_lock_cram_construct_json(cmd, cmd_result, 1024);

            free(cmd);

            /* send command */
            return park_lock_cram_send_msg(cram->gw_fd, cmd_result);
        }

        default:
            break;
    }
    return 0;
}

int park_lock_cram_send_msg(int sockfd, const char *buff)
{
    DEBUG("send %s", buff);
    const char* gw_ip = park_lock_cram_gw_ip();
    unsigned short gw_port = park_lock_cram_port();
    const char* mac = park_lock_cram_mac();

    sockfd = park_lock_cram_gw_register(gw_ip, gw_port);
    if (sockfd < 0) {
        return -1;
    }

    int ret = park_lock_cram_lock_register(mac, &cram_lock);
    if (ret < 0) {
        return -1;
    }

    park_lock_cram_lock_bind(&cram_lock, sockfd);

    /* after gateway connect success, start the message recv thread. */
    ret = pthread_create(&(cram_lock.recv_thread), NULL, cram_recv_thread, NULL);
    if (ret != 0) {
        ERROR("create cram recv thread failed.");
        return -1;
    }

    int size = 0;
    if ((size = send(sockfd, buff, strlen(buff), 0)) <= 0) {
        ERROR("Send msg error!");
        return -errno;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 10;

    pthread_timedjoin_np(cram_lock.recv_thread, NULL, &ts);
    close(cram_lock.gw_fd);
    return 0;
}

#define CRAM_RECV_BUF_SIZE (512)
static int park_lock_cram_recv_msg(int sockfd, char* recv_buff, size_t buff_len);
int park_lock_cram_decode_data(const char* buff, const size_t len);

static void* cram_recv_thread(void* args)
{
	prctl(PR_SET_NAME, "cram_recv");

    int ret = 0;
    char* recv_buff = (char*)malloc(CRAM_RECV_BUF_SIZE);
    if (NULL == recv_buff) {
        ERROR("malloc failed.");
        return NULL;
    }

    while (1) {
        //DEBUG("begin to recv");
        memset(recv_buff, 0x00, CRAM_RECV_BUF_SIZE);
        ret = park_lock_cram_recv_msg(cram_lock.gw_fd, recv_buff, CRAM_RECV_BUF_SIZE);
        if (ret < 0) {
            ERROR("error occured.");
            break;
        } else if (ret == 0) {
            /* try to reconnect to server */
            /* TODO */
            sleep(3);
        }
    }
    return NULL;
}

int park_lock_cram_recv_msg(int sockfd, char* recv_buff, size_t buff_len)
{
	int rec_len = 0;

    while (1){
        memset(recv_buff, 0x00, buff_len);
        //struct timeval timeout = {10,0};
        //setsockopt(sockfd, SOL_SOCKET,SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));
        if ((rec_len = recv(sockfd, recv_buff, buff_len, 0)) == -1) {
            ERROR("Return value: %d errno: %d", rec_len, errno);
            return -1;
        } else if (rec_len == 0) { /* peer close the connection. */
            //DEBUG("recv finishd");
            return 0;
        }
        else {
            DEBUG("%d char recieved data : %s.", rec_len, recv_buff);

#if 0
            /* feed watch dog, otherwise reconnect */
            park_lock_cram_watchdog_feed();
#endif
            /* json decode and payload parse, invoke
             * the callback function for certain action
             */
            //DEBUG("recv message from gw %s", recv_buff);
            park_lock_cram_decode_data(recv_buff, rec_len);
        }
    }
}

static void* park_lock_cram_resp_ack(void* args)
{
    //const char* payload = (const char*)args;
    return NULL;
}

static void* park_lock_cram_resp_state(void* args)
{
    const char* payload = (const char*)args;

    struct park_lock_cram_state{
        char last_error_code;
        char reset_src;
        char alarm_code;
        char rocker_position;
        char energy[5];
    }cram_lock_state;

    cram_lock_state.last_error_code = payload[2] - 0x30;
    cram_lock_state.reset_src = payload[4] - 0x30;
    cram_lock_state.alarm_code = payload[5] - 0x30;
    cram_lock_state.rocker_position = payload[6] - 0x30;

    strncpy(cram_lock_state.energy, &payload[7], 4);
    cram_lock_state.energy[4] = '\0';

    char last_error_code_str[2][32] = {
        [0] = "0 - OK",
        [1] = "1 - mag err"
    };

    char reset_src_str[4][32] = {
        [0] = "0 - 摇臂自动复位",
        [1] = "1 - 远程控制复位",
        [2] = "2 - 蓝牙控制复位",
        [3] = "3 - 超声检测无车复位"
    };

    char alarm_code_str[2][32] = {
        [0] = "0 - 报警音",
        [1] = "1 - 没有报警音"
    };

    char rocker_position_str[4][32] = {
        [0] = "",
        [1] = "1 - 上锁",
        [2] = "2 - 解锁",
        [3] = "3 - 异常",
    };

    INFO("cram_lock state\n错误码 %s\n摇臂复位控制源 %s\n报警码 %s\n摇臂位置 %s\n剩余电量 %s",
         last_error_code_str[(unsigned int)cram_lock_state.last_error_code],
         reset_src_str[(unsigned int)cram_lock_state.reset_src],
         alarm_code_str[(unsigned int)cram_lock_state.alarm_code],
         rocker_position_str[(unsigned int)cram_lock_state.rocker_position],
         cram_lock_state.energy);

    FILE *fp = fopen("/tmp/park_lock_cram_state.txt", "w");
    fwrite(payload, 1, strlen(payload), fp);
    fflush(fp);
    fclose(fp);

    DEBUG("This session is finished.");

    pthread_exit(NULL);
    return NULL;
}

static void* park_lock_cram_resp_refresh_begin(void* args)
{
    return NULL;
}

static void* park_lock_cram_resp_refresh_finish(void* args)
{
    cram_ctrl_t cram_ctrl = {0};
    cram_ctrl.type = 3;
    park_lock_cram_ctrl(&cram_ctrl);

    return NULL;
}

static void* park_lock_cram_resp_refresh_error(void* args)
{
    return NULL;
}

static void* park_lock_cram_resp_lock_list(void* args)
{
    char* payload = (char*)args;

    char* node = "node";
    char *p = strstr(payload, node);
    p += strlen(node);
    p += 3;

    char* end = strstr(p, "'");
    *end = '\0';

    char *token = strtok(p, ",");
    DEBUG("%s", token);

    while((token = strtok(NULL, ",")) != NULL) {
        DEBUG("%s", token);
    }

    DEBUG("This session is finished.");
    pthread_exit(NULL);
    return NULL;
}
