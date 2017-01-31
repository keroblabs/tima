#include "system.h"
#include "odometer_control.h"
#include "timer.h"
#include "pipe.h"
#include "instruments.h"
#include "debug.h"

#include "syscomm.h"
#include "instruments.h"
#include "gui_setup.h"
//#include "setup_data.h"

///////////////////////////////////////////////////////////////

#define SETUP_DATA_HEADER			0x438D
#define ODOMETER_RATIO_COUNTER		2844

#define UPDATE_PERIOD_MS			1500

#define TOTAL_TRIPS					3

///////////////////////////////////////////////////////////////

static bool_t is_trip_ready;

static timer_data_t update_ms;

static pipe_data_t * pipe_request;
static pipe_data_t * pipe_response;

static uint32_t trip_value[3];

///////////////////////////////////////////////////////////////

void odometer_control_init( void )
{
    uint8_t command;
    
    is_trip_ready = FALSE;
    
    pipe_request = syscomm_get_pipe_request();
    pipe_response = syscomm_get_pipe_response();
    
    memset( trip_value, 0x00, sizeof( trip_value ) );
    
    instruments_show_information( trip_value, SETUP_PARAM_LOAD_TRIP );

    // on init, request trip and odometer
    command = SYSCOMM_PIPE_TRIP_READ;
    pipe_send_buffer( pipe_request, &command, sizeof( command ) );
    
    command = SYSCOMM_PIPE_ODOMETER_READ;
    pipe_send_buffer( pipe_request, &command, sizeof( command ) );
    
	timer_Start( &update_ms, UPDATE_PERIOD_MS );
    timer_Stop( &update_ms );
}

void odometer_control_process( void )
{
    uint8_t * pipe_data;
    uint16_t pipe_size;
    uint8_t command;
    uint32_t odometer_value;
    
    if( pipe_response == NULL ) return;
    
    if( !pipe_is_empty( pipe_response ) )
    {
        pipe_data = pipe_read( pipe_response, &pipe_size );
        
        if( pipe_data[0] == SYSCOMM_PIPE_ODOMETER_READ )
        {
            DEBUG_PRINTK( "SYSCOMM_PIPE_ODOMETER_READ\r\n" );
            memcpy( &odometer_value, &pipe_data[1], sizeof( odometer_value ) );
            
            if( is_trip_ready == TRUE )
            {
                is_trip_ready = FALSE;
                instruments_show_information( trip_value, SETUP_PARAM_LOAD_TRIP );
            }
   
            skin_motocycle_show_odometer_main( odometer_value );
            pipe_release_buffer( pipe_data );
            timer_Reload( &update_ms );
        }
        else if( pipe_data[0] == SYSCOMM_PIPE_TRIP_READ )
        {
            is_trip_ready = TRUE;
            
            DEBUG_PRINTK( "SYSCOMM_PIPE_TRIP_READ\r\n" );
            
            memcpy( trip_value, &pipe_data[1], sizeof( trip_value ) );
            pipe_release_buffer( pipe_data );
            timer_Reload( &update_ms );
        }
        else
        {
            // not to be handled, send it back
            //DEBUG_PRINTK( "Unknown response\r\n" );
            pipe_write( pipe_response, pipe_data, pipe_size );
        }
        
    }
    
	if( timer_Check( &update_ms ) )
	{
		timer_Stop( &update_ms );

        command = SYSCOMM_PIPE_ODOMETER_READ;
        pipe_send_buffer( pipe_request, &command, sizeof( command ) );
	}
}


