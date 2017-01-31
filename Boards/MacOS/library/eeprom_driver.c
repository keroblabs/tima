#include "types.h"
#include "system.h"

//////////////////////////////////////////////////////////////////

#define IMAGE_OFFSET			(0x00000)
#define IMAGE_SIZE				(512)

//////////////////////////////////////////////////////////////////

static uint8_t sram_buffer[IMAGE_SIZE];

//////////////////////////////////////////////////////////////////

static int _unit_init( void )
{
    memset( sram_buffer, 0x00, IMAGE_SIZE );
	return MAL_OK;
}

static int _unit_write_block( uint64_t addr, uint8_t * buffer, uint32_t size )
{
    if( addr >= IMAGE_SIZE ) return MAL_FAIL;
    if( ( addr + size ) >= IMAGE_SIZE ) size = (uint32_t)(IMAGE_SIZE - addr);
    memcpy( &sram_buffer[addr], buffer, size );
	return MAL_OK;
}

static int _unit_read_block( uint64_t addr, uint8_t * buffer, uint32_t size )
{
    if( addr >= IMAGE_SIZE ) return MAL_FAIL;
    if( ( addr + size ) >= IMAGE_SIZE ) size = (uint32_t)(IMAGE_SIZE - addr);
    memcpy( buffer, &sram_buffer[addr], size );
	return MAL_OK;
}

static uint32_t _unit_get_image_size( void )
{
	return IMAGE_SIZE/512;
}

//////////////////////////////////////////////////////////////////

static const char flash_mass_name[] = "/eeprom1";

///////////////////////////////////////////////////////////////////////////

static const msd_driver_t eeprom_disc_mount =
{
	flash_mass_name,

    _unit_init,
    _unit_write_block,
    _unit_read_block,
    _unit_get_image_size,
    NULL,
	NULL,
	NULL,
	0,
	0,
	FALSE
};

DECLARE_STORAGE_DEVICE_SECTION( eeprom_disc_mount );
