#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "command_hal.h"
#include "hw_config.h"
#include "system.h"

/////////////////////////////////////////////////////////////////////

#define UART_HW_CONFIG_COUNT            4

#define UART1_TTY_INDEX					5
#define UART2_TTY_INDEX					9
#define UART3_TTY_INDEX					3
#define UART4_TTY_INDEX				    8
#define UART5_TTY_INDEX					10

#define SPI1_DEV_INDEX                  1

#define TX_SERIAL_BUFFER_SIZE           256
#define RX_SERIAL_BUFFER_SIZE           1024

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

static buffer_ctrl_t uart_tx_buffer_ctrl_list[ UART_HW_CONFIG_COUNT ];
static buffer_ctrl_t uart_rx_buffer_ctrl_list[ UART_HW_CONFIG_COUNT ];
static uint8_t uart_tx_buffer_list[ UART_HW_CONFIG_COUNT ][ TX_SERIAL_BUFFER_SIZE ];
static uint8_t uart_rx_buffer_list[ UART_HW_CONFIG_COUNT ][ RX_SERIAL_BUFFER_SIZE ];

/////////////////////////////////////////////////////////////////////

static const gpio_hw_config_t spi_gpio_cs_list[] = 
{
    { GPIOB_CONFIG(8, GPIO_OUTPUT_MODE_ON, 0, GPIO_Mode_OUT ) },
    { GPIOB_CONFIG(9, GPIO_OUTPUT_MODE_ON, 1, GPIO_Mode_OUT ) },
    { GPIOC_CONFIG(5, GPIO_OUTPUT_MODE_ON, 2, GPIO_Mode_OUT ) },
};

static const gpio_hw_config_t adc_gpio_list[] = 
{
    { GPIOB_CONFIG(0, GPIO_CONFIG_ALTERNATE, 8, GPIO_Mode_AN ) },
    { GPIOB_CONFIG(1, GPIO_CONFIG_ALTERNATE, 9, GPIO_Mode_AN ) },
};

static const void * hw_config_list[] =
{
    { GPIOC_CONFIG(0, GPIO_OUTPUT_MODE_ON, 1, GPIO_Mode_OUT ) },
    { GPIOC_CONFIG(1, GPIO_OUTPUT_MODE_ON, 2, GPIO_Mode_OUT ) },
    { GPIOC_CONFIG(2, GPIO_OUTPUT_MODE_ON, 3, GPIO_Mode_OUT ) },
    { GPIOC_CONFIG(3, GPIO_OUTPUT_MODE_ON, 4, GPIO_Mode_OUT ) },
    { GPIOB_CONFIG(7, GPIO_OUTPUT_MODE_ON, 5, GPIO_Mode_OUT ) },

    { UART1_PA9_PA10( UART1_TTY_INDEX, 115200, 0, TX_SERIAL_BUFFER_SIZE, RX_SERIAL_BUFFER_SIZE) },
    { UART2_PA2_PA3(  UART2_TTY_INDEX, 115200, 1, TX_SERIAL_BUFFER_SIZE, RX_SERIAL_BUFFER_SIZE) },
    { UART3_PB10_PB11(UART3_TTY_INDEX, 115200, 2, TX_SERIAL_BUFFER_SIZE, RX_SERIAL_BUFFER_SIZE) },
    { UART4_PC10_PC11(UART4_TTY_INDEX, 115200, 3, TX_SERIAL_BUFFER_SIZE, RX_SERIAL_BUFFER_SIZE) },

    { SPI1_PA5_PA6_PA7, SPI1_DEV_INDEX, HW_CONFIG_LIST( spi_gpio_cs_list ) },

    { ADC1_DEFAULT, HW_CONFIG_LIST( adc_gpio_list ) },

    NULL
};

/////////////////////////////////////////////////////////////////////

DECLARE_HW_CONFIG_SECTION(hw_config_list);
