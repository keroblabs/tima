#include "system.h"
#include "audio_layer.h"
#include "tima_libc.h"
#include "lists.h"

////////////////////////////////////////////////////////////////////////////////

static list_t p_audio_list;
static device_t * p_dev_audio_in;
static device_t * p_dev_audio_out;

////////////////////////////////////////////////////////////////////////////////

void audio_layer_init( void )
{
	p_dev_audio_in = p_dev_audio_out = NULL;
    list_init( &p_audio_list, NULL );
}

audio_data_t * audio_start( uint32_t sample_rate, int16_t * buffer, uint8_t mask )
{
	audio_data_t * p_audio;
	char str_audio_dev[20];

	// allocate memory for audio data
	p_audio = ( audio_data_t * )MMALLOC( sizeof( audio_data_t ) );
	if( p_audio == NULL ) return NULL;

	// setup audio data
	p_audio->buffer = buffer;
	p_audio->sample_rate = sample_rate;
	p_audio->buffer_ready = NULL;
	p_audio->mask = mask;

	p_audio->gain_N = 1;
	p_audio->gain_D = 1;

	p_audio->mask &= ~AUDIO_STATUS_READY;
	p_audio->mask &= ~AUDIO_STATUS_STOP;

	if( p_audio->mask & AUDIO_MODE_STEREO )
	{
		memset( buffer, 0x00, AUDIO_BUFFER_STEREO_SIZE*sizeof( int16_t ) );
	}
	else
	{
		memset( buffer, 0x00, AUDIO_BUFFER_MONO_SIZE*sizeof( int16_t ) );
	}

	// get current audio device, or open new one
	if( p_audio->mask & AUDIO_MODE_OUTPUT )
	{
		if( p_dev_audio_out == NULL )
		{
			sprintk( str_audio_dev, "/dev/audio?%d,out", sample_rate );
			p_dev_audio_out = device_open( str_audio_dev );
		}

		p_audio->audio_device = p_dev_audio_out;
	}
	else
	{
		if( p_dev_audio_in == NULL )
		{
			sprintk( str_audio_dev, "/dev/audio:%d,in", sample_rate );
			p_dev_audio_in = device_open( str_audio_dev );
		}

		p_audio->audio_device = p_dev_audio_in;
	}

	// failed to open audio device
	if( p_audio->audio_device == NULL )
	{
		MFREE( p_audio );
		return NULL;
	}

	// append to the list
	list_insert_tail( &p_audio_list, &p_audio->node );

	return p_audio;
}

void audio_set_gain( audio_data_t * audio, int32_t N, int32_t D )
{
	audio->gain_D = D;
	audio->gain_N = N;
}

uint32_t audio_codec_cmd( audio_data_t * audio, uint32_t command, uint32_t param )
{
	return 0;
}

void audio_stop( audio_data_t * audio )
{
	list_remove( &p_audio_list, &audio->node );
	MFREE( audio );
}

int16_t * audio_layer_get_buffer( void )
{
	int16_t * temp_audio_buffer;

	temp_audio_buffer = ( int16_t * )MMALLOC( AUDIO_BUFFER_STEREO_SIZE*sizeof( int16_t ) );
	if( temp_audio_buffer == NULL ) return NULL;

	return temp_audio_buffer;
}

void audio_layer_release_buffer( int16_t * buffer )
{
	MFREE( buffer );
}

bool_t audio_layer_get_status( device_t * p_dev )
{
	uint32_t status;

	if( p_dev == NULL ) return FALSE;
	status = 0;
	device_ioctrl( p_dev, DEVICE_GET_RUNTIME_VALUE, &status );
	if( status == 0 ) return FALSE;

	return TRUE;
}

