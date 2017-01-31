#include "portaudio.h"
#include "system.h"
#include "pipe.h"
#include "i2c_driver.h"

#ifdef WIN32
#include <windows.h>
CRITICAL_SECTION gCS;
uint8_t is_init = FALSE;
#endif

////////////////////////+-////////////////////////////////////////////////////////

#define AUDIO_TOTAL_BUFFERS				2
#define AUDIO_BUFFER_SIZE				160

#define AUDIO_MODE_STEREO				(1 << 0)
#define AUDIO_MODE_MONO					(1 << 1)
#define AUDIO_MODE_OUTPUT				(1 << 2)
#define AUDIO_MODE_INPUT				(1 << 3)
#define AUDIO_STATUS_READY				(1 << 4)
#define AUDIO_STATUS_STOP				(1 << 5)

////////////////////////////////////////////////////////////////////////////////

typedef struct WaveDeviceDataT
{
    PaStreamParameters streamParameters;
    PaStream *stream;
    
	uint8_t audio_mask;

	pipe_data_t audio_pipe;

    void * wave_hdl;

} wave_dev_t;

////////////////////////////////////////////////////////////////////////////////

static wave_dev_t wave_dev_out;
static wave_dev_t wave_dev_in;

static bool_t _is_out_opened;
static bool_t _is_in_opened;
static bool_t _in_use;

////////////////////////////////////////////////////////////////////////////////

static int audio_pa_playback_callback(  const void *inputBuffer, void *outputBuffer,
										unsigned long framesPerBuffer,
										const PaStreamCallbackTimeInfo* timeInfo,
										PaStreamCallbackFlags statusFlags,
										void *userData )
{
    wave_dev_t *p_data = ( wave_dev_t * )userData; //  is the used defined data

	#ifdef WIN32
	EnterCriticalSection(&gCS);
	#endif

	if( pipe_is_empty( &p_data->audio_pipe ) == FALSE )
	{
		pipe_read_buffer( &p_data->audio_pipe, ( uint8_t * )outputBuffer, AUDIO_BUFFER_SIZE*4 );
	}
	else
	{
		memset( ( uint8_t * )outputBuffer, 0x00, AUDIO_BUFFER_SIZE*4 );
	}
    
	#ifdef WIN32
	LeaveCriticalSection(&gCS);
	#endif

    return 0;
}

static int audio_pa_recording_callback( const void *inputBuffer, void *outputBuffer,
                                        unsigned long framesPerBuffer,
										const PaStreamCallbackTimeInfo* timeInfo,
										PaStreamCallbackFlags statusFlags,
										void *userData )
{
    wave_dev_t *p_data = ( wave_dev_t * )userData; //  is the used defined data

	#ifdef WIN32
	EnterCriticalSection(&gCS);
	#endif

	pipe_send_buffer( &p_data->audio_pipe, ( uint8_t * )inputBuffer, AUDIO_BUFFER_SIZE*4 );

	#ifdef WIN32
	LeaveCriticalSection(&gCS);
	#endif

    return 0;
}

static void audio_driver_global_init( void )
{
	_is_in_opened = FALSE;
	_is_out_opened = FALSE;
}

static int audio_driver_pa_init( uint32_t sample_rate, uint8_t audio_mask )
{
    PaError err;

	#ifdef WIN32
	if( is_init == FALSE )
	{
		InitializeCriticalSection(&gCS);
		is_init = TRUE;
	}
	#endif

	wave_dev_out.audio_mask = 0;
	wave_dev_in.audio_mask = 0;
    
    err = Pa_Initialize();
    if( err != paNoError ) return -1;

    if( audio_mask & AUDIO_MODE_OUTPUT )
	{
		pipe_init( &wave_dev_out.audio_pipe, "AUDIO_PIPE_OUT", 4 );

		wave_dev_out.audio_mask = audio_mask;
		wave_dev_out.streamParameters.device = Pa_GetDefaultOutputDevice();
		wave_dev_out.streamParameters.suggestedLatency = 
			Pa_GetDeviceInfo( wave_dev_out.streamParameters.device )->defaultLowOutputLatency;
		wave_dev_out.streamParameters.channelCount = 2;
		wave_dev_out.streamParameters.sampleFormat = paInt16;
		wave_dev_out.streamParameters.hostApiSpecificStreamInfo = NULL;

		err = Pa_OpenStream( &wave_dev_out.stream,
							 NULL,
							 &wave_dev_out.streamParameters,
							 sample_rate,
							 AUDIO_BUFFER_SIZE,
							 paClipOff,
							 audio_pa_playback_callback,
							 &wave_dev_out );

		Pa_StartStream( &wave_dev_out.stream );
	}

	if( audio_mask & AUDIO_MODE_INPUT ) 
	{
		pipe_init( &wave_dev_in.audio_pipe, "AUDIO_PIPE_IN", 4 );

		wave_dev_in.audio_mask = audio_mask;
		wave_dev_in.streamParameters.device = Pa_GetDefaultInputDevice();
		wave_dev_in.streamParameters.suggestedLatency = 
			Pa_GetDeviceInfo( wave_dev_in.streamParameters.device )->defaultLowInputLatency;
		wave_dev_in.streamParameters.channelCount = 2;
		wave_dev_in.streamParameters.sampleFormat = paInt16;
		wave_dev_in.streamParameters.hostApiSpecificStreamInfo = NULL;

		err = Pa_OpenStream( &wave_dev_in.stream,
							 &wave_dev_in.streamParameters,
							 NULL,
							 sample_rate,
							 AUDIO_BUFFER_SIZE,
							 paClipOff,
							 audio_pa_recording_callback,
							 &wave_dev_in );
	}

    if( err != paNoError ) return -1;

	return 0;
}

