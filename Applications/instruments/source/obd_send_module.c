#include "system.h"
#include "timer.h"
#include "pipe.h"
#include "obd_engine.h"
#include "draw.h"
//#include "debug.h"

///////////////////////////////////////////////////////////////

#define DISPLAY_LINE(x) 			( 150 + ( 8 * ( x ) ) )
#define OBD_ZERO_COUNT_VAL			10
#define OBD_SEND_INTERVAL_MS		500

#define DONT_DISPLAY_OBD_DATA
#define DONT_SEND_ODB_DATA
#define ODB_USE_INSTRUMENTS

///////////////////////////////////////////////////////////////

#ifdef ODB_USE_INSTRUMENTS
#include "instruments.h"
#endif

///////////////////////////////////////////////////////////////

static int obd_rpm;
static int obd_speed;
static int obd_temp_coolant;

static int obd_temp_intake;
static int obd_load;
static int obd_map;

static int obd_rpm_zero_cnt;
static int obd_speed_zero_cnt;
static int obd_temp_coolant_zero_cnt;

#ifdef SEND_ODB_DATA
static device_t * dashboard_dev;
static char obd_string_send[200];
static timer_data_t obd_send_timer;
#endif

static pipe_data_t * obd_pipe;
static obd_data_t * obd_data;

static bool_t obd_ready;

///////////////////////////////////////////////////////////////

static int app_hex_to_int( char * input )
{
	int val1;
	int ret = 0;
	int i;

	for( i = 0; i < 2; i++ )
	{
		val1 = input[i];

		if( ( val1 >= 0x30 ) && ( val1 <= 0x39 ) )
		{
			val1 &= 0x0F;
		}
		else if( ( ( val1 >= 'A' ) && ( val1 <= 'F' ) ) ||
				( ( val1 >= 'a' ) && ( val1 <= 'f' ) ) )
		{
			if( val1 & 0x20 ) val1 &= ~0x20;
			val1 = ( val1 - 'A' ) + 10;
		}

		ret = ( ret * 16 ) + val1;
	}

	return ret;
}

static int app_extract_data( char * input )
{
	int ret = 0;
	int val1, val2;

	if( strlen( input ) > 7 )
	{
		val1 = app_hex_to_int( &input[6] );
		ret = val1;

		if( strlen( input ) > 10 )
		{
			val2 = app_hex_to_int( &input[9] );
			ret = ( ret * 256 ) + val2;
		}
	}

	return ret;
}

static void obd_send_display_data( obd_data_t * data )
{
	int val;
	uint16_t posy;
	
	posy = 150;
	val = app_extract_data(data->resp);

	if( val > 10000 ) return;

	switch( data->command )
	{
		case OBD_COMMAND_RPM:
			//if( val == 0 )
			//{
			//	if( obd_rpm_zero_cnt == 0 ) obd_rpm = val>>2;
			//	else obd_rpm_zero_cnt--;
			//}
			//else
			//{
			//	obd_rpm_zero_cnt = OBD_ZERO_COUNT_VAL;
				obd_rpm = val>>2;
			//}

			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(0), "RPM    %d     ", val>>2 );
			#endif

			DEBUG_PRINTK( "RPM    %d\r\n", val>>2 );

			draw_text_printk( 10, 18, "RPM  %d     ", obd_rpm );

			#ifdef ODB_USE_INSTRUMENTS
			instruments_update_rpm( obd_rpm );
			#endif
			break;

		case OBD_COMMAND_SPEED:
			//if( val == 0 )
			//{
			//	if( obd_speed_zero_cnt == 0 ) obd_speed = val;
			//	else obd_speed_zero_cnt--;
			//}
			//else
			//{
			//	obd_speed_zero_cnt = OBD_ZERO_COUNT_VAL;
				obd_speed = val;
			//}

			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(1), "Speed  %d     ", val );
			#endif

			DEBUG_PRINTK( "Speed  %d\r\n", val );

			#ifdef ODB_USE_INSTRUMENTS
			instruments_update_speed( obd_speed );
			#endif
			break;

		case OBD_COMMAND_TEMPERATURE:
			//if( val == 0 )
			//{
			//	if( obd_temp_coolant_zero_cnt == 0 ) obd_temp_coolant = val-40;
			//	else obd_temp_coolant_zero_cnt--;
			//}
			//else
			//{
			//	obd_temp_coolant_zero_cnt = OBD_ZERO_COUNT_VAL;
				obd_temp_coolant = val-40;
			//}

			obd_temp_coolant = val-40;
			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(2), "Temp   %d     ", val-40 );
			#endif

			DEBUG_PRINTK( "Temp   %d\r\n", val-40 );

			#ifdef ODB_USE_INSTRUMENTS
			instruments_update_temp( obd_temp_coolant );
			#endif
			break;

		case OBD_COMMAND_INTAKE_TEMPERATURE:
			obd_temp_intake = val-40;
			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(3), "Intake %d     ", val-40 );
			#endif
			break;

		case OBD_COMMAND_ENGINE_LOAD:
			obd_load = val;
			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(4), "Load   %d     ", val );
			#endif
			break;

		case OBD_COMMAND_INTAKE_PRESSURE:
			obd_map = val;
			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(5), "MAP    %d     ", val );
			#endif
			break;
	}
}

