#include <stdio.h>
#include "system.h"

////////////////////////////////////////////////////////////////////////////////

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
	FILE * fp;
	uint8_t audio_mask;
    void * wave_hdl;

} wave_dev_t;

////////////////////////////////////////////////////////////////////////////////

static wave_dev_t wave_dev_out;
static wave_dev_t wave_dev_in;

static bool_t _is_out_opened;
static bool_t _is_in_opened;
static bool_t _in_use;

////////////////////////////////////////////////////////////////////////////////

static void audio_driver_global_init( void )
{
	_is_in_opened = FALSE;
	_is_out_opened = FALSE;
}

static int audio_driver_pa_init( uint32_t sample_rate, uint8_t audio_mask )
{
	wave_dev_out.audio_mask = 0;
	wave_dev_in.audio_mask = 0;
    
    if( audio_mask & AUDIO_MODE_OUTPUT )
	{
		wave_dev_out.audio_mask = audio_mask;
	}

	if( audio_mask & AUDIO_MODE_INPUT ) 
	{
		wave_dev_in.audio_mask = audio_mask;
	}

	return 0;
}

static void audio_port_open( uint32_t index )
{
	if( wave_dev_in.audio_mask )
	{
		wave_dev_out.fp = fopen( "wave_in.wav", "rb" );
	}

	if( wave_dev_out.audio_mask )
	{
		wave_dev_out.fp = fopen( "wave_out.wav", "wb" );
		fclose( wave_dev_out.fp );
	}
}

static void audio_port_close( uint32_t index )
{
	if( wave_dev_in.audio_mask )
	{
		_is_in_opened = FALSE;
	}

	if( wave_dev_out.audio_mask )
	{
		_is_out_opened = FALSE;
	}
}

static uint32_t audio_port_receive( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t ret = 0;

	if( !feof( wave_dev_in.fp ) )
	{
		if( buffer == NULL )
		{
			ret =  AUDIO_BUFFER_SIZE*4;
		}
		else
		{
			ret = fread( buffer, size, 1, wave_dev_in.fp );
		}
	}

	return ret;
}

static uint32_t audio_port_transmit( uint32_t index, uint8_t * buffer, uint32_t size )
{
	wave_dev_out.fp = fopen( "wave_out.wav", "ab" );
	fwrite( buffer, size, 1, wave_dev_out.fp );
	fclose( wave_dev_out.fp );
	return size;
}

static uint32_t audio_io_ctrl( uint32_t index, uint32_t param, void * value )
{
	uint32_t * p_value = ( uint32_t * )value;

	if( param == DEVICE_GET_RUNTIME_VALUE )
	{
		if( index == AUDIO_MODE_OUTPUT )
		{
			*p_value = TRUE;
		}
		else if( index == AUDIO_MODE_INPUT )
		{
			*p_value = TRUE;
		}
	}
	else if( param == DEVICE_SET_RUNTIME_VALUE )
	{
		// set volume to the audio DAC, if there is any
	}

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
    
//DECLARE_DEVICE_SECTION(audio_device_driver);
