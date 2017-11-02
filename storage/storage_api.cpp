
#include "storage_common.h"
#include "storage_api.h"
#include "disk_mng.h"
#include "file_system_server.h"
#include "data_process.h"


/*******************************************************************************
 * ������: storage_module_init
 * ��  ��: ��ʼ���洢����ģ��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int storage_module_init(void)
{
	log_debug_storage("Initializes storage module now.\n");

	if ( (disk_mng_create() == 0) && (fs_server_create() == 0) )
		return 0;

	log_error_storage("Initializes storage module error.\n");

	return -1;
}


/*******************************************************************************
 * ������: storage_module_destroy
 * ��  ��: ���ٴ洢����ģ��
 * ����ֵ:
*******************************************************************************/
int storage_module_destroy(void)
{
	log_debug_storage("Destroy storage module now.\n");

	fs_server_delete();

	log_debug_storage("Destroy storage module done.\n");

	return 0;
}


/*******************************************************************************
 * ������: algorithm_results_process
 * ��  ��: �����㷨�����Ľ��
 * ��  ��: image_info��ͼ����Ϣ��video_info����Ƶ��Ϣ
 * ����ֵ: ���ݴ�����
*******************************************************************************/
//int algorithm_results_process(const void *image_info, const void *video_info)
//{
//	return data_process(image_info, video_info);
//}
