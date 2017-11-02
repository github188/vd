#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "ve_cfg.h"
#include "json_cfg.h"
#include "logger/log.h"
#include "json_format.h"
#include "commonfuncs.h"
#include "sys/xfile.h"
#include "sys/heap.h"

C_CODE_BEGIN

typedef struct ve_cfg_mng {
#ifdef __linux__
	pthread_mutex_t mutex;
#endif
	ve_cfg_t cfg;
	char profile[256];
} ve_cfg_mng_t;

static json_cfg_item_t root_entry[] = {
	{
		.field = "global",
		.get = global_cfg_read_new,
		.set = global_cfg_write_new
	},
	{
		.field = "device",
		.get = dev_cfg_read_new,
		.set = dev_cfg_write_new
	},
	{
		.field = "dsp",
		.get = dsp_cfg_read_new,
		.set = dsp_cfg_write_new
	},
	{
		.field = "plateform",
		.get = pfm_cfg_read_new,
		.set = pfm_cfg_write_new
	},
};

static ve_cfg_mng_t ve_cfg_mng;

static void ve_cfg_init(ve_cfg_t *vc)
{
	dsp_cfg_init(&vc->dsp);
	global_cfg_init(&vc->global);
	dev_cfg_init(&vc->dev);
	pfm_cfg_init(&vc->pfm);
}

static __inline__ int32_t mutex_pend(ve_cfg_mng_t *cm)
{
#ifdef __linux__
	return pthread_mutex_lock(&cm->mutex);
#else
	return 0;
#endif
}

static __inline__ int32_t mutex_post(ve_cfg_mng_t *cm)
{
#ifdef __linux__
	return pthread_mutex_unlock(&cm->mutex);
#else
	return 0;
#endif
}

static __inline__ int32_t mutex_init(ve_cfg_mng_t *cm)
{
#ifdef __linux__
	return pthread_mutex_init(&cm->mutex, NULL);
#else
	return 0;
#endif
}

static int32_t __ve_cfg_read_new(ve_cfg_mng_t *cm, const char *json)
{
	json_object *root;
	json_cfg_item_t *item;
	json_cfg_t json_cfg;

	root = json_tokener_parse(json);
	if (is_error(root)) {
		ERROR("Failed to initialize json object, error: %"PRIdPTR,
			  (uintptr_t)root);
		return -1;
	}

	json_cfg_init(&json_cfg, root_entry, numberof(root_entry));

	mutex_pend(cm);

	json_object_object_foreach(root, key, val) {
		item = json_cfg_find(&json_cfg, key);
		if (item) {
			ASSERT(item->get);
			item->get(val, &cm->cfg);
		}
	}

	mutex_post(cm);

	json_object_put(root);

	return 0;

}

static ssize_t __ve_cfg_write_new(ve_cfg_mng_t *cm, char *buf, size_t size)
{
	ssize_t result;

	json_object *root = json_object_new_object();
	if (NULL == root) {
		CRIT("Alloc memory for json object failed!");
		return -1;
	}

	json_cfg_t json_cfg;
	json_cfg_init(&json_cfg, root_entry, numberof(root_entry));

	mutex_pend(cm);

	for (uint_fast16_t i = 0; i < numberof(root_entry); ++i) {
		json_cfg_item_t *item = json_cfg_find(&json_cfg, root_entry[i].field);
		if (item) {
			ASSERT(item->set);
			if (item->set) {
				json_object *obj = item->set(&cm->cfg);
				if (obj) {
					json_object_object_add(root, item->field, obj);
				}
			}
		}

	}

	mutex_post(cm);

	result = strlen(json_object_to_json_string(root));
	if ((result + 1) <= size) {
		memcpy(buf, json_object_to_json_string(root), result);
		buf[result] = 0;
	} else {
		ERROR("Json string buffer is too small, still need %d bytes",
			  (int32_t)(result + 1 - size));
		result = -1;
	}

	json_object_put(root);

	return result;
}

static __inline__ void __ve_cfg_cpy(ve_cfg_t *dest, const ve_cfg_t *src)
{
	memcpy(dest, src, sizeof(*dest));
}

