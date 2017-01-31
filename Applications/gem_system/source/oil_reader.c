#include "system.h"
#include "timer.h"

#include "debug.h"
#include "pio_check.h"
#include "oil_reader.h"

///////////////////////////////////////////////////////////////

#define OIL_READ_TEMP_MS				2000
#define OIL_CHECK_MS					200

///////////////////////////////////////////////////////////////

enum
{
	OIL_READ_IDLE,
	OIL_READ_SWITCH_OFF,
	OIL_READ_SWITCH_ON,
	OIL_READ_WAIT_1ST_HIGH,
	OIL_READ_COUNT_1ST_HIGH,
	OIL_READ_COUNT_2ST_LOW,
	OIL_READ_END
};

///////////////////////////////////////////////////////////////

static device_t * oil_pio_dev;
static device_t * pio_highside;

static bool_t oil_state;
static bool_t oil_prev_state;
//static bool_t oil_pause_pio;

static timer_data_t oil_check_timer;
//static timer_data_t oil_temp_timer;

//static uint32_t oil_switch_off_ms;
//static uint32_t oil_signal1_period_ms;
//static uint32_t oil_signal2_period_ms;

static uint32_t oil_temperature_val;
static uint32_t oil_pressure_val;

//static uint8_t oil_signal_read_state;

///////////////////////////////////////////////////////////////

#if 0
static bool_t oil_reader_get_signals( void )
{
	uint32_t curr_ms;
	bool_t ret = FALSE;

	switch( oil_signal_read_state )
	{
		case OIL_READ_IDLE:
			// switch off oil sensor power control
			pio_check_set_output( pio_highside, FALSE );
			oil_signal_read_state = OIL_READ_SWITCH_OFF;
			oil_switch_off_ms = timer_get_MS() + OIL_CHECK_MS;
			break;

		case OIL_READ_SWITCH_OFF:
			if( oil_switch_off_ms < timer_get_MS() )
			{
				// now switch power on and wait for high level
				pio_check_set_output( pio_highside, TRUE );
				oil_signal_read_state = OIL_READ_SWITCH_ON;
			}
			break;

		case OIL_READ_SWITCH_ON:
			if( pio_check_read_input( oil_pio_dev, FALSE ) )
			{
				oil_signal_read_state = OIL_READ_COUNT_1ST_HIGH;
				oil_signal1_period_ms = timer_get_MS();
			}
			break;

		case OIL_READ_COUNT_1ST_HIGH:
			if( !pio_check_read_input( oil_pio_dev, FALSE ) )
			{
				curr_ms = timer_get_MS();
				oil_signal_read_state = OIL_READ_COUNT_2ST_LOW;
				oil_signal1_period_ms = curr_ms - oil_signal1_period_ms;
				oil_signal2_period_ms = curr_ms;
			}
			break;

		case OIL_READ_COUNT_2ST_LOW:
			if( !pio_check_read_input( oil_pio_dev, FALSE ) )
			{
				oil_signal_read_state = OIL_READ_IDLE;
				oil_signal2_period_ms = timer_get_MS() - oil_signal2_period_ms;
				ret = TRUE;

				oil_pressure_val = oil_signal1_period_ms;
				oil_temperature_val = oil_signal2_period_ms;
			}
			break;
	}

	return ret;
}
#endif

void oil_reader_init( void )
{
	oil_state = FALSE;
	oil_prev_state = FALSE;
	//oil_pause_pio = FALSE;

	//oil_signal_read_state = OIL_READ_IDLE;

	//oil_signal1_period_ms = 0;
	//oil_signal2_period_ms = 0;

	oil_pressure_val = 0;
	oil_temperature_val = 0;

	// oil sensor pio input
	oil_pio_dev = device_open( "/dev/gpio26" );
    if( oil_pio_dev == NULL ) return;

	// oil sensor power control
	pio_highside = pio_check_open_output( "/dev/gpio28", TRUE );

    DEBUG_PRINTK( "Oil Reader Ready\r\n" );

    timer_Start( &oil_check_timer, OIL_CHECK_MS );
    //timer_Stop( &oil_temp_timer );
    //timer_Start( &oil_temp_timer, OIL_READ_TEMP_MS );
}


void oil_reader_process( void )
{
	if( oil_pio_dev == NULL ) return;

	if( timer_Check( &oil_check_timer ) )
	{
		timer_Reload( &oil_check_timer );
		oil_state = pio_check_read_input( oil_pio_dev, TRUE );

		if( oil_state && !oil_prev_state )
		{
			DEBUG_PRINTK( "Oil pressure ok\r\n" );
			oil_prev_state = TRUE;
			pio_check_set_oil_pressure( TRUE );
		}
		else if( !oil_state && oil_prev_state )
		{
			// oil low pressure, stop reading timer
			oil_prev_state = FALSE;
			pio_check_set_oil_pressure( FALSE );

			DEBUG_PRINTK( "Oil pressure low\r\n" );
		}
	}
}

#if 0

void oil_reader_process( void )
{
	if( oil_pio_dev == NULL ) return;

	if( timer_Check( &oil_check_timer ) && ( oil_pause_pio == FALSE ) )
	{
		timer_Reload( &oil_check_timer );
		oil_state = pio_check_read_input( oil_pio_dev, TRUE );

		if( oil_state && !oil_prev_state )
		{
			// oil pressure ready, start reading timer
			//timer_Start( &oil_temp_timer, OIL_READ_TEMP_MS );

			DEBUG_PRINTK( "Oil pressure ok\r\n" );
			oil_prev_state = TRUE;
			pio_check_set_oil_pressure( TRUE );
		}
		else if( !oil_state && oil_prev_state )
		{
			// oil low pressure, stop reading timer
			oil_prev_state = FALSE;
			//timer_Stop( &oil_temp_timer );
			pio_check_set_oil_pressure( FALSE );

			DEBUG_PRINTK( "Oil pressure low\r\n" );
		}
	}

	if( timer_Check( &oil_temp_timer) )
	{
		timer_Reload( &oil_temp_timer );

		if( oil_pause_pio == FALSE )
		{
			oil_pause_pio = TRUE;
			oil_signal_read_state = OIL_READ_IDLE;
		}
		else
		{
			// took too long to read the signals, meaning the oil pressure dropped
			// so cancel reading process
			//timer_Stop( &oil_temp_timer );
			oil_pause_pio = FALSE;
			DEBUG_PRINTK( "Oil reading timeout\r\n" );
		}
	}

	if( oil_pause_pio == TRUE )
	{
		if( oil_reader_get_signals() == TRUE )
		{
			oil_pause_pio = FALSE;
		}
	}
}

#endif

bool_t oil_reader_get_state( void )
{
	return oil_state;
}

uint32_t oil_read_get_temperature( void )
{
	return oil_temperature_val;
}

uint32_t oil_read_get_pressure( void )
{
	return oil_pressure_val;
}

