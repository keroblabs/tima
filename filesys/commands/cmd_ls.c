#include "system.h"
#include "file.h"
#include "efs.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

void cmd_ls_show_directory( device_t * dev, char * dir_path )
{
    tFileInformation file_info;
    char time_str[ TIME_STRING_REQ_SIZE ];
    void * dir;
	uint32_t size = 0;

	// this is declared in cmd_part.c
	void console_show_mounted_devices( device_t * dev );

	console_command_printk( dev, "%s:\r\n", dir_path );

	if( !strcmp( dir_path, "/" ) )
	{
		console_show_mounted_devices(dev);
	}
    else
    {
        if( ( dir = dir_openFileList( dir_path ) ) != NULL )
        {
            while( dir_readFileList( dir, &file_info ) != -1 )
            {
                fs_TimeToString( file_info.date, file_info.time, time_str );
                
                console_command_printk( dev, "%7d %c %s %s", file_info.size, ( file_info.attribute & ATTR_DIRECTORY ) ? 'D' : ' ', time_str, file_info.name );
                size += file_info.size;
                
                if( file_info.attribute & ATTR_DIRECTORY ) console_command_printk( dev, "/" );
                else if( file_info.attribute & ATTR_VOLUME_ID ) console_command_printk( dev, "//" );
                
                console_command_printk( dev, "\r\n" );
            }
            
            console_command_printk( dev, "%7d bytes, %d files\r\n", size, dir_getFileCount(dir_path) );
            console_command_printk( dev, "%7d bytes free", efs_get_free_bytes(dir_path) );
            dir_closeFileList( dir );
        }
    }
}

int cmd_ls_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	bool_t is_found;

	if( input != NULL )
	{
		path = console_concatenate_path( input, &is_found, p_instance );

		if( is_found )
		{
			cmd_ls_show_directory( p_instance->dev_console_tty, path );
		}
		else
		{
			console_command_printk( p_instance->dev_console_tty, "Path not found" );
		}
	}
	else
	{
		cmd_ls_show_directory( p_instance->dev_console_tty, p_instance->cmd_curr_dir );
	}

	return 0;
}

void cmd_ls_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_ls = 
{
    CONSOLE_COMMAND_TYPE,
    "ls",
	cmd_ls_init,
	cmd_ls_handler
};

DECLARE_COMMAND_SECTION( command_ls );
