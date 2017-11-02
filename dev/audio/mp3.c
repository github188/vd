#include <stdio.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
#include <limits.h>
#include <semaphore.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include "sys/config.h"
#include "sys/time_util.h"
#include "sys/heap.h"
#include "list.h"
#include "mad.h"
#include "pcm.h"
#include "logger/log.h"
#include "audio_data.h"
#include "sys/xtimer.h"
#include "mp3.h"

C_CODE_BEGIN


#define MP3_BUF_SIZE	(1024 * 1024)

typedef struct mp3_player {
	list_head_t ready;
	pthread_t tid;
	pthread_mutex_t mutex;
	sem_t sem;
	bool stop;
	bool fault;
	uint32_t cnt;
} mp3_player_t;

typedef struct mp3_item {
	list_head_t link;
	const audio_param_t *ap;
} mp3_item_t;

/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct buffer {
	const uint8_t *start;
	size_t length;
	bool init;
	mp3_player_t *player;
	snd_pcm_t *pcm;
	pcm_info_t pcm_info;
	uint8_t *pcm_buf;
	uint8_t *pcm_wp;
};

static int_fast32_t mp3_player_once(void);

static mp3_player_t mp3_player;

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static enum mad_flow input(void *data, struct mad_stream *stream)
{
	struct buffer *buffer = data;

	if (!buffer->length) {
		return MAD_FLOW_STOP;
	}

	mad_stream_buffer(stream, buffer->start, buffer->length);

	buffer->length = 0;

	return MAD_FLOW_CONTINUE;
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline uint_fast16_t scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE) {
		sample = MAD_F_ONE - 1;
	} else if (sample < -MAD_F_ONE) {
		sample = -MAD_F_ONE;
	}

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/**
 * decode_put_pcm - Put pcm data while decoding
 *
 * @author cyj (2016/8/10)
 *
 * @param data
 * @param sample
 *
 * @return int_fast32_t
 */
static void decode_put_pcm(struct buffer *data, uint_fast32_t sample)
{
	*data->pcm_wp++ = (uint8_t)(sample & 0xFF);
	*data->pcm_wp++ = (uint8_t)((sample & 0xFF00) >> 8);

	if (data->pcm_wp == (data->pcm_buf + data->pcm_info.chunk_bytes)) {
		pcm_writei(data->pcm, data->pcm_buf, data->pcm_info.chunk_size,
				   &data->pcm_info);
		data->pcm_wp = data->pcm_buf;
	}
}

/**
 * decode_done_play_pcm - Play the last frames when decode done
 *
 * @author cyj (2016/8/10)
 *
 * @param data
 */
