#include "obd_engine.h"

#include "system.h"
#include "timer.h"
#include "tima_libc.h"

#include "instruments.h"
#include "gui_setup.h"

//#include "draw.h"
//#include "pipe.h"
#include "debug.h"

///////////////////////////////////////////////////////////////
/*

PID to verify:

at sh A4 10 F1

221172
- 61 11 72 D7 (D7 = voltage?)

*/
///////////////////////////////////////////////////////////////

typedef void ( *obd_handler_t )( void );

///////////////////////////////////////////////////////////////

typedef struct _obd_command_t
{
	uint8_t command;
	obd_handler_t handler;

} obd_command_t;

enum
{
	OBD_AUTOUPDATE_IDLE,
	OBD_AUTOUPDATE_SEND_COMMAND,
	OBD_AUTOUPDATE_WAIT_RESP,
	OBD_AUTOUPDATE_AUTO_DONE,
	OBD_AUTOUPDATE_RESTART,
};

///////////////////////////////////////////////////////////////

static void obd_request_engine_load( void );
static void obd_request_coolant_temperature( void );
static void obd_request_intake_pressure( void );
static void obd_request_rpm( void );
static void obd_request_speed( void );
static void obd_request_intake_temperature( void );

///////////////////////////////////////////////////////////////

#define DISPLAY_OBD_DATA

///////////////////////////////////////////////////////////////

#define OBD_TEMP_ZERO_COUNT_VAL		3
#define OBD_RPM_ZERO_COUNT_VAL		2
#define OBD_SPEED_ZERO_COUNT_VAL	2

#define ELM327_HEADER				"ELM327"
#define ELM327_COMMAND_OK			"OK"
#define ELM327_SEARCHING_STRING		"SEARCHING..."
#define ELM327_NOTFOUND_STRING		"UNABLE TO CONNECT"
#define ELM327_RESP_TIMER			300
#define ELM327_STARTUP_TIMER		1500
#define ELM327_RESP_TIMER_LONG		3000
#define ELM327_LOW_READING_MS		3000

#define TEMPERATURE_WARNING_ON		105
#define TEMPERATURE_WARNING_OFF		100

#define TEMPERATURE_FLASHING_MS		750

#define OBD_CYCLE_DELAY_MS			300
#define OBD_LOW_READING_INDEX		2

static const char response_no_answer[] =
{ 'S','E','A','R','C','H','I','N','G','.','.','.',0x0d,
  'U','N','A','B','L','E',' ','T','O',' ','C','O','N','N','E','C','T',0x0d,0x0d,0x00 };

static const char response_echo_ok[] =
{ 'a','t','e','0',0x0d,
  'O','K',0x0d,0x0d,0x00 };

static const char response_ok[] =
{ 'O','K',0x0d,0x0d,0x00 };

static const char response_no_data[] =
{ 'N','O',' ','D','A','T','A',0x0d,0x0d,0x00 };

static const char response_data_error[] =
{ 'D','A','T','A',' ','E','R','R','O','R',0x0d,0x0d,0x00 };

///////////////////////////////////////////////////////////////

static const obd_command_t obd_command_list[] =
{
	{ OBD_COMMAND_RPM, 					obd_request_rpm },
	{ OBD_COMMAND_SPEED, 				obd_request_speed },


	{ OBD_COMMAND_TEMPERATURE, 			obd_request_coolant_temperature },

	//{ OBD_COMMAND_ENGINE_LOAD,			obd_request_engine_load },
	//{ OBD_COMMAND_FUEL_PRESSURE,		obd_request_fuel_pressure },

	//{ OBD_COMMAND_INTAKE_PRESSURE,		obd_request_intake_pressure },
	//{ OBD_COMMAND_INTAKE_TEMPERATURE, 	obd_request_intake_temperature },

	// { OBD_COMMAND_ODOMETER,				NULL },
	{ OBD_COMMAND_END,					NULL },
};

///////////////////////////////////////////////////////////////

static timer_data_t obd_resp_delay;
device_t * tty_obd;

static uint8_t obd_state;

static bool_t obd_connected;
static bool_t obd_ready_to_send;

static char obd_response[100];
static uint8_t obd_response_index;

static int obd_value_return;
static bool_t obd_return_ready;
static bool_t obd_do_low_reading;
static uint8_t obd_command_status;
static bool_t temp_flashing;
static bool_t temp_flash_state;

static uint8_t obd_auto_state;
static uint8_t obd_auto_index;
static uint8_t obd_auto_stop_pending;

