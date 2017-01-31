#include "system.h"
#include "tima_libc.h"
#include "file.h"
#include "partition.h"
#include "efs.h"

#include "disc_utils.h"

////////////////////////////////////////////////////////////////////////////////////////////////

#define FIRST_LBA_BEGIN			0x80
#define VOLUME_NAME_MAX_SIZE    29

////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct part_data_t_
{
    mount_data_t mnt;
    
    char volume0[VOLUME_NAME_MAX_SIZE+1];
    char volume1[VOLUME_NAME_MAX_SIZE+1];
    char volume2[VOLUME_NAME_MAX_SIZE+1];
    char volume3[VOLUME_NAME_MAX_SIZE+1];
    
    char * volume_list[TOTAL_PARTITIONS];
    uint8_t to_format[TOTAL_PARTITIONS];
    
    uint8_t dev_index;
    
} part_data_t;

////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////

void * disc_utils_open_partition( char * device_name )
{
    part_data_t * p_mnt;
    uint8_t cnt;
    uint8_t device_index;
    
    device_index = 0xFF;
    
    // locate device
    for( cnt = 0; cnt < 100; cnt++ )
    {
        if( strcmp( disc_DevName(cnt), device_name ) == 0 )
        {
            device_index = cnt;
            break;
        }
    }
    
    if( device_index == 0xFF ) return NULL;
    
    // allocate memory for partition data
    p_mnt = ( part_data_t * )MMALLOC( sizeof( part_data_t ) );
    if( p_mnt == NULL ) return NULL;
    
    p_mnt->volume_list[0] = p_mnt->volume0;
    p_mnt->volume_list[1] = p_mnt->volume1;
    p_mnt->volume_list[2] = p_mnt->volume2;
    p_mnt->volume_list[3] = p_mnt->volume3;
    
    p_mnt->dev_index = device_index;
    
    // read partition data
    memcpy( &p_mnt->mnt, efs_get_partition( device_index ), sizeof( mount_data_t ) );
    
    for( cnt = 0; cnt < TOTAL_PARTITIONS; cnt++ )
    {
        p_mnt->to_format[cnt] = FALSE;
        
        strcpy( p_mnt->volume_list[cnt], "" );
        if( p_mnt->mnt.part.partitions[cnt].type != 0 )
        {
            dir_getVolumeName( ( FileSystem * )p_mnt->mnt.part.partitions[cnt].fs, p_mnt->volume_list[cnt] );
        }
    }
    
    return p_mnt;
}

uint32_t disc_utils_get_partition_data( void * p_data, uint8_t index, char * volume )
{
    part_data_t * p_mnt = ( part_data_t * )p_data;
    
    if( ( index > TOTAL_PARTITIONS ) || ( index == 0 ) )
    {
        return 0;
    }
    
    index--;
    
    if( volume != NULL ) strcpy( volume, p_mnt->volume_list[index] );
    return p_mnt->mnt.part.partitions[index].numSectors;
}

uint8_t disc_utils_partition_resize( void * p_data, uint8_t index, uint32_t new_size )
{
    uint8_t cnt;
    uint32_t next_lba;
    part_data_t * p_mnt = ( part_data_t * )p_data;
    
    if( ( index > TOTAL_PARTITIONS ) || ( index == 0 ) )
    {
        return 0;
    }
    
    index--;
    next_lba = 0;
    
    // get next lba
    for( cnt = index+1; cnt < TOTAL_PARTITIONS; cnt++ )
    {
        if( p_mnt->mnt.part.partitions[cnt].type != 0 )
        {
            next_lba = p_mnt->mnt.part.partitions[cnt].LBA_begin;
            break;
        }
    }
    
    if( next_lba == 0 ) next_lba = disc_GetSectorCount( p_mnt->dev_index );
    
    if( ( p_mnt->mnt.part.partitions[index].LBA_begin + new_size ) >= next_lba )
    {
        return 0;
    }
    
    p_mnt->mnt.part.partitions[index].numSectors = new_size;
    p_mnt->mnt.part.partitions[index].type = PT_FAT12;
    if( p_mnt->mnt.part.partitions[index].numSectors > 16384 )
        p_mnt->mnt.part.partitions[index].type = PT_FAT16;
    
    return index+1;
}