void obd_send_init( void )
{
#ifdef SEND_ODB_DATA
	uint32_t baud;
#endif

    obd_rpm = 0;
    obd_speed = 0;
    obd_temp_coolant = 0;
    obd_temp_intake = 0;
    obd_load = 0;
    obd_map = 0;

    obd_rpm_zero_cnt = 0;
    obd_speed_zero_cnt = 0;
    obd_temp_coolant_zero_cnt = 0;

    obd_ready = FALSE;
#ifdef SEND_ODB_DATA
    timer_Start(&obd_send_timer, OBD_SEND_INTERVAL_MS);
#endif

    obd_engine_init();
    obd_pipe = pipe_locate( OBD_PIPE_NAME );

#ifdef SEND_ODB_DATA
    dashboard_dev = device_open( DEV_PANEL );
    if( dashboard_dev != NULL )
    {
    	baud = 115200;
        device_ioctrl( dashboard_dev, DEVICE_SET_CONFIG_DATA, &baud );
    }
#endif
}

void obd_set_ignition_off( void )
{
	obd_rpm = 0;
}

void obd_send_process( void )
{
#ifdef SEND_ODB_DATA
	uint32_t obd_mask;
#endif

    if( ( obd_engine_get_response( NULL ) == OBD_STATUS_IDLE ) && ( obd_ready == FALSE ) )
    {
		DEBUG_PRINTK( "Start Auto\r\n" );

    	obd_engine_autoupdate( TRUE );
        obd_ready = TRUE;
    }

    if( ( obd_ready == TRUE ) && ( pipe_is_empty( obd_pipe ) == FALSE ) )
    {
    	obd_data = ( obd_data_t * )pipe_read( obd_pipe, NULL );
    	obd_send_display_data( obd_data );
    	pipe_release_buffer( ( uint8_t * )obd_data );
    }

#ifdef SEND_ODB_DATA
    if( ( obd_is_data_ready() || timer_Check(&obd_send_timer) ) && ( dashboard_dev != NULL ) )
    {
    	timer_Reload(&obd_send_timer);
    	obd_mask = pio_check_read();
    	sprintk( obd_string_send, "obd_send %d,%d,%d,%d\r\n", obd_rpm, obd_speed, obd_temp_coolant, obd_mask );
    	device_write_buffer( dashboard_dev, ( uint8_t * )obd_string_send, strlen(obd_string_send) );
    }
#endif

    //if( power_get_engine_run_input() == FALSE )
    //{
    //	obd_rpm = 0;
    //}

    obd_engine_process();
}
