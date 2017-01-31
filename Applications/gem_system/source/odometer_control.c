#include "system.h"
#include "timer.h"
#include "nvram_control.h"
#include "debug.h"

#include "odometer_control.h"

///////////////////////////////////////////////////////////////

#define ODOMETER_UPDATE_MS		1000

///////////////////////////////////////////////////////////////

static timer_data_t update_ms;
static timer_data_t safe_time;

static device_t * counter_dev;

static uint32_t odometer_prev_val;
static uint32_t odometer_value;
static uint32_t main_odometer;

///////////////////////////////////////////////////////////////

void odometer_control_init( void )
{
	odometer_value = 0;
	odometer_prev_val = NEG_U32;

	counter_dev = device_open( "/dev/counter0" );
	if( counter_dev != NULL )
	{
		DEBUG_PRINTK( "Counter dev ready\r\n" );
		timer_Start( &update_ms, ODOMETER_UPDATE_MS );
		timer_Start( &safe_time, 2000 );
	}

	nvram_control_read( NVRAM_ADDR_ODOMETER, &main_odometer, sizeof( main_odometer ) );
}

void odometer_control_process( void )
{
	if( counter_dev == NULL ) return;
	if( !timer_Check( &update_ms ) ) return;

	timer_Reload( &update_ms );

	device_read_buffer( counter_dev, ( uint8_t * )&odometer_value, sizeof( odometer_value ) );

	if( odometer_prev_val != odometer_value )
	{
		main_odometer += odometer_value;
		main_odometer -= odometer_prev_val;

		odometer_prev_val = odometer_value;

		if( !timer_Check( &safe_time ) )
		{
			main_odometer--;
		}

		nvram_control_write( NVRAM_ADDR_ODOMETER, &main_odometer, sizeof( main_odometer ) );
	}
}

uint32_t odometer_control_get( void )
{
	return main_odometer;
}

void odometer_control_power_off( void )
{

}

