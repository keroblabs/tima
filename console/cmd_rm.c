#include "system.h"
#include "efs.h"
#include "dir.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

int cmd_rm_handler( console_instance_t * p_instance, char * input )
{
	char ret;
	char * path;
	bool_t is_found;
	device_t * dev = p_instance->dev_console_tty;

	if( input != NULL )
	{
		path = console_concatenate_path( input, &is_found, p_instance );

		if( is_found )
		{
			ret = file_locatePath( path );
			if( ret == FS_IS_FILE )
			{
				file_remove( path );
			}
			else if( ret == FS_IS_DIRECTORY )
			{
				if( dir_isEmpty( path ) == FALSE )
				{
					console_command_printk( dev, "Directory not empty" );
				}
				else
				{
					file_remove( path );
				}
			}
			else
			{
				console_command_printk( dev, "Path not found" );
			}
		}
		else
		{
			console_command_printk( dev, "Path not found" );
		}
	}
	else
	{
		console_command_printk( dev, "rm <file>" );
	}

	return 0;
}

void cmd_rm_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_rm = 
{
	"rm",
	cmd_rm_init,
	cmd_rm_handler
};

DECLARE_COMMAND_SECTION( command_rm );
