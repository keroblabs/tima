#include "system.h"
#include "device.h"

#ifdef USE_MULTITHREAD
#include "t_threads.h"
#endif

#include "tima_libc.h"
#include "commands.h"
#include "debug.h"

#include "crc.h"

////////////////////////////////////////////////////////////////////

#define CONSOLE_STACK_SIZE    1024

enum
{
    CONSOLE_MODE_PRINTK,
    CONSOLE_MODE_COMMAND,
    CONSOLE_MODE_FTM
};

////////////////////////////////////////////////////////////////////

#ifdef USE_MULTITHREAD
static uint8_t console_stack[ CONSOLE_STACK_SIZE ];
static thread_data_t co_console;
#endif

static console_instance_t * p_first;
static crc_data_t ftm_crc8;

////////////////////////////////////////////////////////////////////

static console_instance_t * console_create_instance( void );
static console_instance_t * console_accept_telnet( console_instance_t * p_instance );

static void console_add_instance( console_instance_t * p_instance );

////////////////////////////////////////////////////////////////////

int console_tokenizer( char * input, char ** output, char * delim, int max )
{
	char * p_input;
	char * out;
	char * last;
	char str_val[10];
	int size;
	int i;

	p_input = input;
	i = 0;

	while( tima_tokenizer( p_input, ( const char ** )&out, &size, delim, &last ) )
	{
		strncpy( str_val, out, size );
		str_val[size] = 0;
		strcpy( *output, str_val );
		i++;
		p_input = NULL;
		output++;

		if( i >= max ) break;
	}

	return i;
}

static void console_set_command_mode ( console_instance_t * p_instance )
{
	p_instance->command_mode = CONSOLE_MODE_COMMAND;
	console_command_printk( p_instance->dev_console_tty, "\r\n\r\n%s> ", p_instance->cmd_curr_dir );
}

static void console_execute_command( console_instance_t * p_instance )
{
	static char cmd_caption[128];
	char * cmd_param;
	uint32_t i;
	command_data_t * p_command;

    if( p_instance->buffer && ( p_instance->buffer[0] != 0 ) )
	{
		cmd_param = NULL;
		i = 0;

		strcpy( cmd_caption, ( char * )p_instance->buffer );
		cmd_param = strchr( cmd_caption, ' ' );
		if( cmd_param != NULL ) 
		{
			*cmd_param = 0;
			cmd_param++;
		}

		//CONSOLE_PRINTK( "\r\n" );

		while( ( p_command = GET_COMMAND_SECTION( i ) ) != NULL )
		{
			if( ( p_command->type == CONSOLE_COMMAND_TYPE ) && !strcmp( p_command->name, cmd_caption ) )
			{
				p_command->handler( p_instance, cmd_param );
			}
			i++;
		}

        MFREE( p_instance->buffer );
        p_instance->buffer = NULL;
        p_instance->rx_count = 0;
	}

	if( p_instance->command_mode == CONSOLE_MODE_COMMAND )
	{
		console_command_printk( p_instance->dev_console_tty, "\r\n%s> ", p_instance->cmd_curr_dir );
	}
}

static int console_echo_handler( console_instance_t * p_instance, char * input )
{
	device_t * dev = p_instance->dev_console_tty;

	if( input != NULL )
	{
		if( !strcmp( input, "on" ) )
		{
			p_instance->echo_mode = TRUE;
		}
		else if( !strcmp( input, "off" ) )
		{
			p_instance->echo_mode = FALSE;
		}
	}

	console_command_printk( dev, "Echo %s\r\n", p_instance->echo_mode ? "ON" : "OFF" );

	return 0;
}

static int console_list_command( console_instance_t * p_instance, char * input )
{
	//char * cmd_param;
	uint32_t i;
	command_data_t * p_command;

	i = 0;

	while( ( p_command = ( command_data_t * )GET_COMMAND_SECTION( i ) ) != NULL )
	{
		console_command_printk( p_instance->dev_console_tty, "%s\r\n", p_command->name );
		i++;
	}

	return 0;
}

