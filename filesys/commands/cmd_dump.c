#include "system.h"
#include "efs.h"
#include "commands.h"
#include "device.h"

///////////////////////////////////////////////////////////////

#define DUMP_BUFFER_SIZE		256

static uint8_t dump_men[DUMP_BUFFER_SIZE];

///////////////////////////////////////////////////////////////

void cmd_dump_show_file( device_t * p_device, char * file_path )
{
    File * fp;
	int i,len;
	uint8_t rx_byte;
	bool_t is_loop;

	is_loop = TRUE;

	if( ( fp = file_fopen( file_path, FS_MODE_READ ) ) != NULL )
	{
		while( !file_feof( fp ) && is_loop )
		{
			len = file_fread( dump_men, DUMP_BUFFER_SIZE, 1, fp );
			
			for( i = 0; i < len; i++ )
			{
				if( ( ( i % 16 ) == 0 ) && ( i != 0 ) )
				{
					console_command_printk( p_device, "\r\n" );
					SYSTEM_EVENTS();
					if( device_is_rx_pending(p_device) > 0 )
					{
						device_read_buffer( p_device, (uint8_t *)&rx_byte, 1 );
						if( rx_byte == 0x1B ) is_loop = FALSE;
					}
				}
				console_command_printk( p_device, "%02x ", dump_men[i] );
			}

			console_command_printk( p_device, "\r\n" );
		}

		file_fclose( fp );
	}
}

int cmd_dump_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	bool_t is_found;

	if( input != NULL )
	{
		path = console_concatenate_path( input, &is_found, p_instance );

		if( is_found )
		{
			cmd_dump_show_file( p_instance->dev_console_tty, path );
		}
		else
		{
			console_command_printk( p_instance->dev_console_tty, "Path not found" );
		}
	}
	else
	{
		console_command_printk( p_instance->dev_console_tty, "dump <file>" );
	}

	return 0;
}

void cmd_dump_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_dump = 
{
    CONSOLE_COMMAND_TYPE,
    "dump",
	cmd_dump_init,
	cmd_dump_handler
};

DECLARE_COMMAND_SECTION( command_dump );
