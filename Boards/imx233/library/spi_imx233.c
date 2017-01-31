#include "system_imx233.h"
#include "regsssp.h"
#include "regspinctrl.h"
#include "pinctrl-imx233.h"

#include "buffer.h"
#include "device.h"
#include "system.h"
#include "debug.h"

///////////////////////////////////////////////////////////////

#define SPI_DEV_INDEX						1

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static void spi_init( void )
{
	// setup ports
	imx233_set_pin_function( 0, 24, PINCTRL_FUNCTION_ALT2 ); // spi_clk
	imx233_set_pin_function( 0, 20, PINCTRL_FUNCTION_ALT2 ); // MOSI
	imx233_set_pin_function( 0,  0, PINCTRL_FUNCTION_ALT2 ); // MISO

	HW_SSP_CTRL0_WR( 0x00 );

	HW_SSP_TIMING.CLOCK_RATE = 0x20;
	HW_SSP_TIMING.CLOCK_DIVIDE = 0x30;
	HW_SSP_TIMING.TIMEOUT = 0;

	HW_SSP_CTRL1_WR( BF_SSP_CTRL1_WORD_LENGTH(BV_SSP_CTRL1_WORD_LENGTH__EIGHT_BITS) );
}

void INT_SPI( void )
{
}

static uint32_t spi_port_transmit( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t cnt;

	for( cnt = 0; cnt < size; cnt++ )
	{
		while( HW_SSP_STATUS.BUSY ) {}
		HW_SSP_DATA_WR(buffer[cnt]);
	}

	return size;
}

static uint32_t spi_port_receive( uint32_t index, uint8_t * buffer, uint32_t size )
{
    uint32_t num_bytes_present = 0;
	uint32_t cnt;

    num_bytes_present = buffer_get_size( ( buffer_data_t * )&rx_uart_buffer_data );

    if( ( num_bytes_present > 0 ) && ( buffer != NULL ) )
    {
    	for( cnt = 0; cnt < size; cnt++ )
    	{
    		if( cnt >= num_bytes_present ) break;
    		buffer[cnt] = buffer_read(( buffer_data_t * )&rx_uart_buffer_data);
    	}
    }

    return num_bytes_present;
}

static uint32_t spi_port_ioctl( uint32_t index, uint32_t param, void * value )
{
	if( param == DEVICE_SET_CONFIG_DATA )
	{
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////

static void spi_port_init( uint32_t index )
{
}

static void spi_port_close( uint32_t index )
{
}

static bool_t spi_port_validate( uint32_t index )
{
    if( index == SPI_DEV_INDEX ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

static const device_data_t spi_01_device =
{
    "spi*",
    &_in_use,

	spi_port_init,
	spi_port_close,
    NULL,
	spi_port_validate,
    NULL,
	spi_port_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

	spi_port_receive,
	spi_port_transmit,

};

DECLARE_DEVICE_SECTION( spi_01_device );



