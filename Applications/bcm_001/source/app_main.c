#include "system.h"
#include "tima_libc.h"
#include "timer.h"
#include "debug.h"
#include "pio_check.h"
#include "can_comms.h"

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
    timer_data_t timer_sec;
    static void * led_red;
    static void * led_green;
    uint32_t new_value;
    
    DEBUG_PRINTK( "BCM 01\r\n" );

    can_comms_init();
    console_init();
    console_connect_uart( DEV_PRINTK );

    timer_Start(&timer_sec, 250);
    new_value = 0;
    
    led_red =   pio_check_event_open_output( "/dev/gpio6", FALSE, FALSE );
    led_green = pio_check_event_open_output( "/dev/gpio7", FALSE, FALSE );

    while( 1 )
    {
    	if( timer_Check(&timer_sec) == TRUE )
        {
            pio_check_event_set_output( led_red,   (new_value & 0x01) ? FALSE : TRUE  );
            pio_check_event_set_output( led_green, (new_value & 0x01) ? TRUE  : FALSE );

            timer_Reload( &timer_sec );

            if ((new_value & 0x03) == 0)
    	    {
                //DEBUG_PRINTK( "Counter [%c] = %d\r\n", '-', new_value >> 2 );
    	    }
            new_value++;
        }

        console_process();
        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main );
