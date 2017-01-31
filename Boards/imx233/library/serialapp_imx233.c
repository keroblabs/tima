#include "system_imx233.h"
#include "regsuartapp.h"
#include "regspinctrl.h"
#include "pinctrl-imx233.h"

#include "buffer.h"
#include "device.h"
#include "system.h"
#include "debug.h"

///////////////////////////////////////////////////////////////

#define UART_APP_DEV_INDEX					3
#define RX_SERIAL_BUFFER_SIZE				64
#define TX_SERIAL_BUFFER_SIZE				256

///////////////////////////////////////////////////////////////

static uint8_t         rx_uart_buffer[ RX_SERIAL_BUFFER_SIZE ];
static buffer_data_t   rx_uart_buffer_data;

//static uint8_t         tx_uart_buffer[ TX_SERIAL_BUFFER_SIZE ];
//static buffer_data_t   tx_uart_buffer_data;
//static bool_t		   tx_busy;

///////////////////////////////////////////////////////////////

static void uart_app_init( uint32_t baudrate )
{
	__IO uint32_t divisor;
	__IO uint32_t fraction;
	__IO uint32_t full_value;

	/* setup UART DEBUG */
	imx233_set_pin_function( 0, 30, PINCTRL_FUNCTION_ALT2 );
	imx233_set_pin_function( 0, 31, PINCTRL_FUNCTION_ALT2 );

	full_value = 24000000U * 32 / ( baudrate );
	divisor = full_value >> 6;
	fraction = full_value & 0x3F;

	HW_UARTAPP_CTRL0_CLR( 1, BM_UARTAPP_CTRL0_SFTRST | BM_UARTAPP_CTRL0_CLKGATE );

	HW_UARTAPP_CTRL2_WR(1, (0<<BP_UARTAPP_CTRL2_RXIFLSEL) |
			   	   	  	   (0<<BP_UARTAPP_CTRL2_TXIFLSEL) |
						   (1<<BP_UARTAPP_CTRL2_RXE) |
						   (1<<BP_UARTAPP_CTRL2_TXE) |
						   (1<<BP_UARTAPP_CTRL2_UARTEN));

	HW_UARTAPP_LINECTRL_WR(1, (divisor<<BP_UARTAPP_LINECTRL_BAUD_DIVINT) |
							  (fraction<<BP_UARTAPP_LINECTRL_BAUD_DIVFRAC) |
							  (3<<BP_UARTAPP_LINECTRL_WLEN)  |
							  (1<<BP_UARTAPP_LINECTRL_FEN));
}

void INT_UART_APP( void )
{
}

static void imx233_app_port_process( void )
{
	while( !( HW_UARTAPP_STAT_RD(1) & BM_UARTAPP_STAT_RXFE ) )
	{
		uint8_t rx_byte = HW_UARTAPP_DATA_RD(1);
		buffer_write( &rx_uart_buffer_data, rx_byte );

		//DEBUG_PRINTK( "[%c]", (char)rx_byte );
	}
}

static uint32_t imx233_port_transmit( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t cnt;

	for( cnt = 0; cnt < size; cnt++ )
	{
		while( HW_UARTAPP_STAT_RD(1) & BM_UARTAPP_STAT_TXFF ) {}
		HW_UARTAPP_DATA_WR(1, buffer[cnt]);
	}

	return size;
}

static uint32_t imx233_port_receive( uint32_t index, uint8_t * buffer, uint32_t size )
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

static uint32_t imx233_port_ioctl( uint32_t index, uint32_t param, void * value )
{
	uint32_t * p_baud;

	if( param == DEVICE_SET_CONFIG_DATA )
	{
		p_baud = ( uint32_t * )value;
		uart_app_init( *p_baud );
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////

static void imx233_port_init( uint32_t index )
{
	buffer_init( ( buffer_data_t * )&rx_uart_buffer_data, ( uint8_t * )rx_uart_buffer, RX_SERIAL_BUFFER_SIZE );
	//buffer_init( ( buffer_data_t * )&tx_uart_buffer_data, ( uint8_t * )tx_uart_buffer, TX_SERIAL_BUFFER_SIZE );
	//tx_busy = FALSE;
}

static void imx233_port_close( uint32_t index )
{
}

static bool_t imx233_port_validate( uint32_t index )
{
    if( index == UART_APP_DEV_INDEX ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

static const device_data_t imx233_uart_app_device =
{
    "tty*",
    &_in_use,

	imx233_port_init,
	imx233_port_close,
    NULL,
	imx233_port_validate,
    NULL,
	imx233_port_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

	imx233_port_receive,
	imx233_port_transmit,

};

DECLARE_DEVICE_SECTION( imx233_uart_app_device );
DECLARE_PROCESS_SECTION( imx233_app_port_process );

