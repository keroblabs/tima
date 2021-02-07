#include "system.h"
#include "audio_layer.h"
#include "tima_libc.h"

////////////////////////////////////////////////////////////////////////////////

static list_t     audio_list;
static device_t * p_dev_audio_in;
static device_t * p_dev_audio_out;

////////////////////////////////////////////////////////////////////////////////

static void audio_layer_copy_stereo_to_mono( int16_t * output, int16_t * input )
{
    int16_t * p_sample = output;
    int32_t audio32;
    int cnt;
    
    // for mono instances, mix left and right
    for( cnt = 0; cnt < AUDIO_BUFFER_STEREO_SIZE; cnt += 2 )
    {
        audio32 = ( input[cnt] + input[cnt+1] ) >> 1;
        *( p_sample ) = ( int16_t )audio32;
        p_sample++;
    }
}

static void audio_layer_mix( int16_t * output, int16_t * input, bool_t is_input_stereo, bool_t is_first_mix )
{
    int cnt;
    int16_t * p_sample = input;
    
    if( is_first_mix )
    {
        if( is_input_stereo )
        {
            memcpy( p_sample, input, AUDIO_BUFFER_STEREO_SIZE * sizeof( int16_t ) );
        }
        else
        {
            for( cnt = 0; cnt < AUDIO_BUFFER_STEREO_SIZE; cnt += 2 )
            {
                output[cnt] = *( p_sample );
                output[cnt+1] = *( p_sample );
                p_sample++;
            }
        }
    }
    else
    {
        for( cnt = 0; cnt < AUDIO_BUFFER_STEREO_SIZE; cnt += 2 )
        {
            output[cnt] = ( *( p_sample ) + output[cnt] ) >> 1;
            if( is_input_stereo ) p_sample++;
            output[cnt+1] = ( *( p_sample ) + output[cnt+1] ) >> 1;
            p_sample++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void audio_layer_init( void )
{
	p_dev_audio_in = p_dev_audio_out = NULL;
    list_init( &audio_list, NULL );
}

audio_data_t * audio_start( uint32_t sample_rate, int16_t * buffer, uint8_t mask, audio_process_t proc, void * pp_data )
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
    
    p_audio->proc = proc;
    p_audio->pp_data = pp_data;

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
			sprintk( str_audio_dev, "/dev/audio?%d,in", sample_rate );
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
	list_insert_tail( &audio_list, &p_audio->node );

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
    list_remove( &audio_list, &audio->node );
}

void audio_layer_input_process( void )
{
	audio_data_t * p_audio = NULL;
	int16_t * input_audio_buffer;

	// check audio driver 
    input_audio_buffer = ( int16_t * )device_pipe_read_input( p_dev_audio_in, NULL );
    if( input_audio_buffer == NULL ) return;
    
    while( ( p_audio = ( audio_data_t * )list_get_next( &audio_list, p_audio ? &p_audio->node : NULL ) ) != NULL )
	{
		if( p_audio->mask & AUDIO_MODE_INPUT )
		{
			if( p_audio->mask & AUDIO_MODE_STEREO )
			{
				// if the instance is stereo, copy the whole frame
				memcpy( p_audio->buffer, input_audio_buffer, AUDIO_BUFFER_STEREO_SIZE*sizeof( int16_t ) );
			}
			else
			{
				// for mono instances, mix left and right
                audio_layer_copy_stereo_to_mono( p_audio->buffer, input_audio_buffer );
			}

			// set this instance as ready
			p_audio->mask |= AUDIO_STATUS_READY;
			p_audio->buffer_ready = p_audio->buffer;
            
            if( p_audio->proc != NULL ) p_audio->proc( p_audio->pp_data );
		}
	}
}

void audio_layer_output_process( void )
{
    audio_data_t * p_audio = NULL;
	bool_t is_first;
    int16_t * output_audio_buffer;

    // prepare audio buffer
    output_audio_buffer = ( int16_t * )device_pipe_read_output( p_dev_audio_out, NULL );
    if( output_audio_buffer == NULL ) return;
    
	is_first = TRUE;

    while( ( p_audio = ( audio_data_t * )list_get_next( &audio_list, p_audio ? &p_audio->node : NULL ) ) != NULL )
	{
		if( p_audio->mask & AUDIO_MODE_OUTPUT )
		{
            audio_layer_mix( output_audio_buffer, p_audio->buffer, ( p_audio->mask & AUDIO_MODE_STEREO ) ? TRUE : FALSE, is_first );

			// indicate this instance is processed
			is_first = FALSE;
			p_audio->mask |= AUDIO_STATUS_READY;
			p_audio->buffer_ready = p_audio->buffer;

            if( p_audio->proc != NULL ) p_audio->proc( p_audio->pp_data );            
		}
	}
}

void audio_layer_process( void )
{
    if( list_size( &audio_list ) == 0 ) return;
	if( p_dev_audio_out != NULL ) audio_layer_output_process();
	if( p_dev_audio_in != NULL  ) audio_layer_input_process();
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
