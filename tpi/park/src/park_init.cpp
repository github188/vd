#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "commonfuncs.h"
#include "logger/log.h"
#include "spsystem_api.h"
#include "spzehin_api.h"
#include "interface_alg.h"
#include "tpzehin_common.h"
#include "songli_db_api.h"
#include "park_file_handler.h"
#include "park_platform.h"

using namespace std;

//tpsystem send last records send to dsp
static int tpsystem_sendto_dsp()
{
	str_spzehin_msgreget_table lstr_spzehin_msgreget_table;
	ParkRecordInput park_record_input;
	char lc_values[5][128] = {{0}};

	memset(&lstr_spzehin_msgreget_table, 0, sizeof(str_spzehin_msgreget_table));
	memset(&park_record_input,0,sizeof(ParkRecordInput));

	//send last record message into dsp
	spsystem_select_lastmsg_table(spsystem_db, lc_values);
	park_record_input.flagVehicle = atoi(lc_values[3]);
	strcpy(park_record_input.parkVehicleInfo.strResult, lc_values[1]);
	park_record_input.parkVehicleInfo.objectState = atoi(lc_values[3]);

	park_record_input.parkVehicleInfo.color = atoi(lstr_spzehin_msgreget_table.plate_color);
	park_record_input.parkVehicleInfo.picInfo_park[0].confidence = atoi(lstr_spzehin_msgreget_table.confidence);
	park_record_input.parkVehicleInfo.picInfo_park[1].confidence = atoi(lstr_spzehin_msgreget_table.confidence);

	if(1)
	{

		time_t rec_time=0;
		int ret=get_time_from_log("/mnt/log/status.log", &rec_time);
		if(ret==-1)
		{
			ERROR("get_time_from_log failed\n");
		}
		park_record_input.parkVehicleInfo.picInfo_park[0].pic_time = rec_time;
		park_record_input.parkVehicleInfo.picInfo_park[1].pic_time = rec_time;
		INFO("park leave get_time_from_log : %d\n",rec_time);

		park_record_input.pic_time = rec_time;
		park_record_input.pic_ms = 0;
	}

	INFO(" to send_dsp_msg: flagVehicle=%d,strResult=%s,objectState=%d",
			park_record_input.flagVehicle,
			park_record_input.parkVehicleInfo.strResult,
			park_record_input.parkVehicleInfo.objectState	);


	send_dsp_msg(&park_record_input, sizeof(park_record_input), 6);

	return 0;
}

void park_init(void)
{
	//init park system database
	create_multi_dir("/home/records/park/system/database/");
	create_multi_dir("/home/records/park/system/picture/");
	spsystem_open("/home/records/park/system/database/spsystem.db");
	spsystem_create_picture_table(spsystem_db);
	spsystem_create_config_table(spsystem_db);
	spsystem_create_lastmsg_table(spsystem_db);

	//tpsystem send last records send to dsp
	tpsystem_sendto_dsp();
}

void park_monitor(void)
{
	char values[5][128] = {{0}};
	char cmd[128] = {0};
	int count = spsystem_count_picreget_table(spsystem_db);
	if(count > 0)
	{
		int ret = spsystem_select_picture_table(spsystem_db, values, 1);
		if(ret > 0)
		{
			spsystem_delete_picture_table(spsystem_db, ret);

			//delete the picture
			sprintf(cmd, "rm /home/records/park/system/picture/%s -f", values[1]);
			system(cmd);
		}
	}
}
