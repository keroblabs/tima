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

#define GPIO_CSFLASH_BANK		0
#define GPIO_CSFLASH_PIO		17
#define GPIO_CSFLASH			GPIO_CSFLASH_BANK, GPIO_CSFLASH_PIO

#define GPIO_SPEED_BANK			0
#define GPIO_SPEED_PIO			16
#define GPIO_SPEED				GPIO_SPEED_BANK, GPIO_SPEED_PIO

#define GPIO_HANDBRK0_BANK		1
#define GPIO_HANDBRK0_PIO		20
#define GPIO_HANDBRK0			GPIO_HANDBRK0_BANK, GPIO_HANDBRK0_PIO

#define GPIO_HANDBRK1_BANK		1
#define GPIO_HANDBRK1_PIO		19
#define GPIO_HANDBRK1			GPIO_HANDBRK1_BANK, GPIO_HANDBRK1_PIO

#define GPIO_OIL_BANK			1
#define GPIO_OIL_PIO			21
#define GPIO_OIL				GPIO_OIL_BANK, GPIO_OIL_PIO

#define GPIO_FULLBEAM_BANK		1
#define GPIO_FULLBEAM_PIO		28
#define GPIO_FULLBEAM			GPIO_FULLBEAM_BANK, GPIO_FULLBEAM_PIO

#define GPIO_BATTERY_BANK		0
#define GPIO_BATTERY_PIO		25
#define GPIO_BATTERY			GPIO_BATTERY_BANK, GPIO_BATTERY_PIO

#define GPIO_BRAKE_BANK			0
#define GPIO_BRAKE_PIO			23
#define GPIO_BRAKE				GPIO_BRAKE_BANK, GPIO_BRAKE_PIO

#define GPIO_RPM_BANK			2
#define GPIO_RPM_PIO			27
#define GPIO_RPM				GPIO_RPM_BANK, GPIO_RPM_PIO

#define GPIO_CSTPI_BANK			2
#define GPIO_CSTPI_PIO			28
#define GPIO_CSTPI				GPIO_CSTPI_BANK, GPIO_CSTPI_PIO

#define GPIO_INDLEFT_BANK		1
#define GPIO_INDLEFT_PIO		0
#define GPIO_INDLEFT			GPIO_INDLEFT_BANK, GPIO_INDLEFT_PIO

#define GPIO_INDRIGHT_BANK		1
#define GPIO_INDRIGHT_PIO		3
#define GPIO_INDRIGHT			GPIO_INDRIGHT_BANK, GPIO_INDRIGHT_PIO

#define GPIO_CSGPIO_BANK		1
#define GPIO_CSGPIO_PIO			18
#define GPIO_CSGPIO				GPIO_CSGPIO_BANK, GPIO_CSGPIO_PIO

#define GPIO_MODE_INPUT			0x01
#define GPIO_MODE_OUTPUT		0x02
#define GPIO_MODE_COUNTER		0x04
#define GPIO_OUTPUT_ON			0x08
#define GPIO_OUTPUT_OFF			0x00

#define GPIO_INDEX_INTERNAL		100

#define COUNTER_GPIO_RPM		11
#define COUNTER_GPIO_SPEED		19

////////////////////////////////////////////////////////////////////////////////////////////

typedef struct gpio_data_t_
{
	uint32_t index;
	uint32_t gpio_bank;
	uint32_t gpio_pio;
	uint32_t mode;

} gpio_data_t;

////////////////////////////////////////////////////////////////////////////////////////////

static const gpio_data_t gpio_data[] =
{
	{ 0, GPIO_INDLEFT, 	0 },
	{ 1, GPIO_INDLEFT, 	0 },
	{ 2, GPIO_INDLEFT, 	0 },
	{ 3, GPIO_INDLEFT, 	0 },
	{ 4, GPIO_INDLEFT, 	0 },
	{ 5, GPIO_INDLEFT, 	0 },
	{ 6, GPIO_INDLEFT, 	0 },
	{ 7, GPIO_INDLEFT, 	0 },
	{ 8, GPIO_INDLEFT, 	0 },
	{ 9, GPIO_INDLEFT, 	0 },

	{ 10, GPIO_INDLEFT, 	GPIO_MODE_INPUT },
	{ 11, GPIO_INDRIGHT, 	GPIO_MODE_INPUT },

	{ 12, GPIO_BATTERY, 	GPIO_MODE_INPUT },
	{ 13, GPIO_BRAKE, 		GPIO_MODE_INPUT },

	{ 14, GPIO_FULLBEAM, 	GPIO_MODE_INPUT },
	{ 15, GPIO_OIL, 		GPIO_MODE_INPUT },
	{ 16, GPIO_HANDBRK1, 	GPIO_MODE_INPUT },
	{ 17, GPIO_HANDBRK0, 	GPIO_MODE_INPUT },

	// not accessible
	{ GPIO_INDEX_INTERNAL + 1, GPIO_CSFLASH,	GPIO_MODE_OUTPUT | GPIO_OUTPUT_ON },
	{ GPIO_INDEX_INTERNAL + 2, GPIO_CSTPI,		GPIO_MODE_OUTPUT | GPIO_OUTPUT_ON },

	{ GPIO_INDEX_INTERNAL + 3, GPIO_SPEED,		GPIO_MODE_INPUT | GPIO_MODE_COUNTER },
	{ GPIO_INDEX_INTERNAL + 4, GPIO_RPM,		GPIO_MODE_INPUT | GPIO_MODE_COUNTER },

	{ GPIO_INDEX_INTERNAL + 5, GPIO_CSGPIO,		GPIO_MODE_OUTPUT | GPIO_OUTPUT_ON },

	{ NEG_U32, 0, 0 }

};

