#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "types.h"
#include "system.h"
#include "debug.h"
#include "hw_config.h"

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

static bool_t is_init;

////////////////////////////////////////////////////////////////////////

static void hw_gpio_list_init( void )
{
	uint8_t index = 0;
	gpio_hw_config_t * gpio_hw_config = ( gpio_hw_config_t * )system_Get_Hw_Config();

	while( gpio_hw_config != NULL )
	{
		if( gpio_hw_config->type == HW_CONFIG_TYPE_GPIO )
		{
			gpio_hw_config_init( gpio_hw_config );
		}

		gpio_hw_config++;
	}
}

static void gpio_drv_global_init( void )
{
	is_init = FALSE;
}

static void gpio_drv_init( uint32_t index )
{
	if( is_init == FALSE )
	{
		is_init = TRUE;

		hw_gpio_list_init();
	}

	uint8_t index = 0;
	gpio_hw_config_t * gpio_hw_config = ( gpio_hw_config_t * )system_Get_Hw_Config();

	while( gpio_hw_config != NULL )
	{
		if( ( gpio_hw_config->type == HW_CONFIG_TYPE_GPIO ) && ( gpio_hw_config->id == index ) )
		{
		}

		gpio_hw_config++;
	}

}

static void gpio_drv_close( uint32_t index )
{
}

static uint32_t gpio_drv_ioctl( uint32_t index, uint32_t param, void * value )
{
	return 0;
}

static uint32_t gpio_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
	if( size != sizeof( uint8_t ) ) return 0;

	if( index > GPIO_MAX_INDEX ) return 0;

	if( gpio_map_list[index].gpio != NULL )
	{
		if( gpio_mode_list[index] == GPIO_INIT_MODE_OUTPUT )
		{
            //DEBUG_PRINTK( "Output: \n", buffer[0] );

            if( buffer[0] == 0 )
			{
				GPIO_ResetBits( gpio_map_list[index].gpio, gpio_map_list[index].pin );
			}
			else
			{
				GPIO_SetBits( gpio_map_list[index].gpio, gpio_map_list[index].pin );
			}
		}
	}
	else if( ( gpio_map_list[index].gpio == NULL ) && ( gpio_map_list[index].mode == GPIO_INIT_MODE_EXPAND ) )
	{
		if( gpio_mode_list[index] == GPIO_INIT_MODE_OUTPUT )
		{
		}
	}

	return size;
}

static uint32_t gpio_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	if( size != sizeof( uint8_t ) ) return 0;

	if( index > GPIO_MAX_INDEX ) return 0;

	if( gpio_map_list[index].gpio != NULL )
	{
		if( gpio_mode_list[index] == GPIO_INIT_MODE_INPUT )
		{
		    buffer[0] = GPIO_ReadInputDataBit( gpio_map_list[index].gpio, gpio_map_list[index].pin );
		    //DEBUG_PRINTK( "Read GPIO: %d %d %d %d\r\n", gpio_map_list[index].pin, index, buffer[0], size );
		}
	}
	else if( ( gpio_map_list[index].gpio == NULL ) && ( gpio_map_list[index].mode == GPIO_INIT_MODE_EXPAND ) )
	{
		if( gpio_mode_list[index] == GPIO_INIT_MODE_INPUT )
		{
			buffer[0] = 0;
		}
	}

	return size;
}

////////////////////////////////////////////////////////////////////////

static bool_t gpio_validate( uint32_t index )
{
    if( index <= GPIO_MAX_INDEX ) return TRUE;
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
	gpio_drv_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

    gpio_drv_input,
    gpio_drv_output,

};

DECLARE_DEVICE_SECTION( gpio_device );
DECLARE_INIT_POWERUP_SECTION( gpio_drv_global_init );


