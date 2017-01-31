#include "system.h"
#include "device.h"
#include "file.h"
#include "commands.h"
#include "ymodem.h"

///////////////////////////////////////////////////////////////

static char temp_filename[MAX_LONG_FILENAME_LENGHT]; 

///////////////////////////////////////////////////////////////

int cmd_receive_handler( console_instance_t * p_instance, char * input )
{
	uint32_t size;
	device_t * dev = p_instance->dev_console_tty;

	size = ymodem_receive( dev, temp_filename, p_instance->cmd_curr_dir, MAX_LONG_FILENAME_LENGHT );
	if( size == 0 )
	{
		console_command_printk( dev, "\r\nFailed to receive file" );
	}
	else
	{
		console_command_printk( dev, "\r\nReceived file: %s, %d bytes", temp_filename, size );
	}
	return 0;
}

int cmd_send_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	bool_t is_found;
	device_t * dev = p_instance->dev_console_tty;

	if( input != NULL )
	{
		path = console_concatenate_path( input, &is_found, p_instance );

		if( is_found )
		{
			ymodem_send_file( dev, path );
			console_command_printk( dev, "\r\nSent: %s", path );
		}
		else
		{
			console_command_printk( dev, "File not found" );
		}
	}
	else
	{
		console_command_printk( dev, "send <filename>" );
	}

	return 0;
}

void cmd_ymodem_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_receive = 
{
    CONSOLE_COMMAND_TYPE,
    "receive",
	cmd_ymodem_init,
	cmd_receive_handler
};

const command_data_t command_send = 
{
    CONSOLE_COMMAND_TYPE,
    "send",
	NULL,
	cmd_send_handler
};

DECLARE_COMMAND_SECTION( command_receive );
DECLARE_COMMAND_SECTION( command_send );
