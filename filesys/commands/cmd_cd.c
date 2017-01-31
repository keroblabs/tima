#include "system.h"
#include "file.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

int cmd_cd_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	bool_t is_found;

	if( input != NULL )
	{
		path = console_concatenate_path( input, &is_found, p_instance );

		if( is_found )
		{
			strcpy( p_instance->cmd_curr_dir, path );
		}
		else
		{
			console_command_printk( p_instance->dev_console_tty, "Path not found" );
		}
	}
	else
	{
		console_command_printk( p_instance->dev_console_tty, "%s", p_instance->cmd_curr_dir );
	}

	return 0;
}

void cmd_cd_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_cd = 
{
    CONSOLE_COMMAND_TYPE,
    "cd",
	cmd_cd_init,
	cmd_cd_handler
};

DECLARE_COMMAND_SECTION( command_cd );
