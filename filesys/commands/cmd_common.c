#include "system.h"
#include "file.h"
#include "efs.h"
#include "tima_libc.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

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

