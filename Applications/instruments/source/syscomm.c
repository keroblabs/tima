#include "system.h"
#include "timer.h"
#include "pipe.h"
#include "gui_setup.h"
//#include "nvram_control.h"
#include "debug.h"

#include "instruments.h"
#include "odometer_control.h"
#include "syscomm.h"

///////////////////////////////////////////////////////////////

#define SYSCOMM_TIMEOUT_MS			2500

#define SYSCOMM_POWERUP_REQ         0x9F
#define SYSCOMM_POWERUP_RESP        0x1F

#define SYSCOMM_ODOMETER_REQ		0x08
#define SYSCOMM_ODOMETER_RESP		0x88

#define SYSCOMM_BITMASK_REQ			0x1E
#define SYSCOMM_BITMASK_RESP		0x9E

#define SYSCOMM_STATUS_REQ			0x12
#define SYSCOMM_STATUS_RESP			0x92

#define SYSCOMM_POWERDOWN_REQ		0x0A
#define SYSCOMM_POWERDOWN_RESP		0x8A

#define SYSCOMM_NVRAM_RD_REQ        0x31
#define SYSCOMM_NVRAM_RD_RESP       0xB1

#define SYSCOMM_NVRAM_WR_REQ        0x44
#define SYSCOMM_NVRAM_WR_RESP       0xC4

#define SYSCOMM_MAGIC_VALUE         0x3A681E97

#define NVRAM_ADDR_NVRAM_ID         0x00
#define NVRAM_ADDR_ODOMETER         0x04
#define NVRAM_ADDR_TRIP             0x08
#define NVRAM_ADDR_BITMASK1         0x14
#define NVRAM_ADDR_BITMASK2         0x18

enum
{
	SYSCOMM_REQUEST_NONE,
    SYSCOMM_REQUEST_POWERUP,
    SYSCOMM_REQUEST_TRIP,
    SYSCOMM_REQUEST_SETUP,
    
	SYSCOMM_REQUEST_ODOMETER,
	SYSCOMM_REQUEST_BITMASK,
	SYSCOMM_SEND_STATUS,

    SYSCOMM_REQUEST_POWERDOWN,
    SYSCOMM_REQUEST_NVRAM_WR,
    
	SYSCOMM_MODE_MAX
};

///////////////////////////////////////////////////////////////

static timer_data_t syscomm_timeout;

static uint8_t * pipe_data;
static uint16_t pipe_size;

static device_t * syscomm_dev;
static uint8_t syscomm_buffer[32];

static bool_t is_pending;
static uint8_t rx_pending_bytes;
static uint8_t curr_request;
static uint8_t curr_command;

pipe_data_t syscomm_pipe_request;
static pipe_data_t syscomm_pipe_response;

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static uint8_t syscomm_checksum( uint8_t * buffer, uint32_t size )
{
    uint8_t ret = 0;
    uint32_t cnt;
    
    for( cnt = 0; cnt < size; cnt++ )
    {
        ret += buffer[cnt];
    }

    return ret;
}

static uint8_t syscomm_nvram_read_create_message( uint8_t addr, uint8_t size )
{
    uint8_t index = 0;
    
    syscomm_buffer[index++] = SYSCOMM_NVRAM_RD_REQ;
    syscomm_buffer[index++] = addr;
    syscomm_buffer[index++] = size;
    syscomm_buffer[index] = syscomm_checksum( &syscomm_buffer[1], index-1 );

    return index+1;
}

static uint8_t syscomm_nvram_write_create_message( uint8_t addr, uint8_t * data, uint32_t size )
{
    uint8_t size8 = size & 0x0FF;
    uint8_t cnt;
    uint8_t index;
    
    index = 0;
    
    syscomm_buffer[index++] = SYSCOMM_NVRAM_WR_REQ;
    syscomm_buffer[index++] = 0;
    syscomm_buffer[index++] = addr;
    syscomm_buffer[index++] = size8;
    
    for( cnt = 0; cnt < size8; cnt++ )
    {
        syscomm_buffer[index++] = data[cnt];
    }
    
    syscomm_buffer[index] = syscomm_checksum( &syscomm_buffer[2], index-2 );
    syscomm_buffer[1] = index-1;
    
    return index+1;
}

