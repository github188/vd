#include <string.h>
#include <inttypes.h>
#include "types.h"
#include "mcfw/interfaces/link_api/jpeg_enc_info.h"
#include "ve/dev/roadgate/roadgate_data.h"
#include "sys/time_util.h"
#include "logger/log.h"
#include "toggle/dsp/dsp_config.h"


C_CODE_BEGIN

void rg_status_sendto_dsp(int_fast32_t status)
{
	EntranceControlInput dsp;

	memset(&dsp, 0, sizeof(dsp));
	dsp.roadBrakeStatus = status;

	send_dsp_msg(&dsp, sizeof(dsp), 4);

	DEBUG("Send road gate status to dsp done @ %s, status = %"PRIdFAST32,
		  ustime_msec(), status);
}

void detect_coil_sendto_dsp(int_fast32_t status)
{
	EntranceControlInput dsp;

	memset(&dsp, 0, sizeof(dsp));

	dsp.contorlMode = 2;
	dsp.coilSensorStatus = status;

	send_dsp_msg(&dsp, sizeof(dsp), 4);

	DEBUG("Send detect coil status to dsp done @ %s, status = %"PRIdFAST32,
		  ustime_msec(), status);
}

void safe_coil_sendto_dsp(int_fast32_t status)
{
	EntranceControlInput input;

	memset(&input, 0, sizeof(input));
	input.antiCoilSensorStatus = status;
	input.contorlMode = 2;

	send_dsp_msg(&input, sizeof(input), 4);
	DEBUG("Send safe coil status to dsp done @ %s, status = %"PRIdFAST32,
		  ustime_msec(), status);
}


void ytbrk_rg_status_sendto_dsp(int_fast32_t status)
{
	EntranceControlInput dsp;

	memset(&dsp, 0, sizeof(dsp));
	dsp.roadBrakeStatus = status;

	send_dsp_msg(&dsp, sizeof(dsp), 4);

	DEBUG("Send detect coil status to dsp done @ %s, status = %"PRIdFAST32,
		  ustime_msec(), status);

}

C_CODE_END
