#include "system.h"
#include "timer.h"
#include "nvram_control.h"
#include "power_control.h"
#include "debug.h"

#include "odometer_control.h"

///////////////////////////////////////////////////////////////

#define SYSCOMM_TIMEOUT_MS			500

#define SYSCOMM_MAGIC_VALUE         0x3A681E97

#define SYSCOMM_NO_COMMAND			0x00

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

///////////////////////////////////////////////////////////////

static const uint8_t syscomm_status_response[2] = { SYSCOMM_STATUS_RESP, SYSCOMM_STATUS_REQ };
static const uint8_t syscomm_nvram_wr_response[2] = { SYSCOMM_NVRAM_WR_RESP, SYSCOMM_NVRAM_WR_REQ };
static const uint8_t syscomm_powerdown_response[2] = { SYSCOMM_POWERDOWN_RESP, SYSCOMM_POWERDOWN_REQ };
static const uint8_t syscomm_powerup_response[2] = { SYSCOMM_POWERUP_RESP, SYSCOMM_POWERUP_REQ };

///////////////////////////////////////////////////////////////

static timer_data_t syscomm_timeout;

static device_t * syscomm_dev;
static uint8_t syscomm_buffer[32];
static uint8_t curr_command;
static uint8_t pending_size;
static bool_t is_pending;
static bool_t is_powerup;

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

static void syscomm_powerup_check_message( void )
{
	uint32_t magic_num;

	memcpy( &magic_num, syscomm_buffer, sizeof( magic_num ) );
	if( magic_num == SYSCOMM_MAGIC_VALUE )
	{
		is_powerup = TRUE;
		DEBUG_PRINTK( "Powerup\r\n" );
	}
}

static uint8_t syscomm_odometer_create_message( void )
{
	uint32_t odometer_value;
	uint8_t index;

	index = 0;
	odometer_value = odometer_control_get();

	syscomm_buffer[index++] = SYSCOMM_ODOMETER_RESP;
	syscomm_buffer[index++] = sizeof( odometer_value );
	memcpy( &syscomm_buffer[index], &odometer_value, sizeof( odometer_value ) );
	index += ( uint8_t )sizeof( odometer_value );

	return index;
}

static uint8_t syscomm_bitmask_create_message( void )
{
	uint32_t bitmask;
	uint32_t index;

	index = 0;
	bitmask = 0;

	/*
	 * 0 = door left open
	 * 1 = door right open
	 * 2 = door left locked
	 * 3 = door right locked
	 * 4 = engine run state
	 * 5 = bonnet open
	 * 6 = boot open
	 * 7 = washer level
	 * 8 = headlights on
	 * 9 = daylightrun on
	 * 10 = fog lights on
	 * 11 = ignition
	 */

	if( power_get_ignition_input() ) bitmask |= ( 1 << 11 );
	if( power_get_headlights_state() ) bitmask |= ( 1 << 8 );
	if( power_get_engine_run_state() ) bitmask |= ( 1 << 4 );

	syscomm_buffer[index++] = SYSCOMM_BITMASK_RESP;
	syscomm_buffer[index++] = sizeof( bitmask );

	memcpy( &syscomm_buffer[index], &bitmask, sizeof( bitmask ) );
	index += ( uint8_t )sizeof( bitmask );

	return index;
}

static uint8_t syscomm_nvram_rd_create_message( uint8_t addr, uint8_t size )
{
	uint8_t index;

	index = 0;

	syscomm_buffer[index++] = SYSCOMM_NVRAM_RD_RESP;
	syscomm_buffer[index++] = size+1;

	nvram_control_read( addr, &syscomm_buffer[index], size );
	index += size;
    
    syscomm_buffer[index++] = syscomm_checksum( &syscomm_buffer[2], size );

	return index;
}

void syscomm_init( void )
{
	uint32_t baudrate;

	curr_command = SYSCOMM_NO_COMMAND;
	pending_size = 1;
	is_pending = FALSE;
	is_powerup = FALSE;

	timer_Start( &syscomm_timeout, SYSCOMM_TIMEOUT_MS );
	timer_Stop( &syscomm_timeout );

	syscomm_dev = NULL;
	syscomm_dev = device_open( DEV_SYSCOMM );

	if( syscomm_dev != NULL )
	{
		DEBUG_PRINTK( "syscomm ready\r\n" );
		baudrate = 115200;
        device_ioctrl( syscomm_dev, DEVICE_SET_CONFIG_DATA, &baudrate );
	}
}

