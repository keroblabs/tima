#include "system.h"
#include "timer.h"

#include "pio_check.h"
#include "warning_lights.h"
#include "instruments.h"
#include "battery_check.h"
#include "gui_setup.h"
//#include "setup_data.h"
#include "message.h"
#include "pipe.h"
#include "syscomm.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

#define PIO_CHECK_UPDATE( flag, prev_flag, handler )        if( prev_flag != flag ) \
															{ \
																prev_flag = flag; \
																handler( flag ); \
															}

///////////////////////////////////////////////////////////////

#define PIO_CHECK_UPDATE_MS			400
#define WARNING_CHECK_MS			1000

#define TEMP_FLASHING_MS			800
#define BRAKE_FLASHING_MS			300
#define LONG_PRESS_TIME_MS			1500
#define BITMASK_UPDATE_MS           200

///////////////////////////////////////////////////////////////

static char pio_check_string[50];

static pipe_data_t * pipe_request;
static pipe_data_t * pipe_response;

static device_t * pio_handbrake0_dev;
static device_t * pio_handbrake1_dev;

static void * pio_indicator0_dev;
static void * pio_indicator1_dev;
static void * pio_setup_pio_dev;
static void * pio_fullbeam_dev;
static void * pio_oil_dev;
static void * pio_battery_dev;
static void * pio_brake_dev;
static void * pio_mode_dev;

static uint8_t oil_input_pio;
static uint8_t handbrake0_pio;
static uint8_t handbrake1_pio;
static uint8_t fullbeam_pio;
static uint8_t battery_pio;
static uint8_t brake_pio;
static uint8_t indicator0_pio;
static uint8_t indicator1_pio;
static uint8_t setup_button_pio;
static uint8_t mode_button_pio;

static timer_data_t update_pio_ms;
static timer_data_t flashing_ms;
static timer_data_t long_press_ms;
static timer_data_t update_ms;

static uint32_t bitmask_data;

static uint8_t handbrake_state;
static uint8_t prev_handbrake_state;
static uint8_t prev_button_state;

static bool_t flashing_brake_flag;
static bool_t power_down;

static bool_t engine_run_flag;
static bool_t engine_run_state;
static bool_t headlights_state;

static message_data_t button_msg;

///////////////////////////////////////////////////////////////

typedef struct _gpio_event_t_
{
	bool_t pio_states[3];
	bool_t is_invert;
	bool_t is_init;
	uint8_t pio_flag;
	uint16_t pio_id;
	timer_data_t interval;
	device_t dev;

	void (*event_handler)( void * event, uint16_t id );

	struct _gpio_event_t_ * p_next;

} gpio_event_t;

///////////////////////////////////////////////////////////////

bool_t pio_check_event_read_port( void * p_data, bool_t * state )
{
	gpio_event_t * port_data = ( gpio_event_t * )p_data;
	bool_t ret = FALSE;

	if( p_data == NULL ) return FALSE;

	if( port_data->is_init == TRUE )
	{
		port_data->is_init = FALSE;
		port_data->pio_flag = pio_check_read_input( &port_data->dev, port_data->is_invert ) ? FALSE : TRUE;
		if( state ) *state = port_data->pio_flag;
		return TRUE;
	}

	if( timer_Check( &port_data->interval ) == FALSE ) return FALSE;
	timer_Reload( &port_data->interval );

	port_data->pio_states[1] = port_data->pio_states[2];
	port_data->pio_states[2] = pio_check_read_input( &port_data->dev, port_data->is_invert );

	if( ( port_data->pio_states[1] == port_data->pio_states[2] ) && ( port_data->pio_states[0] != port_data->pio_states[1] ) )
	{
		port_data->pio_states[0] = port_data->pio_states[1];
		if( ( port_data->pio_states[2] ) && ( port_data->pio_flag == FALSE ) )
		{
			port_data->pio_flag = TRUE;
			ret = TRUE;
		}
		else if( ( !port_data->pio_states[2] ) && ( port_data->pio_flag == TRUE ) )
		{
			port_data->pio_flag = FALSE;
			ret = TRUE;
		}

		//DEBUG_PRINTK( "Door left = %d\r\n", pio_door_left[2] );
	}

	if( ( ret == TRUE ) && ( state != NULL ) )
	{
		*state = port_data->pio_flag;
	}

	return ret;
}

