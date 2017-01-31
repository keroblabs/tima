#include "system.h"
#include "timer.h"

#include "draw.h"
#include "pipe.h"
#include "tima_libc.h"

#include "obd_control.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

#define OBD_BUFFER_MAX_SIZE             50

#define OBD_FLUSH_CYCLES                3

#define OBD_TIMEOUT_MS                  5000
#define OBD_FLUSH_INTERVAL              1000

#define OBD_TEMP_ZERO_COUNT_VAL         3
#define OBD_RPM_ZERO_COUNT_VAL          2
#define OBD_SPEED_ZERO_COUNT_VAL        2

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

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

static timer_data_t obd_flush_timer;
static timer_data_t obd_rx_timeout;

static device_t * obd_device;

static pipe_data_t request_pipe;
static pipe_data_t response_pipe;

static uint8_t curr_request;
static uint8_t obd_rx_size;
static uint8_t obd_flush_counter;

static bool_t check_echo_off;

static char obd_buffer[OBD_BUFFER_MAX_SIZE+1];

static int obd_rpm_zero_cnt;
static int obd_speed_zero_cnt;
static int obd_temp_coolant_zero_cnt;

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

static int obd_control_process_data( uint8_t command, char * response )
{
    int val = app_extract_data(response);
    
    switch( command )
    {
        case OBD_REQUEST_RPM:
            if( val > 32000 ) break;
            
            if( val == 0 )
            {
                if( obd_rpm_zero_cnt == 0 ) return val>>2;
                else obd_rpm_zero_cnt--;
            }
            else
            {
                obd_rpm_zero_cnt = OBD_RPM_ZERO_COUNT_VAL;
                return val>>2;
            }
            break;
            
        case OBD_REQUEST_SPEED:
            if( val > 250 ) break;
            
            if( val == 0 )
            {
                if( obd_speed_zero_cnt == 0 ) return val;
                else obd_speed_zero_cnt--;
            }
            else
            {
                obd_speed_zero_cnt = OBD_SPEED_ZERO_COUNT_VAL;
                return val;
            }
            break;
            
        case OBD_REQUEST_TEMPERATURE:
            if( val > 250 ) return 0-40;
            
            if( val < 40 )
            {
                if( obd_temp_coolant_zero_cnt == 0 ) return val-40;
                else obd_temp_coolant_zero_cnt--;
            }
            else
            {
                obd_temp_coolant_zero_cnt = OBD_TEMP_ZERO_COUNT_VAL;
                return val-40;
            }
            break;
    }
    
    return 0;
}

static void obd_send_at_command( const char * cmd )
{
    char cmd_out[20];
    
    strcpy( cmd_out, cmd );
    strcat( cmd_out, "\r" );
    device_write_buffer( obd_device, ( uint8_t * )cmd_out, (uint32_t)strlen( cmd_out ) );
}

static void obd_elm327_request_data( uint32_t mode, uint32_t pid )
{
    char cmd_out[20];
    sprintk( cmd_out, "%02x%02x", mode, pid );
    obd_send_at_command( cmd_out );
}

