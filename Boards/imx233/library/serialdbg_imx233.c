#include "system_imx233.h"
#include "regsuartdbg.h"
#include "serialdbg_imx233.h"
#include "regspinctrl.h"
#include "pinctrl-imx233.h"

#include "buffer.h"
#include "device.h"
#include "system.h"

///////////////////////////////////////////////////////////////

#define UART_DBG_DEV_INDEX					18
#define RX_SERIAL_BUFFER_SIZE				64
#define TX_SERIAL_BUFFER_SIZE				256

///////////////////////////////////////////////////////////////

static uint8_t         rx_uart_buffer[ RX_SERIAL_BUFFER_SIZE ];
static buffer_data_t   rx_uart_buffer_data;

static uint8_t         tx_uart_buffer[ TX_SERIAL_BUFFER_SIZE ];
static buffer_data_t   tx_uart_buffer_data;
static bool_t		   tx_busy;

static __IO bool_t		intr_busy;

///////////////////////////////////////////////////////////////

static void uartdbg_init( uint32_t baudrate )
{
	__IO uint32_t divisor;
	__IO uint32_t fraction;
	__IO uint32_t full_value;

	/* setup UART DEBUG */
	imx233_set_pin_function( 1, 26, PINCTRL_FUNCTION_ALT2 );
	imx233_set_pin_function( 1, 27, PINCTRL_FUNCTION_ALT2 );

	//full_value = ( 24000000 / ( 16 * baudrate ) ) * 100;
	//divisor = full_value / 100;
	//fraction = ( ( full_value - ( full_value * 100 ) ) * 64 ) / 100;
	full_value = 24000000U * 4 / ( baudrate );
	divisor = full_value >> 6;
	fraction = full_value & 0x3F;

	HW_SERIALDBG_UARTCTRL = 0x00;

	HW_SERIALDBG_FRACBAUD = fraction;
	HW_SERIALDBG_INTBAUD = divisor;

	/* Set the UART to be 8 bits, 1 stop bit, no parity, fifo enabled */
	HW_SERIALDBG_LINECTRL = HW_SERIALDBG_LINECTRL__WLEN_8 | HW_SERIALDBG_LINECTRL__FEN;

	/* tx/rx intr on fifo empty */
	HW_SERIALDBG_FIFOINTR = 0; // ((0<<3)|(0<<0));

	/* enable interrupt */
	HW_SERIALDBG_INTRMASK = 0; // HW_SERIALDBG__TXIM|HW_SERIALDBG__RXIM;
	HW_SERIALDBG_CLEARINTR = 0; // HW_SERIALDBG__TXIM|HW_SERIALDBG__RXIM;

	/* enable the UART */
	HW_SERIALDBG_UARTCTRL = HW_SERIALDBG_UARTCTRL__RXE | HW_SERIALDBG_UARTCTRL__TXE | HW_SERIALDBG_UARTCTRL__UARTEN;

	/* enable interrupt */
	//imx233_enable_interrupt(INT_SRC_UART_DEBUG, TRUE);
}

static void uart_dbg_process( void )
{
	while(!(HW_SERIALDBG_FLAGS & HW_SERIALDBG_FLAGS__RXFIFOEMPTY))
	{
		buffer_write( &rx_uart_buffer_data, (uint8_t)HW_SERIALDBG_DR );
	}
}