void console_send_ftm_packet( console_instance_t * p_instance, uint8_t command, uint8_t * buffer, uint16_t size )
{
    /*
     +-------------+-------------+-------------+-------------+-------------+--- - - - - - - ---+-------------+
     | PROTO_START |   command   |  size upper |  size lower |  DATA_START |        data       |    CRC 8    |
     +-------------+-------------+-------------+-------------+-------------+--- - - - - - - ---+-------------+
     */
    
    uint8_t * send_buffer;
    
    if( p_instance->command_mode != CONSOLE_MODE_FTM ) return;
    
    send_buffer = ( uint8_t * )MMALLOC( size+6 );
    if( send_buffer == NULL ) return;
    
    send_buffer[0] = FTM_PACK_PROTO_START;
    send_buffer[1] = command;
    send_buffer[2] = size >> 8;
    send_buffer[3] = size & 0xFF;
    send_buffer[4] = FTM_PACK_DATA_START;
    memcpy( &send_buffer[5], buffer, size );
    send_buffer[5+size] = crc_calculate( &ftm_crc8, buffer, size );
    
    device_write_buffer( p_instance->dev_console_tty, send_buffer, size+6 );
    MFREE( send_buffer );
}

static void console_process_ftm( console_instance_t * p_instance, uint8_t rx_byte )
{
    uint32_t i;
    ftm_command_t * p_command;
    
    /*
     +-------------+-------------+-------------+-------------+-------------+--- - - - - - - ---+-------------+
     | PROTO_START |   command   |  size upper |  size lower |  DATA_START |        data       |    CRC 8    |
     +-------------+-------------+-------------+-------------+-------------+--- - - - - - - ---+-------------+
     */
    
    switch( p_instance->ftm_state )
    {
        case FTM_STATE_NONE:
            if( rx_byte == FTM_PACK_PROTO_START )
            {
                p_instance->ftm_state = FTM_STATE_COMMAND;
            }
            break;
            
        case FTM_STATE_COMMAND:
            p_instance->ftm_state++;
            p_instance->curr_cmd = rx_byte;
            break;
            
        case FTM_STATE_UPPER_SIZE:
            p_instance->ftm_state++;
            p_instance->rx_size = rx_byte;
            p_instance->rx_size <<= 8;
            break;
            
        case FTM_STATE_LOWER_SIZE:
            p_instance->ftm_state++;
            p_instance->rx_size |= rx_byte;
            break;
            
        case FTM_STATE_DATA_START:
            if( rx_byte == FTM_PACK_DATA_START )
            {
                p_instance->buffer = ( uint8_t * )MMALLOC( p_instance->rx_size );
                if( p_instance->buffer == NULL )
                {
                    p_instance->ftm_state = FTM_STATE_NONE;
                }
                else
                {
                    p_instance->rx_count = 0;
                    p_instance->ftm_state++;
                }
            }
            else
            {
                p_instance->ftm_state = FTM_STATE_NONE;
            }
            break;
            
        case FTM_STATE_RX_DATA:
            if( p_instance->rx_count < p_instance->rx_size )
            {
                p_instance->buffer[p_instance->rx_count] = rx_byte;
                p_instance->rx_count++;
                if( p_instance->rx_count == p_instance->rx_size )
                {
                    p_instance->ftm_state++;
                }
            }
            break;
            
        case FTM_STATE_DATA_END:
            if( crc_calculate( &ftm_crc8, p_instance->buffer, p_instance->rx_size ) == rx_byte )
            {
                while( ( p_command = GET_FTM_DATA_SECTION(i) ) != NULL )
                {
                    if( ( p_command->type == FTM_COMMAND_TYPE ) && ( p_command->ftm_cmd == rx_byte ) )
                    {
                        p_command->handler( p_instance );
                    }
                    i++;
                }
            }
            else
            {
                p_instance->ftm_state = FTM_STATE_NONE;
            }
            MFREE( p_instance->buffer );
            break;
    }
}