static __inline__ int32_t __ve_cfg_ram_init(ve_cfg_mng_t *cm,
											const char *profile)
{
	/* Initialize by default parameter */
	ve_cfg_init(&cm->cfg);
	/* Set profile path */
	snprintf(cm->profile, sizeof(cm->profile), "%s", profile);
	/* Initialize mutex */
	return mutex_init(cm);
}

static __inline__ void __ve_cfg_set(ve_cfg_mng_t *cm, const ve_cfg_t *cfg)
{
	mutex_pend(cm);
	__ve_cfg_cpy(&cm->cfg, cfg);
	mutex_post(cm);
}

static __inline__ void __ve_cfg_get(ve_cfg_mng_t *cm, ve_cfg_t *cfg)
{
	mutex_pend(cm);
	__ve_cfg_cpy(cfg, &cm->cfg);
	mutex_post(cm);
}

static int_fast32_t __ve_cfg_load(ve_cfg_mng_t *cm)
{
	ssize_t size = get_file_size(cm->profile);
	if (size < 0) {
		return -EPERM;
	}

	/* Add a byte for EOF */
	char *buf = (char *)xmalloc(size + 1);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	FILE *f = fopen(cm->profile, "r");
	if (f == NULL) {
		ERROR("Failed to open ve profile, %s", cm->profile);
		xfree(buf);
		return -EPERM;
	}

	size_t l = fread(buf, sizeof(char), size, f);
	fclose(f);

	buf[l] = 0;
	int_fast32_t result = __ve_cfg_read_new(cm, buf);
	xfree(buf);

	return result;
}

/**
 * @brief Load ve configures 
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_load(void)
{
	return __ve_cfg_load(&ve_cfg_mng);
}

static int32_t __ve_cfg_dump(ve_cfg_mng_t *cm)
{
	char bak_path[PATH_MAX + 1];

	snprintf(bak_path, sizeof(bak_path), "%s.bak", cm->profile);

	if (access(cm->profile, F_OK) == 0) {
		/* If the profile is exist, backup it */
		rename(cm->profile, bak_path);
	}

	FILE *f = fopen(cm->profile, "w");
	if (NULL == f) {
		ERROR("Open ve profile(%s) failed!", cm->profile);
		return -1;
	}

	size_t size = 1024 * 1024;
	ssize_t len = -1;
	char *buf = (char *)xmalloc(size);
	if (buf) {
		__ve_cfg_write_new(cm, buf, size);
		len = json_format_stream(buf, 4, f);
		fflush(f);
		fsync(fileno(f));
		fclose(f);
		xfree(buf);
	} else {
		CRIT("Alloc memory for ve config's json failed");
	}

	if (0 == access(bak_path, F_OK)) {
		if (len > 0) {
			remove(bak_path);
		} else {
			/*
			 * Dump to profile failed, resume it
			 */
			ERROR("Dump to %s failed, resume it", cm->profile);
			rename(bak_path, cm->profile);
		}
	}

	return 0;
}

/**
 * @brief Dump ve configures 
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_dump(void)
{
	return __ve_cfg_dump(&ve_cfg_mng);
}

static __inline__ int32_t __ve_cfg_send_to_dsp(ve_cfg_mng_t *cm)
{
	size_t size = 1024 * 1024;
	char *buf = (char *)xmalloc(size);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	int32_t result = -1;
	ssize_t len = __ve_cfg_write_new(cm, buf, size);
	if (len > 0) {
		result = send_dsp_msg(buf, len + 1, 12);
	}
	xfree(buf);

	return result;
}

/**
 * @brief Send ve configure to dsp core
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_send_to_dsp(void)
{
	return __ve_cfg_send_to_dsp(&ve_cfg_mng);
}

/**
 * @brief Set new configure
 * 
 * @author cyj (2016/7/20)
 * 
 * @param cfg New configure 
 */
void ve_cfg_set(const ve_cfg_t *cfg)
{
	__ve_cfg_set(&ve_cfg_mng, cfg);
}

/**
 * @brief Get new configure
 * 
 * @author cyj (2016/7/20)
 * 
 * @param cfg 
 */
void ve_cfg_get(ve_cfg_t *cfg)
{
	__ve_cfg_get(&ve_cfg_mng, cfg);
}

/**
 * @brief Initialize memory for ve configure
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_ram_init(void)
{
	return __ve_cfg_ram_init(&ve_cfg_mng, VE_CFG_FILE_PATH);
}

C_CODE_END
