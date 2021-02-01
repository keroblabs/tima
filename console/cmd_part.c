#include "system.h"
#include "tima_libc.h"
#include "file.h"
#include "partition.h"
#include "efs.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

#define TOTAL_PARAM_LIST		4
#define FIRST_LBA_BEGIN			0x80

///////////////////////////////////////////////////////////////

static bool_t is_loaded;
static uint8_t curr_device;
static mount_data_t mnt;

//static PartitionField partition_table[TOTAL_PARTITIONS];
static bool_t to_format[TOTAL_PARTITIONS];

static char param0[30];
static char param1[30];
static char param2[30];
static char param3[30];
static char * param_list[TOTAL_PARAM_LIST];

static char volume0[30];
static char volume1[30];
static char volume2[30];
static char volume3[30];
static char * volume_list[TOTAL_PARAM_LIST];

///////////////////////////////////////////////////////////////

void cmd_part_list_devices( device_t * dev )
{
	uint8_t index;
	const char * name;

	index = 0;

	while( ( name = disc_DevName( index ) ) != NULL )
	{
		console_command_printk( dev, "%2d: %s\r\n", index+1, name );
		index++;
	}
}

uint8_t cmd_part_total_devices( void )
{
	uint8_t index;
	const char * name;

	index = 0;

	while( ( name = disc_DevName( index ) ) != NULL )
	{
		index++;
	}

	return index;
}

void cmd_part_show_table( device_t * dev )
{
	uint8_t index;
	char volume_name[20];

	for( index = 0; index < TOTAL_PARTITIONS; index++ )
	{
		memset( volume_name, 0x20, sizeof( volume_name ) );
		memcpy( volume_name, volume_list[index], strlen( volume_list[index] ) );
		volume_name[19] = 0;

		console_command_printk( dev, "%d: %s - %6d sectors%s\r\n",   index+1, 
														volume_name,
														mnt.part.partitions[index].numSectors,
														to_format[index]?" - [format]":"" );
	}
}

void cmd_part_resize( device_t * dev, uint8_t index, uint32_t new_size )
{
	uint8_t cnt;
	uint32_t next_lba;

	if( ( index > TOTAL_PARTITIONS ) || ( index == 0 ) )
	{
		console_command_printk( dev, "Invalid partition index\r\n" );
		return;
	}

	index--;
	next_lba = 0;

	// get next lba
	for( cnt = index+1; cnt < TOTAL_PARTITIONS; cnt++ )
	{
		if( mnt.part.partitions[cnt].type != 0 )
		{
			next_lba = mnt.part.partitions[cnt].LBA_begin;
			break;
		}
	}

	if( next_lba == 0 ) next_lba = disc_GetSectorCount( curr_device );
	
	if( ( mnt.part.partitions[index].LBA_begin + new_size ) >= next_lba )
	{
		console_command_printk( dev, "Invalid partition size\r\n" );
		return;
	}

	mnt.part.partitions[index].numSectors = new_size;
	mnt.part.partitions[index].type = PT_FAT12;
	if( mnt.part.partitions[index].numSectors > 16384 ) mnt.part.partitions[index].type = PT_FAT16;

	console_command_printk( dev, "New partition size: %d\r\n", new_size );
}

void cmd_part_delete_partition( device_t * dev, uint8_t index )
{
	if( ( index > TOTAL_PARTITIONS ) || ( index == 0 ) )
	{
		console_command_printk( dev, "Invalid partition index\r\n" );
		return;
	}

	index--;

	mnt.part.partitions[index].type = 0;
	mnt.part.partitions[index].LBA_begin = 0;
	mnt.part.partitions[index].numSectors = 0;

	if( index < ( TOTAL_PARTITIONS-1 ) )
	{
		memcpy( &mnt.part.partitions[index], &mnt.part.partitions[index+1], sizeof( PartitionField ) );
		mnt.part.partitions[TOTAL_PARTITIONS-1].type = 0;
		mnt.part.partitions[TOTAL_PARTITIONS-1].LBA_begin = 0;
		mnt.part.partitions[TOTAL_PARTITIONS-1].numSectors = 0;
	}

	console_command_printk( dev, "Partition %d deleted\r\n", index+1 );
}

void cmd_part_create_partition( device_t * dev )
{
	uint8_t index;
	uint32_t new_lba;
	uint8_t available_part;

	// find out the beginning of all partitions
	if( mnt.part.partitions[0].type != 0 )
	{
		new_lba = mnt.part.partitions[0].LBA_begin;
	}
	else
	{
		new_lba = FIRST_LBA_BEGIN;
	}

	available_part = 0xFF;

	// calculate available space
	for( index = 0; index < TOTAL_PARTITIONS; index++ )
	{
		if( mnt.part.partitions[index].type != 0 )
		{
			new_lba = mnt.part.partitions[index].LBA_begin + mnt.part.partitions[index].numSectors;
		}
		else if( available_part == 0xFF )
		{
			available_part = index;
		}
	}

	if( ( new_lba < disc_GetSectorCount( curr_device ) ) && ( available_part != 0xFF ) )
	{
		mnt.part.partitions[available_part].LBA_begin = new_lba;
		mnt.part.partitions[available_part].numSectors = disc_GetSectorCount( curr_device ) - new_lba;
		mnt.part.partitions[available_part].type = PT_FAT12;
		if( mnt.part.partitions[available_part].numSectors > 16384 ) 
			mnt.part.partitions[available_part].type = PT_FAT16;
		console_command_printk( dev, "New partition: %d, %d sectors\r\n", available_part+1, mnt.part.partitions[available_part].numSectors );
	}
	else
	{
		console_command_printk( dev, "No space available\r\n" );
	}
}