static timer_data_t obd_command_cycle_ms;
static timer_data_t obd_auto_wait_ms;
static timer_data_t obd_low_reading_timer;
static timer_data_t temp_flash_timer;

static int obd_rpm_zero_cnt;
static int obd_speed_zero_cnt;
static int obd_temp_coolant_zero_cnt;

static int obd_rpm;
static int obd_speed;
static int obd_temp_coolant;

//static int obd_temp_intake;
//static int obd_load;
//static int obd_map;

///////////////////////////////////////////////////////////////

void skin_motocycle_draw_temperature_gauge( int input_temperature );
void skin_motocycle_draw_instrument_rpm( int rpm_value );

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

static void obd_send_at_command( char * cmd )
{
	char cmd_out[20];

	strcpy( cmd_out, cmd );
	strcat( cmd_out, "\r" );
	device_write_buffer( tty_obd, ( uint8_t * )cmd_out, (uint32_t)strlen( cmd_out ) );
}

static void obd_elm327_request_data( uint32_t mode, uint32_t pid )
{
	char cmd_out[20];
	sprintk( cmd_out, "%02x%02x", mode, pid );
	obd_send_at_command( cmd_out );
}

static void obd_request_engine_load( void )
{
	obd_elm327_request_data( 0x01, 0x04 );
}

static void obd_request_coolant_temperature( void )
{
	obd_elm327_request_data( 0x01, 0x05 );
}

static void obd_request_intake_pressure( void )
{
	obd_elm327_request_data( 0x01, 0x10 );
}

static void obd_request_rpm( void )
{
	obd_elm327_request_data( 0x01, 0x0c );
}

static void obd_request_speed( void )
{
	obd_elm327_request_data( 0x01, 0x0d );
}

static void obd_request_intake_temperature( void )
{
	obd_elm327_request_data( 0x01, 0x0F );
}

static void obd_engine_send_data( uint8_t command, char * response )
{
#ifdef DISPLAY_OBD_DATA
	char debug_string[50];
#endif

	int val = app_extract_data(response);

	switch( command )
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
			//sprintk( debug_string, "RPM    %d     ", val>>2 );
			//instruments_show_information( debug_string, SETUP_PARAM_DISPLAY2_LINE1 );
			#endif

			skin_motocycle_draw_instrument_rpm( ( int )obd_rpm );
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
			//sprintk( debug_string, "Speed  %d     ", val );
			//instruments_show_information( debug_string, SETUP_PARAM_DISPLAY2_LINE2 );
			#endif

			skin_motocycle_draw_instrument_speed( ( int )obd_speed );
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

			if( obd_temp_coolant >= TEMPERATURE_WARNING_ON )
			{
				temp_flashing = TRUE;
			}
			else if( obd_temp_coolant < TEMPERATURE_WARNING_OFF )
			{
				temp_flashing = FALSE;
			}

			#ifdef DISPLAY_OBD_DATA
			sprintk( debug_string, "Temp   %d     ", val-40 );
			instruments_show_information( debug_string, SETUP_PARAM_DISPLAY2_LINE4 );
			#endif

			skin_motocycle_draw_temperature_gauge( ( int )obd_temp_coolant );
			break;
	}
}

