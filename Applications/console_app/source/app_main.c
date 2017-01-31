#include "system.h"
#include "debug.h"
#include "tima_libc.h"
#include "device.h"
//#include "commands.h"
#include "timer.h"
#include "graphics.h"
#include "draw.h"

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
	timer_data_t timer_ms;
	uint32_t counter;
	device_t * dev_counter;

	graphics_init();
	graphics_init_auto_vsync();

	graphics_fill_box( 10, 10, 50, 150, APP_RGB(255,0,0));

	draw_font_init(FONT_12x16);
	draw_text_printk( 10, 200, "Tima Test" );

	dev_counter = device_open( "/dev/counter11" );
	//dev_counter = device_open( "/dev/gpio11" );
	if( dev_counter == NULL )
	{
		draw_text_printk( 180, 200, "No counter" );
	}

	DEBUG_PRINTK( "Console test\r\n" );

    timer_Start( &timer_ms, 1000 );
	counter = 0;
    
    while( 1 )
    {
		if( timer_Check( &timer_ms ) )
		{
			timer_Reload( &timer_ms );
			DEBUG_PRINTK( "counter = %d\r\n", counter );
			draw_text_printk( 10, 220, "counter = %d ", counter );

			counter++;

			if( dev_counter != NULL )
			{
				uint32_t cnt;
				device_read_buffer( dev_counter, ( uint8_t * )&cnt, sizeof( cnt ) );
				draw_text_printk( 10, 240, "intr = %d ", cnt );
				//uint8_t state;
				//device_read_buffer( dev_counter, ( uint8_t * )&state, sizeof( state ) );
				//draw_text_printk( 10, 240, "state = %d ", state );
			}
		}
        
        //console_process();
		SYSTEM_EVENTS();
    }
}

#if 0
void app_main( void )
{
	device_t * dev_telnet;
	timer_data_t timer_ms;
	uint32_t counter;

	timer_Start( &timer_ms, 1000 );
	dev_telnet = NULL;
	counter = 0;

	console_init();
	console_connect_uart( DEV_TTY_COMMAND );
	console_listen_telnet( 12222 );

	webserver_init( 4545 );

	DEBUG_PRINTK( "Console test\r\n" );

    while( 1 )
    {
		if( timer_Check( &timer_ms ) )
		{
			timer_Reload( &timer_ms );
			DEBUG_PRINTK( "counter = %d\r\n", counter++ );
		}

		console_process();
		webserver_process();
		SYSTEM_EVENTS();
    }
}
#endif

#if 0
void app_main( void )
{
	device_t * dev;
	device_t * accept_dev;
	//uint8_t ip_addr[4];

	DEBUG_PRINTK( "Console test\r\n" );

#if 0
	tima_str_to_ip( "127.0.0.1", ip_addr );
	dev = device_socket_open( ip_addr, 18023 );
	if( dev != NULL )
	{
		char text[20];
		strcpy( text, "Hello\r\n" );
		device_write_buffer( dev, ( uint8_t * )text, strlen( text ) );
	}
#endif

	dev = device_socket_listen( 28120 );
	accept_dev = NULL;

    while( 1 )
    {
		if( ( dev != NULL ) && ( accept_dev == NULL ) )
		{
			if( ( accept_dev = device_socket_accept( dev ) ) != NULL )
			{
				char text[20];
				strcpy( text, "Hello\r\n" );
				device_write_buffer( accept_dev, ( uint8_t * )text, strlen( text ) );
			}
		}

		SYSTEM_EVENTS();
    }
}
#endif


#if 0
void app_main( void )
{
	uint32_t counter;
	timer_data_t timer_ms;
	
	counter = 0;
	timer_Start( &timer_ms, 1000 );
	
	DEBUG_PRINTK( "Console test\r\n" );

    while( 1 )
    {
    	if( timer_Check( &timer_ms ) )
    	{
    		timer_Reload( &timer_ms );
    		DEBUG_PRINTK( "counter = %d\r\n", counter );
    		
    		counter++;
    	}
    	
		SYSTEM_EVENTS();
    }
}
#endif

DECLARE_APPLICATION_SECTION( app_main );