void syscomm_process( void )
{
	uint8_t rx_byte;
	uint8_t send_size;
	uint32_t rx_size;

	if( syscomm_dev == NULL ) return;

	rx_size = device_is_rx_pending( syscomm_dev );

	if( device_is_rx_pending( syscomm_dev ) )
	{
		timer_Reload( &syscomm_timeout );

		if( is_pending == FALSE )
		{
			rx_byte = device_read( syscomm_dev );
			pending_size = 0;

			if( ( is_powerup == TRUE ) || ( rx_byte == SYSCOMM_POWERUP_REQ ) )
			{
				switch( rx_byte )
				{
					case SYSCOMM_POWERUP_REQ:
						//DEBUG_PRINTK( "SYSCOMM_POWERUP_REQ\r\n" );
						pending_size = 4;
						curr_command = rx_byte;
						is_pending = TRUE;
						break;

					case SYSCOMM_ODOMETER_REQ:
						//DEBUG_PRINTK( "SYSCOMM_ODOMETER_REQ\r\n" );
						send_size = syscomm_odometer_create_message();
						device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
						break;

					case SYSCOMM_BITMASK_REQ:
						//DEBUG_PRINTK( "SYSCOMM_BITMASK_REQ\r\n" );
						send_size = syscomm_bitmask_create_message();
						device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
						break;

					case SYSCOMM_POWERDOWN_REQ:
						//DEBUG_PRINTK( "SYSCOMM_POWERDOWN_REQ\r\n" );
						device_write_buffer( syscomm_dev, ( uint8_t * )syscomm_powerdown_response, sizeof( syscomm_powerdown_response ) );
            			power_set_instruments_state( FALSE );
						break;

					case SYSCOMM_NVRAM_RD_REQ:
						DEBUG_PRINTK( "SYSCOMM_NVRAM_RD_REQ\r\n" );
						pending_size = 3;
						curr_command = rx_byte;
						is_pending = TRUE;
						break;

					case SYSCOMM_STATUS_REQ:
						//DEBUG_PRINTK( "SYSCOMM_STATUS_REQ\r\n" );
						pending_size = 1;
						curr_command = rx_byte;
						is_pending = TRUE;
						break;

					case SYSCOMM_NVRAM_WR_REQ:
						DEBUG_PRINTK( "SYSCOMM_NVRAM_WR_REQ\r\n" );
						curr_command = rx_byte;
						is_pending = TRUE;
						break;

					default:
						timer_Stop( &syscomm_timeout );
						break;
				}
			}
			else
			{
				timer_Stop( &syscomm_timeout );
			}
		}
		else
		{
			if( pending_size == 0 )
			{
				// next byte is size
				pending_size = device_read( syscomm_dev );
			}
	        else
	        {
	            // receive everything else
	            if( device_is_rx_pending( syscomm_dev ) >= pending_size )
	            {
	            	device_read_buffer( syscomm_dev, syscomm_buffer, pending_size );

	            	switch( curr_command )
	            	{
	            		case SYSCOMM_POWERUP_REQ:
	            			syscomm_powerup_check_message();
	            			if( is_powerup == TRUE )
	            			{
		            			device_write_buffer( syscomm_dev, ( uint8_t * )syscomm_powerup_response, sizeof( syscomm_powerup_response ) );
		            			power_set_instruments_state(TRUE);
	            			}
	            			is_pending = FALSE;
	            			break;

	            		case SYSCOMM_NVRAM_RD_REQ:
	            			if( syscomm_buffer[2] == syscomm_checksum( syscomm_buffer, 2 ) )
	            			{
                                DEBUG_PRINTK( "nvram read\r\n" );
                                send_size = syscomm_nvram_rd_create_message( syscomm_buffer[0], syscomm_buffer[1] );
		            			device_write_buffer( syscomm_dev, syscomm_buffer, send_size );
	            			}
	            			is_pending = FALSE;
	            			break;

	            		case SYSCOMM_STATUS_REQ:
	            			device_write_buffer( syscomm_dev, ( uint8_t * )syscomm_status_response, sizeof( syscomm_status_response ) );
	            			is_pending = FALSE;
	            			break;

	            		case SYSCOMM_NVRAM_WR_REQ:
	            			if( syscomm_buffer[pending_size-1] == syscomm_checksum( syscomm_buffer, pending_size-1 ) )
	            			{
                                DEBUG_PRINTK( "nvram write\r\n" );
		            			nvram_control_write( syscomm_buffer[0], &syscomm_buffer[2], syscomm_buffer[1] );
		            			device_write_buffer( syscomm_dev, ( uint8_t * )syscomm_nvram_wr_response, sizeof( syscomm_nvram_wr_response ) );
	            			}
	            			is_pending = FALSE;
	            			break;
	            	}
	            }
	        }

			if( is_pending == FALSE )
			{
				curr_command = SYSCOMM_NO_COMMAND;
				timer_Stop( &syscomm_timeout );
				pending_size = 0;
			}
		}
	}

	if( timer_Check( &syscomm_timeout ) )
	{
		timer_Stop( &syscomm_timeout );
		is_pending = FALSE;
		curr_command = SYSCOMM_NO_COMMAND;
		pending_size = 0;
	}
}


