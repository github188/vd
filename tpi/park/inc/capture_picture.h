#ifndef __CAPTURE_PICTURE__
#define __CAPTURE_PICTURE__

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Name:        data_transfer_cb
 * Description: callback function used to transfer picture to
 *				platform.
 * Paramaters:  const unsigned char* pic_data: the picture buffer.
 *				const unsigned int pic_size: buffer size.
 * Return:      int
 */
typedef int (*data_transfer_cb)(const unsigned char* pic_data,
		                          const unsigned int pic_size);

/*
 * Name:        do_cap_pic_upload
 * Description: capture picture and send to platform via cb_handler
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
int do_cap_pic_upload(data_transfer_cb cb_handler);

#ifdef __cplusplus
}
#endif
#endif
