#include "system.h"
#include "buffer.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

////////////////////////////////////////////////////////////////////

#define RX_SERIAL_BUFFER_SIZE          1024
#define TX_SERIAL_BUFFER_SIZE          1024

#define SERIAL_TOTAL					4

#define UART1_INDEX						0
#define UART2_INDEX						1
#define UART3_INDEX						2
#define UART5_INDEX						3
// #define UART4_INDEX					2

#define UART1_TTY_INDEX					10
#define UART2_TTY_INDEX					3
#define UART3_TTY_INDEX					8
//#define UART4_TTY_INDEX				9
#define UART5_TTY_INDEX					5

////////////////////////////////////////////////////////////////////

typedef struct _serial_buffer_data_t
{
	__IO uint8_t         rx_uart_buffer[ RX_SERIAL_BUFFER_SIZE ];
	__IO buffer_data_t   rx_uart_buffer_data;
	__IO uint8_t         tx_uart_buffer[ TX_SERIAL_BUFFER_SIZE ];
	__IO buffer_data_t   tx_uart_buffer_data;
	__IO bool_t          tx_busy;
	__IO USART_TypeDef*  USARTx;
	void ( *init )( uint32_t index );

} serial_buffer_data_t;

static serial_buffer_data_t serial_buffer_data[SERIAL_TOTAL];

static GPIO_InitTypeDef GPIO_InitStructure;
static USART_InitTypeDef USART_InitStructure;
static NVIC_InitTypeDef NVIC_InitStructure;

////////////////////////////////////////////////////////////////////

static void init_uart5( uint32_t baud )
{
	/* UART5 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

	/* GPIOC and GPIOD clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/* Connect USART pins to AF */
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_UART5);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_UART5);

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
	USART_ITConfig(UART5, USART_IT_TC, ENABLE);

	/* USARTx configuration ------------------------------------------------------*/
	/* USARTx configured as follow:
	- BaudRate = 115200 baud
	- Word Length = 8 Bits
	- One Stop Bit
	- No parity
	- Hardware flow control disabled (RTS and CTS signals)
	- Receive and transmit enabled
	*/
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(UART5, &USART_InitStructure);
	USART_Cmd(UART5, ENABLE);
}

static void init_uart3( uint32_t baud )
{
	/* USART2 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	/* Connect USART pins to AF */
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART3, USART_IT_TC, ENABLE);

	/* USARTx configuration ------------------------------------------------------*/
	/* USARTx configured as follow:
	- BaudRate = 115200 baud
	- Word Length = 8 Bits
	- One Stop Bit
	- No parity
	- Hardware flow control disabled (RTS and CTS signals)
	- Receive and transmit enabled
	*/
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure);
	USART_Cmd(USART3, ENABLE);
}


static void init_uart2( uint32_t baud )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* USART1 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Connect USART pins to AF */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART2, USART_IT_TC, ENABLE);

	/* USARTx configuration ------------------------------------------------------*/
	/* USARTx configured as follow:
	- BaudRate = 115200 baud
	- Word Length = 8 Bits
	- One Stop Bit
	- No parity
	- Hardware flow control disabled (RTS and CTS signals)
	- Receive and transmit enabled
	*/
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART2, &USART_InitStructure);
	USART_Cmd(USART2, ENABLE);
}

static void init_uart1( uint32_t baud )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* USART1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Connect USART pins to AF */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART1, USART_IT_TC, ENABLE);

	/* USARTx configuration ------------------------------------------------------*/
	/* USARTx configured as follow:
	- BaudRate = 115200 baud
	- Word Length = 8 Bits
	- One Stop Bit
	- No parity
	- Hardware flow control disabled (RTS and CTS signals)
	- Receive and transmit enabled
	*/
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);
}

void USART_IRQHandler_Base(serial_buffer_data_t * p_buffer)
{
	 uint16_t RxByte = 0x00;

	 if (USART_GetITStatus((USART_TypeDef*)(p_buffer->USARTx), USART_IT_TC) == SET)
	 {
		 if (USART_GetFlagStatus((USART_TypeDef*)(p_buffer->USARTx), USART_FLAG_TC))
		 {
			 if( buffer_is_empty( ( buffer_data_t * )( &p_buffer->tx_uart_buffer_data ) ) == FALSE )
			 {
				 RxByte = buffer_read( ( buffer_data_t * )&p_buffer->tx_uart_buffer_data );
				 USART_SendData((USART_TypeDef*)(p_buffer->USARTx), RxByte);
			 }
			 else
			 {
				 p_buffer->tx_busy = FALSE;
			 }
		 }

		 USART_ClearITPendingBit((USART_TypeDef*)(p_buffer->USARTx), USART_IT_TC);
	 }

	 if (USART_GetITStatus((USART_TypeDef*)(p_buffer->USARTx), USART_IT_RXNE) == SET)
	 {
		 if (USART_GetFlagStatus((USART_TypeDef*)(p_buffer->USARTx), USART_FLAG_RXNE))
		 {
			 RxByte = USART_ReceiveData((USART_TypeDef*)(p_buffer->USARTx));
			 buffer_write( ( buffer_data_t * )( &p_buffer->rx_uart_buffer_data ), ( uint8_t )RxByte );
		 }

		 USART_ClearITPendingBit((USART_TypeDef*)(p_buffer->USARTx), USART_IT_RXNE);
	 }
}


void USART1_IRQHandler(void)
{
	USART_IRQHandler_Base( ( serial_buffer_data_t * )&serial_buffer_data[UART1_INDEX] );
}

void USART2_IRQHandler(void)
{
	USART_IRQHandler_Base( ( serial_buffer_data_t * )&serial_buffer_data[UART2_INDEX] );
}

