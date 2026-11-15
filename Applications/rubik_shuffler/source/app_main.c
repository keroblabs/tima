#include "system.h"
#include "tima_libc.h"
#include "timer.h"
#include "debug.h"
#include "device.h"

/////////////////////////////////////////////////////////////////////////////////////////

// ./st-flash --reset --format ihex write ~/Projects/GIT_Dev/timaos_v42/Applications/rubik_shuffler/make_nucleo446/rubik_shuffler.hex

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
    timer_data_t servo_timer;
    timer_data_t timer_sec;
    timer_data_t update_timer;

    device_t * gpio_device;
    device_t * btn_device;

    uint32_t new_value;
    uint64_t dutty_servo = 0L;

    bool_t btn_state = TRUE;
    bool_t servo_on = FALSE;

    timer_Start(&update_timer, 200);
    timer_Start(&timer_sec, 1000);
    new_value = 0;



    btn_device = device_open( "/dev/gpio4");
    if( btn_device == NULL )
    {
        DEBUG_PRINTK( "Failed to open button\r\n" );
    }
    else
    {
        DEBUG_PRINTK( "Open button %d\r\n", btn_device->device_index );
    }

    gpio_device = device_open( "/dev/gpio6" );
    timer_Start(&servo_timer, 20);

    if( gpio_device == NULL )
    {
        DEBUG_PRINTK( "Failed to open servo\r\n" );
    }
    else
    {
        DEBUG_PRINTK( "Open servo %d\r\n", gpio_device->device_index );
    }

    DEBUG_PRINTK( "Rubik Shuffler\r\n" );

    while( 1 )
    {
        if( timer_Check(&update_timer) == TRUE )
        {
            timer_Reload( &update_timer );

            if( btn_device != NULL )
            {
                btn_state = device_read( btn_device );
            }
        }

    	if( timer_Check(&timer_sec) == TRUE )
        {
        	timer_Reload( &timer_sec );
        	DEBUG_PRINTK( "Counter = %d\r\n", new_value );
        	new_value++;
        }

        if( ( gpio_device != NULL ) && ( timer_Check(&servo_timer) == TRUE ) )
        {
            timer_Reload( &servo_timer );
            device_write( gpio_device, 1 );
            dutty_servo = timer_get_US() + (btn_state ? 2050L : 1750L);
            servo_on = TRUE;
        }

        if( ( servo_on == TRUE ) && ( dutty_servo < timer_get_US() ) )
        {
            servo_on = FALSE;
            device_write( gpio_device, 0 );
        }

        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main )

