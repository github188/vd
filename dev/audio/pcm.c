#include <alsa/asoundlib.h>
#include <inttypes.h>
#include "types.h"
#include "logger/log.h"
#include "pcm.h"

C_CODE_BEGIN

int_fast32_t pcm_open(snd_pcm_t **pcm, const char *name)
{
	int_fast32_t ret;

	ret = snd_pcm_open(pcm, name, SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0) {
		ERROR("Open sound pcm device failed, %s", snd_strerror(ret));
		return -1;
	}

	return 0;
}

int_fast32_t pcm_set_param(snd_pcm_t *pcm, pcm_info_t *info,
						   const pcm_param_t *param)
{
	int_fast32_t ret, result;
	snd_pcm_hw_params_t *hw_param;
	snd_pcm_sw_params_t *sw_param;
	const int_fast32_t start_delay = 0;
	const int_fast32_t stop_delay = 0;

	snd_pcm_hw_params_alloca(&hw_param);
	snd_pcm_sw_params_alloca(&sw_param);

	do {
		result = -1;

		/*
		 * Initizlize the parameter struct
		 */
		ret = snd_pcm_hw_params_any(pcm, hw_param);
		if (ret < 0) {
			ERROR("Initialze sound pcm's parameters failed, %s",
				  snd_strerror(ret));
			break;
		}

		/*
		 * Set access permission
		 */
		ret =  snd_pcm_hw_params_set_access(pcm, hw_param,
											SND_PCM_ACCESS_RW_INTERLEAVED);
		if (ret < 0) {
			ERROR("Set sound pcm's access permission failed, %s",
				  snd_strerror(ret));
			break;
		}

		/*
		 * Set sample format
		 */
		ret = snd_pcm_hw_params_set_format(pcm, hw_param, param->fmt);
		if (ret < 0) {
			ERROR("Set sound pcm's format failed, %s", snd_strerror(ret));
			break;
		}

		/*
		 * Set channels number
		 */
		ret = snd_pcm_hw_params_set_channels(pcm, hw_param, param->nr_chn);
		if (ret < 0) {
			ERROR("Set sound pcm's channel failed, %s", snd_strerror(ret));
			break;
		}

		/*
		 * Set sample rate
		 */
		int dir = 0;
		unsigned int rate = param->rate;
		ret = snd_pcm_hw_params_set_rate_near(pcm, hw_param, &rate, &dir);
		if (ret < 0) {
			ERROR("Set sound pcm's sample rate failed, %s",
				  snd_strerror(ret));
			break;
		}

		unsigned int buf_tm;
		ret = snd_pcm_hw_params_get_buffer_time_max(hw_param, &buf_tm, NULL);
		ASSERT(ret >= 0);
		if (buf_tm > 500000) {
			buf_tm = 500000;
		}

		unsigned int period_tm = buf_tm / 4;
		ret = snd_pcm_hw_params_set_period_time_near(pcm, hw_param,
													 &period_tm, NULL);
		ASSERT(ret >= 0);
		ret = snd_pcm_hw_params_set_buffer_time_near(pcm, hw_param,
													 &buf_tm, NULL);
		ASSERT(ret >= 0);

		/*
		 * Install hardware parameter
		 */
		ret = snd_pcm_hw_params(pcm, hw_param);
		if (ret < 0) {
			ERROR("Install sound pcm's parameters failed, %s",
				  snd_strerror(ret));
			break;
		}

		snd_pcm_uframes_t chunk_size, buf_size;

		snd_pcm_hw_params_get_period_size(hw_param, &chunk_size, NULL);
		snd_pcm_hw_params_get_buffer_size(hw_param, &buf_size);
		if (chunk_size == buf_size) {
			ERROR("Can't use period equal to buffer size (%lu == %lu)",
				  chunk_size, buf_size);
			break;
		}

		snd_pcm_sw_params_current(pcm, sw_param);

		snd_pcm_uframes_t n = chunk_size;
		ret = snd_pcm_sw_params_set_avail_min(pcm, sw_param, n);

		snd_pcm_uframes_t start_threshold, stop_threshold;

		rate = param->rate;

		/*
		 * Set start threshold
		 */
		if (start_delay <= 0) {
			start_threshold = n + (double)rate * start_delay / 1000000;
		} else {
			start_threshold = (double)rate * start_delay / 1000000;
		}
		if (start_threshold < 1) {
			start_threshold = 1;
		}
		if (start_threshold > n) {
			start_threshold = n;
		}
		ret = snd_pcm_sw_params_set_start_threshold(pcm, sw_param,
													start_threshold);
		ASSERT(ret >= 0);

		/*
		 * Set stop threshold
		 */
		if (stop_delay <= 0) {
			stop_threshold = buf_size + (double)rate * stop_delay / 1000000;
		} else {
			stop_threshold = (double)rate * stop_delay / 1000000;
		}
		ret = snd_pcm_sw_params_set_stop_threshold(pcm, sw_param,
												   stop_threshold);
		ASSERT(ret >= 0);

		if (snd_pcm_sw_params(pcm, sw_param) < 0) {
			ERROR("unable to install sw params:");
			break;
		}

		int_fast32_t bits_per_sample;

		bits_per_sample = snd_pcm_format_physical_width(param->fmt);
		info->bits_per_frame = bits_per_sample * param->nr_chn;
		info->nchn = param->nr_chn;
		info->fmt = param->fmt;
		info->chunk_size = chunk_size;
		info->chunk_bytes = chunk_size * BITS2BYTES(info->bits_per_frame);

		result = 0;
	} while (0);

	return result;
}