void * pio_check_event_open_read_port( const char * dev_name, void * event_handler, uint16_t id, bool_t is_invert, uint32_t interval )
{
	gpio_event_t * port_data;

	port_data = ( gpio_event_t * )MMALLOC( sizeof( gpio_event_t ) );
	if( port_data == NULL ) return NULL;

	device_open_ex( dev_name, &port_data->dev );
	if( device_open_ex( dev_name, &port_data->dev ) != FALSE )
	{
		if( port_data != NULL )
		{
			DEBUG_PRINTK( "%s OK\r\n", dev_name );

			port_data->is_init = TRUE;
			port_data->event_handler = event_handler;
			port_data->pio_id = id;
			port_data->is_invert = is_invert;
			memset( port_data->pio_states, 0x00, sizeof( port_data->pio_states ) );

			port_data->pio_flag = pio_check_read_input( &port_data->dev, port_data->is_invert ) ? FALSE : TRUE;
			port_data->pio_states[0] = port_data->pio_states[1] = port_data->pio_states[2] = port_data->pio_flag;

			timer_Start( &port_data->interval, interval );
		}
	}
	else
	{
		MFREE( port_data );
        return NULL;
	}

	return port_data;
}

bool_t pio_check_event_get_state( void * p_event )
{
	gpio_event_t * event = ( gpio_event_t * )p_event;
	if( p_event == NULL ) return FALSE;
	return pio_check_read_input( &event->dev, event->is_invert );
}

////////////////////////////////////////////////////////////////////////////////////////

device_t * pio_check_open_output( const char * dev_name, uint8_t state )
{
	device_t * dev_ret;

	dev_ret = device_open( dev_name );
	if( dev_ret != NULL )
	{
		DEBUG_PRINTK( "%s OK\r\n", dev_name );
		device_write_buffer( dev_ret, &state, sizeof( state ) );
	}

	return dev_ret;
}

device_t * pio_check_open_input( const char * dev_name, uint8_t * pio_var )
{
	device_t * dev_ret;

	dev_ret = device_open( dev_name );
	if( dev_ret != NULL )
	{
		//DEBUG_PRINTK( "%s (0x%08x) OK\r\n", dev_name, (int)dev_ret );
		DEBUG_PRINTK( "%s OK\r\n", dev_name );

		if( pio_var != NULL )
			device_read_buffer( dev_ret, pio_var, sizeof( uint8_t ) );
	}

	return dev_ret;
}

uint8_t pio_check_read_input( device_t * p_dev, bool_t invert )
{
	uint8_t pio_var = FALSE;

	if( p_dev != NULL )
	{
		device_read_buffer( p_dev, &pio_var, sizeof( uint8_t ) );
		if( invert ) pio_var = pio_var ? FALSE : TRUE;
	}

	return pio_var;
}

void pio_check_set_output( device_t * p_dev, uint8_t state )
{
	if( p_dev != NULL )
	{
		device_write_buffer( p_dev, &state, sizeof( state ) );
	}
}

void pio_check_save_trip_data( void )
{
    uint8_t * buffer;
    uint32_t trip_data[3];
    
    instruments_show_information( trip_data, SETUP_PARAM_SAVE_TRIP );

    buffer = pipe_alloc_buffer(sizeof( trip_data ) + 1 );
    buffer[0] = SYSCOMM_PIPE_TRIP_WRITE;
    memcpy( &buffer[1], trip_data, sizeof( trip_data ) );
    
    // request save the trip value
    pipe_write( pipe_request, buffer, sizeof( trip_data ) + 1 );
}

void pio_check_send_bitmask_data( void )
{
    uint8_t bitmask8;
    uint8_t * buffer;

    bitmask8 = 0;
    
    if( battery_pio )    bitmask8 |= 0x01;
    if( oil_input_pio )  bitmask8 |= 0x02;
    if( fullbeam_pio  )  bitmask8 |= 0x04;
    if( handbrake0_pio ) bitmask8 |= 0x08;
    if( handbrake1_pio ) bitmask8 |= 0x10;
    
    buffer = pipe_alloc_buffer( sizeof( bitmask8 ) + 1 );
    buffer[0] = SYSCOMM_PIPE_BITMASK_WRITE;
    buffer[1] = bitmask8;
    pipe_write( pipe_request, buffer, sizeof( bitmask8 ) + 1 );
}

