#include <string.h>
#include <new>
#include "Appro_interface.h"
#include "logger/log.h"
#include "capture_picture.h"

/*
 * Name:        do_cap_pic_upload
 * Description: capture picture and send to platform via cb_handler
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
int do_cap_pic_upload(data_transfer_cb cb_handler)
{
    do
    {
        AV_DATA av_data = {0};
        int ret = -1;

        if (GetAVData(AV_OP_GET_MJPEG_SERIAL, -1, &av_data) != RET_SUCCESS)
        {
            ERROR("Can't get JPEG image\n");
            return ret;
        }

        DEBUG("serial: %u\n", av_data.serial);

        if (GetAVData(AV_OP_LOCK_MJPEG, av_data.serial, &av_data) != RET_SUCCESS)
        {
            ERROR("lock jpeg %d failed\n", av_data.serial);
            return ret;
        }

		unsigned char * pic_data = NULL;
		try {
			 pic_data = new unsigned char[av_data.size];
		} catch (const std::bad_alloc& e) {
			ERROR(e.what());
			return ret;
		}

		memset(pic_data, 0, sizeof(unsigned char) * av_data.size);
		memcpy(pic_data, av_data.ptr, sizeof(unsigned char) * av_data.size);
        GetAVData(AV_OP_UNLOCK_MJPEG, av_data.serial, &av_data);

		// Picture capture finished, it's time to transfer the picture data.
		if (NULL == cb_handler) {
			ERROR("callback handler is null.");
			delete [] pic_data;
			return ret;
		}

		cb_handler(pic_data, av_data.size);
		delete [] pic_data;
	} while (0);
	return 0;
}
