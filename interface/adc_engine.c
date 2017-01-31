#include "pipe.h"
#include "adc_engine.h"
#include "random.h"
#include "system.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static pipe_data_t adc_pipe_trigger;
static pipe_data_t adc_pipe_value;

static adc_read_t * curr_adc_request;

static int64_t adc_rand_seed;

static device_t * adc_device_list[MAX_ADC_INDEX];

///////////////////////////////////////////////////////////////

void adc_engine_process( void )
{
	uint16_t adc_value;

	if( ( curr_adc_request == NULL ) && ( pipe_is_empty( &adc_pipe_trigger ) == FALSE ) )
	{
		curr_adc_request = ( adc_read_t * )pipe_read( &adc_pipe_trigger, NULL );

		if( ( curr_adc_request->index >= MAX_ADC_INDEX ) || ( adc_device_list[ curr_adc_request->index ] != NULL ) )
		{
			curr_adc_request->value = 0;
			pipe_write( &adc_pipe_value, ( uint8_t * )curr_adc_request, sizeof( adc_read_t ) );
		}
		else
		{
			device_ioctrl( adc_device_list[curr_adc_request->index], DEVICE_SET_RUNTIME_VALUE, NULL );
		}
	}

	if( ( curr_adc_request != NULL ) && 
		( device_read_buffer( adc_device_list[curr_adc_request->index], ( uint8_t * )&adc_value, sizeof( uint16_t ) ) ) )
	{
		curr_adc_request->value = adc_value;
		pipe_write( &adc_pipe_value, ( uint8_t * )curr_adc_request, sizeof( adc_read_t ) );
		curr_adc_request = NULL;
	}
}

void adc_conversion_start( adc_read_t * adc_data, uint16_t index )
{
	adc_data->index = index;
	adc_data->sender = rand_get_next( &adc_rand_seed );
	pipe_write( &adc_pipe_trigger, ( uint8_t * )adc_data, sizeof( adc_read_t ) );
}

int adc_conversion_is_complete( adc_read_t * adc_data )
{
	adc_read_t * p_adc_req;
	uint16_t total_pipe;
	uint16_t cnt;

	total_pipe = pipe_get_used( &adc_pipe_value );

	if( pipe_is_empty( &adc_pipe_value ) == FALSE )
	{
		for( cnt = 0; cnt < total_pipe; cnt++ )
		{
			p_adc_req = ( adc_read_t * )pipe_read( &adc_pipe_value, NULL );

			if( p_adc_req->sender == adc_data->sender )
			{
				memcpy( adc_data, p_adc_req, sizeof( adc_read_t ) );
				return (int)adc_data->value;
			}

			pipe_write( &adc_pipe_value, ( uint8_t * )p_adc_req, sizeof( adc_read_t ) );
		}
	}

	return -1;
}

void adc_engine_init( void )
{
	uint32_t i;
	char adc_dev_str[10];

	rand_get_seed( &adc_rand_seed );

	pipe_init( &adc_pipe_trigger, PIPE_ADC_TRIGGER_NAME, 16 );
	pipe_init( &adc_pipe_value, PIPE_ADC_VALUE_NAME, 16 );

	curr_adc_request = NULL;

	for( i = 0; i < MAX_ADC_INDEX; i++ )
	{
		strcpy( adc_dev_str, "/dev/adc*" );
		adc_dev_str[8] = i + 0x30;
		adc_device_list[i] = device_open( adc_dev_str );
	}
}

///////////////////////////////////////////////////////////////

#if 0 // def WANT_ADC_INTERFACE

DECLARE_INIT_SECTION( adc_engine_init );
DECLARE_PROCESS_SECTION( adc_engine_process );

#endif

