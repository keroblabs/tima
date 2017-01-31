#include "system.h"
#include "timer.h"

#include "debug.h"

#include "wiper_control.h"

///////////////////////////////////////////////////////////////

#define WIPERS_CHECK_MS					2000

///////////////////////////////////////////////////////////////

static device_t * adc_wiper_r_dev;
static device_t * adc_ref_dev;

static timer_data_t wipers_check_timer;

static uint16_t adc_value;

///////////////////////////////////////////////////////////////

void wiper_control_init( void )
{
	adc_wiper_r_dev = device_open( "/dev/adc0" );
	adc_ref_dev = device_open( "/dev/adc1" );

	if( !adc_wiper_r_dev || !adc_ref_dev ) return;

	DEBUG_PRINTK( "Wiper Ready\r\n" );

	timer_Start( &wipers_check_timer, WIPERS_CHECK_MS );

	device_ioctrl( adc_wiper_r_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
	device_ioctrl( adc_ref_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
}

void wiper_control_process( void )
{
	if( adc_wiper_r_dev == NULL ) return;
	if( adc_ref_dev == NULL ) return;

	if( timer_Check( &wipers_check_timer ) == FALSE ) return;
	timer_Reload( &wipers_check_timer );

	if( device_read_buffer( adc_wiper_r_dev, ( uint8_t * )&adc_value, 2 ) == 2 )
	{
		device_ioctrl( adc_wiper_r_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
		DEBUG_PRINTK( "Wiper R = %d\r\n", adc_value );
	}

	if( device_read_buffer( adc_ref_dev, ( uint8_t * )&adc_value, 2 ) == 2 )
	{
		device_ioctrl( adc_ref_dev, DEVICE_SET_RUNTIME_VALUE, NULL );
		DEBUG_PRINTK( "Reference = %d\r\n", adc_value );
	}
}