static void audio_port_open( uint32_t index )
{
	if( wave_dev_in.audio_mask )
	{
		Pa_StartStream( wave_dev_in.stream );
	}

	if( wave_dev_out.audio_mask )
	{
		Pa_StartStream( wave_dev_out.stream );
	}
}

static void audio_port_close( uint32_t index )
{
	if( wave_dev_in.audio_mask )
	{
		Pa_StopStream( wave_dev_in.stream );
		Pa_CloseStream( wave_dev_in.stream );
		_is_in_opened = FALSE;
	}

	if( wave_dev_out.audio_mask )
	{
		Pa_StopStream( wave_dev_out.stream );
		Pa_CloseStream( wave_dev_out.stream );
		_is_out_opened = FALSE;
	}
}

static uint32_t audio_port_receive( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t ret = 0;

	#ifdef WIN32
	EnterCriticalSection(&gCS);
	#endif

	if( pipe_is_empty( &wave_dev_in.audio_pipe ) == FALSE )
	{
		if( buffer == NULL )
		{
			ret =  AUDIO_BUFFER_SIZE*4;
		}
		else
		{
			ret = pipe_read_buffer( &wave_dev_in.audio_pipe, buffer, size );
		}
	}

	#ifdef WIN32
	LeaveCriticalSection(&gCS);
	#endif

	return ret;
}

static uint32_t audio_port_transmit( uint32_t index, uint8_t * buffer, uint32_t size )
{
	#ifdef WIN32
	EnterCriticalSection(&gCS);
	#endif

	if( !pipe_is_full( &wave_dev_out.audio_pipe ) )
	{
		pipe_send_buffer( &wave_dev_out.audio_pipe, buffer, size );
	}

	#ifdef WIN32
	LeaveCriticalSection(&gCS);
	#endif

	return size;
}

static uint32_t audio_io_ctrl( uint32_t index, uint32_t param, void * value )
{
	uint32_t * p_value = ( uint32_t * )value;

	#ifdef WIN32
	EnterCriticalSection(&gCS);
	#endif

	if( param == DEVICE_GET_RUNTIME_VALUE )
	{
		if( index == AUDIO_MODE_OUTPUT )
		{
			*p_value = pipe_is_full( &wave_dev_out.audio_pipe ) ? FALSE : TRUE;
		}
		else if( index == AUDIO_MODE_INPUT )
		{
			*p_value = pipe_is_empty( &wave_dev_in.audio_pipe );
		}
	}
	else if( param == DEVICE_SET_RUNTIME_VALUE )
	{
		// set volume to the audio DAC, if there is any
	}

	#ifdef WIN32
	LeaveCriticalSection(&gCS);
	#endif

	return 0;
}

static int audio_get_index( char * param )
{
	char temp_param[20];
	char * arg0, * arg1;
	uint32_t sample_rate;
	uint8_t audio_mask;
	int ret = -1;

	if( param[0] != '?' ) return -1;

	strncpy( temp_param, param, 19 );

	arg0 = &temp_param[1];
	arg1 = strchr( arg0, ',' ); if( arg1 == NULL ) return -1; *arg1 = 0; arg1++;

	audio_mask = 0;

	if( !strcmp( arg1, "out" ) ) 
	{
		audio_mask |= AUDIO_MODE_OUTPUT;
		if( _is_out_opened == TRUE ) return -1;
		_is_out_opened = TRUE;
		ret = AUDIO_MODE_OUTPUT;
	}
	else if( !strcmp( arg1, "in" ) ) 
	{
		audio_mask |= AUDIO_MODE_INPUT;
		if( _is_in_opened == TRUE ) return -1;
		_is_in_opened = TRUE;
		ret = AUDIO_MODE_INPUT;
	}
	else return -1;

	if( !strcmp( arg0, "8000" ) ) sample_rate = 8000;
	else return -1;
	
	if( audio_driver_pa_init( sample_rate, audio_mask ) == -1 ) return -1;

	return ret;
}

static bool_t audio_validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

static bool_t audio_is_open( uint32_t index )
{
	if( index == AUDIO_MODE_INPUT ) return _is_in_opened;
	else if( index == AUDIO_MODE_OUTPUT ) return _is_out_opened;
	return FALSE;
}

const static device_data_t audio_device_driver = 
{
    "audio?",
    &_in_use,
    
    audio_port_open,
    audio_port_close,
    NULL,
    audio_validate,
    audio_get_index,
    audio_io_ctrl,
    audio_is_open,
    
    DEV_MODE_CHANNEL,
    
    audio_port_receive,
    audio_port_transmit,
    
};
    
DECLARE_DEVICE_SECTION(audio_device_driver);
