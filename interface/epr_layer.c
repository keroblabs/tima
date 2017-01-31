#include "epr_layer.h"
#include "mass_mal.h"
#include "debug.h"

///////////////////////////////////////////////////////////////

epr_data_t * epr_layer_init( const char * dev, uint32_t data_size, uint32_t offset )
{
	epr_data_t * ret = ( epr_data_t * )MMALLOC( sizeof( epr_data_t ) );
	if( ret == NULL )
	{
		DEBUG_PRINTK( "EPR error: no memory 1\r\n" );
		return NULL;
	}

	ret->buffer = ( uint8_t * )MMALLOC( data_size );
	if( ret->buffer == NULL )
	{
		DEBUG_PRINTK( "EPR error: no memory 2\r\n" );
		MFREE( ret );
		return NULL;
	}

	ret->addr_offset = offset;
	ret->data_size = data_size;

	ret->dev_lun = MAL_GetLun( ( char * )dev );
	if( ret->dev_lun == LUN_NOT_FOUND )
	{
		DEBUG_PRINTK( "EPR error: no dev\r\n" );
		MFREE( ret->buffer );
		MFREE( ret );
		return NULL;
	}

	if( ( ( data_size + offset ) / 512 ) >= MAL_GetBlockCount( ret->dev_lun ) )
	{
		DEBUG_PRINTK( "EPR error: no size\r\n" );
		MFREE( ret->buffer );
		MFREE( ret );
		return NULL;
	}

	MAL_Init( ret->dev_lun );

	return ret;
}

void * epr_layer_read( epr_data_t * p_data )
{
	MAL_Read( p_data->dev_lun, p_data->addr_offset, p_data->buffer, p_data->data_size );
	return p_data->buffer;
}

void epr_layer_flush( epr_data_t * p_data )
{
	MAL_Write( p_data->dev_lun, p_data->addr_offset, p_data->buffer, p_data->data_size );
}

void epr_layer_close( epr_data_t * p_data )
{
	MFREE( p_data->buffer );
	MFREE( p_data );
}
