#include "system.h"
#include "debug.h"
#include "tima_libc.h"
#include "device.h"
#include "timer.h"
#include "commands.h"

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
	timer_data_t timer_ms;
	uint32_t counter;

	timer_Start( &timer_ms, 1000 );
	counter = 0;
    
    console_init();
    console_listen_telnet( 12222 );
    
	DEBUG_PRINTK( "Console test\r\n" );

    while( 1 )
    {
		if( timer_Check( &timer_ms ) )
		{
			timer_Reload( &timer_ms );
			DEBUG_PRINTK( "counter = %d\r\n", counter++ );
		}

        console_process();
        SYSTEM_EVENTS();
    }
}

DECLARE_APPLICATION_SECTION( app_main );
