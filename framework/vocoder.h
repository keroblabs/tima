#ifndef _VOCODER_DRIVER_H_
#define _VOCODER_DRIVER_H_

/////////////////////////////////////////////////////////////////////

#include "types.h"

/////////////////////////////////////////////////////////////////////

// 160 is the usual decoded size for 8kbps
#define DECODED_SIZE_8KBPS      160
#define MAX_ENCODED_SIZE        64
#define ENCODED_SIZE_8KHZ       20

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

void * vocoder_init( uint8_t mode );
void * vocoder_speex_init( uint8_t mode );
void vocoder_destroy( void * pp_data );

uint16_t vocoder_encode_voice( void * pp_data, int16_t * input, uint8_t * output );

void vocoder_decode_voice( void * pp_data, uint8_t * input, uint16_t size, int16_t * output );

void vocoder_audio_stop( void * pp_data );

bool_t vocoder_audio_output_process( void * pp_data, uint8_t * buffer, uint16_t size );
uint16_t vocoder_audio_input_process( void * pp_data, uint8_t * buffer, uint16_t max_size );

/////////////////////////////////////////////////////////////////////

#endif // _VOCODER_DRIVER_H_

