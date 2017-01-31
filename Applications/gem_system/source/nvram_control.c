#include "system.h"
#include "timer.h"
#include "debug.h"

#include "nvram_control.h"

///////////////////////////////////////////////////////////////

#define NVRAM_MAX_SIZE		4096

#define NVRAM_ID			0x0120F58A

///////////////////////////////////////////////////////////////

static uint8_t local_nvram[NVRAM_MAX_SIZE];
static uint8_t mirror_nvram[NVRAM_MAX_SIZE];

static uint32_t nvram_size;
static uint8_t nvram_lun;

///////////////////////////////////////////////////////////////

void nvram_control_init( void )
{
	uint32_t nvram_id;

	nvram_size = 0;

	nvram_lun = MAL_GetLun( "/eeprom" );
	if( nvram_lun == LUN_NOT_FOUND )
	{
		DEBUG_PRINTK( "eeprom not found\r\n" );
		return;
	}

    MAL_Init( nvram_lun );

    nvram_size = MAL_GetBlockCount(nvram_lun) * MAL_GetBlockSize(nvram_lun);
    if( nvram_size > NVRAM_MAX_SIZE ) nvram_size = NVRAM_MAX_SIZE;

    MAL_Read( nvram_lun, 0, local_nvram, nvram_size );
    memcpy( mirror_nvram, local_nvram, nvram_size );

    nvram_control_read( NVRAM_ADDR_NVRAM_ID, &nvram_id, sizeof( nvram_id ) );

    DEBUG_PRINTK( "eeprom ID = 0x%08x\r\n", nvram_id );

    if( nvram_id != NVRAM_ID )
    {
    	DEBUG_PRINTK( "Format eeprom\r\n" );

    	nvram_id = 0x0120F58A;
        nvram_control_write( NVRAM_ADDR_NVRAM_ID, &nvram_id, sizeof( nvram_id ) );

    	nvram_id = 0;
    	nvram_control_write( NVRAM_ADDR_ODOMETER, &nvram_id, sizeof( nvram_id ) );

        nvram_control_flush();
    }
}

void nvram_control_read( uint32_t address, void * data, uint32_t size )
{
	uint32_t cnt;
	uint8_t * p_data = ( uint8_t * )data;

	if( nvram_size == 0 ) return;
	if( size == 0 ) return;

	for( cnt = address; cnt < nvram_size; cnt++ )
	{
		*p_data = local_nvram[cnt];

		//DEBUG_PRINTK( "0x%02x ", *p_data );

		p_data++;
		if( --size == 0 ) break;
	}

	//DEBUG_PRINTK( "\r\n" );
}

void nvram_control_write( uint32_t address, void * data, uint32_t size )
{
	uint32_t cnt;
	uint8_t * p_data = ( uint8_t * )data;

	if( nvram_size == 0 ) return;
	if( size == 0 ) return;

	for( cnt = address; cnt < nvram_size; cnt++ )
	{
		local_nvram[cnt] = *p_data;

		//DEBUG_PRINTK( "0x%02x ", local_nvram[cnt] );

		p_data++;
		if( --size == 0 ) break;
	}

	//DEBUG_PRINTK( "\r\n" );
}

void nvram_control_flush( void )
{
	uint32_t cnt;

#ifdef DEBUG_ENABLED
	uint32_t write_cnt = 0;
#endif

	if( nvram_size == 0 ) return;

	for( cnt = 0; cnt < nvram_size; cnt++ )
	{
		if( local_nvram[cnt] != mirror_nvram[cnt] )
		{
			mirror_nvram[cnt] = local_nvram[cnt];
			MAL_Write( nvram_lun, cnt, &local_nvram[cnt], 1 );

#ifdef DEBUG_ENABLED
			write_cnt++;
#endif
		}
	}

#ifdef DEBUG_ENABLED
	DEBUG_PRINTK( "Total nvram update = %d\r\n", write_cnt );
#endif
}