static void obd_request_echo_off( void )
{
    obd_send_at_command( "ate0" );
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

static void obd_control_send_pipe_result( uint8_t result, int value )
{
    obd_resp_t * resp;
    
    resp = (obd_resp_t * )pipe_alloc_buffer( sizeof( obd_resp_t ) );
    resp->request = curr_request;
    resp->response = result;
    resp->value = value;
    resp->timestamp = timer_get_MS();
    pipe_write( &response_pipe, ( uint8_t * )resp, sizeof( resp ) );
}

static void obd_process_command_response( void )
{
    uint8_t result = OBD_RESULT_NONE;
    int value = 0;
    
    //DEBUG_PRINTK( "RX: %s\r\n", obd_buffer );

    if( !strcmp( obd_buffer, response_no_answer ) )
	{
        result = OBD_RESULT_NO_ANSWER;
    }
	else if( !strcmp( obd_buffer, response_no_data ) )
    {
        result = OBD_RESULT_NO_DATA;
    }
    else if( !strcmp( obd_buffer, response_data_error ) )
    {
        result = OBD_RESULT_DATA_ERROR;
    }
    else if( !strcmp( obd_buffer, response_echo_ok ) )
    {
        result = OBD_RESULT_ECHO_OFF_OK;
    }
    else if( !strcmp( obd_buffer, response_ok ) )
    {
        result = OBD_RESULT_OK;
    }
    else if( strlen( obd_buffer ) >= 4 )
	{
        char * p_end;
        
        if( ( obd_buffer[2] == 0x20 ) && ( obd_buffer[5] == 0x20 ) )
        {
            // process response and update status
            p_end = strchr( obd_buffer, 0x0d );
            if( p_end != NULL ) *p_end = 0;
            
            result = OBD_RESULT_OK;
            value = obd_control_process_data( curr_request, obd_buffer );
        }
        else
        {
            result = OBD_RESULT_DATA_ERROR;
            obd_rx_size = 0;
            obd_buffer[0] = 0;
        }
    }

    obd_control_send_pipe_result( result, value );
    curr_request = OBD_RESULT_NONE;
}

static void obd_control_flush_tty( void )
{
    curr_request = OBD_REQUEST_FLUSH;
    obd_flush_counter = OBD_FLUSH_CYCLES;
    
    timer_Start( &obd_flush_timer, OBD_FLUSH_INTERVAL );
}

void obd_control_init( void )
{
	uint32_t baud = 115200;

    obd_device = device_open( DEV_OBD_TTY );
    if( obd_device != NULL )
    {
    	device_ioctrl( obd_device, DEVICE_SET_CONFIG_DATA, &baud );
    	DEBUG_PRINTK( "OBD ready\r\n" );

        pipe_init( &request_pipe, "OBD_REQUEST" , 8 );
        pipe_init( &response_pipe, "OBD_RESPONSE" , 8 );
        
        // first of all request echo off
        //obd_control_send_request( OBD_REQUEST_ECHO_OFF );
        check_echo_off = FALSE;
        
        obd_control_flush_tty();
    }
}

obd_resp_t * obd_control_get_response( void )
{
    obd_resp_t * rep;
    
    if( pipe_is_empty( &response_pipe ) ) return NULL;
    rep = ( obd_resp_t * )pipe_get( &response_pipe, NULL );
    
    return rep;
}

void obd_control_remove_response( void )
{
    uint8_t * p_data;
    
    p_data = pipe_read( &response_pipe, NULL );
    pipe_release_buffer( p_data );
}

void obd_control_send_request( uint8_t req )
{
    pipe_send_buffer( &request_pipe, &req, sizeof( req ) );
}

void obd_control_process( void )
{
    uint8_t rx_byte = 0;
    uint8_t * pipe_data;
    uint16_t pipe_size;
    
    if( obd_device == NULL ) return;
    
    if( curr_request == OBD_REQUEST_FLUSH )
    {
        if( timer_Check( &obd_flush_timer ) )
        {
            // send CR until the one before the last
            //if( obd_flush_counter ) obd_send_at_command( "" );
            
            // always flush
            device_read_buffer( obd_device, ( uint8_t * )obd_buffer, OBD_BUFFER_MAX_SIZE );

            // if on the last, stop timer and go to no request
            if( obd_flush_counter == 0 )
            {
                timer_Stop( &obd_flush_timer );
                curr_request = OBD_REQUEST_NONE;
            }

            // decrement only at the end
            if( obd_flush_counter ) obd_flush_counter--;
        }

        if( timer_Check( &obd_rx_timeout ) )
        {
            // flush tty, read data from time to time
            timer_Reload( &obd_rx_timeout );
        }
        
        // just return because in flush no data is processed
        return;
    }
    
    if( check_echo_off == TRUE )
    {
        obd_resp_t * p_resp;
        if( ( p_resp = obd_control_get_response() ) != NULL )
        {
            if( p_resp->request == OBD_REQUEST_ECHO_OFF )
            {
                check_echo_off = FALSE;
                obd_control_remove_response();
            }
        }
    }
    
    if( ( curr_request == OBD_REQUEST_NONE ) && ( !pipe_is_empty( &request_pipe ) ) )
    {
        // process request
        pipe_data = pipe_read( &request_pipe, &pipe_size );
        curr_request = pipe_data[0];
        
        switch( curr_request )
        {
            case OBD_REQUEST_SPEED:
                obd_request_speed();
                break;
                
            case OBD_REQUEST_RPM:
                obd_request_rpm();
                break;
                
            case OBD_REQUEST_TEMPERATURE:
                obd_request_coolant_temperature();
                break;
                
            case OBD_REQUEST_ECHO_OFF:
                obd_request_echo_off();
                break;
                
            case OBD_REQUEST_INTAKE_PRESSURE:
                obd_request_intake_pressure();
                break;

            case OBD_REQUEST_INTAKE_TEMP:
                obd_request_intake_temperature();
                break;
            
            case OBD_REQUEST_ENGINE_LOAD:
                obd_request_engine_load();
                break;
                
            case OBD_REQUEST_FLUSH:
                // exception case, return from here
                // so flush start at next run
                return;
                
            default:
                curr_request = OBD_REQUEST_NONE;
                break;
        }
        
        // release after handled
        pipe_release_buffer( pipe_data );
        
        // valid request: reset buffer and start timeout
        if( curr_request != OBD_REQUEST_NONE )
        {
            timer_Start( &obd_rx_timeout, OBD_TIMEOUT_MS );
            obd_rx_size = 0;
        }
    }
    
    if( curr_request != OBD_REQUEST_NONE )
    {
        // get rx bytes
        while( device_is_rx_pending( obd_device ) )
        {
            rx_byte = device_read( obd_device );

            // if it's prompt, process command
            if( rx_byte == '>' )
            {
                timer_Stop( &obd_rx_timeout );
                obd_process_command_response();
                obd_rx_size = 0;

                break;
            }
            
            // append received data
            obd_buffer[obd_rx_size++] = rx_byte;
            obd_buffer[obd_rx_size] = 0;
        }
    }
    
    if( timer_Check( &obd_rx_timeout ) )
    {
        // on timeout, flush tty
        obd_control_send_pipe_result( OBD_RESULT_TIMEOUT, 0 );
        timer_Stop( &obd_rx_timeout );

        //obd_control_flush_tty();
        curr_request = OBD_REQUEST_NONE;
    }
}

