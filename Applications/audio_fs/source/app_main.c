#include "system.h"
#include "tima_libc.h"
#include "timer.h"
#include "pipe.h"
#include "commands.h"
#include "efs.h"
#include "debug.h"

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
    timer_data_t timer_sec;
    uint32_t new_value;

    timer_Start(&timer_sec, 1000);
    new_value = 0;
    
    console_init();
    console_connect_uart( DEV_TTY_COMMAND );
    //console_listen_telnet( 12222 );

    efs_init();
    //MAL_Init_All();

    DEBUG_PRINTK( "Tima Test\r\n" );

    while( 1 )
    {
    	if( timer_Check(&timer_sec) == TRUE )
        {
        	timer_Reload( &timer_sec );
        	DEBUG_PRINTK( "Counter = %d\r\n", new_value );
        	new_value++;
        }

        console_process();

        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main )
