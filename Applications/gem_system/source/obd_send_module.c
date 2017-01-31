#include "system.h"
#include "timer.h"
#include "pipe.h"
#include "odometer_control.h"
#include "obd_engine.h"
#include "draw.h"
#include "debug.h"
#include "power_control.h"

///////////////////////////////////////////////////////////////

#define DISPLAY_LINE(x) 			( 150 + ( 8 * ( x ) ) )
#define OBD_TEMP_ZERO_COUNT_VAL		3
#define OBD_RPM_ZERO_COUNT_VAL		2
#define OBD_SPEED_ZERO_COUNT_VAL	2
#define OBD_SEND_TIMEOUT_MS			1000
#define OBD_MESSAGE_ACK             0x9A

#define DISPLAY_OBD_DATA

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

static device_t * dashboard_dev = NULL;

static char obd_string_send[20];

//static timer_data_t obd_send_timer;
static pipe_data_t * obd_pipe;
static obd_data_t * obd_data;
static timer_data_t obd_send_timeout;

static bool_t obd_ready;
static bool_t obd_send;

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
	uint16_t posy;
	posy = 150;
	int val = app_extract_data(data->resp);

	switch( data->command )
	{
		case OBD_COMMAND_RPM:
			if( val > 32000 ) break;

			if( val == 0 )
			{
				if( obd_rpm_zero_cnt == 0 ) obd_rpm = val>>2;
				else obd_rpm_zero_cnt--;
			}
			else
			{
				obd_rpm_zero_cnt = OBD_RPM_ZERO_COUNT_VAL;
				obd_rpm = val>>2;
			}

			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(0), "RPM    %d     ", val>>2 );
			#endif
			break;

		case OBD_COMMAND_SPEED:
			if( val > 250 ) break;

			if( val == 0 )
			{
				if( obd_speed_zero_cnt == 0 ) obd_speed = val;
				else obd_speed_zero_cnt--;
			}
			else
			{
				obd_speed_zero_cnt = OBD_SPEED_ZERO_COUNT_VAL;
				obd_speed = val;
			}

			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(1), "Speed  %d     ", val );
			#endif
			break;

		case OBD_COMMAND_TEMPERATURE:
			if( val > 250 ) return;

			if( val < 40 )
			{
				if( obd_temp_coolant_zero_cnt == 0 ) obd_temp_coolant = val-40;
				else obd_temp_coolant_zero_cnt--;
			}
			else
			{
				obd_temp_coolant_zero_cnt = OBD_TEMP_ZERO_COUNT_VAL;
				obd_temp_coolant = val-40;
			}

			#ifdef DISPLAY_OBD_DATA
			draw_text_printk( 10, DISPLAY_LINE(2), "Temp   %d     ", val-40 );
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
	uint32_t baud;

    obd_rpm = 0;
    obd_speed = 0;
    obd_temp_coolant = 0;
    obd_temp_intake = 0;
    obd_load = 0;
    obd_map = 0;

    obd_send = TRUE;

    obd_rpm_zero_cnt = 0;
    obd_speed_zero_cnt = 0;
    obd_temp_coolant_zero_cnt = 0;

    obd_ready = FALSE;
    //timer_Start(&obd_send_timer, OBD_SEND_INTERVAL_MS);

    obd_engine_init();
    obd_pipe = pipe_locate( OBD_PIPE_NAME );

    timer_Stop( &obd_send_timeout );

    dashboard_dev = device_open( DEV_PANEL );
    if( dashboard_dev != NULL )
    {
    	baud = 115200;
        device_ioctrl( dashboard_dev, DEVICE_SET_CONFIG_DATA, &baud );
    }

}

void obd_set_ignition_off( void )
{
	obd_rpm = 0;
}

void obd_send_process( void )
{
	uint8_t rx_byte;
	uint16_t obd_mask;
	uint16_t temp_val;
	uint32_t odometer_cnt = 0;

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

    if( ( dashboard_dev != NULL ) && ( timer_Check( &obd_send_timeout ) ) )
    {
    	timer_Stop( &obd_send_timeout );
    	obd_send = TRUE;
    }

    if( dashboard_dev != NULL )
    {
    	if( device_is_rx_pending( dashboard_dev ) )
    	{
    		if( device_read_buffer( dashboard_dev, &rx_byte, sizeof( rx_byte ) ) )
    		{
    			if( rx_byte == OBD_MESSAGE_ACK )
    			{
    				//DEBUG_PRINTK( "Instrument Resp\r\n" );
    				timer_Stop( &obd_send_timeout );
    				obd_send = TRUE;
    			}
    		}
    	}
    }

    if( ( obd_send == TRUE ) && ( dashboard_dev != NULL ) )
    {
    	timer_Start( &obd_send_timeout, OBD_SEND_TIMEOUT_MS );
    	obd_send = FALSE;
    	obd_mask = pio_check_read();
    	odometer_cnt = odometer_control_get();

    	obd_string_send[0] = 0xF8;
    	obd_string_send[1] = ( uint8_t )obd_speed;

    	temp_val = ( uint16_t )obd_temp_coolant + 40;
    	if( temp_val > 255 ) temp_val = 255;

        memcpy( &obd_string_send[2], &obd_rpm, sizeof( uint16_t ) );
        memcpy( &obd_string_send[4], &odometer_cnt, sizeof( uint32_t ) );
        memcpy( &obd_string_send[8], &obd_mask, sizeof( uint16_t ) );
        memcpy( &obd_string_send[10], &temp_val, sizeof( uint8_t ) );

        obd_string_send[11] = 0xA8;

    	//sprintk( obd_string_send, "obd_send %d,%d,%d,%d\r\n", obd_rpm, obd_speed, obd_temp_coolant, obd_mask );
    	device_write_buffer( dashboard_dev, ( uint8_t * )obd_string_send, 12 );
    }

    obd_engine_process();
}
