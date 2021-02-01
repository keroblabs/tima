#include "system.h"
#include "tima_libc.h"
#include "timer.h"
#include "debug.h"
#include "filesystem_client.h"
#include "security_client.h"
#include "datalink.h"

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
    timer_data_t timer_sec;
    uint32_t new_value;
    char rx_byte;
    char show_byte;
    
    DEBUG_PRINTK( "Tima Test 04\r\n" );

    datalink_global_init();

    tima_link_filesystem_client_init();
    tima_link_filesystem_transaction_open_file( "config_gem.txt", "r" );

    security_client_init();

    timer_Start(&timer_sec, 1000);
    new_value = 0;
    show_byte = 0x20;
    
    while( 1 )
    {
    	if( timer_Check(&timer_sec) == TRUE )
        {
        	timer_Reload( &timer_sec );
        	//DEBUG_PRINTK( "Counter [%c] = %d\r\n", show_byte, new_value );
        	new_value++;
        }

    	if( debug_read_input( &rx_byte ) == TRUE )
    	{
    		if( ( rx_byte >= 0x20 ) && ( rx_byte < 0x7F ) )
    		{
    			show_byte = rx_byte;
    			DEBUG_PRINTK( "Counter [%c] = %d\r\n", show_byte, new_value );
    		}
    	}
        
    	tima_link_filesystem_process();
        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main );