void pio_check_init( void )
{
    handbrake_state = PIO_HANDBRAKE_MOVING;
	prev_handbrake_state = 0xFF;

	prev_button_state = FALSE;
    power_down = FALSE;

    engine_run_state = FALSE;
    engine_run_flag = FALSE;
    headlights_state = FALSE;
    
    bitmask_data = 0;
    
    pipe_request = syscomm_get_pipe_request();
    pipe_response = syscomm_get_pipe_response();

	pio_handbrake0_dev = pio_check_open_input( "/dev/gpio17", &handbrake0_pio );
	pio_handbrake1_dev = pio_check_open_input( "/dev/gpio16", &handbrake1_pio );

	pio_oil_dev =        pio_check_event_open_read_port( "/dev/gpio15", NULL, 0, FALSE, 50 );
	pio_fullbeam_dev =   pio_check_event_open_read_port( "/dev/gpio14", NULL, 0, TRUE, 50  );
	pio_brake_dev =      pio_check_event_open_read_port( "/dev/gpio13", NULL, 0, TRUE, 50  );
	pio_battery_dev =    pio_check_event_open_read_port( "/dev/gpio12", NULL, 0, FALSE, 50 );
	pio_indicator0_dev = pio_check_event_open_read_port( "/dev/gpio10", NULL, 0, TRUE, 50 );
	pio_indicator1_dev = pio_check_event_open_read_port( "/dev/gpio11", NULL, 0, TRUE, 50 );

	pio_setup_pio_dev =  pio_check_event_open_read_port( "/dev/gpio4",  NULL, 0, FALSE, 50 );
	pio_mode_dev =       pio_check_event_open_read_port( "/dev/gpio3",  NULL, 0, TRUE, 50 );

	timer_Start( &update_pio_ms, PIO_CHECK_UPDATE_MS );
	timer_Start( &flashing_ms, BRAKE_FLASHING_MS );
    timer_Start( &update_ms, BITMASK_UPDATE_MS );
	timer_Stop( &long_press_ms );

	message_Init( &button_msg );
}

void pio_check_bitmask_handler( void )
{
    uint8_t command;

    // engine run flag
    if( !engine_run_flag && ( bitmask_data & GEM_FLAG_ENGINE_RUN ) )
    {
    	engine_run_flag = TRUE;
    	skin_motocycle_show_heaplamp_warning( TRUE );
    }
    else if( engine_run_flag && !( bitmask_data & GEM_FLAG_ENGINE_RUN ) )
    {
    	engine_run_flag = FALSE;
    	skin_motocycle_show_heaplamp_warning( FALSE );
    }

    // headlights flag
    if( !headlights_state && ( bitmask_data & GEM_FLAG_HEADLIGHTS ) )
    {
    	headlights_state = TRUE;
    	if( engine_run_flag ) skin_motocycle_show_heaplamp_warning( FALSE );
    }
    else if( headlights_state && !( bitmask_data & GEM_FLAG_HEADLIGHTS ) )
    {
    	headlights_state = FALSE;
    	if( engine_run_flag ) skin_motocycle_show_heaplamp_warning( TRUE );
    }

    // power detection control
    if( !( bitmask_data & GEM_FLAG_ACC_ON ) && ( power_down == FALSE ) )
    {
        // acc turned off
        power_down = TRUE;

        // request save the trip value
        pio_check_save_trip_data();
    }
    else if( ( bitmask_data & GEM_FLAG_ACC_ON ) && ( power_down == TRUE ) )
    {
        // acc back on
        power_down = FALSE;

        // send power up
        command = SYSCOMM_PIPE_POWERUP;
        pipe_send_buffer( pipe_request, &command, sizeof( command ) );
    }
}

