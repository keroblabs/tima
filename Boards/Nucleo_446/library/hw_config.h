#ifndef _HW_CONFIG_H_
#define _HW_CONFIG_H_

/////////////////////////////////////////////////////////////////////

#include "types.h"
#include "buffer.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

/////////////////////////////////////////////////////////////////////

#define GPIO_CONFIG_NONE			0
#define GPIO_CONFIG_INPUT		    ( 1 << 1 )
#define GPIO_CONFIG_OUTPUT	        ( 1 << 2 )
#define GPIO_CONFIG_PULLUP		    ( 1 << 3 )
#define GPIO_CONFIG_OPENDRAIN   	( 1 << 4 )
#define GPIO_CONFIG_PUSHPULL	    ( 1 << 5 )
#define GPIO_CONFIG_ALTERNATE       ( 1 << 6 )
#define GPIO_CONFIG_OUTPUT_EN       ( 1 << 7 )
#define GPIO_CONFIG_ANALOG          ( 1 << 8 )

#define HW_CONFIG_LIST(list)        (sizeof(list) / sizeof(list[0])), list

#define GPIO_OUTPUT_MODE_ON         ( GPIO_CONFIG_OUTPUT | GPIO_CONFIG_OUTPUT_EN | GPIO_CONFIG_PUSHPULL )

/////////////////////////////////////////////////////////////////////

enum
{
    HW_CONFIG_TYPE_UART,
    HW_CONFIG_TYPE_SPI,
    HW_CONFIG_TYPE_SPI_CS,
    HW_CONFIG_TYPE_GPIO,
    HW_CONFIG_TYPE_ADC,
};

/////////////////////////////////////////////////////////////////////

typedef void ( *clock_cb_config_t )( uint32_t perith, FunctionalState state );

typedef struct common_hw_config_t_
{
    uint8_t             type;
    clock_hw_config_t   clock;
	void               *TYPEx;

} common_hw_config_t;

typedef struct
{
    clock_cb_config_t    handler;
    uint32_t             data;

} clock_hw_config_t;

typedef struct
{
    uint8_t             type;
    clock_hw_config_t   clock;
	GPIO_TypeDef        *GPIOx;

	uint16_t            source;
	uint16_t            mode;
    uint16_t            id;
    uint8_t             alt_func;

} gpio_hw_config_t;

typedef struct
{
    uint8_t             type;
    clock_hw_config_t   clock;
    SPI_TypeDef         *SPIx;

    gpio_hw_config_t    gpio_mosi;
    gpio_hw_config_t    gpio_miso;
    gpio_hw_config_t    gpio_sclk;

    uint8_t             id;

    uint8_t             gpio_count;
    gpio_hw_config_t    *gpio_list;

} spi_hw_config_t;

typedef struct _buffer_ctrl_t_
{
    __IO buffer_data_t   buffer_data;
	__IO bool_t          is_busy;

} buffer_ctrl_t;

typedef struct _buffer_config_t
{
    __IO buffer_ctrl_t   *control;
	__IO uint8_t         *buffer;
	__IO uint32_t        buffer_size;

} buffer_hw_config_t;

typedef struct uart_hw_config_t_
{
    uint8_t             type;
    clock_hw_config_t   clock;
    USART_TypeDef       *UARTx;
    
    uint8_t             irq;
    uint8_t             tty_index;
    uint16_t            baudrate;

    buffer_hw_config_t  tx_buffer_config;
    buffer_hw_config_t  rx_buffer_config;

    gpio_hw_config_t    gpio_txd;
    gpio_hw_config_t    gpio_rxd;

} uart_hw_config_t;

typedef struct adc_hw_config_t_
{
    uint8_t             type;
    clock_hw_config_t   clock;
    ADC_TypeDef         *ADCx;

    uint32_t            resolution;

    uint8_t             gpio_count;
    gpio_hw_config_t    *gpio_list;

} adc_hw_config_t;

/////////////////////////////////////////////////////////////////////

#define BUFFER_DEFAULT(n, s_tx, s_rx)           { &uart_tx_buffer_ctrl_list[n], uart_tx_buffer_list[n], s_tx }, { &uart_rx_buffer_ctrl_list[n], uart_rx_buffer_list[n], s_rx }

#define UART1_DEFAULT(i, b)                     HW_CONFIG_TYPE_UART, { RCC_APB2PeriphClockCmd, RCC_APB2Periph_USART1 }, USART1, USART1_IRQn, i, b
#define UART2_DEFAULT(i, b)                     HW_CONFIG_TYPE_UART, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_USART2 }, USART2, USART2_IRQn, i, b
#define UART3_DEFAULT(i, b)                     HW_CONFIG_TYPE_UART, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_USART3 }, USART3, USART3_IRQn, i, b
#define UART4_DEFAULT(i, b)                     HW_CONFIG_TYPE_UART, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_UART4  },  UART4,  UART4_IRQn, i, b
#define UART5_DEFAULT(i, b)                     HW_CONFIG_TYPE_UART, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_UART5  },  UART5,  UART5_IRQn, i, b
#define UART6_DEFAULT(i, b)                     HW_CONFIG_TYPE_UART, { RCC_APB2PeriphClockCmd, RCC_APB2Periph_USART6 }, USART6, USART6_IRQn, i, b

#define SPI1_DEFAULT                            HW_CONFIG_TYPE_SPI, { RCC_APB2PeriphClockCmd, RCC_APB2Periph_SPI1 }, SPI1
#define SPI2_DEFAULT                            HW_CONFIG_TYPE_SPI, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_SPI2 }, SPI2
#define SPI3_DEFAULT                            HW_CONFIG_TYPE_SPI, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_SPI3 }, SPI3

