#include "system.h"
#include "timer.h"

#include "draw.h"
#include "debug.h"

#include "battery_check.h"

///////////////////////////////////////////////////////////////

#define BATTERY_CHECK_MS				500
#define BATTERY_CHARGING_THRESHOLD		2950
#define BATTERY_LOW_COUNT				5

///////////////////////////////////////////////////////////////

static device_t * adc_battery_dev;

static timer_data_t battery_check_timer;

static uint8_t battery_state_count;
static uint8_t battery_state;
static uint16_t adc_value;

///////////////////////////////////////////////////////////////

void battery_init( void )
{
	battery_state = BATTERY_ERROR;
	adc_value = 0;
	battery_state_count = 0;

    adc_battery_dev = device_open( "/dev/adc3" );
    if( adc_battery_dev == NULL ) return;

    DEBUG_PRINTK( "Battery Ready\r\n" );

    timer_Start( &battery_check_timer, BATTERY_CHECK_MS );
    battery_state = BATTERY_READY;
	device_ioctrl( adc_battery_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
}

void battery_process( void )
{
	if( adc_battery_dev == NULL ) return;

	if( timer_Check( &battery_check_timer ) == TRUE )
	{
		if( device_read_buffer( adc_battery_dev, ( uint8_t * )&adc_value, 2 ) == 2 )
		{
			timer_Reload( &battery_check_timer );

			// draw_text_printk( 10, DISPLAY_LINE(6), "ADC    %d      ", adc_value );
			device_ioctrl( adc_battery_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
			//DEBUG_PRINTK( "Battery %d\r\n", adc_value );

			if( adc_value > BATTERY_CHARGING_THRESHOLD )
			{
				battery_state_count = BATTERY_LOW_COUNT;
				battery_state = BATTERY_CHARGING;
			}
			else
			{
				if( battery_state_count == 0 ) battery_state = BATTERY_READY;
				else battery_state--;
			}
		}
	}
}

uint8_t battery_get_state( void )
{
	return battery_state;
}

