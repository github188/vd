/*
 * This file includes the control functions for parking lock.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "logger/log.h"
#include "park_lock_control.h"

static pthread_mutex_t parking_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int flag_parking_lock;//地锁使用标志
static int get_cmd_info(const char *cmd, char *result);

extern park_lock_operation_t *g_park_lock_operation;

static int parking_lock_lock(void);
static int parking_lock_unlock(void);

static park_lock_operation_t parklock_onboard_operation;
int park_lock_onboard_init(void)
{
    g_park_lock_operation = &parklock_onboard_operation;
    parklock_onboard_operation.lock = parking_lock_lock;
    parklock_onboard_operation.unlock = parking_lock_unlock;
    return 0;
}
/*
 * Name:        parking_lock_lock
 * Description: up the parking_lock
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
int parking_lock_lock(void)
{
    INFO("parking_lock_lock");

	pthread_mutex_lock(&parking_lock_mutex);

    char result[64] = {0};
    char cmd[64] = {0};

    if(flag_parking_lock == 1) {
		get_cmd_info("i2cget -y 3 0x38", result);
		int get_value = strtol(result, NULL, 16);
		int set_value = get_value & 0xfe;
		sprintf(cmd, "i2cset -y 3 0x38 0x%x", set_value);
		system(cmd);

		// restore
		usleep(100000);
		set_value = get_value | 0x01;
		sprintf(cmd, "i2cset -y 3 0x38 0x%x", set_value);
		system(cmd);
    } else {
    	system("echo 1 > /sys/class/gpio/gpio24/value");
    	system("echo 0 > /sys/class/gpio/gpio13/value");
    	usleep(1000000);
    	system("echo 1 > /sys/class/gpio/gpio13/value");
    	//system("echo 1 > /sys/class/gpio/gpio24/value");
    }

	pthread_mutex_unlock(&parking_lock_mutex);
	return 0;
}

/*
 * Name:        parking_lock_unlock
 * Description: down the parking_lock
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
int parking_lock_unlock(void)
{
    INFO("parking_lock_unlock");

	pthread_mutex_lock(&parking_lock_mutex);

    char result[64] = {0};
    char cmd[64] = {0};

    if(flag_parking_lock == 1) {
		get_cmd_info("i2cget -y 3 0x38", result);
		int get_value = strtol(result, NULL, 16);
		int set_value = get_value & 0xfd;
		sprintf(cmd, "i2cset -y 3 0x38 0x%x", set_value);
		system(cmd);
		// restore
		usleep(100000);
		set_value = get_value | 0x02;
		sprintf(cmd, "i2cset -y 3 0x38 0x%x", set_value);
		system(cmd);
    } else {
    	system("echo 1 > /sys/class/gpio/gpio13/value");
    	system("echo 0 > /sys/class/gpio/gpio24/value");
    	usleep(1000000);
    	//system("echo 1 > /sys/class/gpio/gpio13/value");
    	system("echo 1 > /sys/class/gpio/gpio24/value");

    }

	pthread_mutex_unlock(&parking_lock_mutex);
	return 0;
}

/*
 * Name:        get_cmd_info
 * Description: the wrapper for popen
 * Paramaters:  cmd -- buffer for command
 *              result -- buffer for result
 * Return:      0 OK
 *              -1 fail
 */
static int get_cmd_info(const char *cmd, char *result)
{
    FILE *fp;
    if ((fp = popen(cmd, "r")) == NULL) {
        pclose(fp);
        return -1;
    }

    if (fgets(result, 64, fp) == NULL) {
        pclose(fp);
        return -1;
    } else {
        int len = strlen(result);
        int j = 0;
        for (j = 0; j < len; j++) {
            if (result[j] == '\n')
                result[j] = 0;
        }

        pclose(fp);
        return 0;
    }
}
