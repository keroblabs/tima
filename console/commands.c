#include "system.h"
#include "device.h"
#include "file.h"
#include "efs.h"

#ifdef USE_MULTITHREAD
#include "t_threads.h"
#endif

#include "tima_libc.h"
#include "commands.h"
#include "ymodem.h"

////////////////////////////////////////////////////////////////////

#define CONSOLE_STACK_SIZE    1024

////////////////////////////////////////////////////////////////////

#ifdef USE_MULTITHREAD
static uint8_t console_stack[ CONSOLE_STACK_SIZE ];
static thread_data_t co_console;
#endif

static console_instance_t * p_first;

////////////////////////////////////////////////////////////////////

void console_show_mounted_devices( device_t * dev )
{
	uint8_t index;
	const char * name;
	uint8_t c;

	static mount_data_t local_mnt;
	static char local_vol_name[40];

	index = 0;

	while( ( name = disc_DevName( index ) ) != NULL )
	{
		memcpy( &local_mnt, efs_get_partition( index ), sizeof( local_mnt ) );

		for( c = 0; c < TOTAL_PARTITIONS; c++ )
		{
			strcpy( local_vol_name, "" );
			if( part_isFatPart( local_mnt.part.partitions[c].type ) )
			{
				dir_getVolumeName( ( FileSystem * )local_mnt.part.partitions[c].fs, local_vol_name );
			}
            else if( part_isExtPart( local_mnt.part.partitions[c].type ) )
            {
                sprintk( local_vol_name, "%s%d", name, c+1 );
            }

			if( strlen( local_vol_name ) )
			{
				console_command_printk( dev, "%s:\r\n", local_vol_name );
			}
		}

		index++;
	}
}

char * console_concatenate_path( char * input, bool_t * is_found, console_instance_t * p_instance )
{
	static char temp_curr_dir[MAX_LONG_FILENAME_LENGHT]; 

	if( is_found != NULL ) *is_found = FALSE;

	if( !strcmp( input, "/" ) )
	{
		*is_found = TRUE;
		return input;
	}

	if( input[0] == '/' )
	{
		strcpy( temp_curr_dir, input );
	}
	else
	{
		strcpy( temp_curr_dir, p_instance->cmd_curr_dir );
		if( temp_curr_dir[strlen(temp_curr_dir)-1] != '/' ) strcat( temp_curr_dir, "/" ); 
		strcat( temp_curr_dir, input ); 
	}

	if( ( strlen( temp_curr_dir ) > 1 ) && ( temp_curr_dir[strlen(temp_curr_dir)-1] == '/' ) )
	{
		temp_curr_dir[strlen(temp_curr_dir)-1] = 0;
	}

	if( ( is_found != NULL ) && ( file_locatePath( temp_curr_dir ) > 0 ) )
	{
		*is_found = TRUE;
	}

	return temp_curr_dir;
}

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

void console_set_command_mode ( console_instance_t * p_instance )
{
	p_instance->command_mode = TRUE;
	console_command_printk( p_instance->dev_console_tty, "\r\n\r\n%s> ", p_instance->cmd_curr_dir );
}

void console_execute_command( console_instance_t * p_instance )
{
	static char cmd_caption[128];
	char * cmd_param;
	uint32_t i;
	command_data_t * p_command;

	if( p_instance->command_buffer[0] != 0 )
	{
		cmd_param = NULL;
		i = 0;

		strcpy( cmd_caption, p_instance->command_buffer );
		cmd_param = strchr( cmd_caption, ' ' );
		if( cmd_param != NULL ) 
		{
			*cmd_param = 0;
			cmd_param++;
		}

		//CONSOLE_PRINTK( "\r\n" );
        // #define GET_FONT_SECTION(i)                         ( font_data_t * )system_List_Get_Item(FONT_SECTION_ID,i)

		while( ( p_command = GET_COMMAND_SECTION( i ) ) != NULL )
		{
			if( !strcmp( p_command->name, cmd_caption ) )
			{
				p_command->handler( p_instance, cmd_param );
			}
			i++;
		}

		p_instance->command_pointer = 0;
		p_instance->command_buffer[0] = 0;
	}

	if( p_instance->command_mode == TRUE )
	{
		console_command_printk( p_instance->dev_console_tty, "\r\n%s> ", p_instance->cmd_curr_dir );
	}
}

