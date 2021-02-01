#include "system.h"
#include "efs.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

#define FILE_LINE_SIZE		100

static char file_line[FILE_LINE_SIZE+1];

///////////////////////////////////////////////////////////////

void cmd_cat_show_file( device_t * dev_tty, char * file_path )
{
    File * fp;

	if( ( fp = file_fopen( file_path, FS_MODE_READ ) ) != NULL )
	{
		while( !file_feof( fp ) )
		{
			file_fgets( file_line, FILE_LINE_SIZE, fp );
			console_command_printk( dev_tty, file_line );
		}

		file_fclose( fp );
	}
}

int cmd_cat_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	bool_t is_found;

	if( input != NULL )
	{
		path = console_concatenate_path( input, &is_found, p_instance );

		if( is_found )
		{
			cmd_cat_show_file( p_instance->dev_console_tty, path );
		}
		else
		{
			console_command_printk( p_instance->dev_console_tty, "Path not found" );
		}
	}
	else
	{
		console_command_printk( p_instance->dev_console_tty, "cat <file>" );
	}

	return 0;
}

void cmd_cat_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_cat = 
{
	"cat",
	cmd_cat_init,
	cmd_cat_handler
};

DECLARE_COMMAND_SECTION( command_cat );
