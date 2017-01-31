#include "system.h"
#include "types.h"

#include "regsclkctrl.h"
#include "pinctrl-imx233.h"
#include "system_imx233.h"

#include "debug.h"

////////////////////////////////////////////////////////////////////////////////////////////

#define GPIO_CSFLASH_BANK		0
#define GPIO_CSFLASH_PIO		17
#define GPIO_CSFLASH			GPIO_CSFLASH_BANK, GPIO_CSFLASH_PIO

#define GPIO_SPEED_BANK			0
#define GPIO_SPEED_PIO			16
#define GPIO_SPEED				GPIO_SPEED_BANK, GPIO_SPEED_PIO

#define GPIO_UNUSED1_BANK		1
#define GPIO_UNUSED1_PIO		28
#define GPIO_UNUSED1			GPIO_UNUSED1_BANK, GPIO_UNUSED1_PIO

#define GPIO_MUX_A_BANK			1
#define GPIO_MUX_A_PIO			20
#define GPIO_MUX_A				GPIO_MUX_A_BANK, GPIO_MUX_A_PIO

#define GPIO_MUX_B_BANK			1
#define GPIO_MUX_B_PIO			19
#define GPIO_MUX_B				GPIO_MUX_B_BANK, GPIO_MUX_B_PIO

#define GPIO_MUX_C_BANK			1
#define GPIO_MUX_C_PIO			21
#define GPIO_MUX_C				GPIO_MUX_C_BANK, GPIO_MUX_C_PIO

#define GPIO_BATTERY_BANK		0
#define GPIO_BATTERY_PIO		25
#define GPIO_BATTERY			GPIO_BATTERY_BANK, GPIO_BATTERY_PIO

#define GPIO_MUX_PIN_BANK		0
#define GPIO_MUX_PIN_PIO		23
#define GPIO_MUX_PIN			GPIO_MUX_PIN_BANK, GPIO_MUX_PIN_PIO

#define GPIO_RPM_BANK			2
#define GPIO_RPM_PIO			27
#define GPIO_RPM				GPIO_RPM_BANK, GPIO_RPM_PIO

#define GPIO_CSTPI_BANK			2
#define GPIO_CSTPI_PIO			28
#define GPIO_CSTPI				GPIO_CSTPI_BANK, GPIO_CSTPI_PIO

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
	// not accessible
	{ GPIO_INDEX_INTERNAL + 1, GPIO_CSFLASH,	GPIO_MODE_OUTPUT | GPIO_OUTPUT_OFF },
	{ GPIO_INDEX_INTERNAL + 2, GPIO_CSTPI,		GPIO_MODE_OUTPUT | GPIO_OUTPUT_ON },

	{ GPIO_INDEX_INTERNAL + 3, GPIO_SPEED,		GPIO_MODE_INPUT /* | GPIO_MODE_COUNTER */ },
	{ GPIO_INDEX_INTERNAL + 4, GPIO_RPM,		GPIO_MODE_INPUT /* | GPIO_MODE_COUNTER */ },

	{ GPIO_INDEX_INTERNAL + 5, GPIO_CSGPIO,		GPIO_MODE_OUTPUT | GPIO_OUTPUT_ON },

	{ GPIO_INDEX_INTERNAL + 12, GPIO_BATTERY, 	GPIO_MODE_INPUT },

	{ GPIO_INDEX_INTERNAL + 14, GPIO_MUX_A, 	GPIO_MODE_OUTPUT | GPIO_OUTPUT_OFF },
	{ GPIO_INDEX_INTERNAL + 15, GPIO_MUX_B, 	GPIO_MODE_OUTPUT | GPIO_OUTPUT_OFF },
	{ GPIO_INDEX_INTERNAL + 16, GPIO_MUX_C, 	GPIO_MODE_OUTPUT | GPIO_OUTPUT_OFF },
	{ GPIO_INDEX_INTERNAL + 17, GPIO_MUX_PIN, 	GPIO_MODE_INPUT },

	{ NEG_U32, 0, 0 }
};

const static uint32_t gpio_index_lut[8] =
{
		9,	// washer level
		10, // indicator 1
		11, // indicator 2
		16, // handbrake 0
		17, // handbrake 1
		13, // brake level
		14, // full beam
		15, // oil pressure
};

////////////////////////////////////////////////////////////////////////////////////////////

static __IO uint32_t rpm_counter;
static __IO uint32_t speed_counter;

static uint32_t gpio_input_state;
static uint32_t gpio_mux_state;

///////////////////////////////////////////////////////////////////////////

static void gpio_read_process( void )
{
	uint32_t index;
	bool_t pio_state;

	if( ( gpio_mux_state & 0x01 ) == 0 )
	{
		if( gpio_mux_state & 0x02 ) imx233_set_gpio_output( GPIO_MUX_A, TRUE );
		else imx233_set_gpio_output( GPIO_MUX_A, FALSE );

		if( gpio_mux_state & 0x04 ) imx233_set_gpio_output( GPIO_MUX_B, TRUE );
		else imx233_set_gpio_output( GPIO_MUX_B, FALSE );

		if( gpio_mux_state & 0x08 ) imx233_set_gpio_output( GPIO_MUX_C, TRUE );
		else imx233_set_gpio_output( GPIO_MUX_C, FALSE );
	}
	else
	{
		pio_state = imx233_get_gpio_input( GPIO_MUX_PIN ) ? TRUE : FALSE;
		index = gpio_index_lut[ ( gpio_mux_state >> 1 ) & 0x07 ];

		gpio_input_state &= ~( 1 << index );
		if( pio_state ) gpio_input_state |= ( 1 << index );

		gpio_input_state &= ~( 1 << 3 );
		if( imx233_get_gpio_input( GPIO_SPEED ) ) gpio_input_state |= ( 1 << 3 );

		gpio_input_state &= ~( 1 << 4 );
		if( imx233_get_gpio_input( GPIO_RPM ) ) gpio_input_state |= ( 1 << 4 );

		gpio_input_state &= ~( 1 << 12 );
		if( imx233_get_gpio_input( GPIO_BATTERY ) ) gpio_input_state |= ( 1 << 12 );
	}

	gpio_mux_state++;
}

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

	gpio_mux_state = 0;
	gpio_input_state = 0;

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
	return size;
}

static uint32_t gpio_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	if( size != 1 ) return 0;

	*buffer = 0;
	if( gpio_input_state & ( 1 << index ) ) *buffer = 1;

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

	if( i < 32 ) return TRUE;
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

DECLARE_PROCESS_SECTION( gpio_read_process );
DECLARE_DEVICE_SECTION( gpio_device );
DECLARE_DEVICE_SECTION( counter_device );
DECLARE_INIT_DEVICE_SECTION( counter_gpio_global_init );