static uint8_t syscomm_powerup_create_message( void )
{
    uint32_t magic_data;
    uint8_t index;
    
    index = 0;
    magic_data = SYSCOMM_MAGIC_VALUE;
    
    syscomm_buffer[index++] = SYSCOMM_POWERUP_REQ;
    memcpy( &syscomm_buffer[index], &magic_data, sizeof( magic_data ) );
    index += ( uint8_t )sizeof( magic_data );
    
    return index;
}

static uint8_t syscomm_status_create_message( uint8_t status )
{
    uint32_t index;
    
    index = 0;
    
    /*
     * 0 = battery indicator
     * 1 = oil indicator
     * 2 = full beam indicator
     * 3 = brake pulled
     * 4 = brake released
     */
    
    syscomm_buffer[index++] = SYSCOMM_STATUS_REQ;
    memcpy( &syscomm_buffer[index], &status, sizeof( status ) );
    index += ( uint8_t )sizeof( status );
    
    return index;
}

static void syscomm_process_data_and_send( uint8_t req, uint8_t * data, uint16_t size )
{
    uint8_t send_size;
    
    // send data
    switch( req )
    {
        case SYSCOMM_PIPE_POWERUP:
            send_size = syscomm_powerup_create_message();
            device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
            break;
            
        case SYSCOMM_PIPE_TRIP_READ:
            send_size = syscomm_nvram_read_create_message( NVRAM_ADDR_TRIP, sizeof( uint32_t ) * 3 );
            device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
            break;
            
        case SYSCOMM_PIPE_SETUP_READ:
            send_size = syscomm_nvram_read_create_message( NVRAM_ADDR_BITMASK2, sizeof( uint32_t ) );
            device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
            break;
            
        case SYSCOMM_PIPE_ODOMETER_READ:
            device_write( syscomm_dev, SYSCOMM_ODOMETER_REQ );
            break;
            
        case SYSCOMM_PIPE_BITMASK_READ:
            device_write( syscomm_dev, SYSCOMM_BITMASK_REQ );
            break;
            
        case SYSCOMM_PIPE_BITMASK_WRITE:
            send_size = syscomm_status_create_message( data[1] );
            device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
            break;
            
        case SYSCOMM_PIPE_TRIP_WRITE:
            send_size = syscomm_nvram_write_create_message( NVRAM_ADDR_TRIP, &data[1], size-1 );
            device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
            break;
            
        case SYSCOMM_PIPE_SETUP_WRITE:
            send_size = syscomm_nvram_write_create_message( NVRAM_ADDR_BITMASK2, &data[1], size-1 );
            device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
            break;
            
        case SYSCOMM_PIPE_POWERDOWN:
            device_write( syscomm_dev, SYSCOMM_POWERDOWN_REQ );
            break;
    }
}

void syscomm_init( void )
{
    uint32_t baudrate;
    uint8_t command;

    curr_request = SYSCOMM_PIPE_NO_REQUEST;
    curr_command = 0;
    
	syscomm_dev = NULL;
	syscomm_dev = device_open( DEV_SYSCOMM );

	if( syscomm_dev != NULL )
	{
        timer_Start( &syscomm_timeout, SYSCOMM_TIMEOUT_MS );
        timer_Stop( &syscomm_timeout );
        
        is_pending = FALSE;
        
		DEBUG_PRINTK( "syscomm ready\r\n" );
		baudrate = 115200;
        device_ioctrl( syscomm_dev, DEVICE_SET_CONFIG_DATA, &baudrate );

        pipe_init( &syscomm_pipe_request, "SYSCOMM_REQ", 8 );
        pipe_init( &syscomm_pipe_response, "SYSCOMM_RESP", 8 );
        
        // first send powerup command
        command = SYSCOMM_PIPE_POWERUP;
        pipe_send_buffer( &syscomm_pipe_request, &command, sizeof( command ) );
    }
}

pipe_data_t * syscomm_get_pipe_request( void )
{
    return &syscomm_pipe_request;
}

pipe_data_t * syscomm_get_pipe_response( void )
{
    return &syscomm_pipe_response;
}