static void console_local_process( console_instance_t * p_instance )
{
    uint8_t rx_byte;

#ifdef USE_MULTITHREAD
    while( 1 )
    {
        thread_wait( device_is_rx_pending(dev_tty) > 0 );
#else
    if( device_is_rx_pending(p_instance->dev_console_tty) > 0 )
    {
#endif
        // receive data
        
		while( device_read_buffer( p_instance->dev_console_tty, (uint8_t *)&rx_byte, 1 ) > 0 )
		{
			if( p_instance->command_mode == CONSOLE_MODE_PRINTK )
			{
				if( rx_byte == 0x1B )
				{
					//p_instance->command_mode = TRUE;
					//console_command_printk( p_instance->dev_console_tty, "\r\n\r\n%s> ", p_instance->cmd_curr_dir );
                    if( p_instance->buffer != NULL )
                    {
                        MFREE( p_instance->buffer );
                        p_instance->buffer = NULL;
                    }
                    console_set_command_mode( p_instance );
				}
                else if( rx_byte == 0x1A )
                {
                    // set to FTM mode
                    p_instance->command_mode = CONSOLE_MODE_FTM;
                }
                
                if( p_instance->command_mode != CONSOLE_MODE_FTM )
                {
                    p_instance->carriage_returned = FALSE;
                }
            }
            else if( p_instance->command_mode == CONSOLE_MODE_FTM )
            {
                console_process_ftm( p_instance, rx_byte );
            }
            else if( p_instance->command_mode == CONSOLE_MODE_COMMAND )
            {
                if( p_instance->buffer == NULL )
                {
                    p_instance->buffer = ( uint8_t * )MMALLOC( MAX_LONG_FILENAME_LENGHT );
                    if( p_instance->buffer == NULL ) return;
                }

                if( ( rx_byte == 13 ) || ( rx_byte == 10 ) )
                {
					if( p_instance->carriage_returned == FALSE )
					{
						console_command_printk( p_instance->dev_console_tty, "\r\n" );
						p_instance->buffer[p_instance->rx_count] = 0;
						console_execute_command( p_instance );
						p_instance->carriage_returned = TRUE;
					}
					else
					{
						p_instance->carriage_returned = FALSE;
					}
                }
                else
                {
					p_instance->carriage_returned = FALSE;

                    if( ( rx_byte == 127 ) || ( rx_byte == 8 ) )
                    {
                        if( p_instance->rx_count )
                        {
                            p_instance->buffer[p_instance->rx_count] = 0;
                            p_instance->rx_count--;
                            rx_byte = 8;
                        }
                    }
                    else
                    {
                        p_instance->buffer[p_instance->rx_count++] = rx_byte;
                    }
                    
					if( ( p_instance->command_mode == CONSOLE_MODE_COMMAND ) && ( p_instance->echo_mode == TRUE ) )
					{
						console_command_printk( p_instance->dev_console_tty, "%c", rx_byte );
					}
                }
            }
        }
    }
}

int console_command_printk( device_t * dev_tty, const char *fmt, ... )
{
	va_list args;	
	int printed_len;
    
	/* Emit the output into the temporary buffer */ 
	va_start(args, fmt);	
	printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args); 
	va_end(args);	

    return device_write_buffer( dev_tty, ( uint8_t * )vsf_printk_buf, printed_len );
}

int console_printk( const char *fmt, ... )
{
	va_list args;	
	int printed_len;
	console_instance_t * p_instance;
    
	/* Emit the output into the temporary buffer */ 
	va_start(args, fmt);	
	printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args); 
	va_end(args);	

	p_instance = p_first;

	while( p_instance != NULL )
	{
        if( ( p_instance->command_mode != CONSOLE_MODE_FTM ) &&
           ( p_instance->is_ready == TRUE ) )
        {
			device_write_buffer( p_instance->dev_console_tty, ( uint8_t * )vsf_printk_buf, printed_len );
		}
		p_instance = p_instance->p_next;
	}

	return printed_len;
}

int console_flaged_printk( const char *fmt, ... )
{
	va_list args;	
	int printed_len;
	console_instance_t * p_instance;

	/* Emit the output into the temporary buffer */ 
	va_start(args, fmt);	
	printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args); 
	va_end(args);	

	p_instance = p_first;

	while( p_instance != NULL )
	{
		if( ( p_instance->command_mode == CONSOLE_MODE_PRINTK ) &&
            ( p_instance->is_ready == TRUE ) )
		{
			device_write_buffer( p_instance->dev_console_tty, ( uint8_t * )vsf_printk_buf, printed_len );
		}
		p_instance = p_instance->p_next;
	}

	return printed_len;
}

void console_process( void )
{
	console_instance_t * p_instance;
	p_instance = p_first;

	while( p_instance != NULL )
	{
		if( p_instance->telnet_port != 0 )
		{
			console_accept_telnet( p_instance );
		}
		else
		{
			console_local_process( p_instance );
		}

		p_instance = p_instance->p_next;
	}
}

static console_instance_t * console_create_instance( void )
{
	console_instance_t * p_instance;

	p_instance = ( console_instance_t * ) MMALLOC( sizeof( console_instance_t ) );
	if( p_instance == NULL ) return NULL;
	memset( p_instance, 0x00, sizeof( console_instance_t ) );

	p_instance->command_mode = CONSOLE_MODE_PRINTK;
	p_instance->carriage_returned = FALSE;
    p_instance->is_ready = TRUE;

	strcpy( p_instance->cmd_curr_dir, "/" );

	p_instance->buffer = NULL;
    p_instance->rx_count = 0;
    p_instance->rx_size = 0;

	return p_instance;
}

