#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "8127.h"
#include "types.h"
#include "mp3.h"
#include "list.h"
#include "logger/log.h"
#include "sys/xstring.h"
#include "sys/heap.h"

C_CODE_BEGIN

#define MP3_FILE_DIR	"/opt/ipnc/rc"
#define MP3_FILE_PATH(name)		(MP3_FILE_DIR"/"name)

#define MP3_MAP_FILE	"/mnt/nand/mp3.map"
#define MP3_MAP_HASH_TAB_SIZE		32

#define AUDIO_STR_LEN	64

typedef struct mp3_map {
	hlist_node_t link;
	char str[AUDIO_STR_LEN];
	char file[PATH_MAX + 1];
} mp3_map_t;

typedef struct brd_audio{
	pthread_mutex_t mutex;
	hlist_head_t mp3_map_hash[MP3_MAP_HASH_TAB_SIZE];
} brd_audio_t;

static void mp3_map_free(brd_audio_t *audio);
static void mp3_map_show(brd_audio_t *audio);

static brd_audio_t *brd_audio;

static uintptr_t cal_str_hash_val(const char *s)
{
	uintptr_t value = 0;

	while (*s && s){
		value = value * 37 + (uintptr_t)(*s++);
	}

	return value;
}

static uintptr_t audio_cal_str_hash_pos(const char *s)
{
#if ((MP3_MAP_HASH_TAB_SIZE % 8) == 0)
	return cal_str_hash_val(s) & (MP3_MAP_HASH_TAB_SIZE - 1);
#else
	return cal_str_hash_val(s) % MP3_MAP_HASH_TAB_SIZE;
#endif
}

static void mp3_map_hlist_init(brd_audio_t *audio)
{
	for (uint_fast16_t i = 0; i < numberof(audio->mp3_map_hash); ++i) {
		INIT_HLIST_HEAD(&audio->mp3_map_hash[i]);
	}
}

static int_fast8_t mp3_map_add(brd_audio_t *audio, const mp3_map_t *item)
{
	mp3_map_t *_new = xmalloc(sizeof(mp3_map_t));
	if (!_new) {
		return -1;
	}

	strlcpy(_new->str, item->str, sizeof(_new->str));
	strlcpy(_new->file, item->file, sizeof(_new->file));

	uintptr_t pos = audio_cal_str_hash_pos(item->str);

	hlist_add_head(&_new->link, &audio->mp3_map_hash[pos]);

	return 0;
}

int_fast8_t __mp3_map_search(brd_audio_t *audio, mp3_map_t *map,
									const char *str)
{
	hlist_node_t *pos, *n;
	uintptr_t addr;
	int_fast8_t result = -1;

	pthread_mutex_lock(&audio->mutex);

	addr = audio_cal_str_hash_pos(str);
	hlist_for_each_safe(pos, n, &audio->mp3_map_hash[addr]) {
		mp3_map_t *item = hlist_entry(pos, mp3_map_t, link);
		if (strlen(str) == strlen(item->str)
			&& (strcmp(item->str, str) == 0)) {
			strcpy(map->str, item->str);
			strcpy(map->file, item->file);
			result = 0;
			break;
		}
	}

	pthread_mutex_unlock(&audio->mutex);

	return result;
}

static int_fast8_t mp3_map_item_parse(mp3_map_t *item, const char *s)
{
	const char *space;
	const char *s_end = s + strlen(s);

	if ((space = strchr(s, ' ')) == NULL) {
		return -1;
	}

	while ((*space == ' ') && (space < s_end)) {
		space++;
	}

	if ((*space == '\r') || (*space == '\n') || (space  == s_end)) {
		return -1;
	}

	const char *c;
	char *d, *d_end;

	c = s;
	d = item->str;
	d_end = item->str + sizeof(item->str) - 1;
	while ((*c != ' ') && (d < d_end)) {
		*d++ = *c++;
	}
	*d = '\0';

	c = space;
	d = item->file;
	d_end = item->file + sizeof(item->file) - 1;
	while ((*c != '\r') && (*c != '\n') && (d < d_end)) {
		*d++ = *c++;
	}
	*d = '\0';

	return 0;
}

static int_fast8_t mp3_map_refresh(brd_audio_t *audio, const char *file)
{
	FILE *f = fopen(file, "rt");
	if (!f) {
		ERROR("Open mp3 map file failed, %s", file);
		return -1;
	}

	/* Lock the mutex */
	pthread_mutex_lock(&audio->mutex);

	/* First free old map */
	mp3_map_free(audio);

	while (!feof(f)) {
		/* Txt line is up to 1024 characters */
		char line[1024];

		if (fgets(line, sizeof(line), f)) {
			mp3_map_t map_item;
			if (0 == mp3_map_item_parse(&map_item, line)) {
				mp3_map_add(audio, &map_item);
			}
		}
	}

	fclose(f);
	mp3_map_show(audio);

	/* Unlock the mutex */
	pthread_mutex_unlock(&audio->mutex);

	return 0;
}

