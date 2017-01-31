#ifndef _DISC_UTILS_H_
#define _DISC_UTILS_H_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define VOLUME_NAME_MAX_SIZE    29

//////////////////////////////////////////////////////////////////////////////////////

void * disc_utils_open_partition( char * device_name );
void disc_utils_close_partition( void * p_data );

void disc_utils_delete_all_partitions( void * p_data );

uint8_t disc_utils_create_partition( void * p_data, uint64_t * p_size );
uint8_t disc_utils_partition_resize( void * p_data, uint8_t index, uint32_t new_size );
uint8_t disc_utils_delete_partition( uint8_t index, void * p_data );

uint32_t disc_utils_get_partition_data( void * p_data, uint8_t index, char * volume );

//////////////////////////////////////////////////////////////////////////////////////

#endif // _DISC_UTILS_H_

