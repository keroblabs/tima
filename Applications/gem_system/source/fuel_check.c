#include "system.h"
#include "timer.h"
#include "lp_filter.h"

#include "draw.h"
//#include "debug.h"

#include "battery_check.h"

///////////////////////////////////////////////////////////////

#define FUEL_CHECK_MS					200
#define FUEL_LP_FILTER_LEN				128
#define FUEL_LEVEL_DIFF_VAL				35000

///////////////////////////////////////////////////////////////

static device_t * adc_fuel_dev;
static timer_data_t fuel_check_timer;

static uint32_t fuel_lp_boot;
static uint16_t adc_value;

static uint32_t fuel_lp_buffer[FUEL_LP_FILTER_LEN];
static lp_filter_t fuel_lp_filter;

///////////////////////////////////////////////////////////////

void fuel_reader_init( void )
{
	fuel_lp_boot = FUEL_LP_FILTER_LEN+1;
	adc_value = 0;

	lp_filter_init( &fuel_lp_filter, fuel_lp_buffer, FUEL_LP_FILTER_LEN );

	adc_fuel_dev = device_open( "/dev/adc0" );
    if( adc_fuel_dev == NULL ) return;

    memset( fuel_lp_buffer, 0x00, sizeof( fuel_lp_buffer ) );

    DEBUG_PRINTK( "Fuel Reader Ready\r\n" );

    timer_Start( &fuel_check_timer, FUEL_CHECK_MS );
    device_ioctrl( adc_fuel_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
}

void fuel_reader_process( void )
{
	if( adc_fuel_dev == NULL ) return;

	if( fuel_lp_boot )
	{
		if( device_read_buffer( adc_fuel_dev, ( uint8_t * )&adc_value, 2 ) == 2 )
		{
			fuel_lp_boot--;
			lp_filter_add( &fuel_lp_filter, adc_value );
			device_ioctrl( adc_fuel_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
			adc_value = 0;
		}
	}
	else if( timer_Check( &fuel_check_timer ) == TRUE )
	{
		//DEBUG_PRINTK( "Fuel timer\r\n" );
		timer_Reload( &fuel_check_timer );

		if( device_read_buffer( adc_fuel_dev, ( uint8_t * )&adc_value, 2 ) == 2 )
		{
			lp_filter_add( &fuel_lp_filter, adc_value );
			adc_value = ( uint16_t )lp_filter_get_value( &fuel_lp_filter );

			adc_value = FUEL_LEVEL_DIFF_VAL - adc_value;
			//DEBUG_PRINTK( "Fuel Reader %d\r\n", adc_value );
			// draw_text_printk( 10, DISPLAY_LINE(6), "ADC    %d      ", adc_value );
			device_ioctrl( adc_fuel_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
		}
	}
}

uint16_t fuel_reader_get( void )
{
	return adc_value;
}

