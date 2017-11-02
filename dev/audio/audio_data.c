#include "audio_data.h"
#include "sys/heap.h"
#include "logger/log.h"

C_CODE_BEGIN


audio_param_t *audio_param_alloc(void)
{
	return (audio_param_t *)xmalloc(sizeof(audio_param_t));
}

void audio_param_free(audio_param_t *ptr)
{
	xfree(ptr);
}



C_CODE_END