void USART3_IRQHandler(void)
{
	USART_IRQHandler_Base( ( serial_buffer_data_t * )&serial_buffer_data[UART3_INDEX] );
}

void UART5_IRQHandler(void)
{
	USART_IRQHandler_Base( ( serial_buffer_data_t * )&serial_buffer_data[UART5_INDEX] );
}

static void uart_drv_init( uint32_t index )
{
	serial_buffer_data_t * p_buffer;

	switch( index )
	{
		case UART1_TTY_INDEX:
			p_buffer = &serial_buffer_data[0];
			p_buffer->init = init_uart1;
			p_buffer->USARTx = USART1;
			break;

		case UART2_TTY_INDEX:
			p_buffer = &serial_buffer_data[1];
			p_buffer->init = init_uart2;
			p_buffer->USARTx = USART2;
			break;

		case UART3_TTY_INDEX:
			p_buffer = &serial_buffer_data[2];
			p_buffer->init = init_uart3;
			p_buffer->USARTx = USART3;
			break;

		case UART5_TTY_INDEX:
			p_buffer = &serial_buffer_data[3];
			p_buffer->init = init_uart5;
			p_buffer->USARTx = UART5;
			break;

		default: return;
	}

	buffer_init( ( buffer_data_t * )&p_buffer->rx_uart_buffer_data, ( uint8_t * )p_buffer->rx_uart_buffer, RX_SERIAL_BUFFER_SIZE );
	buffer_init( ( buffer_data_t * )&p_buffer->tx_uart_buffer_data, ( uint8_t * )p_buffer->tx_uart_buffer, TX_SERIAL_BUFFER_SIZE );
	p_buffer->tx_busy = FALSE;
}

static uint32_t uart_drv_ioctl( uint32_t index, uint32_t param, void * value )
{
	serial_buffer_data_t * p_buffer;
	uint32_t * p_baud;

	switch( index )
	{
		case UART1_TTY_INDEX:
			p_buffer = &serial_buffer_data[0];
			break;

		case UART2_TTY_INDEX:
			p_buffer = &serial_buffer_data[1];
			break;

		case UART3_TTY_INDEX:
			p_buffer = &serial_buffer_data[2];
			break;

		case UART5_TTY_INDEX:
			p_buffer = &serial_buffer_data[3];
			break;

		default: return 0;
	}

	if( param == DEVICE_SET_CONFIG_DATA )
	{
		p_baud = ( uint32_t * )value;
		p_buffer->init( *p_baud );
	}

	return 0;
}

static uint32_t uart_read_buffer( uint32_t index, uint8_t * buffer, uint32_t size )
{
    uint32_t num_bytes_present = 0;
	serial_buffer_data_t * p_buffer;
	uint32_t cnt;

	switch( index )
	{
		case UART1_TTY_INDEX:
			p_buffer = &serial_buffer_data[0];
			break;

		case UART2_TTY_INDEX:
			p_buffer = &serial_buffer_data[1];
			break;

		case UART3_TTY_INDEX:
			p_buffer = &serial_buffer_data[2];
			break;

		case UART5_TTY_INDEX:
			p_buffer = &serial_buffer_data[3];
			break;

		default: return 0;
	}

    num_bytes_present = buffer_get_size( ( buffer_data_t * )&p_buffer->rx_uart_buffer_data );

    if( ( num_bytes_present > 0 ) && ( buffer != NULL ) )
    {
    	for( cnt = 0; cnt < size; cnt++ )
    	{
    		if( cnt >= num_bytes_present ) break;
    		buffer[cnt] = buffer_read(( buffer_data_t * )&p_buffer->rx_uart_buffer_data);
    	}
    }

    return num_bytes_present;
}

static uint32_t uart_write_buffer( uint32_t index, uint8_t * buffer, uint32_t size )
{
    uint32_t num_bytes_present = 0;
	serial_buffer_data_t * p_buffer;
	uint32_t cnt;
	uint16_t RxByte = 0x00;

	switch( index )
	{
		case UART1_TTY_INDEX:
			p_buffer = &serial_buffer_data[0];
			break;

		case UART2_TTY_INDEX:
			p_buffer = &serial_buffer_data[1];
			break;

		case UART3_TTY_INDEX:
			p_buffer = &serial_buffer_data[2];
			break;

		case UART5_TTY_INDEX:
			p_buffer = &serial_buffer_data[3];
			break;

		default: return 0;
	}

	buffer_write_buffer( ( buffer_data_t * )&p_buffer->tx_uart_buffer_data, buffer, size );
	if( p_buffer->tx_busy == FALSE )
	{
		 RxByte = buffer_read( ( buffer_data_t * )&p_buffer->tx_uart_buffer_data );
		 USART_SendData((USART_TypeDef*)p_buffer->USARTx, RxByte);
		 p_buffer->tx_busy = TRUE;
	}

	return size;
}

///////////////////////////////////////////////////////////////////////////

static void uart_drv_close( uint32_t index )
{
}

static bool_t uart_drv_validate( uint32_t index )
{
    if( index == UART1_TTY_INDEX ) return TRUE;
    if( index == UART2_TTY_INDEX ) return TRUE;
    if( index == UART3_TTY_INDEX ) return TRUE;
    if( index == UART5_TTY_INDEX ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const device_data_t console_tty_device =
{
    "tty*",
    &_in_use,

	uart_drv_init,
	uart_drv_close,
    NULL,
    uart_drv_validate,
    NULL,
	uart_drv_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

	uart_read_buffer,
	uart_write_buffer,

};

DECLARE_DEVICE_SECTION( console_tty_device );


