#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "baokang_handler.h"
#include "baokang_pthread.h"
#include "baokang_timer.cpp"
#include "vd_msg_queue.h"
#include "logger/log.h"

#include <sys/types.h>
#include <fcntl.h>

extern int errno;

int g_cmd_fd = 0;
int g_pic_fd = 0;
timer_t g_hb_timer = NULL;

baokang_info_s g_baokang_info[VP_BK_PIC_COUNT];

/*******************************************************************************
 * Function   : baokang_timeout_cb
 * Description: callback function for baokang heartbeat timer.
 * Input      : union sigval sig
 * Output     : void
 ******************************************************************************/
void baokang_timeout_cb(union sigval sig)
{
    //get the connected fd from sigevent
    int pic_fd = sig.sival_int;
    static int i = 0;

    //send heartbeat socket to fd;
    char data[4] = {0};
    int ret = send(pic_fd, &data, sizeof(data), 0);
    if(-1 == ret)
    {
        ERROR("send heart_beat failed. errno = %d pic_fd = %d", errno, pic_fd);
        baokang_delete_timer(&g_hb_timer);
        close(pic_fd);
        g_pic_fd = 0;
        return;
    }
    else
    {
        i++;
        if (i >= 6) {
            INFO("send heart_beat success. pic_fd = %d", pic_fd);
            i = 0;
        }
    }

    //start the timer again.
    baokang_set_timer(&g_hb_timer, 5000);
}

/*******************************************************************************
 * Function   : baokang_create_server
 * Description: listen to port
 * Input      : port
 * Output     : handler state
 ******************************************************************************/
int baokang_create_server(int port)
{
    errno = 0;
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == lfd)
    {
        ERROR("lfd failed errno = %d\n", errno);
        return -1;
    }

    int option = 1;
    errno = 0;
    if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        ERROR("lfd set resuse failed. errno = %d\n", errno);
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    errno = 0;
    if(-1 == bind(lfd, (sockaddr*)&servaddr, sizeof(servaddr)))
    {
        ERROR("bind error. errno = %d\n", errno);
        return -1;
    }

    errno = 0;
    if(-1 == listen(lfd, 20))
    {
        ERROR("listen error. errno = %d\n", errno);
        return -1;
    }

    return lfd;
}

//get the maximum size of the picture
int baokang_get_pic_buf_size(void)
{
    int maximum = 0;

    for (int i=0; i<VP_BK_PIC_COUNT; i++) {
        if (maximum < g_baokang_info[i].pic_size) {
            maximum = g_baokang_info[i].pic_size;
        }
    }

    return (maximum > VP_PICTURE_BASESIZE ? VP_PICTURE_BASESIZE : maximum);
}

/*******************************************************************************
 * Function   : baokang_pic_task
 * Description: receive pic from mq and send to baokang platform.
 * Input      : para -- not used
 * Output     : not used
 ******************************************************************************/
void* baokang_pic_task(void* para)
{
    str_vp_msg vp_msg = {0};

    while(true)
    {
        memset(&vp_msg, 0, sizeof(str_vp_msg));

        errno = 0;
        int msg_len = sizeof(str_vp_msg) - sizeof(long);
        if(-1 == msgrcv(MSG_VP_ID, &vp_msg, msg_len, MSG_VP_BAOKANG_TYPE, 0))
        {
            ERROR("msg receive failed. errno = %d\n", errno);
            usleep(10000);
            continue;
        }

        DEBUG("send pic info to baokang platform\n");
        // get the pic & send 2 baokang.
        //baokang_info_s resp[VP_BK_PIC_COUNT];
        //memcpy(&resp, &g_baokang_info, sizeof(baokang_info_s) * VP_BK_PIC_COUNT);
        int maximum_size = baokang_get_pic_buf_size();
        char resp[maximum_size+4];

        //traversal all the pics.
        for(int j = 0; j < VP_BK_PIC_COUNT; j++)
        {
            if((0 == g_baokang_info[j].pic_size) || (0 == g_pic_fd))
                break;

            int pack_length = sizeof(int) + g_baokang_info[j].pic_size;
            DEBUG("send to fd %d %d\n", g_pic_fd, pack_length);
            g_baokang_info[j].pic_size = g_baokang_info[j].pic_size;
            memset(resp, 0x0, sizeof(resp));
			int *presp = (int *)resp;
            *presp = g_baokang_info[j].pic_size;
            memcpy(resp+4, g_baokang_info[j].pic, g_baokang_info[j].pic_size);

            //retry 3times if failed.
            for(int i = 0; i < 3; i++)
            {
                errno = 0;
                int ret = send(g_pic_fd, resp, pack_length, 0);
                if(-1 == ret)
                {
                    ERROR("send failed. errno = %d", errno);
                }
                else
                {
                    INFO("send success.");
                    break;
                }
            }
            // with a break before send 2nd pic.
            sleep(1);
        }
    }
    return NULL;
}