void pio_check_bitmask_process( void )
{
    uint8_t * pipe_data;
    uint16_t pipe_size;
    uint8_t command;
    
    if( pipe_response == NULL ) return;
    
    if( !pipe_is_empty( pipe_response ) )
    {
        pipe_data = pipe_read( pipe_response, &pipe_size );
        
        if( pipe_data[0] == SYSCOMM_PIPE_BITMASK_READ )
        {
            memcpy( &bitmask_data, &pipe_data[1], sizeof( bitmask_data ) );
            
            // update bitmask
            pio_check_bitmask_handler();
            
            // send flags to GEM
            pio_check_send_bitmask_data();
            pipe_release_buffer( pipe_data );
        }
        else if( pipe_data[0] == SYSCOMM_PIPE_BITMASK_WRITE )
        {
            // after flags sent to GEM, start timer
            timer_Reload( &update_ms );
            pipe_release_buffer( pipe_data );
        }
        else if( pipe_data[0] == SYSCOMM_PIPE_TRIP_WRITE )
        {
            // after trip data was saved, request powerdown
            command = SYSCOMM_PIPE_POWERDOWN;
            pipe_send_buffer( pipe_request, &command, sizeof( command ) );
            pipe_release_buffer( pipe_data );
        }
        else if( pipe_data[0] == SYSCOMM_PIPE_POWERDOWN )
        {
            // after powerdown, do nothing
            pipe_release_buffer( pipe_data );
        }
        else
        {
            // not to be handled, send it back
            pipe_write( pipe_response, pipe_data, pipe_size );
        }
    }
    
    if( timer_Check( &update_ms ) )
    {
        timer_Stop( &update_ms );
        command = SYSCOMM_PIPE_BITMASK_READ;
        pipe_send_buffer( pipe_request, &command, sizeof( command ) );
    }
}

