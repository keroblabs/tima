#include "config_reader.h"
#include "mass_mal.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

uint16_t config_reader_load( const char * mal_device, uint8_t * data, uint16_t max_size )
{
    uint8_t lun = MAL_GetLun( mal_device );
    if( lun == LUN_NOT_FOUND ) return 0;

    return ( MAL_Read( lun, 0, data, max_size ) == MAL_OK ) ? max_size : 0;
}

bool_t config_reader_save( const char * mal_device, uint8_t * data, uint16_t size )
{
    return FALSE;
}
