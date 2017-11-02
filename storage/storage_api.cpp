
#include "storage_common.h"
#include "storage_api.h"
#include "disk_mng.h"
#include "file_system_server.h"
#include "data_process.h"


/*******************************************************************************
 * 函数名: storage_module_init
 * 功  能: 初始化存储管理模块
 * 返回值: 成功，返回0；失败，返回-1
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
 * 函数名: storage_module_destroy
 * 功  能: 销毁存储管理模块
 * 返回值:
*******************************************************************************/
int storage_module_destroy(void)
{
	log_debug_storage("Destroy storage module now.\n");

	fs_server_delete();

	log_debug_storage("Destroy storage module done.\n");

	return 0;
}


/*******************************************************************************
 * 函数名: algorithm_results_process
 * 功  能: 处理算法传来的结果
 * 参  数: image_info，图像信息；video_info，视频信息
 * 返回值: 数据处理函数
*******************************************************************************/
//int algorithm_results_process(const void *image_info, const void *video_info)
//{
//	return data_process(image_info, video_info);
//}
