#include "system.h"
#include "types.h"

#include "regsclkctrl.h"
#include "pinctrl-imx233.h"
#include "system_imx233.h"

#include "debug.h"

////////////////////////////////////////////////////////////////////////////////////////////

#define GPIO_COUNTER_BANK		2
#define GPIO_COUNTER_PIO		28
#define GPIO_COUNTER			GPIO_COUNTER_BANK, GPIO_COUNTER_PIO

////////////////////////////////////////////////////////////////////////////////////////////

static __IO uint32_t eint11_counter;

static bool_t is_init = FALSE;

////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////

static void counter_gpio_irq( int bank, int pin )
{
	if( ( bank == GPIO_COUNTER_BANK ) && ( pin == GPIO_COUNTER_PIO ) )
	{
		eint11_counter++;
	}
}

static void hw_counter_init( void )
{
	eint11_counter = 0;

	// setup pio as input
	imx233_set_pin_function( GPIO_COUNTER, PINCTRL_FUNCTION_GPIO );
	imx233_enable_gpio_output( GPIO_COUNTER, FALSE );
	imx233_enable_pin_pullup( GPIO_COUNTER, TRUE );

	imx233_setup_pin_irq( GPIO_COUNTER, TRUE, PINCTRL_IRQ_MODE_EDGE, PINCTRL_IRQ_MODE_LOW, counter_gpio_irq );
}

////////////////////////////////////////////////////////////////////////

static void counter_driver_global_init( void )
{
	is_init = FALSE;
}

static void counter_drv_init( uint32_t index )
{
	hw_counter_init();
}

static void counter_drv_close( uint32_t index )
{
}

static uint32_t counter_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t * p_value;

	if( size != sizeof( uint32_t ) ) return size;

	if( index == 11 )
	{
		DI();
		p_value = ( uint32_t * )buffer;
		eint11_counter = *p_value;
		EI();
	}

	return size;
}

static uint32_t counter_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t * p_value;

	uint32_t _arm926_get_mode( void );

	if( size != sizeof( uint32_t ) ) return size;

	if( index == 11 )
	{
		DI();
		p_value = ( uint32_t * )buffer;
		*p_value = eint11_counter;
		EI();
	}

	return size;
}

uint32_t counter_io_ctrl( uint32_t index, uint32_t param, void * value )
{
	uint64_t * p_value = ( uint64_t * )value;

	if( param == DEVICE_GET_RUNTIME_VALUE )
	{
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////

static bool_t counter_validate( uint32_t index )
{
    if( index == 11 ) return TRUE;
    return FALSE;
}

static bool_t _in_use;

const static device_data_t counter_device =
{
    "counter*",
    &_in_use,

    counter_drv_init,
    counter_drv_close,
    NULL,
	counter_validate,
    NULL,
    counter_io_ctrl,
    NULL,

    DEV_MODE_CHANNEL,

    counter_drv_input,
    counter_drv_output,

};

DECLARE_DEVICE_SECTION( counter_device );
DECLARE_CRITICAL_SECTION( counter_driver_global_init );


///////////////////////////////////////////////////////////////////////////

static void gpio_drv_init( uint32_t index )
{
	// setup pio as input
	imx233_set_pin_function( GPIO_COUNTER, PINCTRL_FUNCTION_GPIO );
	imx233_enable_gpio_output( GPIO_COUNTER, FALSE );
	imx233_enable_pin_pullup( GPIO_COUNTER, TRUE );
}

static void gpio_drv_close( uint32_t index )
{
}

static uint32_t gpio_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
	return 0;
}

static uint32_t gpio_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	if( index != 11 ) return 0;

	*buffer = imx233_get_gpio_input( GPIO_COUNTER ) ? TRUE : FALSE;

	return size;
}

///////////////////////////////////////////////////////////////////////////

static bool_t gpio_validate( uint32_t index )
{
    if( index == 11 ) return TRUE;
    return FALSE;
}

static bool_t _in_use;

const static device_data_t gpio_device =
{
    "gpio*",
    &_in_use,

    gpio_drv_init,
    gpio_drv_close,
    NULL,
    gpio_validate,
    NULL,
    NULL,
    NULL,

    DEV_MODE_CHANNEL,

    gpio_drv_input,
    gpio_drv_output,

};

DECLARE_DEVICE_SECTION( gpio_device );
