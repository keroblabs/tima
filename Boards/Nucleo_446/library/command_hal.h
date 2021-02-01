#ifndef BOARDS_NUCLEO_446_LIBRARY_COMMAND_HAL_H_
#define BOARDS_NUCLEO_446_LIBRARY_COMMAND_HAL_H_

////////////////////////////////////////////////////////////////////

#include "types.h"

////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////

typedef void ( *clock_cmd_t )( uint32_t perith, FunctionalState state );

typedef struct _cmd_data_t
{
    clock_cmd_t          handler;
    uint32_t             data;
} cmd_data_t;

typedef struct _serial_pin_data_t
{
    uint8_t          pin_source;
    uint8_t          gpio_af;
    GPIO_TypeDef    *gpio;
    cmd_data_t       gpio_cmd;

} pin_data_t;

typedef struct _uart_command_t
{
    USART_TypeDef       *uart;
    uint8_t              irq;
    cmd_data_t           uart_cmd;

} uart_command_t;

////////////////////////////////////////////////////////////////////

static INLINE void command_clock_enable( const cmd_data_t * command )
{
    command->handler( command->data, ENABLE );
}

static INLINE void command_clock_disable( const cmd_data_t * command )
{
    command->handler( command->data, ENABLE );
}

static INLINE void command_setup_gpio( const pin_data_t * pin )
{
    GPIO_InitTypeDef GPIO_InitStructure;

    command_clock_enable( &pin->gpio_cmd );

    GPIO_PinAFConfig( pin->gpio, pin->pin_source, pin->gpio_af );

    GPIO_InitStructure.GPIO_Pin =  ( uint16_t )( 0x01 << pin->pin_source );
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
    GPIO_Init(pin->gpio, &GPIO_InitStructure);
}

static INLINE void command_setup_irq( uint8_t irq )
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

////////////////////////////////////////////////////////////////////

#endif /* BOARDS_NUCLEO_446_LIBRARY_COMMAND_HAL_H_ */