void syscomm_process( void )
{
    uint8_t rx_byte;
    uint8_t * resp_pipe_data;
    uint8_t checksum;
    
    if( ( curr_request == SYSCOMM_PIPE_NO_REQUEST ) && ( !pipe_is_empty( &syscomm_pipe_request ) ) )
    {
        // incoming requests
        pipe_data = pipe_read( &syscomm_pipe_request, &pipe_size );
        curr_request = pipe_data[0];
        
        // start timeout
        timer_Reload( &syscomm_timeout );
        syscomm_process_data_and_send( curr_request, pipe_data, pipe_size );
    }
    
    if( ( curr_request != SYSCOMM_PIPE_NO_REQUEST ) && ( device_is_rx_pending( syscomm_dev ) >= 2 ) )
    {
        if( is_pending == FALSE )
        {
            // start timeout
            timer_Reload( &syscomm_timeout );

            // first byte received
            curr_command = device_read( syscomm_dev );
            
            //DEBUG_PRINTK( "%02X ", curr_command );

            switch( curr_command )
            {
                case SYSCOMM_POWERUP_RESP:
                    // powerup does not send response via pipe
                    rx_byte = device_read( syscomm_dev );
                    curr_request = SYSCOMM_PIPE_NO_REQUEST;
                    break;
                    
                case SYSCOMM_POWERDOWN_RESP:
                case SYSCOMM_STATUS_RESP:
                case SYSCOMM_NVRAM_WR_RESP:
                    // single byte response
                    pipe_send_buffer( &syscomm_pipe_response, &curr_request, sizeof( curr_request ) );
                    rx_byte = device_read( syscomm_dev );
                    curr_request = SYSCOMM_PIPE_NO_REQUEST;
                    break;
                    
                // cases below: more bytes to receive
                case SYSCOMM_ODOMETER_RESP:
                case SYSCOMM_BITMASK_RESP:
                case SYSCOMM_NVRAM_RD_RESP:
                    rx_pending_bytes = 0;
                    is_pending = TRUE;
                    break;
            }
        }
        else
        {
            // more bytes to receive
            if( rx_pending_bytes == 0 )
            {
                // the next byte is the size
                rx_byte = device_read( syscomm_dev );
                rx_pending_bytes = rx_byte;
            }
            else
            {
                // receive everything else
                if( device_is_rx_pending( syscomm_dev ) >= rx_pending_bytes )
                {
                    // in case of nvram read, discount one byte of checksum
                    if( curr_command == SYSCOMM_NVRAM_RD_RESP )
                    {
                        rx_pending_bytes--;
                    }

                    // pack and send response
                    resp_pipe_data = pipe_alloc_buffer( rx_pending_bytes+1 );
                    
                    resp_pipe_data[0] = curr_request;
                    device_read_buffer( syscomm_dev, &resp_pipe_data[1], rx_pending_bytes );
                    
                    // checksum
                    if( curr_command == SYSCOMM_NVRAM_RD_RESP )
                    {
                        checksum = device_read( syscomm_dev );
                        
                        if( checksum != syscomm_checksum( &resp_pipe_data[1], rx_pending_bytes ) )
                        {
                            curr_request = SYSCOMM_PIPE_NO_REQUEST;
                        }
                    }
                    
                    if( curr_request != SYSCOMM_PIPE_NO_REQUEST )
                    {
                        pipe_write( &syscomm_pipe_response, resp_pipe_data, rx_pending_bytes+1 );
                    }

                    // no pending request
                    curr_request = SYSCOMM_PIPE_NO_REQUEST;
                }
            }
        }

        // check if we said there is no pending request
        if( curr_request == SYSCOMM_PIPE_NO_REQUEST )
        {
            // release request pipe
            pipe_release_buffer( pipe_data );

            is_pending = FALSE;
            timer_Stop( &syscomm_timeout );
        }
    }
    
    // got timeout
    if( timer_Check( &syscomm_timeout ) )
    {
        syscomm_process_data_and_send( curr_request, pipe_data, pipe_size );
        is_pending = FALSE;
        
        // start timeout
        timer_Reload( &syscomm_timeout );
    }
}