static void decode_done_play_pcm(struct buffer *data)
{
	size_t n = (size_t)(data->pcm_wp - data->pcm_buf);
	uint_fast32_t nfrm = BYTES2FRAMES(n, data->pcm_info.bits_per_frame);

	pcm_writei(data->pcm, data->pcm_buf, nfrm, &data->pcm_info);
	free(data->pcm_buf);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

static enum mad_flow output(void *data,
							struct mad_header const *hdr,
							struct mad_pcm *pcm)
{
	uint_fast32_t nchannels, samplerate, nsamples;
	const mad_fixed_t *left_ch, *right_ch;
	struct buffer *buffer = data;
	mp3_player_t *player = buffer->player;

	nchannels = pcm->channels;
	nsamples = pcm->length;
	left_ch = pcm->samples[0];
	right_ch = pcm->samples[1];
	samplerate = pcm->samplerate;

	if (!buffer->init) {
		pcm_param_t pcm_param;

		pcm_param.rate = samplerate;
		pcm_param.nr_chn = nchannels;
		pcm_param.fmt = SND_PCM_FORMAT_S16_LE;

		if(0 != pcm_set_param(buffer->pcm, &buffer->pcm_info, &pcm_param)){
			return MAD_FLOW_STOP;
		}

		buffer->pcm_buf = (uint8_t *)xmalloc(buffer->pcm_info.chunk_bytes);
		if (!buffer->pcm_buf) {
			return MAD_FLOW_STOP;
		}
		buffer->pcm_wp = buffer->pcm_buf;

		buffer->init = true;
	}

	while (nsamples--) {
		/* output sample(s) in 16-bit signed little-endian PCM */
		uint_fast16_t sample = scale(*left_ch++);
		decode_put_pcm(buffer, sample);

		if (2 == nchannels) {
			sample = scale(*right_ch++);
			decode_put_pcm(buffer, sample);
		}
	}

	/*
	 * Get stop flag
	 */
	pthread_mutex_lock(&player->mutex);
	bool stop = player->stop;
	pthread_mutex_unlock(&player->mutex);

	return stop ? MAD_FLOW_STOP : MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

static enum mad_flow error(void *data, struct mad_stream *stream,
						   struct mad_frame *frame)
{
	struct buffer *buffer = data;
	ERROR("this is mad_flow error");
	ERROR("decoding error 0x%"PRIX32" (%s) at byte offset %"PRIuPTR,
		  stream->error, mad_stream_errorstr(stream),
		  (uintptr_t)(stream->this_frame - buffer->start));

	/* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

	return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

static int_fast32_t decode(mp3_player_t *player, snd_pcm_t *pcm,
						   const uint8_t *start, size_t length)
{
	struct buffer buffer;
	struct mad_decoder decoder;
	int_fast32_t result;

	/* initialize our private message structure */

	buffer.start = start;
	buffer.length = length;
	buffer.pcm = pcm;
	buffer.init = false;
	buffer.player = player;

	/* configure input, output, and error functions */

	mad_decoder_init(&decoder,
					 &buffer,
					 input,
					 NULL,		/* Header */
					 NULL,		/* filter */
					 output,
					 error,
					 NULL);		/* message */

	/* start decoding */

	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	decode_done_play_pcm(&buffer);

	/* release the decoder */

	mad_decoder_finish(&decoder);

	return result;
}

int_fast32_t mp3_play_file(mp3_player_t *player, const mp3_item_t *item)
{
	return 0;

	int_fast32_t result;
	snd_pcm_t *pcm = NULL;
	void *fdm = MAP_FAILED;
	struct stat stat;
	const audio_param_t *ap = item->ap;
	int fd = -1;

	ASSERT(ap);
	ASSERT((ap->volume >= 0) && (ap->volume <= 100));

	do {
		result = -1;

		fd = open(ap->file, O_RDONLY);
		if (-1 == fd) {
			int _errno = errno;
			ERROR("Open %s file failed:%s", ap->file, strerror(_errno));
			break;
		}

		if ((-1 == fstat(fd, &stat)) || (0 == stat.st_size)) {
			int _errno = errno;
			ERROR("fstat %s failed, %s", ap->file, strerror(_errno));
			break;
		}

		fdm = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (MAP_FAILED == fdm) {
			int _errno = errno;
			ERROR("Map failed:%s", strerror(_errno));
			break;
		}

		if (0 != pcm_open(&pcm, "default")) {
			break;
		}

		snd_pcm_nonblock(pcm, 0);

		if(0 != mixer_set_volume(ap->volume)){
			break;
		}

		if (0 != decode(player, pcm, fdm, stat.st_size)) {
			break;
		}

		result = 0;
	} while (0);

	if (MAP_FAILED != fdm) {
		munmap(fdm, stat.st_size);
	}

	if (-1 != fd) {
		close(fd);
	}

	if (pcm) {
		pcm_close(pcm);
	}
	snd_config_update_free_global();

	return result;
}

int_fast32_t mp3_play_buf(mp3_player_t *player, const mp3_item_t *item)
{
	int_fast32_t result;
	snd_pcm_t *pcm = NULL;
	const audio_param_t *ap = item->ap;

	ASSERT((ap->volume >= 0) && (ap->volume <= 100));

	do {
		result = -1;

		if (0 != pcm_open(&pcm, "default")) {
			break;
		}

		snd_pcm_nonblock(pcm, 0);

		if(0 != mixer_set_volume(ap->volume)){
			break;
		}

		if (0 != decode(player, pcm, ap->buf, ap->len)) {
			break;
		}

		result = 0;
	} while (0);

	if (pcm) {
		pcm_close(pcm);
	}
	snd_config_update_free_global();

	return result;
}

int_fast32_t mp3_play(mp3_player_t *player, const mp3_item_t *item)
{
	if (item->ap->flag & AUDIO_FILE) {
		mp3_play_file(player, item);
	} else if (item->ap->flag & AUDIO_BUF) {
		mp3_play_buf(player, item);
	}

	return 0;
}

static void fault_callback(union sigval sig)
{
	int_fast32_t ret;
	mp3_player_t *player = (mp3_player_t *)sig.sival_ptr;
	ASSERT(player != NULL);

#if MP3_PALY_BY_MPLAY

	(void)ret;
	CRIT("MP3 play occur fault at %"PRIu32" times, kill mplay", player->cnt);
	system("killall -9 mplay");

#else

	CRIT("MP3 play occur fault at %"PRIu32" times, pause it", player->cnt);

	if ((ret = pthread_mutex_lock(&player->mutex)) != 0) {
		ERROR("Mutex lock failed, %s", strerror(ret));
		return;
	}

	player->fault = true;

	if ((ret = pthread_mutex_unlock(&player->mutex)) != 0) {
		ERROR("Mutex unlock failed, %s", strerror(ret));
		return;
	}
#endif
}

#if MP3_PALY_BY_MPLAY
static int_fast32_t mp3_play_by_mplay(mp3_player_t *player,
									  const mp3_item_t *item)
{
	int_fast32_t result = 0;
	const audio_param_t *ap = item->ap;

	ASSERT((ap->volume >= 0) && (ap->volume <= 100));

	/*
	 * Generate mp3 file's path
	 */
	uuid_t uuid;
	char uuid_str[36];
	char path[PATH_MAX + 1];

	uuid_generate(uuid);
	uuid_unparse_lower(uuid, uuid_str);
	snprintf(path, sizeof(path), "%s/%s.mp3", RUN_DIR, uuid_str);

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		int _errno = errno;
		ERROR("Open %s failed: %s", path, strerror(_errno));
		return -EPERM;
	}

	bool ok = (fwrite(ap->buf, sizeof(uint8_t), ap->len, f) >= ap->len);
	fflush(f);
	fclose(f);

	/*
	 * If write to file success, play it ,otherwise drop this time
	 */
	if(ok){
		char cmd[PATH_MAX + 256];

		snprintf(cmd, sizeof(cmd), MPLAY" -v %"PRIu8" %s", ap->volume, path);
		system(cmd);
	} else {
		ERROR("Write mp3 file %s failed", path);
		result = -EPERM;
	}

	remove(path);

	return result;
}
#endif

static int_fast32_t mp3_play_daemon(mp3_player_t *player)
{
	int_fast32_t ret;

	if(0 != sem_wait(&player->sem)){
		int _errno = errno;
		ERROR("Wait for tts's semaphore failed, %s", strerror(_errno));
		return -EPERM;
	}

	if (0 != (ret = pthread_mutex_lock(&player->mutex))) {
		ERROR("Mutex lock failed, %s", strerror(ret));
		return -EPERM;
	}

	mp3_item_t *item = NULL;

	if (!list_empty(&player->ready)) {
		/* Get the first item */
		item = list_entry(player->ready.next, mp3_item_t, link);
		/* Delte from list */
		list_del(&item->link);
	}

	/* Clear stop flag */
	player->stop = false;

	if (0 != (ret = pthread_mutex_unlock(&player->mutex))) {
		ERROR("Mutex unlock failed, %s", strerror(ret));
		return -EPERM;
	}

	if (item) {
		DEBUG("Play the %"PRIu32" audio", player->cnt++);

		xtimer_t tmr;
		int_fast32_t tmr_ret = xtimer_create(&tmr, fault_callback, player);
		if (tmr_ret == 0) {
			xtimer_settime(&tmr, 30000, 0);
		}

		/* If get mp3 item success, play it */
#if MP3_PALY_BY_MPLAY
		mp3_play_by_mplay(player, item);
#else
		mp3_play(player, item);
#endif

		/*
		 * Free the resources
		 */
		audio_param_free((audio_param_t *)item->ap);
		xfree(item);

		if (tmr_ret == 0) {
			xtimer_del(&tmr);
		}

		/*
		 * Clear fault flag
		 */
		if ((ret = pthread_mutex_lock(&player->mutex)) != 0) {
			ERROR("Mutex lock failed, %s", strerror(ret));
			return -EPERM;
		}

		player->fault = false;

		if ((ret = pthread_mutex_unlock(&player->mutex)) != 0) {
			ERROR("Mutex unlock failed, %s", strerror(ret));
			return -EPERM;
		}
	}

	return 0;
}

static void *mp3_play_thread(void *arg)
{
	mp3_player_t *player = (mp3_player_t *)arg;

	ASSERT(player);

	/* Run once */
	mp3_player_once();

	for(;;) {
		pthread_testcancel();
		if(mp3_play_daemon(player) != 0){
			ERROR("MP3 player daemon run error");
		}
	}

	return NULL;
}

static __inline__ int_fast32_t __mp3_play_put(mp3_player_t *player,
											  const audio_param_t *ap)
{
	mp3_item_t *item;
	int_fast32_t _errno, ret;

	/* Run once */
	mp3_player_once();

	if(0 != (ret = pthread_mutex_lock(&player->mutex))){
		ERROR("Mutex lock failed, %s", strerror(ret));
		return -1;
	}

	/*
	 * Check if the player is fault
	 */
	if (player->fault) {
		/*
		 * If occur fault, exit
		 */
		audio_param_free((audio_param_t *)ap);
		if ((ret = pthread_mutex_unlock(&player->mutex)) != 0) {
			ERROR("Mutex unlock failed, %s", strerror(ret));
			return -EPERM;
		}

		return -EPERM;
	}

	item = (mp3_item_t *)xmalloc(sizeof(mp3_item_t));
	if (item == NULL) {
		ERROR("Alloc memory for mp3_item_t failed");

		audio_param_free((audio_param_t *)ap);

		if (0 != (ret = pthread_mutex_unlock(&player->mutex))) {
			ERROR("Mutex unlock failed, %s", strerror(ret));
			return -EPERM;
		}

		return -ENOBUFS;
	}

	item->ap = ap;

	list_add_tail(&item->link, &player->ready);

	if(0 != (ret = pthread_mutex_unlock(&player->mutex))){
		ERROR("Mutex unlock failed, %s", strerror(ret));
		return -1;
	}

	if(0 != sem_post(&player->sem)){
		_errno = errno;
		ERROR("MP3 player's semphore post failed, %s", strerror(_errno));
		return -1;
	}

	return 0;
}

static __inline__ int_fast32_t __mp3_player_ram_init(mp3_player_t *player)
{
	int_fast32_t ret, _errno;

	INIT_LIST_HEAD(&player->ready);
	player->fault = false;
	player->cnt = 0;

	if (0 != (ret = pthread_mutex_init(&player->mutex, NULL))) {
		ERROR("Initialize pthread mutex failed:%s", strerror(ret));
		return -1;
	}

	if (0 != sem_init(&player->sem, 0, 0)) {
		_errno = errno;
		ERROR("Initialize semaphore failed:%s", strerror(_errno));
		return -1;
	}

	return 0;
}

static int_fast32_t __mp3_player_stop(mp3_player_t *player)
{
	int_fast32_t ret;

	if(0 != (ret = pthread_mutex_lock(&player->mutex))){
		ERROR("Mutex lock failed, %s", strerror(ret));
		return -1;
	}

	player->stop = true;

	if(0 != (ret = pthread_mutex_unlock(&player->mutex))){
		ERROR("Mutex unlock failed, %s", strerror(ret));
		return -1;
	}

	return 0;
}

static int_fast32_t __mp3_player_destroy(mp3_player_t *player)
{
	DEBUG("Mp3 player destroying");

	pthread_cancel(player->tid);
	pthread_join(player->tid, NULL);
	sem_destroy(&player->sem);
	pthread_mutex_destroy(&player->mutex);

	list_head_t *pos, *n;
	list_for_each_safe(pos, n, &player->ready){
		/* Get the first item */
		mp3_item_t *item = list_entry(pos, mp3_item_t, link);
		/* Delte from list */
		list_del(&item->link);
		audio_param_free((audio_param_t *)item->ap);
		xfree(item);
	}

	DEBUG("Mp3 player destroyed");

	return 0;
}

static void mp3_player_ram_init(void)
{
	if (0 != __mp3_player_ram_init(&mp3_player)) {
		exit(1);
	}
}

static int_fast32_t mp3_player_once(void)
{
	int_fast32_t ret;
	static pthread_once_t once = PTHREAD_ONCE_INIT;

	if(0 != (ret = pthread_once(&once, mp3_player_ram_init))){
		ERROR("pthread_once failed @ (%s:%d): %s",
			  __FILE__, __LINE__, strerror(ret));
		return -1;
	}

	return 0;
}


static __inline__ int_fast32_t __mp3_player_init(mp3_player_t *player)
{
	int_fast32_t ret;

	if(0 != (ret = pthread_create(&player->tid, NULL,
								  mp3_play_thread, player))){
		ERROR("Create pthread failed, %s", strerror(ret));
		return -1;
	}

	return 0;
}

int_fast32_t mp3_player_init(void)
{
	return __mp3_player_init(&mp3_player);
}

int_fast32_t mp3_play_put(const audio_param_t *ap)
{
	return __mp3_play_put(&mp3_player, ap);
}

int_fast32_t mp3_set_volume(int_fast32_t volume)
{
	return mixer_set_volume(volume);
}

int_fast32_t mp3_player_stop(void)
{
	return __mp3_player_stop(&mp3_player);
}

int_fast32_t mp3_player_destroy(void)
{
	return __mp3_player_destroy(&mp3_player);
}


C_CODE_END
