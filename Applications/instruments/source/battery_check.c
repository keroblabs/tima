#include "system.h"
#include "timer.h"

#include "draw.h"
#include "debug.h"

#include "battery_check.h"

///////////////////////////////////////////////////////////////

#define BATTERY_CHECK_MS				1000
#define BATTERY_CHARGING_THRESHOLD		650

///////////////////////////////////////////////////////////////

static device_t * adc_battery_dev = NULL;

static timer_data_t battery_check_timer;

static uint8_t battery_state;
static uint16_t battery_adc_value;

///////////////////////////////////////////////////////////////

void battery_init( void )
{
	battery_state = BATTERY_ERROR;
	battery_adc_value = 0;

    adc_battery_dev = device_open( "/dev/adc3" );
    if( adc_battery_dev == NULL ) return;

    DEBUG_PRINTK( "Battery Ready\r\n", (int)adc_battery_dev );

    timer_Start( &battery_check_timer, BATTERY_CHECK_MS );
    battery_state = BATTERY_ENABLED;
	device_ioctrl( adc_battery_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
}

void battery_process( void )
{
	if( adc_battery_dev == NULL ) return;

	if( timer_Check( &battery_check_timer ) == TRUE )
	{
		if( device_read_buffer( adc_battery_dev, ( uint8_t * )&battery_adc_value, 2 ) == 2 )
		{
			timer_Reload( &battery_check_timer );

			device_ioctrl( adc_battery_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
			//DEBUG_PRINTK( "Battery ADC = %d\r\n", battery_adc_value );

			if( battery_adc_value > BATTERY_CHARGING_THRESHOLD )
			{
				battery_state = BATTERY_CHARGING;
			}
			else
			{
				battery_state = BATTERY_ENABLED;
			}
		}
	}
}

uint16_t battery_adc_get_value( void )
{
	return battery_adc_value;
}

uint8_t battery_get_state( void )
{
	return battery_state;
}

