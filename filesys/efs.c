#include "system.h"
#include "efs.h"
#include "mass_mal.h"
#include "debug.h"

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

#define MAX_DEVICE_LIST			4
#define FS_MAX_FILE_HANDLER		6

///////////////////////////////////////////////////////////////////////////////

static mount_data_t mount_list[ MAX_DEVICE_LIST ];

static bool_t is_init = FALSE;

static FileSystem * first_fs;
static FileSystem * last_fs;

///////////////////////////////////////////////////////////////////////////////

int _efs_mount_unit( uint8_t lun, mount_data_t * mnt )
{
	uint8_t c;
	FileSystem * fs;

	DEBUG_PRINTK( "_efs_mount_unit\r\n" );

	// low level initialization
	if( disc_InitLun( lun ) )
	{
		return -3;
	}

	// load MBR
	if( part_initPartition( &mnt->part, lun ) )
	{
		return -4;
	}

	// load device name
	strcpy( mnt->device_name, disc_DevName( lun ) );

	// mount file system partitions
	for( c = 0; c < TOTAL_PARTITIONS; c++ )
	{
		if( mnt->part.partitions[c].type != 0 )
		{
			// allocate memory for fs
			fs = ( FileSystem * )MMALLOC( sizeof( FileSystem ) );
			if( fs == NULL ) return -5;
			memset( fs, 0x00, sizeof( FileSystem ) );
			fs->partition = &mnt->part.partitions[c];
			mnt->part.partitions[c].fs = fs;

			// init fs
			if(fs_initFs(fs))
			{
				MFREE( fs );
				continue;
			}

			// link fs list
			if( first_fs == NULL ) first_fs = fs;
			else last_fs->p_next = fs;
			last_fs = fs;
		}
	}

	return 0;
}

uint32_t efs_get_free_bytes( char * full_filename )
{
	FileSystem * fs;
	char * filename;
	char * file_getVolume( const char * full_filename, char * volume );
	extern char volume[];

	filename = file_getVolume( full_filename, volume );

	fs = efs_get_filesystem( volume );
	if( fs != NULL ) 
	{
		fs_updateFreeClusters(fs);
		return fs->FreeClusterCount * fs->volumeId.BytesPerSector * fs->volumeId.SectorsPerCluster;
	}

	return 0;
}

FileSystem * efs_get_filesystem( char * volume )
{
	FileSystem * fs = first_fs;
	char output[15];

	while( fs != NULL )
	{
		// search in the recent file systems
		dir_getVolumeName( fs, output );
		if( !strcmp( volume, output ) ) return fs;

		fs = fs->p_next;
	}

    return( NULL );
}

mount_data_t * efs_get_partition( uint8_t lun )
{
	if( lun >= MAX_DEVICE_LIST ) return NULL;
	return &mount_list[lun];
}

void efs_init( void )
{
	int i;

	if( is_init == TRUE ) return;
	first_fs = last_fs = NULL;

	// mount installed units
    for( i = 0; i < MAX_DEVICE_LIST; i++ )
    {
        _efs_mount_unit( i, &mount_list[i] );
    }

	is_init = TRUE;
}

DECLARE_INIT_MODULE_SECTION(efs_init);