static console_instance_t * console_accept_telnet( console_instance_t * p_instance )
{
	device_t * dev_accept;
	console_instance_t * new_instance;

	if( ( dev_accept = device_socket_accept( p_instance->dev_console_tty ) ) != NULL )
	{
		new_instance = console_create_instance();
		if( new_instance == NULL )
		{
			MFREE( new_instance );
			return NULL;
		}

		new_instance->dev_console_tty = dev_accept;
        new_instance->is_ready = TRUE;
        
		// by default telnet is echo OFF
		new_instance->echo_mode = FALSE;

		// by default telnet starts in command mode
		console_set_command_mode( new_instance );

		console_add_instance( new_instance );
		return new_instance;
	}

	return NULL;
}

console_instance_t * console_listen_telnet( uint16_t telnet_port )
{
	device_t * dev_listen;
	console_instance_t * p_instance;

	if( ( dev_listen = device_socket_listen( telnet_port ) ) != NULL )
	{
		DEBUG_PRINTK( "Started to listen\r\n" );
		p_instance = console_create_instance();
		if( p_instance == NULL )
		{
			MFREE( p_instance );
			return NULL;
		}

		p_instance->dev_console_tty = dev_listen;
		p_instance->telnet_port = telnet_port;
        p_instance->is_ready = FALSE;

		console_add_instance( p_instance );
		return p_instance;
	}

	DEBUG_PRINTK( "Failed to listen\r\n" );
	return NULL;
}

console_instance_t * console_connect_uart( const char * dev_name )
{
	uint32_t i;
	console_instance_t * p_instance;

	p_instance = console_create_instance();
	if( p_instance == NULL ) return NULL;

	p_instance->dev_console_tty = device_open( dev_name );
	if( p_instance->dev_console_tty == NULL )
	{
		MFREE( p_instance );
		return NULL;
	}

	// by default UART is echo ON
	p_instance->echo_mode = TRUE;

	i = 115200;
	device_ioctrl( p_instance->dev_console_tty, DEVICE_SET_CONFIG_DATA, &i );
	console_add_instance( p_instance );

	return p_instance;
}

static void console_add_instance( console_instance_t * p_instance )
{
	p_instance->p_next = p_first;
	p_first = p_instance;
}

void console_init( void )
{
	uint32_t i;
	command_data_t * p_command;

	p_first = NULL;

	i = 0;

	while( ( p_command = GET_COMMAND_SECTION(i) ) != NULL )
	{
		if( p_command->init != NULL )
		{
			p_command->init();
		}
		i++;
	}
    
    crc_init( &ftm_crc8, 0x18, 0xFF );
    
    #ifdef USE_MULTITHREAD
	thread_task_create( &co_console, console_process, NULL, console_stack, CONSOLE_STACK_SIZE );
    #endif
}

int cmd_exit_handler( console_instance_t * p_instance, char * input )
{
	p_instance->command_mode = CONSOLE_MODE_PRINTK;
	return 0;
}

////////////////////////////////////////////////////////////////////

const command_data_t command_exit = 
{
    CONSOLE_COMMAND_TYPE,
    "exit",
	NULL,
	cmd_exit_handler
};

const command_data_t command_help = 
{
    CONSOLE_COMMAND_TYPE,
    "help",
	NULL,
	console_list_command
};

const command_data_t command_echo = 
{
    CONSOLE_COMMAND_TYPE,
    "echo",
	NULL,
	console_echo_handler
};

////////////////////////////////////////////////////////////////////
    
void cmd_chrdir_handler( console_instance_t * p_instance )
{
    strncpy( p_instance->cmd_curr_dir, ( char * )p_instance->buffer, p_instance->rx_size );
}
    
const ftm_command_t command_ftm_chdir =
{
    FTM_COMMAND_TYPE,
    0xA2,
    NULL,
    cmd_chrdir_handler
};
    
///////////////////////////////////////////////////////////////////////////

void console_debug_drv_init( uint32_t index )
{
}

uint32_t console_debug_output( uint32_t index, uint8_t * text, uint32_t size )
{
	console_flaged_printk( (char*)text );
    return size;
}

uint32_t console_debug_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
    return 0;
}

static bool_t validate( uint32_t index )
{
    if( index == 8 ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t console_cmd_device =
{
    "tty*",
    &_in_use,
    
    console_debug_drv_init,
    NULL,
    NULL,
    validate,
    NULL,
    NULL,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    console_debug_input,
    console_debug_output,
    
};

DECLARE_COMMAND_SECTION( command_exit );
DECLARE_COMMAND_SECTION( command_help );
DECLARE_COMMAND_SECTION( command_echo );

#ifndef WANT_TELNET
DECLARE_DEVICE_SECTION( console_cmd_device );
#endif