#define GPIOA_CONFIG(src, mode, id, alt)        { HW_CONFIG_TYPE_GPIO, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOA }, GPIOA, GPIO_PinSource##src,  mode, id,  alt }
#define GPIOB_CONFIG(src, mode, id, alt)        { HW_CONFIG_TYPE_GPIO, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOB }, GPIOB, GPIO_PinSource##src,  mode, id,  alt }
#define GPIOC_CONFIG(src, mode, id, alt)        { HW_CONFIG_TYPE_GPIO, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOC }, GPIOC, GPIO_PinSource##src,  mode, id,  alt }
#define GPIOD_CONFIG(src, mode, id, alt)        { HW_CONFIG_TYPE_GPIO, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOD }, GPIOD, GPIO_PinSource##src,  mode, id,  alt }

#define ADC1_DEFAULT                            HW_CONFIG_TYPE_ADC, { RCC_APB2PeriphClockCmd, RCC_APB2Periph_ADC1 }, ADC1, ADC_Resolution_12b

#define SPI1_PA5_PA6_PA7                        SPI1_DEFAULT,                                               \
                                                GPIOA_CONFIG(7,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI1),   \
                                                GPIOA_CONFIG(6,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI1),   \
                                                GPIOA_CONFIG(4,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI1)

#define SPI1_PB3_PB4_PB5                        SPI1_DEFAULT,                                               \
                                                GPIOB_CONFIG(5,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI1),   \
                                                GPIOB_CONFIG(4,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI1),   \
                                                GPIOB_CONFIG(3,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI1)

#define SPI2_PB13_PB14_PB15                     SPI2_DEFAULT,                                               \
                                                GPIOB_CONFIG(15,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI2),  \
                                                GPIOB_CONFIG(14,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI2),  \
                                                GPIOB_CONFIG(13,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI2)

#define SPI2_PC2_PC3_PC7                        SPI2_DEFAULT,                                               \
                                                GPIOC_CONFIG(3,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI2),   \
                                                GPIOC_CONFIG(2,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI2),   \
                                                GPIOC_CONFIG(7,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI2)

#define SPI3_PC10_PC11_PC12                     SPI3_DEFAULT,                                               \
                                                GPIOC_CONFIG(12,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI3),  \
                                                GPIOC_CONFIG(11,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI3),  \
                                                GPIOC_CONFIG(10,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_SPI3)


#define UART1_PA9_PA10(i, b, n, s_tx, s_rx)     UART1_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOA_CONFIG(9,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART1), \
                                                GPIOA_CONFIG(10, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART1)

#define UART1_PB6_PB7(i, b, n, s_tx, s_rx)      UART1_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOB_CONFIG(6, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART1),  \
                                                GPIOB_CONFIG(7, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART1)

#define UART2_PA2_PA3(i, b, n, s_tx, s_rx)      UART2_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOA_CONFIG(2, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART2),  \
                                                GPIOA_CONFIG(3, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART2)

#define UART2_PD5_PD6(i, b, n, s_tx, s_rx)      UART2_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOC_CONFIG(5, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART2),  \
                                                GPIOC_CONFIG(6, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART2)

#define UART3_PB10_PB11(i, b, n, s_tx, s_rx)    UART3_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOB_CONFIG(10, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART3), \
                                                GPIOB_CONFIG(11, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART3)

#define UART3_PD8_PD9(i, b, n, s_tx, s_rx)      UART3_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOC_CONFIG(8, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART3),  \
                                                GPIOC_CONFIG(9, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART3)

#define UART3_PC10_PC11(i, b, n, s_tx, s_rx)    UART3_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOC_CONFIG(10, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART3), \
                                                GPIOC_CONFIG(11, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART3)

#define UART4_PA0_PA1(i, b, n, s_tx, s_rx)      UART4_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOA_CONFIG(0, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_UART4),   \
                                                GPIOA_CONFIG(1, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_UART4)

#define UART4_PC10_PC11(i, b, n, s_tx, s_rx)    UART4_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOC_CONFIG(10, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_UART4),  \
                                                GPIOC_CONFIG(11, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_UART4)

#define UART5_PC12_PD2(i, b, n, s_tx, s_rx)     UART5_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOC_CONFIG(12, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_UART5),  \
                                                GPIOD_CONFIG(2,  GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_UART5)

#define UART6_PC6_PC7(i, b, n, s_tx, s_rx)      UART6_DEFAULT(i, b), BUFFER_DEFAULT(n, s_tx, s_rx),         \
                                                GPIOC_CONFIG(6, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART6),  \
                                                GPIOC_CONFIG(7, GPIO_CONFIG_ALTERNATE, 0, GPIO_AF_USART6)

/////////////////////////////////////////////////////////////////////

void clock_hw_config_enable( const clock_hw_config_t * clock_config );
void clock_hw_config_disable( const clock_hw_config_t * clock_config );

void irq_hw_config( uint8_t irq );

void gpio_hw_config_init( const gpio_hw_config_t * gpio_config );
void uart_hw_config_init( const uart_hw_config_t * uart_hw_config );
void spi_hw_config_init( const spi_hw_config_t *spi_hw_config );
void adc_hw_config_init( const adc_hw_config_t * adc_hw_config );

/////////////////////////////////////////////////////////////////////

#endif // _HW_CONFIG_H_


