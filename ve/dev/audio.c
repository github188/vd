#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include "audio.h"
#include "types.h"
#include "logger/log.h"
#include "config/ve_cfg.h"
#include "dev/audio/8127.h"
#include "dev/audio/audio_data.h"

C_CODE_BEGIN

typedef struct audio_dev {
	uint8_t model;
	int_fast32_t (*init)(void);
	int_fast32_t (*set_volume)(int_fast32_t volume);
	int_fast32_t (*play)(const audio_param_t *ap);
	int_fast32_t (*stop)(void);
	int_fast32_t (*destroy)(void);
}audio_dev_t;

typedef struct audio {
	const audio_dev_t *dev;
	pthread_mutex_t mutex;
	uint8_t volume;
}audio_t;


static const audio_dev_t audio_dev_tab[] = {
	{
		.model = AUDIO_MODEL_OFF,
		.init = NULL,
		.set_volume = NULL,
		.play = NULL,
		.stop = NULL,
		.destroy = NULL,
	},
	{
		.model = AUDIO_MODEL_LO,
		.init = dm8127_audio_init,
		.set_volume = dm8127_set_volume,
		.play = dm8127_audio_play,
		.stop = dm8127_audio_stop,
		.destroy = dm8127_audio_destory
	},

};

static audio_t audio;

static const audio_dev_t *__audio_dev_search(audio_t *audio,
											 uint_fast8_t model)
{
	ASSERT(audio);

	const audio_dev_t *p = audio_dev_tab;
	const audio_dev_t *end = audio_dev_tab + numberof(audio_dev_tab);

	for ( ; p < end; ++p) {
		if (p->model == model) {
			return p;
		}
	}

	return NULL;
}

static int_fast32_t __audio_dev_change(audio_t *audio, uint_fast8_t model)
{
	int_fast32_t result;

	pthread_mutex_lock(&audio->mutex);

	do {
		result = -1;

		if ((audio->dev) && (audio->dev->model == model)) {
			result = 0;
			break;
		}

		/*
		 * If current device is exist, destory it
		 */
		if (audio->dev && audio->dev->destroy) {
			if(audio->dev->destroy() != 0) {
				ERROR("Audio device %d destory failed", audio->dev->model);
				break;
			}
		}

		if ((audio->dev = __audio_dev_search(audio, model)) == NULL) {
			/* If not find, turn off audio */
			audio->dev = &audio_dev_tab[0];
		}

		if (audio->dev->init) {
			/* Initialize new audo device */
			if(audio->dev->init() != 0) {
				break;
			}
		}

		INFO("Audio card changed: %d", audio->dev->model);

		result = 0;
	} while (0);

	pthread_mutex_unlock(&audio->mutex);

	return result;
}

static int_fast32_t __audio_dev_set_volume(audio_t *audio,
										   uint_fast32_t volume)
{
	pthread_mutex_lock(&audio->mutex);

	audio->volume = volume;
	if (audio->volume > 100) {
		audio->volume = 100;
	}

	INFO("Audio volume changed: %d", audio->volume);

	pthread_mutex_unlock(&audio->mutex);

	if (audio->dev && audio->dev->set_volume) {
		return audio->dev->set_volume(audio->volume);
	}

	return 0;
}

static void __audio_adjust_param(audio_param_t *ap)
{
	ve_cfg_t cfg;
	ve_cfg_get(&cfg);

	if (ap->volume < 0) {
		ap->volume = 0;
	}

	if (ap->volume > cfg.dev.audio.volume) {
		ap->volume = cfg.dev.audio.volume;
	}
}

static int_fast32_t __audio_put(audio_t *audio, const audio_param_t *ap)
{
	int_fast32_t result;

	pthread_mutex_lock(&audio->mutex);

	DEBUG("Put new audio, volume: %"PRId8", "
		  "flag: 0x%02"PRIX8", length: %"PRIuPTR,
		  ap->volume, ap->flag, ap->len);

	/*
	 * If the audio card is off, free the resource, else play the audio
	 */
	if ((audio->dev == NULL) || (audio->dev->model == AUDIO_MODEL_OFF)) {
		audio_param_free((audio_param_t *)ap);
	} else {
		if (audio->dev->play(ap) != 0) {
			ERROR("Audio device %d play failed", audio->dev->model);
			result = -EPERM;
		}
	}

	pthread_mutex_unlock(&audio->mutex);

	return result;
}

static int_fast32_t __audio_stop(audio_t *audio)
{
	int_fast32_t result;

	pthread_mutex_lock(&audio->mutex);

	do {
		result = -1;

		if (AUDIO_MODEL_OFF == audio->dev->model) {
			result = 0;
			break;
		}

		if(0 != audio->dev->stop()) {
			ERROR("Audio device %d play failed", audio->dev->model);
			break;
		}

		result = 0;
	} while (0);

	pthread_mutex_unlock(&audio->mutex);

	return result;
}

static __inline__ int_fast32_t __audio_ram_init(audio_t *audio)
{
	int_fast32_t ret;

	audio->dev = &audio_dev_tab[0];

	if ((ret = pthread_mutex_init(&audio->mutex, NULL)) != 0) {
		ERROR("Initialize pthread mutex failed:%s", strerror(ret));
		return -1;
	}

	return 0;
}

int_fast32_t ve_audio_ram_init(void)
{
	if (__audio_ram_init(&audio) != 0) {
		ERROR("Audio ram initialize failed");
		return -1;
	}

	return 0;
}

int_fast32_t audio_put(const audio_param_t *ap)
{
	return __audio_put(&audio, ap);
}

void audio_adjust_param(audio_param_t *ap)
{
	__audio_adjust_param(ap);
}

int_fast32_t audio_dev_change(uint_fast8_t model)
{
	return __audio_dev_change(&audio, model);
}

int_fast32_t audio_dev_set_volume(uint_fast8_t volume)
{
	return __audio_dev_set_volume(&audio, volume);
}

int_fast32_t audio_stop(void)
{
	return __audio_stop(&audio);
}

C_CODE_END