static void obd_engine_update_data( void )
{
	//obd_data_t * obd_resp;

	switch( obd_auto_state )
	{
		case OBD_AUTOUPDATE_IDLE:
			break;

		case OBD_AUTOUPDATE_SEND_COMMAND:
			if( obd_auto_stop_pending == TRUE )
			{
				obd_auto_stop_pending = FALSE;
				obd_auto_state = OBD_AUTOUPDATE_IDLE;
				break;
			}

			if( obd_command_list[obd_auto_index].command == OBD_COMMAND_END )
			{
				obd_auto_state = OBD_AUTOUPDATE_RESTART;

				obd_auto_index = 0;
				obd_do_low_reading = FALSE;
				obd_ready_to_send = TRUE;

				if( timer_Check( &obd_low_reading_timer ) )
				{
					timer_Reload( &obd_low_reading_timer );
					obd_do_low_reading = TRUE;
				}
				break;
			}

			if( ( obd_auto_index >= OBD_LOW_READING_INDEX ) && ( obd_do_low_reading == FALSE ) )
			{
				// at the moment skip low reading
				obd_auto_index++;
				break;
			}

			//DEBUG_PRINTK( "Sending %d\r\n", obd_auto_index );
			obd_engine_request( obd_command_list[obd_auto_index].command );

			if( obd_command_status == OBD_STATUS_WAITING )
			{
				obd_auto_state = OBD_AUTOUPDATE_WAIT_RESP;
				timer_Start( &obd_auto_wait_ms, ELM327_RESP_TIMER_LONG );
			}
			else
			{
				obd_auto_state = OBD_AUTOUPDATE_AUTO_DONE;
			}
			break;

		case OBD_AUTOUPDATE_WAIT_RESP:
			if( ( obd_command_status > OBD_STATUS_DONE ) || ( timer_Check( &obd_auto_wait_ms ) ) )
			{
				if( ( obd_command_status == OBD_STATUS_OK ) && ( obd_response[0] != 0 ) )
					obd_engine_send_data( obd_command_list[obd_auto_index].command, obd_response );
				//else
				//	obd_engine_send_data( obd_command_list[obd_auto_index].command, "---" );

				obd_auto_state = OBD_AUTOUPDATE_SEND_COMMAND;

				obd_auto_index++;
				timer_Stop( &obd_auto_wait_ms );
			}
			break;

		case OBD_AUTOUPDATE_RESTART:
			if( timer_Check( &obd_command_cycle_ms ) )
			{
				timer_Reload( &obd_command_cycle_ms );
				obd_auto_state = OBD_AUTOUPDATE_SEND_COMMAND;
			}
			break;
	}
}

void obd_engine_request( uint8_t cmd )
{
	uint8_t cnt;

	cnt = 0;
	while( obd_command_list[cnt].handler != NULL )
	{
		if( obd_command_list[cnt].command == cmd )
		{
			if( obd_command_list[cnt].handler != NULL ) obd_command_list[cnt].handler();
			obd_response_index = 0;
			obd_response[0] = 0;
			obd_return_ready = FALSE;
			obd_value_return = 0;
			obd_command_status = OBD_STATUS_WAITING;
			timer_Start( &obd_resp_delay, ELM327_RESP_TIMER_LONG );
			break;
		}

        cnt++;
	}

	if( obd_command_status != OBD_STATUS_WAITING )
	{
		obd_command_status = OBD_STATUS_ERROR;
	}
}

uint8_t obd_engine_get_response( int * value )
{
	if( value != NULL ) *value = obd_value_return;
	return obd_command_status;
}

void obd_engine_autoupdate( bool_t state )
{
	if( state )
	{
		obd_auto_state = OBD_AUTOUPDATE_SEND_COMMAND;
		timer_Start( &obd_command_cycle_ms, OBD_CYCLE_DELAY_MS );
	}
	else
	{
		obd_auto_stop_pending = TRUE;
	}
}