void INT_UART_DEBUG( void )
{
	//if( intr_busy == TRUE ) return;

	// interrupt due to TX buffer empty
	if(HW_SERIALDBG_RAWISTATUS & HW_SERIALDBG__TXIM)
	{
		//intr_busy = TRUE;
		tx_busy = FALSE;
		while(!(HW_SERIALDBG_FLAGS & HW_SERIALDBG_FLAGS__TXFIFOFULL))
		{
			if( buffer_is_empty(&tx_uart_buffer_data) ) break;
			HW_SERIALDBG_DR = buffer_read( &tx_uart_buffer_data );
			tx_busy = TRUE;
		}

		HW_SERIALDBG_CLEARINTR = HW_SERIALDBG__TXIM;
	}

	// interrupt due to RX buffer full
	if (HW_SERIALDBG_RAWISTATUS & HW_SERIALDBG__RXIM)
	{
		//intr_busy = TRUE;
		while(!(HW_SERIALDBG_FLAGS & HW_SERIALDBG_FLAGS__RXFIFOEMPTY))
		{
			//uint8_t rx_byte = HW_SERIALDBG_DR;

			buffer_write( &rx_uart_buffer_data, (uint8_t)HW_SERIALDBG_DR );
		}

		HW_SERIALDBG_CLEARINTR = HW_SERIALDBG__RXIM;
	}

	//intr_busy = FALSE;
}

static uint32_t imx233_port_transmit( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint8_t tx_byte;

	uint32_t cnt;

	for( cnt = 0; cnt < size; cnt++ )
	{
		while(HW_SERIALDBG_FLAGS & HW_SERIALDBG_FLAGS__TXFIFOFULL) {}
		HW_SERIALDBG_DR = buffer[cnt];
	}

	return size;
#if 0
	//imx233_enable_interrupt(INT_SRC_UART_DEBUG, FALSE);

	if( HW_SERIALDBG_FLAGS & HW_SERIALDBG_FLAGS__TXFIFOEMPTY )
	{
		tx_busy = FALSE;
	}

	buffer_write_buffer( &tx_uart_buffer_data, buffer, size );

	if( tx_busy == FALSE )
	{
		while(!(HW_SERIALDBG_FLAGS & HW_SERIALDBG_FLAGS__TXFIFOFULL))
		{
			if( buffer_is_empty(&tx_uart_buffer_data) ) break;

			tx_byte = buffer_read( &tx_uart_buffer_data );
			HW_SERIALDBG_DR = tx_byte;
		}

		tx_busy = TRUE;
	}

	//imx233_enable_interrupt(INT_SRC_UART_DEBUG, TRUE);

	return size;
#endif
}

static uint32_t imx233_port_receive( uint32_t index, uint8_t * buffer, uint32_t size )
{
    uint32_t num_bytes_present = 0;
	uint32_t cnt;

	//imx233_enable_interrupt(INT_SRC_UART_DEBUG, FALSE);

    num_bytes_present = buffer_get_size( ( buffer_data_t * )&rx_uart_buffer_data );

    if( ( num_bytes_present > 0 ) && ( buffer != NULL ) )
    {
    	for( cnt = 0; cnt < size; cnt++ )
    	{
    		if( cnt >= num_bytes_present ) break;
    		buffer[cnt] = buffer_read(( buffer_data_t * )&rx_uart_buffer_data);
    	}
    }

    //imx233_enable_interrupt(INT_SRC_UART_DEBUG, TRUE);

    return num_bytes_present;
}

static uint32_t imx233_port_ioctl( uint32_t index, uint32_t param, void * value )
{
	uint32_t * p_baud;

	if( param == DEVICE_SET_CONFIG_DATA )
	{
		p_baud = ( uint32_t * )value;
		uartdbg_init( *p_baud );
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////

static void imx233_port_init( uint32_t index )
{
	buffer_init( ( buffer_data_t * )&rx_uart_buffer_data, ( uint8_t * )rx_uart_buffer, RX_SERIAL_BUFFER_SIZE );
	buffer_init( ( buffer_data_t * )&tx_uart_buffer_data, ( uint8_t * )tx_uart_buffer, TX_SERIAL_BUFFER_SIZE );
	tx_busy = FALSE;
	intr_busy = FALSE;
}

static void imx233_port_close( uint32_t index )
{
}

static bool_t imx233_port_validate( uint32_t index )
{
    if( index == UART_DBG_DEV_INDEX ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

static const device_data_t imx233_uart_dbg_device =
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

DECLARE_DEVICE_SECTION( imx233_uart_dbg_device );
DECLARE_PROCESS_SECTION( uart_dbg_process );