uint8_t disc_utils_create_partition( void * p_data, uint64_t * p_size )
{
    uint8_t index;
    uint32_t new_lba;
    uint8_t available_part;
    
    part_data_t * p_mnt = ( part_data_t * )p_data;
    
    // find out the beginning of all partitions
    if( p_mnt->mnt.part.partitions[0].type != 0 )
    {
        new_lba = p_mnt->mnt.part.partitions[0].LBA_begin;
    }
    else
    {
        new_lba = FIRST_LBA_BEGIN;
    }
    
    available_part = 0xFF;
    
    // calculate available space
    for( index = 0; index < TOTAL_PARTITIONS; index++ )
    {
        if( p_mnt->mnt.part.partitions[index].type != 0 )
        {
            new_lba = p_mnt->mnt.part.partitions[index].LBA_begin + p_mnt->mnt.part.partitions[index].numSectors;
        }
        else if( available_part == 0xFF )
        {
            available_part = index;
        }
    }
    
    if( ( new_lba < disc_GetSectorCount( p_mnt->dev_index ) ) && ( available_part != 0xFF ) )
    {
        p_mnt->mnt.part.partitions[available_part].LBA_begin = new_lba;
        p_mnt->mnt.part.partitions[available_part].numSectors = disc_GetSectorCount( p_mnt->dev_index ) - new_lba;
        
        p_mnt->mnt.part.partitions[available_part].type = PT_FAT12;
        if( p_mnt->mnt.part.partitions[available_part].numSectors > 16384 )
            p_mnt->mnt.part.partitions[available_part].type = PT_FAT16;
    }
    else
    {
        return 0;
    }
    
    return available_part+1;
}

void disc_utils_delete_all_partitions( void * p_data )
{
    uint8_t cnt;
    
    for( cnt = 0; cnt < TOTAL_PARTITIONS; cnt++ )
    {
        disc_utils_delete_partition( cnt+1, p_data );
    }
}

uint8_t disc_utils_delete_partition( uint8_t index, void * p_data )
{
    part_data_t * p_mnt = ( part_data_t * )p_data;
    
    if( ( index > TOTAL_PARTITIONS ) || ( index == 0 ) )
    {
        return 0;
    }
    
    index--;
    
    p_mnt->mnt.part.partitions[index].type = 0;
    p_mnt->mnt.part.partitions[index].LBA_begin = 0;
    p_mnt->mnt.part.partitions[index].numSectors = 0;
    
    if( index < ( TOTAL_PARTITIONS-1 ) )
    {
        memcpy( &p_mnt->mnt.part.partitions[index], &p_mnt->mnt.part.partitions[index+1], sizeof( PartitionField ) );
        p_mnt->mnt.part.partitions[TOTAL_PARTITIONS-1].type = 0;
        p_mnt->mnt.part.partitions[TOTAL_PARTITIONS-1].LBA_begin = 0;
        p_mnt->mnt.part.partitions[TOTAL_PARTITIONS-1].numSectors = 0;
    }
    
    return index+1;
}

uint8_t disc_utils_format_partition( void * p_data, uint8_t index, char * volume, uint8_t type )
{
    part_data_t * p_mnt = ( part_data_t * )p_data;
    
    if( ( index > TOTAL_PARTITIONS ) || ( index == 0 ) )
    {
        return 0;
    }
    
    if( ( type != PARTITION_FAT ) && ( type != PARTITION_EXT ) ) return 0;
    
    index--;
    
    p_mnt->to_format[index] = type;
    strncpy( p_mnt->volume_list[index], volume, VOLUME_NAME_MAX_SIZE );
    
    return index+1;
}

void disc_utils_close_partition( void * p_data )
{
    part_data_t * p_mnt = ( part_data_t * )p_data;
    uint8_t c;
    
    part_UpdatePartitionTable( &p_mnt->mnt.part );
    
    for( c = 0; c < TOTAL_PARTITIONS; c++ )
    {
        if( p_mnt->to_format[c] != 0 )
        {
            part_FormatPartition( &p_mnt->mnt.part.partitions[c], c, p_mnt->volume_list[c], p_mnt->to_format[c] );
        }
    }
    
    MFREE( p_mnt );
}
