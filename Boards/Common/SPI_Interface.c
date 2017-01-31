#include "system.h"
#include "hardware.h"

////////////////////////////////////////////////////////////////////

static uint16_t spi_init_mask = 0;

////////////////////////////////////////////////////////////////////

spi_driver_t * spi_driver = NULL;

////////////////////////////////////////////////////////////////////

void SPI_interface_init( void )
{
	File spi_driver_hdl;
	
	spi_driver = NULL;
	
    if( file_fopen( &spi_driver_hdl, "/dev/spi", FS_MODE_DEVICE ) == 0 )
	{
        lcd_driver = ( spi_driver_t * )file_fioctrl( &spi_driver_hdl, DEVICE_GET_API, NULL );
		file_fclose( &spi_driver_hdl );
	}	
}

void SPI_Open( uint8_t index )
{
	if( spi_driver == NULL ) return;
	
	if( index >= spi_driver->num_ports() ) return;
    if( spi_init_mask & ( 1 << index ) ) return;
	
	spi_driver->init( index );
    spi_init_mask |= ( 1 << index );
}

void SPI_SendData( uint8_t index, uint8_t * buffer, uint16_t size )
{
	uint16_t counter;

	if( index >= SPI_get_total_ports() ) return;
	
	for( counter = 0; counter < size; counter++ )
	{
		spi_driver->send( index, buffer[ counter ] );
	}
}

void SPI_ReceiveData( uint8_t index, uint8_t * buffer, uint16_t size )
{
	uint16_t counter;

	if( index >= SPI_get_total_ports() ) return;
	
	for( counter = 0; counter < size; counter++ )
	{
		buffer[ counter ] = spi_driver->receive( index );
	}
}

void SPI_ExchangeData_Send(  uint8_t index,
							 uint8_t command, uint8_t * param, uint16_t param_size, 
						     uint8_t * data, uint16_t data_size )
{
	uint16_t counter;
	
	if( index >= SPI_get_total_ports() ) return;
	
	// send command
	spi_driver->send( index, command );

	// send parameters
	if( param != NULL )
	{
		SPI_SendData( index, param, param_size );
	}

	// send/receive data
	if( data != NULL )
	{
		SPI_SendData( index, data, data_size );
	}
	
}

void SPI_ExchangeData_Receive(  uint8_t index,
							    uint8_t command, uint8_t * param, uint16_t param_size, 
						        uint8_t * data, uint16_t data_size )
{
	uint16_t counter;

	if( index >= SPI_get_total_ports() ) return;
	
	// send command
	spi_driver->send( index, command );

	// send parameters
	if( param != NULL )
	{
		SPI_SendData( index, param, param_size );
	}

	// send/receive data
	if( data != NULL )
	{
		SPI_ReceiveData( index, data, data_size );
	}
}

//////////////////////////////////////////////////////////////////////////////////////