void obd_engine_process( void )
{
	char data;

	if( tty_obd == NULL ) return;

	if( temp_flashing || temp_flash_state )
	{
		if( timer_Check( &temp_flash_timer ) )
		{
			timer_Reload( &temp_flash_timer );
			temp_flash_state = ( temp_flash_state ) ? FALSE : TRUE;
			skin_motocycle_show_temperature_warning( temp_flash_state );
		}
	}

	obd_engine_update_data();

	if( obd_command_status < OBD_STATUS_DONE )
	{
		if( device_is_rx_pending( tty_obd ) )
		{
			device_read_buffer( tty_obd, ( uint8_t * )&data, 1 );

			//DEBUG_PRINTK( "%s\r\n", obd_response );
			if( data == '>' )
			{
				timer_Expire( &obd_resp_delay );

				if( !strcmp( obd_response, response_no_answer ) )
				{
					DEBUG_PRINTK( "No Response\r\n" );
					//draw_text_printk( 10, 90, "No Response  " );

					obd_command_status = OBD_STATUS_NO_RESPONSE;
                    obd_response_index = 0;
                    obd_response[0] = 0;
				}
				else if( !strcmp( obd_response, response_no_data ) )
				{
					DEBUG_PRINTK( "No Data\r\n" );
					//draw_text_printk( 10, 90, "No Data     " );

					obd_command_status = OBD_STATUS_NO_DATA;
                    obd_response_index = 0;
                    obd_response[0] = 0;
				}
				else if( !strcmp( obd_response, response_echo_ok ) )
				{
					DEBUG_PRINTK( "Echo off\r\n" );
					//draw_text_printk( 10, 90, "Echo off     " );

					obd_command_status = OBD_STATUS_OK;
                    obd_response_index = 0;
                    obd_response[0] = 0;
				}
				else if( !strcmp( obd_response, response_ok ) )
				{
					DEBUG_PRINTK( "Command OK\r\n" );
					//draw_text_printk( 10, 90, "Command OK     " );

					obd_command_status = OBD_STATUS_OK;
                    obd_response_index = 0;
                    obd_response[0] = 0;
				}
				else if( !strcmp( obd_response, response_data_error ) )
				{
					DEBUG_PRINTK( "Data Error\r\n" );
					//draw_text_printk( 10, 90, "Data Error     " );

					obd_command_status = OBD_STATUS_OK;
                    obd_response_index = 0;
                    obd_response[0] = 0;
				}
				else if( obd_state == OBD_RESPONSE_FLUSH )
				{
					DEBUG_PRINTK( "Ready\r\n" );
					//draw_text_printk( 10, 90, "Ready     " );

					obd_command_status = OBD_STATUS_OK;
                    obd_response_index = 0;
                    obd_response[0] = 0;
				}
				else if( strlen( obd_response ) >= 4 )
				{
					char * p_end;

					//DEBUG_PRINTK( "[Resp] %s\r\n", obd_response );
					//draw_text_printk( 10, 90, "[Resp] %s\r\n", obd_response );

					if( ( obd_response[2] == 0x20 ) && ( obd_response[5] == 0x20 ) )
					{
						// process response and update status
						p_end = strchr( obd_response, 0x0d );
						if( p_end != NULL ) *p_end = 0;
					}
					else
					{
	                    obd_response_index = 0;
	                    obd_response[0] = 0;
					}

					obd_command_status = OBD_STATUS_OK;
				}
			}
			else // if( data >= 0x20 )
            {
                obd_response[obd_response_index++] = data;
                obd_response[obd_response_index] = 0;
            }
		}
	}

	if( timer_Check( &obd_resp_delay ) )
	{
		timer_Stop( &obd_resp_delay );

		//DEBUG_PRINTK( "obd_command_status = %d\r\n", obd_command_status );
		//DEBUG_PRINTK( "obd_state = %d\r\n", obd_state );

		if( obd_command_status < OBD_STATUS_DONE )
		{
			// cursor not received, ignore data
            obd_response_index = 0;
            obd_response[0] = 0;
		}

		if( obd_state == OBD_RESPONSE_FLUSH )
		{
			obd_state = OBD_RESPONSE_INIT + 1;
		}

		if( ( obd_state >= OBD_RESPONSE_INIT ) && ( obd_state < OBD_RESPONSE_INIT_END ) )
		{
			timer_Start( &obd_resp_delay, ELM327_RESP_TIMER );

            obd_response_index = 0;
            obd_response[0] = 0;

			obd_command_status = OBD_STATUS_WAITING;

			obd_state++;
		}
		else if( obd_state == OBD_RESPONSE_INIT_END )
		{
            obd_response_index = 0;
            obd_response[0] = 0;

			obd_state = OBD_RESPONSE_READY;
			obd_auto_index = 0;
			obd_command_status = OBD_STATUS_IDLE;
		}
	}
}

bool_t obd_is_data_ready( void )
{
	bool_t ret = obd_ready_to_send;
	if( ret == TRUE )
		obd_ready_to_send = FALSE;
	return ret;
}

void obd_engine_init( void )
{
	uint32_t baud = 115200;
	tty_obd = device_open( DEV_OBD_TTY );
	if( tty_obd == NULL ) return;

	device_ioctrl( tty_obd, DEVICE_SET_CONFIG_DATA, &baud );

	timer_Start( &temp_flash_timer, TEMPERATURE_FLASHING_MS );

	obd_state = OBD_RESPONSE_FLUSH;
	obd_command_status = OBD_STATUS_IDLE;
	obd_auto_state = OBD_AUTOUPDATE_IDLE;

	temp_flashing = FALSE;
	temp_flash_state = FALSE;

	obd_auto_stop_pending = FALSE;
	obd_do_low_reading = FALSE;
	obd_connected = FALSE;
	obd_ready_to_send = FALSE;

	if( tty_obd != NULL )
	{
        obd_response_index = 0;
        obd_response[0] = 0;

		obd_command_status = OBD_STATUS_WAITING;
		obd_state = OBD_RESPONSE_FLUSH;

		// wait for longer timer than necessary to initialize the LCD
		// if this timer expires before the process runs, flushing doesnt work
		timer_Start( &obd_resp_delay, ELM327_STARTUP_TIMER );

		timer_Start( &obd_low_reading_timer, ELM327_LOW_READING_MS );

		timer_Stop( &obd_command_cycle_ms );
	}
}