/*******************************************************************************
 * Function   : baokang_pthread
 * Description: main thread for baokang platform
 *              begin to listen to 35000 45000 61001 and handle new connection,
 *              send pic to platform.
 * Input      : arg -- not used
 * Output     : not used
 ******************************************************************************/
void* baokang_pthread(void* arg)
{
    DEBUG("##### BAOKANG_PTHREAD start #####\n");
    pthread_t bk_pic_id;
    int tmpfd = -1;
    if(pthread_create(&bk_pic_id, NULL, baokang_pic_task, NULL) != 0)
    {
        ERROR("create baokang_pic_task failed.\n");
        return NULL;
    }

    struct epoll_event ev, events[6];

    // size of epoll_create must lager than 0 but make no effect.
    int epfd = epoll_create(6);

    int lcmd_fd   = baokang_create_server(35000);
    int lpic_fd   = baokang_create_server(45001);

    ev.data.fd = lcmd_fd;
    ev.events = EPOLLIN;
    errno = 0;
    if(-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, lcmd_fd, &ev))
    {
        ERROR("epoll_ctl failed errno = %d\n", errno);
        return NULL;
    }

    ev.data.fd = lpic_fd;
    ev.events = EPOLLIN;
    errno = 0;
    if(-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, lpic_fd, &ev))
    {
        ERROR("epoll_ctl failed errno = %d\n", errno);
        return NULL;
    }

    while(true)
    {
        int fd_num = epoll_wait(epfd, events, 20, 500);
        for(int i = 0; i < fd_num; i++)
        {
            if(lcmd_fd == events[i].data.fd)
            {
                struct sockaddr_in addr;
                socklen_t len = sizeof(addr);

                errno = 0;
                g_cmd_fd = accept(lcmd_fd, (sockaddr*)&addr, &len);
                if(-1 == g_cmd_fd )
                {
                    ERROR("accept failed errno = %d\n", errno);
                    continue;
                }

                INFO("accept cmd connection from %s port = %d fd = %d\n",
                        inet_ntoa(addr.sin_addr), addr.sin_port, g_cmd_fd);

                struct timeval timeout={5,0};//5s
                setsockopt(g_cmd_fd, SOL_SOCKET, SO_SNDTIMEO,
                        (const void*)&timeout, sizeof(timeout));
                setsockopt(g_cmd_fd, SOL_SOCKET, SO_RCVTIMEO,
                        (const void*)&timeout, sizeof(timeout));

                int keepAlive = 1;    // open keepalive property. dft: 0(close)
                int keepIdle = 60;    // probe if 60s without action. dft:7200s
                int keepInterval = 5; // time interval 5s when probe. dft:75s
                int keepCount = 3;    // retry times default:9
                setsockopt(g_cmd_fd, SOL_SOCKET, SO_KEEPALIVE,
                        (const void*)&keepAlive, sizeof(keepAlive));
                setsockopt(g_cmd_fd, SOL_TCP, TCP_KEEPIDLE,
                        (const void*)&keepIdle, sizeof(keepIdle));
                setsockopt(g_cmd_fd, SOL_TCP, TCP_KEEPINTVL,
                        (const void*)&keepInterval, sizeof(keepInterval));
                setsockopt(g_cmd_fd, SOL_TCP, TCP_KEEPCNT,
                        (const void*)&keepCount, sizeof(keepCount));

                ev.data.fd = g_cmd_fd;
                ev.events = EPOLLIN;
                errno = 0;
                if(-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, g_cmd_fd, &ev))
                {
                    ERROR("epoll_ctl failed errno = %d\n", errno);
                    return NULL;
                }
            }

            else if(lpic_fd == events[i].data.fd)
            {
                struct sockaddr_in addr;
                socklen_t len = sizeof(addr);

                errno = 0;
                tmpfd = accept(lpic_fd, (sockaddr*)&addr, &len);
                if(-1 == tmpfd )
                {
                    ERROR("accept failed errno = %d\n", errno);
                    continue;
                } else {
                    if (g_pic_fd <= 0) {
                        g_pic_fd = tmpfd;
                        INFO("accept pic connection from %s port = %d fd = %d\n",
                            inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), g_pic_fd);
                    } else {
                        INFO("accept pic connection from %s port = %d fd = %d, but it's established\n",
                            inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), g_pic_fd);
                        close(tmpfd);
                        continue;
                    }
                }

                struct timeval timeout={5,0};//5s
                setsockopt(g_pic_fd, SOL_SOCKET, SO_SNDTIMEO,
                        (const void*)&timeout, sizeof(timeout));
                setsockopt(g_pic_fd, SOL_SOCKET, SO_RCVTIMEO,
                        (const void*)&timeout, sizeof(timeout));

                int keepAlive = 1;    // open keepalive property. dft: 0(close)
                int keepIdle = 60;    // probe if 60s without action. dft:7200s
                int keepInterval = 5; // time interval 5s when probe. dft:75s
                int keepCount = 3;    // retry times default:9
                setsockopt(g_pic_fd, SOL_SOCKET, SO_KEEPALIVE,
                        (const void*)&keepAlive, sizeof(keepAlive));
                setsockopt(g_pic_fd, SOL_TCP, TCP_KEEPIDLE,
                        (const void*)&keepIdle, sizeof(keepIdle));
                setsockopt(g_pic_fd, SOL_TCP, TCP_KEEPINTVL,
                        (const void*)&keepInterval, sizeof(keepInterval));
                setsockopt(g_pic_fd, SOL_TCP, TCP_KEEPCNT,
                        (const void*)&keepCount, sizeof(keepCount));

                ev.data.fd = g_pic_fd;
                ev.events = EPOLLIN;
                errno = 0;
                if(-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, g_pic_fd, &ev))
                {
                    ERROR("epoll_ctl failed errno = %d\n", errno);
                    return NULL;
                }

                // when baokang platform connected pic sock, heartbreat should
                // be sent until the connection lost when server receive 0xffff.
                baokang_create_timer(&g_hb_timer, baokang_timeout_cb, g_pic_fd);
                baokang_set_timer(&g_hb_timer, 5000);
            }

            // if connected user & received data, read data.
            else if(events[i].events & EPOLLIN)
            {
                if(events[i].data.fd == g_cmd_fd)
                    baokang_cmd_handler(events[i].data.fd);
                else if(events[i].data.fd == g_pic_fd)
                    baokang_pic_handler(events[i].data.fd);
                else
                {
                    ERROR("recv bad request errno = %d fd = %d\n", errno, events[i].data.fd);
                    //return NULL;
                }
            }
            // if connected user & has data to send, write data.
            else if(events[i].events & EPOLLOUT)
            {
                ERROR("epoll EPOLLOUT.\n");
            }
            else
            {
                ERROR("epoll failed.\n");
            }
        }
    }
}