////////////////////////////////////////////////////////////////////////////////////////////

static __IO uint32_t rpm_counter;
static __IO uint32_t speed_counter;

///////////////////////////////////////////////////////////////////////////

static void counter_gpio_irq( int bank, int pin )
{
	if( ( bank == GPIO_RPM_BANK ) && ( pin == GPIO_RPM_PIO ) )
	{
		rpm_counter++;
	}

	if( ( bank == GPIO_SPEED_BANK ) && ( pin == GPIO_SPEED_PIO ) )
	{
		speed_counter++;
	}
}

static void counter_gpio_global_init( void )
{
	int i;

	speed_counter = 0;
	rpm_counter = 0;

	for( i = 0; i < 255; i++ )
	{
		if( gpio_data[i].index == NEG_U32 ) break;
		if( gpio_data[i].mode == 0 ) continue;

		if( gpio_data[i].mode & GPIO_MODE_INPUT )
		{
			// setup pio as input
			imx233_set_pin_function( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, PINCTRL_FUNCTION_GPIO );
			imx233_enable_gpio_output( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, FALSE );
			imx233_enable_pin_pullup( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, TRUE );

			if( gpio_data[i].mode & GPIO_MODE_COUNTER )
			{
				imx233_setup_pin_irq( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, TRUE, PINCTRL_IRQ_MODE_EDGE, PINCTRL_IRQ_MODE_LOW, counter_gpio_irq );
			}
		}
		else if( gpio_data[i].mode & GPIO_MODE_OUTPUT )
		{
			imx233_set_pin_function( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, PINCTRL_FUNCTION_GPIO );
			imx233_enable_gpio_output( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, TRUE );

			if( gpio_data[i].mode & GPIO_OUTPUT_ON )
			{
				imx233_set_gpio_output( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, TRUE );
			}
			else
			{
				imx233_set_gpio_output( gpio_data[i].gpio_bank, gpio_data[i].gpio_pio, FALSE );
			}
		}
	}
}

static void gpio_drv_init( uint32_t index )
{
}

static void gpio_drv_close( uint32_t index )
{
}

static void counter_drv_init( uint32_t index )
{
}

static void counter_drv_close( uint32_t index )
{
}

static uint32_t counter_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t * p_value;

	if( size != sizeof( uint32_t ) ) return size;

	if( index == COUNTER_GPIO_RPM )
	{
		DI();
		p_value = ( uint32_t * )buffer;
		rpm_counter = *p_value;
		EI();
	}
	else if( index == COUNTER_GPIO_SPEED )
	{
		DI();
		p_value = ( uint32_t * )buffer;
		speed_counter = *p_value;
		EI();
	}

	return size;
}

static uint32_t counter_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t * p_value;

	if( size != sizeof( uint32_t ) ) return size;

	if( index == COUNTER_GPIO_RPM )
	{
		DI();
		p_value = ( uint32_t * )buffer;
		*p_value = rpm_counter;
		EI();
	}
	else if( index == COUNTER_GPIO_SPEED )
	{
		DI();
		p_value = ( uint32_t * )buffer;
		*p_value = speed_counter;
		EI();
	}

	return size;
}

static uint32_t gpio_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
	if( size != 1 ) return 0;

	if( gpio_data[index].mode & GPIO_MODE_OUTPUT )
	{
		if( buffer[0] )
		{
			imx233_set_gpio_output( gpio_data[index].gpio_bank, gpio_data[index].gpio_pio, TRUE );
		}
		else
		{
			imx233_set_gpio_output( gpio_data[index].gpio_bank, gpio_data[index].gpio_pio, FALSE );
		}
	}
	return size;
}

static uint32_t gpio_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	if( size != 1 ) return 0;

	if( gpio_data[index].mode & GPIO_MODE_INPUT )
	{
		*buffer = imx233_get_gpio_input( gpio_data[index].gpio_bank, gpio_data[index].gpio_pio ) ? TRUE : FALSE;
	}

	return size;
}

static uint32_t counter_io_ctrl( uint32_t index, uint32_t param, void * value )
{
	uint64_t * p_value = ( uint64_t * )value;

	if( param == DEVICE_GET_RUNTIME_VALUE )
	{
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////

static bool_t gpio_validate( uint32_t index )
{
	int i;

	for( i = 0; i < 255; i++ )
	{
		if( gpio_data[i].index == NEG_U32 ) break;
		if( gpio_data[i].mode == 0 ) continue;

		if( gpio_data[i].index >= GPIO_INDEX_INTERNAL ) break;

		if( gpio_data[i].index == index ) return TRUE;
	}

    return FALSE;
}


static bool_t counter_validate( uint32_t index )
{
	if( index == COUNTER_GPIO_RPM )
	{
		return TRUE;
	}
	else if( index == COUNTER_GPIO_SPEED )
	{
		return TRUE;
	}

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

DECLARE_DEVICE_SECTION( gpio_device );
DECLARE_DEVICE_SECTION( counter_device );
DECLARE_INIT_DEVICE_SECTION( counter_gpio_global_init );