static void mp3_map_free(brd_audio_t *audio)
{
	ASSERT(audio);

	uint_fast16_t i;

	for (i = 0; i < numberof(audio->mp3_map_hash); ++i) {
		hlist_node_t *pos,*n;

		hlist_for_each_safe(pos, n, &audio->mp3_map_hash[i]) {
			mp3_map_t *item = hlist_entry(pos, mp3_map_t, link);
			hlist_del(&item->link);
			xfree(item);
		}
	}
}

static void mp3_map_show(brd_audio_t *audio)
{
	ASSERT(audio);

	uint_fast16_t i;

	DEBUG("%-8s%-8s%-24s%s", "addr", "depth", "string", "path");
	for (i = 0; i < numberof(audio->mp3_map_hash); ++i) {
		uint_fast16_t depth = 0;
		hlist_node_t *pos,*n;

		hlist_for_each_safe(pos, n, &audio->mp3_map_hash[i]) {
			mp3_map_t *item = hlist_entry(pos, mp3_map_t, link);
			DEBUG("%-8d%-8d%-24s%s", i, ++depth, item->str, item->file);
		}
	}
}

static __inline__ int_fast8_t __dm8127_audio_init(brd_audio_t **audio)
{
	int_fast32_t ret;

	*audio = xmalloc(sizeof(**audio));
	if (*audio == NULL) {
		ERROR("Alloc memory for audio struct failed");
		return -1;
	}

	ret = pthread_mutex_init(&(*audio)->mutex, NULL);
	if (ret != 0) {
		ERROR("Initialize mutex for 8127 audio failed:%s", strerror(ret));
		xfree(*audio);
		return -1;
	}

	mp3_map_hlist_init(*audio);

	if(0 != mp3_map_refresh(*audio, MP3_MAP_FILE)){
		ERROR("Refresh mp3 mp3 failed");
	}

	if(0 != mp3_player_init()){
		ERROR("Initialize mp3 player failed");
		return -1;
	}

	return 0;
}

static __inline__ int_fast32_t __dm8127_audio_destory(brd_audio_t *audio)
{
	mp3_map_free(audio);
	pthread_mutex_destroy(&audio->mutex);
	xfree(audio);
	mp3_player_destroy();
	return 0;
}

int_fast32_t get_mp3_path(char *path, size_t pathsz, const char *name)
{
	ssize_t l = snprintf(path, pathsz, MP3_FILE_DIR"/%s", name);
	if ((l + 1) > pathsz) {
		return -1;
	}

	return 0;
}

static int_fast32_t __dm8127_audio_play_text(brd_audio_t *audio,
											 const audio_param_t *ap)
{
#if 0
	int_fast32_t num;

	if (1 == sscanf(s, "½»·Ñ%"SCNdFAST32"Ôª", &num)) {
		char name[NAME_MAX + 1], path[PATH_MAX + 1];

		snprintf(name, sizeof(name), "voice_%04"PRIdFAST32".mp3", num);
		get_mp3_path(path, sizeof(path), name);

		if (access(path, F_OK) != 0) {
			int _errno = errno;
			ERROR("Access file '%s' failed:%s", path, strerror(_errno));
			return -1;
		}

		if (access(MP3_FILE_PATH("voice_prompt.mp3"), F_OK) != 0) {
			int _errno = errno;
			ERROR("Access file '%s' failed:%s", path, strerror(_errno));
			return -1;
		}


		//mp3_play_put(MP3_FILE_PATH("voice_prompt.mp3"), volume);
		//mp3_play_put(path, volume);

		return 0;
	}

	mp3_map_t mp3_map;

	if (__mp3_map_search(audio, &mp3_map, s) == 0) {
		if (access(mp3_map.file, F_OK) != 0) {
			int _errno = errno;
			ERROR("Access file '%s' failed:%s",
				  mp3_map.file, strerror(_errno));
			return -1;
		}

		mp3_play_put(mp3_map.file, volume);
		return 0;
	}

	ERROR("Search %s from mp3 map table failed", s);

	return -1;
#endif
	return 0;
}

static __inline__ int_fast8_t __dm8127_audio_play(brd_audio_t *audio,
												  const audio_param_t *ap)
{
	if (ap->flag & AUDIO_TEXT) {
		return __dm8127_audio_play_text(audio, ap);
	} else {
		return mp3_play_put(ap);
	}

	return -1;
}

int_fast32_t dm8127_set_volume(int_fast32_t volume)
{
	return mp3_set_volume(volume);
}

int_fast32_t dm8127_audio_init(void)
{
	return __dm8127_audio_init(&brd_audio);
}

int_fast32_t dm8127_audio_play(const audio_param_t *ap)
{
	return __dm8127_audio_play(brd_audio, ap);
}

int_fast32_t dm8127_audio_stop(void)
{
	return 0;
}

int_fast32_t dm8127_audio_destory(void)
{
	return __dm8127_audio_destory(brd_audio);
}

C_CODE_END
