#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

#include "types.h"

C_CODE_BEGIN

#define RUN_DIR	"/var/run/vd"
#define PID_FILE		RUN_DIR"/vd.pid"
#define EVENT_FILE		RUN_DIR"/event.log"
#define VERSION_FILE		RUN_DIR"/version"
#define LOST_DIR		"lost"
#define LOST_DB		"lost.db"

C_CODE_END

#endif
