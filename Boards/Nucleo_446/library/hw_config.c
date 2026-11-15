#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "command_hal.h"
#include "hw_config.h"
#include "system.h"

/////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

void clock_hw_config_enable( const clock_hw_config_t * clock_config )
{
    clock_config->handler( clock_config->data, ENABLE );
}

void clock_hw_config_disable( const clock_hw_config_t * clock_config )
{
    clock_config->handler( clock_config->data, DISABLE );
}

void irq_hw_config( uint8_t irq )
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void gpio_hw_config_init( const gpio_hw_config_t * gpio_config )
{
    GPIO_InitTypeDef GPIO_InitStructure;

    clock_hw_config_enable( &gpio_config->clock );

    GPIO_InitStructure.GPIO_Pin = ( uint16_t )( 0x01 << gpio_config->source );

    if( gpio_config->mode & GPIO_CONFIG_INPUT )
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

        if( gpio_config->mode & GPIO_CONFIG_PULLUP )
        {
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        }
    }
    else if( gpio_config->mode & GPIO_CONFIG_OUTPUT )
    {
        GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType   = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;

        if( gpio_config->mode & GPIO_CONFIG_PULLUP )
        {
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        }

        if( gpio_config->mode & GPIO_CONFIG_PUSHPULL )
        {
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        }
    }
    else if( gpio_config->mode & GPIO_CONFIG_ALTERNATE )
    {
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
        GPIO_PinAFConfig( gpio_config->GPIOx, gpio_config->source, gpio_config->alt_func );
    }
    else if( gpio_config->mode & GPIO_CONFIG_ANALOG )
    {
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AN;
    }

    GPIO_InitStructure.GPIO_Speed   = GPIO_High_Speed;
    GPIO_Init(gpio_config->GPIOx, &GPIO_InitStructure);

    if( gpio_config->mode & GPIO_CONFIG_OUTPUT )
    {
        uint16_t pin_mask = ( uint16_t )( 0x01 << gpio_config->source );

        if( gpio_config->mode & GPIO_CONFIG_OUTPUT_EN )
        {
            GPIO_SetBits( gpio_config->GPIOx, pin_mask );
        }
        else
        {
            GPIO_ResetBits( gpio_config->GPIOx, pin_mask );
        }        
    }
}

void uart_hw_config_init( const uart_hw_config_t * uart_hw_config )
{
    USART_InitTypeDef USART_InitStructure;

    /* Enable UART clock */
    clock_hw_config_enable( &uart_hw_config->clock );

    /* Setup USART pins */
    gpio_hw_config_init( &uart_hw_config->gpio_txd );
    gpio_hw_config_init( &uart_hw_config->gpio_rxd );

    /* setup IRQ */
    irq_hw_config( uart_hw_config->irq );

    USART_ITConfig(uart_hw_config->UARTx, USART_IT_ORE_RX, ENABLE);
    USART_ITConfig(uart_hw_config->UARTx, USART_IT_RXNE, ENABLE);
    USART_ITConfig(uart_hw_config->UARTx, USART_IT_TC, ENABLE);

    /* USART configuration */
    USART_InitStructure.USART_BaudRate              = uart_hw_config->baudrate;
    USART_InitStructure.USART_WordLength            = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits              = USART_StopBits_1;
    USART_InitStructure.USART_Parity                = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl   = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                  = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(uart_hw_config->UARTx, &USART_InitStructure);

    USART_Cmd(uart_hw_config->UARTx, ENABLE);

    /* UART buffer configuration */
    buffer_init( ( buffer_data_t * )&uart_hw_config->rx_buffer_config.control->buffer_data, 
                    ( uint8_t * )&uart_hw_config->rx_buffer_config.buffer, 
                    uart_hw_config->rx_buffer_config.buffer_size );

    buffer_init( ( buffer_data_t * )&uart_hw_config->tx_buffer_config.control->buffer_data, 
                    ( uint8_t * )&uart_hw_config->tx_buffer_config.buffer, 
                    uart_hw_config->tx_buffer_config.buffer_size );

    uart_hw_config->tx_buffer_config.control->is_busy = FALSE;
}

void spi_hw_config_init( const spi_hw_config_t *spi_hw_config )
{
	  SPI_InitTypeDef  SPI_InitStructure;
    uint8_t i;

    /* Enable SPI clock */
    clock_hw_config_enable( &spi_hw_config->clock );

    /* Setup SPI pins */
    gpio_hw_config_init( &spi_hw_config->gpio_mosi );
    gpio_hw_config_init( &spi_hw_config->gpio_miso );
    gpio_hw_config_init( &spi_hw_config->gpio_sclk );

    /* Setup SPI CS pins */
    for( i = 0; i < spi_hw_config->gpio_count; i++ )
    {
        gpio_hw_config_init( &spi_hw_config->gpio_list[ i ] );
    }

    /* SPI configuration */
    SPI_StructInit( &SPI_InitStructure );
    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft; //  | SPI_NSSInternalSoft_Set;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;

    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(spi_hw_config->SPIx, &SPI_InitStructure);
    SPI_Cmd(spi_hw_config->SPIx, ENABLE);
}

void adc_hw_config_init( const adc_hw_config_t * adc_hw_config )
{
    ADC_InitTypeDef   ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    uint8_t i;

    /* Enable ADC clock */
    clock_hw_config_enable( &adc_hw_config->clock );

    /* Setup ADC pins */
    for( i = 0; i < adc_hw_config->gpio_count; i++ )
    {
        gpio_hw_config_init( &adc_hw_config->gpio_list[ i ] );
    }

    /* ADC Common configuration */
    ADC_CommonStructInit(&ADC_CommonInitStructure);
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC Configuration */
    ADC_InitStructure.ADC_Resolution 			    = adc_hw_config->resolution;
    ADC_InitStructure.ADC_ScanConvMode 			    = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode 	    = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge 	    = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign 			    = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion 		    = 1; //ADC_DMA_SIZE;
    ADC_Init(adc_hw_config->ADCx, &ADC_InitStructure);

    ADC_ContinuousModeCmd(adc_hw_config->ADCx, DISABLE);

  	/* enable temperature sensor */
    ADC_TempSensorVrefintCmd(ENABLE); //%%
}

/////////////////////////////////////////////////////////////////////