int_fast32_t pcm_writei(snd_pcm_t *pcm, uint8_t *frm, uint_fast32_t nfrm,
						const pcm_info_t *pcm_info)
{
	int_fast32_t result = 0;

	ASSERT(pcm_info);
	ASSERT(pcm);
	ASSERT(frm);

	if (nfrm < pcm_info->chunk_size) {
		uint8_t *wp = frm + nfrm * BITS2BYTES(pcm_info->bits_per_frame);
		unsigned int n = (pcm_info->chunk_size - nfrm) * pcm_info->nchn;
		snd_pcm_format_set_silence(pcm_info->fmt, wp, n);
		nfrm = pcm_info->chunk_size;
	}

	uint8_t *rp = frm;

	while (nfrm > 0) {
		int_fast32_t ret;
		int_fast32_t writen = snd_pcm_writei(pcm, rp, nfrm);

		if ((writen == -EAGAIN) || (writen >= 0 && ((size_t)writen < nfrm))) {
			if((ret = snd_pcm_wait(pcm, 1000)) < 0) {
				ERROR("Sound pcm wait failed: %s", snd_strerror(ret));
				return -1;
			}
		} else if (writen == -EPIPE) {
			/* EPIPE means underrun */
			if ((ret = snd_pcm_prepare(pcm)) < 0) {
				ERROR("Can't recovery from suspend, prepare failed: %s",
					  snd_strerror(ret));
				return -1;
			}
		} else if (writen == -ESTRPIPE) {
			/* suspend(); */
			while ((ret = snd_pcm_resume(pcm)) == -EAGAIN) {
				/* wait until the suspend flag is released */
				sleep(1);
			}
			if (ret < 0) {
				if ((ret = snd_pcm_prepare(pcm)) < 0) {
					ERROR("Can't recovery from suspend, prepare failed: %s",
						  snd_strerror(ret));
					return -1;
				}
			}
		} else if (writen < 0) {
			ERROR("Sound pcm write error: %s", snd_strerror(writen));
			return -1;
		}

		if (writen > 0) {
			nfrm -= writen;
			result += writen;
			rp += writen * BITS2BYTES(pcm_info->bits_per_frame);
		}
	}

	return result;
}

int_fast32_t pcm_close(snd_pcm_t *pcm)
{
	/* Wait all done */
	snd_pcm_drain(pcm);
	snd_pcm_close(pcm);

	return 0;
}


int_fast32_t mixer_set_volume(int_fast32_t volume)
{
	snd_mixer_t *mixer;
	snd_mixer_elem_t *elem;
	int_fast32_t ret;
	int_fast32_t result = -1;
	bool attached = false;
	bool loaded = false;

	ASSERT((volume >= 0) && (volume <= 100));

	/*
	 * Open mixer
	 */
	if ((ret = snd_mixer_open(&mixer, 0)) < 0) {
		ERROR("Open sound mixer failed, %s", snd_strerror(ret));
		return -1;
	}

	do {

		/* Attach a mixer */
		if ((ret = snd_mixer_attach(mixer, "default")) < 0) {
			ERROR("Attach sound mixer failed, %s", snd_strerror(ret));
			break;
		}

		attached = true;

		/* Register mixer */
		if ((ret = snd_mixer_selem_register(mixer, NULL, NULL)) < 0) {
			ERROR("Register sound mixer failed, %s", snd_strerror(ret));
			break;
		}

		/* Load mixer */
		if ((ret = snd_mixer_load(mixer)) < 0) {
			ERROR("Load mixer failed, %s", snd_strerror(ret));
			break;
		}

		loaded = true;

		for (elem = snd_mixer_first_elem(mixer);
			  elem; elem = snd_mixer_elem_next(elem)) {
			if ((SND_MIXER_ELEM_SIMPLE == snd_mixer_elem_get_type(elem))
				&& snd_mixer_selem_is_active(elem)) {

				const char *name = snd_mixer_selem_get_name(elem);
				const char master[] = "Master";

				if((strlen(name) == (sizeof(master) - 1))
				   && (strcmp(master, name) == 0)) {

					long min, max, v;

					/* Get the volume range */
					snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
					/* Calculate new volume value */
					v = min +(max - min)* volume / 100;

					/* Set playback volume */
					snd_mixer_selem_set_playback_volume_all(elem, v);

				}
			}
		}

		result = 0;
	} while (0);

	if (loaded) {
		snd_mixer_free(mixer);
	}

	if (attached) {
		if ((ret = snd_mixer_detach(mixer, "default")) < 0) {
			ERROR("Detach default mixer failed: %s", snd_strerror(ret));
		}
	}

	snd_mixer_close(mixer);
	snd_config_update_free_global();

	return result;
}

C_CODE_END