void pio_check_pio_process( void )
{
	if( pio_check_event_read_port( pio_fullbeam_dev, &fullbeam_pio ) )
	{
		skin_motocycle_show_fullbeam_warning(fullbeam_pio);
	}

	if( pio_check_event_read_port( pio_oil_dev, &oil_input_pio ) )
	{
		skin_motocycle_show_oil_warning(oil_input_pio);
	}

	if( pio_check_event_read_port( pio_battery_dev, &battery_pio ) )
	{
		skin_motocycle_show_battery_warning(battery_pio);
	}

	if( pio_check_event_read_port( pio_indicator0_dev, &indicator0_pio ) )
	{
		skin_motocycle_show_indicator_warning( INDICATOR_NONE_FLAG );

        if( indicator0_pio ) {
        	if( indicator1_pio ) skin_motocycle_show_indicator_warning( INDICATOR_HAZARD_FLAG | INDICATOR_LEFT_FLAG );
        	else skin_motocycle_show_indicator_warning( INDICATOR_LEFT_FLAG );
        } else if( indicator1_pio ) skin_motocycle_show_indicator_warning( INDICATOR_RIGHT_FLAG );
	}

	if( pio_check_event_read_port( pio_indicator1_dev, &indicator1_pio ) )
	{
		skin_motocycle_show_indicator_warning( INDICATOR_NONE_FLAG );

		if( indicator1_pio ) {
        	if( indicator0_pio ) skin_motocycle_show_indicator_warning( INDICATOR_HAZARD_FLAG | INDICATOR_RIGHT_FLAG );
        	else skin_motocycle_show_indicator_warning( INDICATOR_RIGHT_FLAG );
        } else if( indicator0_pio ) skin_motocycle_show_indicator_warning( INDICATOR_LEFT_FLAG );
	}

    /////////
	if( pio_check_event_read_port( pio_setup_pio_dev, &setup_button_pio ) )
	{
		if( setup_button_pio )
		{
			//instruments_show_information( "down   ", SETUP_PARAM_DISPLAY2_LINE3 );
			if( prev_button_state == FALSE ) timer_Start( &long_press_ms, LONG_PRESS_TIME_MS );
			prev_button_state = TRUE;
		}
		else if( prev_button_state == TRUE )
		{
			if( ( timer_Check( &long_press_ms ) == FALSE ) && ( !power_down ) )
			{
				message_Post( &button_msg, MESSAGE_BUTTON_SHORT );
			}

			//instruments_show_information( "up     ", SETUP_PARAM_DISPLAY2_LINE3 );
			prev_button_state = FALSE;
			timer_Stop( &long_press_ms );
		}
		else
		{
			//instruments_show_information( "up     ", SETUP_PARAM_DISPLAY2_LINE3 );
		}
	}

    if( engine_run_state != engine_run_flag )
    {
    	engine_run_state = engine_run_flag;
    }

	if( timer_Check( &long_press_ms ) )
	{
		timer_Stop( &long_press_ms );
		prev_button_state = FALSE;

		if( setup_button_pio && !power_down && engine_run_state )
		{
			//instruments_show_information( "long   ", SETUP_PARAM_DISPLAY2_LINE3 );
			message_Post( &button_msg, MESSAGE_BUTTON_LONG );
		}
	}
	/////////

	if( pio_check_event_read_port( pio_mode_dev, &mode_button_pio ) )
	{
		if( mode_button_pio ) message_Post( &button_msg, MESSAGE_MODE_ON );
		else message_Post( &button_msg, MESSAGE_MODE_OFF );

		if( mode_button_pio ) instruments_show_information( "ON ", SETUP_PARAM_DISPLAY2_LINE2 );
		else instruments_show_information( "off", SETUP_PARAM_DISPLAY2_LINE2 );
	}

	if( pio_check_event_read_port( pio_brake_dev, &brake_pio ) )
	{
		skin_motocycle_show_brake_warning(brake_pio);
		if( brake_pio ) instruments_show_information( "Low brake fluid", SETUP_PARAM_DISPLAY1_LINE3 );
		else instruments_show_information( "                  ", SETUP_PARAM_DISPLAY1_LINE3 );
	}

	if( timer_Check( &update_pio_ms ) )
	{
		timer_Reload( &update_pio_ms );

		handbrake0_pio = pio_check_read_input( pio_handbrake0_dev, TRUE );
		handbrake1_pio = pio_check_read_input( pio_handbrake1_dev, TRUE );

        if( brake_pio ) handbrake_state = PIO_HANDBRAKE_PULLED;
        else if( handbrake0_pio ) handbrake_state = PIO_HANDBRAKE_PULLED;
		else if( handbrake1_pio ) handbrake_state = PIO_HANDBRAKE_RELEASED;
		else handbrake_state = PIO_HANDBRAKE_MOVING;
	}

	if( handbrake_state != prev_handbrake_state )
	{
		prev_handbrake_state = handbrake_state;

		if( handbrake_state == PIO_HANDBRAKE_PULLED )
		{
			skin_motocycle_show_brake_warning( TRUE );
		}
		else if( handbrake_state == PIO_HANDBRAKE_RELEASED )
		{
			skin_motocycle_show_brake_warning( FALSE );
		}
	}

	if( handbrake_state == PIO_HANDBRAKE_MOVING )
	{
		if( timer_Check( &flashing_ms ) )
		{
			timer_Reload( &flashing_ms );
			flashing_brake_flag = flashing_brake_flag ? FALSE : TRUE;
			skin_motocycle_show_brake_warning( flashing_brake_flag );
		}
	}
}

void pio_check_process( void )
{
    pio_check_pio_process();
    pio_check_bitmask_process();
}

uint16_t pio_check_get_message( void )
{
	return message_Get( &button_msg );
}

void * pio_check_reset_message( void )
{
	message_Init( &button_msg );
	return &button_msg;
}

bool_t pio_check_setup_button( void )
{
	return setup_button_pio;
}

bool_t pio_check_mode_button( void )
{
	return mode_button_pio;
}

bool_t pio_check_get_fullbeam( void )
{
	return fullbeam_pio;
}

bool_t pio_check_get_oil_pressure( void )
{
	return oil_input_pio;
}

uint8_t pio_check_get_handbrake_state( void )
{
	return handbrake_state;
}

uint8_t pio_check_get_battery_state( void )
{
	return battery_pio;
}

bool_t pio_check_get_brake_level_state( void )
{
	return brake_pio;
}

bool_t pio_check_engine_run_state( void )
{
	return engine_run_state;
}

bool_t pio_check_power_down_flag( void )
{
	return power_down;
}
