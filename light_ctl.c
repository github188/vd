#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "list.h"
#include "logger/log.h"

/* --------- GPIO RELATED --------- */
#define GPIO_MODE_VAL (0x80)
#define PWM_MODE_VAL (0x40)

#define RED_LIGHT_MUXMODE_ADDR    (0x48140a3c) /* gpio 73  red    */
#define BLUE_LIGHT_MUXMODE_ADDR   (0x48140b48) /* gpio 99  blue   */
#define GREEN_LIGHT_MUXMODE_ADDR  (0x48140b4c) /* gpio 100 green  */
#define LIGHT_SWITCH_MUXMODE_ADDR (0x4814083c) /* gpio 9   switch */
#define WHITE_LIGHT_MUXMODE_ADDR  (0x48140ac8) /* gpio 9   switch */

#define RED_LIGHT_GPIO_NUM    (73)
#define BLUE_LIGHT_GPIO_NUM   (99)
#define GREEN_LIGHT_GPIO_NUM  (100)
#define WHITE_LIGHT_GPIO_NUM  (85)
#define LIGHT_SWITCH_GPIO_NUM (9)

enum GPIO_DIRECTION
{
    GPIO_DIRECTION_IN,
    GPIO_DIRECTION_OUT
};

static int pinmuxset(const unsigned int addr, const unsigned int value)
{
    const char *file_name = "/dev/pinmux";
    int fd = open(file_name, O_RDWR);
    if (fd < 0) {
        ERROR("Open %s failed. %s", file_name, strerror(errno));
        return -1;
    }

    if (ioctl(fd, addr, value) == -1) {
        ERROR("ioctl %s failed. %s", file_name, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

static int export_gpio(const unsigned int gpio_num)
{
	/* if /sys/class/gpio/gpio{gpio_num} exist,
	 * export ok, no need to export again */
	char gpio_folder_name[32] = {0};
	sprintf(gpio_folder_name, "/sys/class/gpio/gpio%d", gpio_num);
	if (access(gpio_folder_name, F_OK) == 0) {
		return 0;
	}

    const char *file_name = "/sys/class/gpio/export";
    int fd = open(file_name, O_WRONLY);
    if (fd < 0) {
        ERROR("Open %s failed. %s", file_name, strerror(errno));
        return -1;
    }

    char buf[8] = {0};
    sprintf(buf, "%d", gpio_num);
    if (write(fd, buf, sizeof(buf)) < 0) {
        ERROR("write %s failed. %s", file_name, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int set_gpio_direction(const unsigned int gpio_num,
                              const enum GPIO_DIRECTION direction)
{
    char file_name[128] = {0};
    sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio_num);
    int fd = open(file_name, O_WRONLY);
    if (fd < 0) {
        ERROR("Open %s failed. %s", file_name, strerror(errno));
        return -1;
    }

    char buf[8] = {0};
    if (direction == GPIO_DIRECTION_IN) {
        sprintf(buf, "%s", "in");
    } else {
        sprintf(buf, "%s", "out");
    }

    if (write(fd, buf, sizeof(buf)) < 0) {
        ERROR("write %s failed. %s", file_name, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int set_gpio_val(const unsigned int gpio_num, char gpio_val)
{
    char file_name[64];
    sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio_num);
    int fd = open(file_name, O_WRONLY);
    if(fd < 0){
        ERROR("Open %s failed. %s", file_name, strerror(errno));
        return -1;
    }

	gpio_val = gpio_val + 0x30; /* convert num to char */
    if (write(fd, &gpio_val, sizeof(char)) < 0) {
        ERROR("write %s failed. %s", file_name, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    usleep(5000);

    return 0;
}

/* --------- LIGHT CONTROL RELATED --------- */
#include "light_ctl.h"

typedef void (*light_set_func)(unsigned char brightness);

static void red_light_set(unsigned char brightness);
static void blue_light_set(unsigned char brightness);
static void green_light_set(unsigned char brightness);
static void white_light_set(unsigned char brightness);

static light_set_func light_set_list[LIGHT_COLOR_MAX] = {
    [LIGHT_COLOR_RED] = red_light_set,
    [LIGHT_COLOR_BLUE] = blue_light_set,
    [LIGHT_COLOR_GREEN] = green_light_set,
    [LIGHT_COLOR_WHITE] = white_light_set
};

typedef light_ctl_msg_t* (*prio_list_handler)(light_ctl_msg_t *msg);
typedef struct _light_action_prio_list_t {
    light_ctl_msg_t list[LIGHT_PRIO_MAX];
    prio_list_handler insert_item;
    prio_list_handler delete_item;
    prio_list_handler change_item;
    prio_list_handler select_item;
}light_action_prio_list_t;

light_action_prio_list_t* light_action_prio_list_alloc(void);
static light_ctl_msg_t* change_light_action_prio_list(light_ctl_msg_t *msg)
{
    if (NULL == msg) {
        ERROR("msg is null");
        return 0;
    }

    light_ctl_msg_t *list = light_action_prio_list_alloc()->list;
    enum LIGHT_PRIO priority = msg->priority;
    memcpy(&list[priority], msg, sizeof(light_ctl_msg_t));
    return 0;
}

static light_ctl_msg_t* insert_light_action_prio_list(light_ctl_msg_t *msg)
{
    change_light_action_prio_list(msg);
    return 0;
}

static light_ctl_msg_t* delete_light_action_prio_list(light_ctl_msg_t *msg)
{
    if (NULL == msg) {
        ERROR("msg is null");
        return 0;
    }

    light_ctl_msg_t *list = light_action_prio_list_alloc()->list;
    enum LIGHT_PRIO priority = msg->priority;
    memset(&list[priority], 0x00, sizeof(light_ctl_msg_t));
    list[priority].priority = LIGHT_PRIO_NULL;
    return 0;
}

/**
 * @brief select the highest priority light action
 *
 * @param msg
 *
 * @return not NULL the highest priority light action
 *         NULL there is no light action
 */
static light_ctl_msg_t* select_light_action_prio_list(light_ctl_msg_t *msg)
{
    light_ctl_msg_t *list = light_action_prio_list_alloc()->list;
    for (int i = LIGHT_PRIO_MAX - 1; i != LIGHT_PRIO_NULL; i--) {
        if (list[i].priority != LIGHT_PRIO_NULL) {
            INFO("light priority [%d] red [%d] interval [%d] "
                  "blue [%d] interval [%d] "
                  "green [%d] interval [%d] "
                  "white [%d] interval [%d]",
                  list[i].priority,
                  list[i].light_action_list[0].brightness,
                  list[i].light_action_list[0].flash_interval,
                  list[i].light_action_list[1].brightness,
                  list[i].light_action_list[1].flash_interval,
                  list[i].light_action_list[2].brightness,
                  list[i].light_action_list[2].flash_interval,
                  list[i].light_action_list[3].brightness,
                  list[i].light_action_list[3].flash_interval);
            return &(list[i]);
        }
    }
    return NULL;
}

light_action_prio_list_t* light_action_prio_list_alloc(void)
{
    static light_action_prio_list_t *p = NULL;
    if (NULL == p) {
        static light_action_prio_list_t list;
        memset(&list, 0x00, sizeof(light_action_prio_list_t));
        list.insert_item = insert_light_action_prio_list;
        list.delete_item = delete_light_action_prio_list;
        list.change_item = change_light_action_prio_list;
        list.select_item = select_light_action_prio_list;
        p = &list;
    }
    return p;
}

static int light_init()
{
    pinmuxset(RED_LIGHT_MUXMODE_ADDR, GPIO_MODE_VAL);    /* gpio 73  red    */
    export_gpio(RED_LIGHT_GPIO_NUM);
    set_gpio_direction(RED_LIGHT_GPIO_NUM, GPIO_DIRECTION_OUT);
    set_gpio_val(RED_LIGHT_GPIO_NUM, 0);

    pinmuxset(BLUE_LIGHT_MUXMODE_ADDR, GPIO_MODE_VAL);   /* gpio 99  blue   */
    export_gpio(BLUE_LIGHT_GPIO_NUM);
    set_gpio_direction(BLUE_LIGHT_GPIO_NUM, GPIO_DIRECTION_OUT);
    set_gpio_val(BLUE_LIGHT_GPIO_NUM, 0);

    pinmuxset(GREEN_LIGHT_MUXMODE_ADDR, GPIO_MODE_VAL);  /* gpio 100 green  */
    export_gpio(GREEN_LIGHT_GPIO_NUM);
    set_gpio_direction(GREEN_LIGHT_GPIO_NUM, GPIO_DIRECTION_OUT);
    set_gpio_val(GREEN_LIGHT_GPIO_NUM, 0);

    system("./bin/mem_rdwr.out --wr 48042038 1843 >/dev/null 2>&1");
    system("./bin/mem_rdwr.out --wr 48042040 FFFFFF38 >/dev/null 2>&1");
    system("./bin/mem_rdwr.out --wr 4804203c FFFFE31F >/dev/null 2>&1");
    system("./bin/mem_rdwr.out --wr 4804204c FFFFFFA0 >/dev/null 2>&1");

    /* set LIGHT_SWITCH_GPIO_NUM to pwm mode */
    pinmuxset(LIGHT_SWITCH_MUXMODE_ADDR, GPIO_MODE_VAL); /* gpio 9   switch */
    export_gpio(LIGHT_SWITCH_GPIO_NUM);
    /* set it to in direction so no signal output */
    set_gpio_direction(LIGHT_SWITCH_GPIO_NUM, GPIO_DIRECTION_IN);
    return 0;
}

static void set_light_brightness(const unsigned char brightness)
{
    int value = 0;
    unsigned char tmp_brightness = brightness;

    if (brightness == 128) tmp_brightness = 133;
    value = tmp_brightness * (0xFFFFFFFE - 0xFFFFFF38) / 255;
    value += 0xFFFFFF38;

    char cmd[128] = {0};
    sprintf(cmd, "./bin/mem_rdwr.out --wr 4804204c %x >/dev/null 2>&1", value);

    system(cmd);
}

static void set_white_light_brightness(const unsigned char brightness)
{
    int value = 0;
    unsigned char tmp_brightness = brightness;

    if (brightness == 128) tmp_brightness = 133;
    value = tmp_brightness * (0xFFFFFFF5 - 0xFFFFFF38) / 255;
    value += 0xFFFFFF38;

    char cmd[128] = {0};
    sprintf(cmd, "./bin/mem_rdwr.out --wr 4804a04c %x >/dev/null 2>&1", value);

    system(cmd);
}

static void red_light_on(void)
{
    set_gpio_val(RED_LIGHT_GPIO_NUM, 1);
    pinmuxset(LIGHT_SWITCH_MUXMODE_ADDR, PWM_MODE_VAL);
}

static void blue_light_on(void)
{
    set_gpio_val(BLUE_LIGHT_GPIO_NUM, 1);
    pinmuxset(LIGHT_SWITCH_MUXMODE_ADDR, PWM_MODE_VAL);
}

static void green_light_on(void)
{
    set_gpio_val(GREEN_LIGHT_GPIO_NUM, 1);
    pinmuxset(LIGHT_SWITCH_MUXMODE_ADDR, PWM_MODE_VAL);
}

static void white_light_on(void)
{
    pinmuxset(WHITE_LIGHT_MUXMODE_ADDR, PWM_MODE_VAL);
    system("./bin/mem_rdwr.out --wr 4804a040 FFFFFF38 >/dev/null 2>&1");
    return;
}

static void red_light_off(void)
{
    set_gpio_val(RED_LIGHT_GPIO_NUM, 0);
}

static void blue_light_off(void)
{
    set_gpio_val(BLUE_LIGHT_GPIO_NUM, 0);
}

static void green_light_off(void)
{
    set_gpio_val(GREEN_LIGHT_GPIO_NUM, 0);
}

static void white_light_off(void)
{
    pinmuxset(WHITE_LIGHT_MUXMODE_ADDR, GPIO_MODE_VAL);
    set_gpio_direction(WHITE_LIGHT_GPIO_NUM, GPIO_DIRECTION_OUT);
    set_gpio_val(WHITE_LIGHT_GPIO_NUM, 0);
    return;
}

static void red_light_set(unsigned char brightness) {
    if (brightness) {
        set_light_brightness(brightness);
        red_light_on();
    }
    else {
        red_light_off();
    }
}

static void blue_light_set(unsigned char brightness) {
    if (brightness) {
        set_light_brightness(brightness);
        blue_light_on();
    }
    else {
        blue_light_off();
    }
}

static void green_light_set(unsigned char brightness) {
    if (brightness) {
        set_light_brightness(brightness);
        green_light_on();
    }
    else {
        green_light_off();
    }
}

static void white_light_set(unsigned char brightness) {
    if (brightness) {
        set_white_light_brightness(brightness);
        white_light_on();
    }
    else {
        white_light_off();
    }
}

#include "timer.h"

#define LIGHT_FLASH_BASE (100)

typedef struct _light_flash_node
{
    list_head_t list;
    light_ctl_t *light;
}light_flash_node_t;

static list_head_t light_flash_list;
static pthread_mutex_t light_flash_mutex = PTHREAD_MUTEX_INITIALIZER;

static int light_flash_start(light_ctl_t *p)
{
    if (NULL == p) {
        ERROR("null is light_action_list");
        return -1;
    }

    pthread_mutex_lock(&light_flash_mutex);

    if ((p->brightness > 0) && (p->flash_interval > 0)) {
        size_t size = sizeof(light_flash_node_t);
        light_flash_node_t *node = (light_flash_node_t*)malloc(size);
        if (NULL == node) {
            ERROR("malloc node failed.");
            pthread_mutex_unlock(&light_flash_mutex);
            return -1;
        }
        node->light = p;
        list_add_tail(&(node->list), &light_flash_list);
    }

    pthread_mutex_unlock(&light_flash_mutex);
    return 0;
}

static void light_flash_stop(void)
{
    list_head_t *pos, *n;

    pthread_mutex_lock(&light_flash_mutex);
    if (!list_empty(&light_flash_list)) {
        list_for_each_safe(pos, n, &light_flash_list) {
            light_flash_node_t *p = list_entry(pos, light_flash_node_t, list);
            list_del(pos);
            free(p);
        }
    }
    pthread_mutex_unlock(&light_flash_mutex);
    return;
}

#include <sys/ipc.h>
#include <sys/msg.h>

static light_ctl_msg_t* get_current_light_state(void)
{
    light_action_prio_list_t *list = light_action_prio_list_alloc();
    light_ctl_msg_t *active_light = list->select_item(NULL);
    return active_light;
}

extern int push_record_2_history_list(const int objectState,
									  const char* plate,
									  const char* park_time,
									  const char* pic_name1,
									  const char* pic_name2,
									  const char* light);

static void put_current_light_state(light_ctl_msg_t *msg_data)
{
    if (NULL == msg_data) {
        return;
    }

    light_ctl_msg_t *light = msg_data;
    light_action_prio_list_t *list = light_action_prio_list_alloc();

    if (light->action == LIGHT_ACTION_START) {
        list->insert_item(light);
    } else if (light->action == LIGHT_ACTION_STOP) {
        list->delete_item(light);
    } else if (light->action == LIGHT_ACTION_CHANGE) {
        list->change_item(light);
    }

    /* select the item with highest priority to handle */
    static int has_light_flash = 0;
    light_ctl_msg_t *active_light = list->select_item(NULL);

    if (active_light == NULL) {
        return;
    }

    /* guarantee more than 1 light is on, otherwise no light could been
     * light on; so first tun on the light and tun off the lights */
    light_ctl_t *light_action_list = active_light->light_action_list;

    /* XXX if all red blue green light tunoff, there is no method to tun on
     * again, so DO NOT do this action. */
    if ((light_action_list[0].brightness |
        light_action_list[1].brightness |
        light_action_list[2].brightness) == 0) {
            return;
        }

    if (has_light_flash) {
        light_flash_stop();
        has_light_flash = 0;
    }

    for (int i = 0; i < LIGHT_COLOR_MAX; i++) {
        light_ctl_t *p = &(light_action_list[i]);
        if ((p->brightness > 0) && (p->flash_interval == 0)) {
            light_set_list[p->id](p->brightness);
        } else if ((p->brightness > 0) && (p->flash_interval > 0)) {
            light_flash_start(p);
            has_light_flash = 1;
        }
    }

    for (int i = 0; i < LIGHT_COLOR_MAX; i++) {
        light_ctl_t *p = &(light_action_list[i]);
        if (p->brightness == 0) {
            light_set_list[p->id](p->brightness);
        }
    }

	const char * light_str = "";
    if (light_action_list[0].brightness) {
        if (light_action_list[0].flash_interval) {
            light_str = "红闪";
        } else {
            light_str = "红灯";
        }
    } else if (light_action_list[1].brightness) {
        if (light_action_list[1].flash_interval) {
            light_str = "蓝闪";
        } else {
            light_str = "蓝灯";
        }
    } else if (light_action_list[2].brightness) {
        if (light_action_list[2].flash_interval) {
            light_str = "绿闪";
        } else {
            light_str = "绿灯";
        }
    }

    push_record_2_history_list(0, NULL, NULL, NULL, NULL, light_str);
    return;
}

void* light_flash_thread(void* args)
{
    prctl(PR_SET_NAME, "light_flash");
    INIT_LIST_HEAD(&light_flash_list);

    while(1) {
        usleep(1000);

        pthread_mutex_lock(&light_flash_mutex);
        if (list_empty(&light_flash_list)) {
            pthread_mutex_unlock(&light_flash_mutex);
            continue;
        }

        list_head_t *pos;

        pos = light_flash_list.next;

        light_flash_node_t *entry = list_entry(pos, light_flash_node_t, list);
        light_ctl_t *p = entry->light;

#if 1
        light_set_list[p->id](p->brightness);

        usleep(p->flash_interval * LIGHT_FLASH_BASE * 1000);

        pinmuxset(LIGHT_SWITCH_MUXMODE_ADDR, GPIO_MODE_VAL);
        light_set_list[p->id](0);
        usleep(p->flash_interval * LIGHT_FLASH_BASE * 1000);

        if (!list_is_singular(&light_flash_list)) {
            list_move_tail(pos, &light_flash_list);
        }
#else
        static unsigned char brightness = 10;
        static stronger = 1;

        INFO("%d %d", stronger, brightness);
        if (brightness >= 100)
            stronger = 0;
        else if (brightness <= 2)
            stronger = 1;

        if (stronger) {
            brightness += 2;
            light_set_list[p->id](brightness++);}
        else {
            brightness -= 2;
            light_set_list[p->id](brightness--); }
#endif
        pthread_mutex_unlock(&light_flash_mutex);
    }
}

void* LightCtlFun(void* args)
{
    int ret = 0;

    prctl(PR_SET_NAME, "light_ctl");
    light_init();

    pthread_t flash_thread;
    if (pthread_create(&flash_thread, NULL, light_flash_thread, NULL)) {
        ERROR("thread create failed.");
        return NULL;
    }

    int msgid = msgget(0x4C494748, IPC_CREAT | IPC_EXCL | 0666);
    if (msgid == -1) {
        if (errno == EEXIST) {
            msgid = msgget(0x4C494748, IPC_EXCL | 0666);
        } else {
            ERROR("msgget failed. %s", strerror(errno));
            return NULL;
        }
    }

    int msg_size = 0;
    struct light_ctl_mq_msg msg = {0};
	while (1) {
		// TODO this may cause segv. comment it.
		//memset(&msg, 0x00, sizeof(struct light_ctl_mq_msg));

		/**
		 * LIGHT_CTL_TYPE_GET and LIGHT_CTL_TYPE_SET
		 * but **NOT** LIGHT_CTL_TYPE_GET_RESP
		 */
		size_t msg_len = sizeof(struct light_ctl_mq_msg) - sizeof(long);
		msg_size = msgrcv(msgid, &msg, msg_len,
				LIGHT_CTL_TYPE_GET_RESP, MSG_EXCEPT);
		if (msg_size < 0) {
			ERROR("msgrcv failed. %s", strerror(errno));
			sleep(1);
			continue;
		}

		switch (msg.type) {
		case LIGHT_CTL_TYPE_GET: {
			light_ctl_msg_t *active_light = get_current_light_state();
			memset(&msg, 0x00, sizeof(struct light_ctl_mq_msg));
			msg.type = LIGHT_CTL_TYPE_GET_RESP;
			memcpy(&(msg.msg_data), active_light, sizeof(light_ctl_msg_t));
			ret = msgsnd(msgid, &msg,
					     sizeof(struct light_ctl_mq_msg) - sizeof(long), 0);
			if (ret == -1) {
				ERROR("msgsnd failed %s", strerror(errno));
			}
			break;
		}
		case LIGHT_CTL_TYPE_SET:
			put_current_light_state(&(msg.msg_data));
			break;
		default:
			ERROR("recv msg type %ld not supported", msg.type);
			break;
		}
	}
    return 0;
}