int console_echo_handler( console_instance_t * p_instance, char * input )
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

int console_list_command( console_instance_t * p_instance, char * input )
{
	//char * cmd_param;
	uint32_t i;
	command_data_t * p_command;

	i = 0;

	while( ( p_command = GET_COMMAND_SECTION( i ) ) != NULL )
	{
		console_command_printk( p_instance->dev_console_tty, "%s\r\n", p_command->name );
		i++;
	}

	return 0;
}

void console_local_process( console_instance_t * p_instance )
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
			if( p_instance->command_mode == FALSE )
			{
				if( rx_byte == 0x1B )
				{
					//p_instance->command_mode = TRUE;
					//console_command_printk( p_instance->dev_console_tty, "\r\n\r\n%s> ", p_instance->cmd_curr_dir );
					console_set_command_mode( p_instance );
				}
                
				p_instance->carriage_returned = FALSE;
            }
            else
            {
                if( ( rx_byte == 13 ) || ( rx_byte == 10 ) )
                {
					if( p_instance->carriage_returned == FALSE )
					{
						console_command_printk( p_instance->dev_console_tty, "\r\n" );
						p_instance->command_buffer[p_instance->command_pointer] = 0;
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
                        if( p_instance->command_pointer )
                        {
                            p_instance->command_buffer[p_instance->command_pointer] = 0;
                            p_instance->command_pointer--;
                            rx_byte = 8;
                        }
                    }
                    else
                    {
                        p_instance->command_buffer[p_instance->command_pointer++] = rx_byte;
                    }
                    
					if( ( p_instance->command_mode == TRUE ) && ( p_instance->echo_mode == TRUE ) )
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
		device_write_buffer( p_instance->dev_console_tty, ( uint8_t * )vsf_printk_buf, printed_len );
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
		if( ( p_instance->command_mode == FALSE ) && ( p_instance->is_open == TRUE ) )
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

console_instance_t * console_create_instance( void )
{
	console_instance_t * p_instance;

	p_instance = ( console_instance_t * ) MMALLOC( sizeof( console_instance_t ) );
	if( p_instance == NULL ) return NULL;
	memset( p_instance, 0x00, sizeof( console_instance_t ) );

	p_instance->command_mode = FALSE;
	p_instance->carriage_returned = FALSE;

	strcpy( p_instance->cmd_curr_dir, "/" );

	p_instance->command_pointer = 0;
    p_instance->command_buffer[0] = 0;

	return p_instance;
}

console_instance_t * console_accept_telnet( console_instance_t * p_instance )
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
		new_instance->is_open = TRUE;

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
		p_instance = console_create_instance();
		if( p_instance == NULL )
		{
			MFREE( p_instance );
			return NULL;
		}

		p_instance->dev_console_tty = dev_listen;
		p_instance->telnet_port = telnet_port;
		p_instance->is_open = FALSE;

		console_add_instance( p_instance );
		return p_instance;
	}

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
	p_instance->is_open = TRUE;

	i = 115200;
	device_ioctrl( p_instance->dev_console_tty, DEVICE_SET_CONFIG_DATA, &i );
	console_add_instance( p_instance );

	return p_instance;
}

void console_add_instance( console_instance_t * p_instance )
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

	while( ( p_command = GET_COMMAND_SECTION( i ) ) != NULL )
	{
		if( p_command->init != NULL )
		{
			p_command->init();
		}
		i++;
	}

    #ifdef USE_MULTITHREAD
	thread_task_create( &co_console, console_process, NULL, console_stack, CONSOLE_STACK_SIZE );
    #endif
}

int cmd_exit_handler( console_instance_t * p_instance, char * input )
{
	p_instance->command_mode = FALSE;
	return 0;
}

////////////////////////////////////////////////////////////////////

const command_data_t command_exit = 
{
	"exit",
	NULL,
	cmd_exit_handler
};

const command_data_t command_help = 
{
	"help",
	NULL,
	console_list_command
};

const command_data_t command_echo = 
{
	"echo",
	NULL,
	console_echo_handler
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


