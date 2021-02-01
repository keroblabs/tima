#include "system.h"
#include "file.h"
#include "dir.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

int cmd_mkdir_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	char ret;

	if( input != NULL )
	{
		path = console_concatenate_path( input, NULL, p_instance );
		ret = dir_createDirectory( path );

		if( ret < 0 )
		{
			console_command_printk( p_instance->dev_console_tty, "Path invalid" );
		}
	}

	return 0;
}

void cmd_mkdir_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_mkdir = 
{
	"mkdir",
	cmd_mkdir_init,
	cmd_mkdir_handler
};

DECLARE_COMMAND_SECTION( command_mkdir );