void cmd_part_load_partition_table( device_t * dev, uint8_t index )
{
	uint8_t c;

	memcpy( &mnt, efs_get_partition( curr_device ), sizeof( mnt ) );
	is_loaded = TRUE;
	console_command_printk( dev, "Loaded device: %s\r\n", disc_DevName(index) );

	for( c = 0; c < TOTAL_PARTITIONS; c++ )
	{
		strcpy( volume_list[c], "" );
		if( mnt.part.partitions[c].type != 0 )
		{
			dir_getVolumeName( ( FileSystem * )mnt.part.partitions[c].fs, volume_list[c] );
		}
	}
}

void cmd_part_update_partition_table( device_t * dev, uint8_t index )
{
	uint8_t c;

	console_command_printk( dev, "Updating device: %s\r\n", disc_DevName(index) );
	part_UpdatePartitionTable( &mnt.part );

	for( c = 0; c < TOTAL_PARTITIONS; c++ )
	{
		if( to_format[c] == TRUE )
		{
			part_FormatPartition( &mnt.part.partitions[c], c, volume_list[c] );
		}
	}

	console_command_printk( dev, "Reboot required\r\n" );
}

int cmd_part_handler( console_instance_t * p_instance, char * input )
{
	int argc;
	uint8_t value;
	device_t * dev = p_instance->dev_console_tty;

	if( input != NULL )
	{
		argc = console_tokenizer( input, param_list, " ", TOTAL_PARAM_LIST );

		if( argc == 0 )
		{
			// show help
			return 0;
		}

		if( !strcmp( param_list[0], "device" ) )
		{
			if( argc == 1 ) 
			{
				if( curr_device != LUN_NOT_FOUND )
				{
					console_command_printk( dev, "Selected device: %s\r\n", disc_DevName(curr_device) );
				}

				cmd_part_list_devices(dev);
			}
			else
			{
				value = ( uint8_t )tima_atoi( param_list[1] );
				if( ( value > cmd_part_total_devices() ) || ( value == 0 ) )
				{
					console_command_printk( dev, "Invalid device index\r\n" );
				}
				else
				{
					value--;
					curr_device = value;
					cmd_part_load_partition_table(dev, value);
				}
			}
		}
		else if( !strcmp( param_list[0], "mounted" ) )
		{
			console_command_printk( dev, "/:\r\n" );
			console_show_mounted_devices( dev );
		}
		else if( !strcmp( param_list[0], "list" ) )
		{
			if( is_loaded == FALSE )
			{
				console_command_printk( dev, "No partition table loaded\r\n" );
			}
			else
			{
				cmd_part_show_table(dev);
			}
		}
		else if( !strcmp( param_list[0], "resize" ) )
		{
			if( argc >= 3 )
			{
				cmd_part_resize( dev, (uint8_t)tima_atoi( param_list[1] ), (uint32_t)tima_atoi( param_list[2] ) );
			}
			else
			{
				console_command_printk( dev, "part resize <part_num> <sectors>\r\n" );
			}
		}
		else if( !strcmp( param_list[0], "create" ) )
		{
			cmd_part_create_partition(dev);
		}
		else if( !strcmp( param_list[0], "delete" ) )
		{
			if( argc >= 2 )
			{
				cmd_part_delete_partition( dev, (uint8_t)tima_atoi( param_list[1] ) );
			}
			else
			{
				console_command_printk( dev, "part delete <part_num>\r\n" );
			}
		}
		else if( !strcmp( param_list[0], "apply" ) )
		{
			cmd_part_update_partition_table( dev, curr_device );
		}
		else if( !strcmp( param_list[0], "format" ) )
		{
			if( argc == 3 )
			{
				value = (uint8_t)tima_atoi( param_list[1] );
				if( ( value > TOTAL_PARTITIONS ) || ( value == 0 ) )
				{
					console_command_printk( dev, "Invalid partition index\r\n" );
					return 0;
				}
				to_format[value-1] = TRUE;
				strncpy( volume_list[value-1], param_list[2], 10 );
			}
			else
			{
				console_command_printk( dev, "part format <part_num> <volume_name>\r\n" );
			}
		}
		else if( !strcmp( param_list[0], "clear" ) )
		{
			memset( mnt.part.partitions, 0x00, sizeof( mnt.part.partitions ) );
			memset( to_format, 0x00, sizeof( to_format ) );

			is_loaded = FALSE;
			curr_device = LUN_NOT_FOUND;
		}
	}
	else
	{
		// show help
		console_command_printk( dev, "part device\r\n" );
		console_command_printk( dev, "part device <dev_num>\r\n" );
		console_command_printk( dev, "part list\r\n" );
		console_command_printk( dev, "part mounted\r\n" );
		console_command_printk( dev, "part resize <part_num> <sectors>\r\n" );
		console_command_printk( dev, "part create\r\n" );
		console_command_printk( dev, "part delete <part_num>\r\n" );
		console_command_printk( dev, "part apply\r\n" );
		console_command_printk( dev, "part format <part_num> <volume_name>\r\n" );
		console_command_printk( dev, "part clear\r\n" );
	}

	return 0;
}

void cmd_part_init( void )
{
	memset( mnt.part.partitions, 0x00, sizeof( mnt.part.partitions ) );
	memset( to_format, 0x00, sizeof( to_format ) );

	is_loaded = FALSE;
	curr_device = LUN_NOT_FOUND;

	param_list[0] = param0;
	param_list[1] = param1;
	param_list[2] = param2;
	param_list[3] = param3;

	volume_list[0] = volume0;
	volume_list[1] = volume1;
	volume_list[2] = volume2;
	volume_list[3] = volume3;
}

///////////////////////////////////////////////////////////////

const command_data_t command_part = 
{
	"part",
	cmd_part_init,
	cmd_part_handler
};

DECLARE_COMMAND_SECTION( command_part );
