#ifndef _VOCODER_DRIVER_H_
#define _VOCODER_DRIVER_H_

/////////////////////////////////////////////////////////////////////

#include "types.h"
#include "pipe.h"

/////////////////////////////////////////////////////////////////////

// 160 is the usual decoded size for 8kbps
#define DECODED_SIZE_8KBPS      160
#define MAX_ENCODED_SIZE        64
#define ENCODED_SIZE_8KHZ       20

typedef enum
{
    VOCODER_MODE_NONE,
    VOCODER_MODE_SPEEX,
    VOCODER_MODE_BYPASS,
} vocoder_mode_t;

/////////////////////////////////////////////////////////////////////

typedef void * vocoder_t;

/////////////////////////////////////////////////////////////////////

vocoder_t vocoder_start( uint8_t vocoder_mode, uint16_t pipe_size, uint16_t decode_threshold );

void vocoder_audio_stop( vocoder_t pp_data );

pipe_data_t * vocoder_decoder_get_pipe( vocoder_t pp_data );
pipe_data_t * vocoder_encoder_get_pipe( vocoder_t pp_data );

/////////////////////////////////////////////////////////////////////

#endif // _VOCODER_DRIVER_H_

