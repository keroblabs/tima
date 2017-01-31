#ifndef _AUDIO_LAYER_H_
#define _AUDIO_LAYER_H_

////////////////////////////////////////////////////////////////////////////////

#include "types.h"
//#include "device.h"
#include "lists.h"

////////////////////////////////////////////////////////////////////////////////

#define AUDIO_BUFFER_SIZE				160
#define AUDIO_NUM_CHANNELS				2
#define AUDIO_BITS_SAMPLE				16

#define AUDIO_BUFFER_MONO_SIZE			AUDIO_BUFFER_SIZE
#define AUDIO_BUFFER_STEREO_SIZE		(AUDIO_BUFFER_SIZE*AUDIO_NUM_CHANNELS)

#define AUDIO_MODE_STEREO				(1 << 0)
#define AUDIO_MODE_MONO					(1 << 1)
#define AUDIO_MODE_OUTPUT				(1 << 2)
#define AUDIO_MODE_INPUT				(1 << 3)
#define AUDIO_STATUS_READY				(1 << 4)
#define AUDIO_STATUS_STOP				(1 << 5)

////////////////////////////////////////////////////////////////////////////////

typedef struct _audio_data_t_
{
	LIST_HEADER;

	void * audio_device;
    
	int16_t * buffer;
	int16_t * buffer_ready;
	uint32_t sample_rate;

	int32_t gain_N;
	int32_t gain_D;

	uint8_t mask;

} audio_data_t;

////////////////////////////////////////////////////////////////////////////////

audio_data_t * audio_start( uint32_t sample_rate, int16_t * buffer, uint8_t mask );

void audio_stop( audio_data_t * audio );

void audio_set_gain( audio_data_t * audio, int32_t N, int32_t D );

uint32_t audio_codec_cmd( audio_data_t * audio, uint32_t command, uint32_t param );

int16_t * audio_get_buffer( audio_data_t * audio );
bool_t audio_is_ready( audio_data_t * audio );

////////////////////////////////////////////////////////////////////////////////

#endif // _AUDIO_LAYER_H_
