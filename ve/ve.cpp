#include <inttypes.h>
#include <pthread.h>
#include "types.h"
#include "logger/log.h"
#include "ve.h"
#include "sys/prov_city.h"
#include "config/hide_param.h"
#include "config/ve_cfg.h"
#include "config/cam_param.h"
#include "ve/dev/led.h"
#include "ve/dev/roadgate/roadgate.h"
#include "ve/dev/light.h"
#include "ve/dev/light_ports.h"
#include "ve/dev/roadgate/roadgate.h"
#include "platform/bitcom/thread.h"
#include "dev/audio/mp3.h"
#include "dev/audio.h"
#include "event.h"


C_CODE_BEGIN


int32_t ve_init(void)
{
	habitcom_init();
	prov_city_init();
	prov_city_show();

	if(0 != ve_cfg_ram_init()){
		CRIT("Failed to initialize memory for ve profile");
		return -1;
	}

	if (0 != ve_cfg_load()) {
		CRIT("Failed to initialize memory for ve profile");
		return -1;
	}

	if (0 != vip_info_init()) {
		CRIT("Failed to initialize ve vip information");
		return -1;
	}

	if(0 != cam_param_ram_init()){
		CRIT("Failed to initialize camera parameter ram");
		return -1;
	}

	if (0 != ve_audio_ram_init()) {
		CRIT("Failed to initialize audio ram");
		return -1;
	}

	if(0 != ve_led_ram_init()){
		CRIT("Failed to initialize led ram");
		return -1;
	}

	if(0 != ve_led_thread_init()){
		CRIT("Failed to create led thread");
		return -1;
	}

	if (0 != light_thread_init()) {
		CRIT("Failed to initialize light thread.");
		return -1;
	}

	if (roadgate_init() != 0) {
		CRIT("Failed to initialize roadgate thread.");
		return -1;
	}

	if(0 != ve_pfm_bitcom_init()){
		CRIT("Failed to initialize bitcom platform");
		return -1;
	}

	if(0 != cam_param_thread_init()) {
		CRIT("Failed to initialize camera parameter thread");
		return -1;
	}

	if (event_init() != 0) {
		CRIT("Initialize event failed!");
		return -1;
	}

	if (0 != ve_cfg_send_to_dsp()) {
		CRIT("Send ve cfg to dsp failed!");
		return -1;
	}

	return 0;
}

C_CODE_END