void audio_layer_input_process( void )
{
	audio_data_t * p_audio = NULL;
	int16_t * p_sample;
	int16_t * temp_audio_buffer;
	int32_t audio32;

	// check audio driver 
	if( audio_layer_get_status( p_dev_audio_in ) == FALSE ) return;

	// prepare temporary audio buffer
	temp_audio_buffer = audio_layer_get_buffer();
	if( temp_audio_buffer == NULL ) return;

	// read the incoming audio
	device_read_buffer( p_dev_audio_in, ( uint8_t * )temp_audio_buffer, AUDIO_BUFFER_STEREO_SIZE*sizeof( int16_t ) );

    while( ( p_audio = ( audio_data_t * )list_get_next( &p_audio_list, p_audio ? &p_audio->node : NULL ) ) != NULL )
	{
		if( p_audio->mask & AUDIO_MODE_INPUT )
		{
			// get an input instance
			p_sample = p_audio->buffer;
	
			if( p_audio->mask & AUDIO_MODE_STEREO )
			{
				// if the instance is stereo, copy the whole frame
				memcpy( p_sample, temp_audio_buffer, AUDIO_BUFFER_STEREO_SIZE*sizeof( int16_t ) );
			}
			else
			{
				// for mono instances, mix left and right
				for( int cnt = 0; cnt < AUDIO_BUFFER_STEREO_SIZE; cnt += 2 )
				{
					audio32 = ( temp_audio_buffer[cnt] + temp_audio_buffer[cnt+1] ) >> 1;
					*( p_sample ) = audio32;
					p_audio++;
				}
			}

			// set this instance as ready
			p_audio->mask |= AUDIO_STATUS_READY;
			p_audio->buffer_ready = p_audio->buffer;
		}
	}

	audio_layer_release_buffer( temp_audio_buffer );
}

void audio_layer_output_process( void )
{
	audio_data_t * p_audio = NULL;
	bool_t is_first;
	int16_t * p_sample;
	int16_t * temp_audio_buffer;

	// check for audio status
	if( audio_layer_get_status( p_dev_audio_out ) == FALSE ) return;

	// prepare temporary audio buffer
	temp_audio_buffer = audio_layer_get_buffer();
	if( temp_audio_buffer == NULL ) return;

	// prepare audio buffer
	is_first = TRUE;
	memset( temp_audio_buffer, 0x00, AUDIO_BUFFER_STEREO_SIZE*sizeof( int16_t ) );

    while( ( p_audio = ( audio_data_t * )list_get_next( &p_audio_list, p_audio ? &p_audio->node : NULL ) ) != NULL )
	{
		if( p_audio->mask & AUDIO_MODE_OUTPUT )
		{
			// get an output instance
			p_sample = p_audio->buffer;

			// for the first instance in the list, just place the audio onto the buffer (stereo)
			// or duplicate the audio for both channels (mono).
			// for the following instances, mix the audio
			if( is_first )
			{
				// in order to optimize
				for( int sample_cnt = 0; sample_cnt < AUDIO_BUFFER_STEREO_SIZE; sample_cnt += 2 )
				{
					temp_audio_buffer[sample_cnt] = *( p_sample );
					if( p_audio->mask & AUDIO_MODE_STEREO ) p_sample++;
					temp_audio_buffer[sample_cnt+1] = *( p_sample );

					p_sample++;
				}
			}
			else
			{
				for( int sample_cnt = 0; sample_cnt < AUDIO_BUFFER_STEREO_SIZE; sample_cnt += 2 )
				{
					temp_audio_buffer[sample_cnt] = ( *( p_sample ) + temp_audio_buffer[sample_cnt] ) >> 1;
					if( p_audio->mask & AUDIO_MODE_STEREO ) p_sample++;
					temp_audio_buffer[sample_cnt+1] = ( *( p_sample ) + temp_audio_buffer[sample_cnt+1] ) >> 1;

					p_sample++;
				}
			}

			// indicate this instance is processed
			is_first = FALSE;
			p_audio->mask |= AUDIO_STATUS_READY;
			p_audio->buffer_ready = p_audio->buffer;
		}
	}

	// if there was any processed instance, send the audio to the driver
	if( is_first == FALSE )
	{
		device_write_buffer( p_dev_audio_out, ( uint8_t * )temp_audio_buffer, AUDIO_BUFFER_STEREO_SIZE*sizeof( int16_t ) );
	}

	audio_layer_release_buffer( temp_audio_buffer );
}

void audio_layer_process( void )
{
	if( list_is_empty( &p_audio_list ) ) return;
	audio_layer_output_process();
	audio_layer_input_process();
}

int16_t * audio_get_buffer( audio_data_t * audio )
{
	int16_t * ret;

	if( audio->buffer_ready == NULL ) return NULL;

	ret = audio->buffer_ready;
	audio->buffer_ready = NULL;

	return ret;
}

bool_t audio_is_ready( audio_data_t * audio )
{
	bool_t ret = FALSE;

	if( audio->mask & AUDIO_STATUS_READY )
	{
		ret = TRUE;
		audio->mask &= ~AUDIO_STATUS_READY;
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

#if 0 // def WANT_AUDIO_INTERFACE

DECLARE_INIT_SECTION( audio_layer_init );
DECLARE_PROCESS_SECTION( audio_layer_process );

#endif
