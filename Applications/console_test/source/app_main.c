#include "system.h"
#include "debug.h"
#include "tima_libc.h"
#include "device.h"
#include "commands.h"
#include "timer.h"
#include "graphics.h"
#include "draw.h"
#include "efs.h"
//#include "webserver.h"

//#include "regsclkctrl.h"
//#include "serialdbg_imx233.h"

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

static char debug_string[100];

#if 0
void app_main( void )
{
	timer_data_t timer_ms;
	uint32_t counter;
	device_t * dev_gpio;
	device_t * dev_counter;
	device_t * tty_app;
	char rx_byte;
	char debug_char;
	uint16_t key_posx;
	uint16_t rx_posx;

	graphics_init();
	graphics_init_auto_vsync();

	graphics_fill_box( 10, 10, 50, 150, APP_RGB(255,0,0));

	draw_font_init(FONT_12x16);
	draw_text_printk( 10, 200, "Tima Test --" );

	dev_counter = device_open( "/dev/counter11" );
	if( dev_counter == NULL )
	{
		draw_text_printk( 180, 200, "No counter" );
	}

	dev_gpio = device_open( "/dev/gpio15" );
	if( dev_gpio == NULL )
	{
		draw_text_printk( 180, 200, "No GPIO" );
	}

	tty_app = device_open( "/dev/tty18" );
	if( tty_app != NULL )
	{
		counter = 9600;
		//DEBUG_PRINTK( "TTY18 opened 0x%08x\r\n", HW_CLKCTRL_XTAL_RD() );
		device_ioctrl( tty_app, DEVICE_SET_CONFIG_DATA, &counter );
	}

	DEBUG_PRINTK( "Console test\r\n" );

    timer_Start( &timer_ms, 1000 );
	counter = 0;
	key_posx = 10;
	rx_posx = 10;

    
    while( 1 )
    {
		if( timer_Check( &timer_ms ) )
		{
			timer_Reload( &timer_ms );
			DEBUG_PRINTK( "counter = %d\r\n", counter );

			//sprintk( debug_string, "[0x%08x]\r\n", HW_SERIALDBG_FLAGS );
			device_write_buffer( tty_app, ( uint8_t * )debug_string, (uint32_t)strlen( debug_string ) );

			draw_text_printk( 10, 220, "counter = %d ", counter );

			counter++;

			if( dev_gpio != NULL )
			{
				uint8_t state;
				device_read_buffer( dev_gpio, ( uint8_t * )&state, sizeof( state ) );
				draw_text_printk( 10, 240, "state = %d ", state );
			}

			if( dev_counter != NULL )
			{
				uint32_t cnt;
				device_read_buffer( dev_counter, ( uint8_t * )&cnt, sizeof( cnt ) );
				draw_text_printk( 10, 260, "intr = %x ", cnt );
			}
		}

		if( device_is_rx_pending( tty_app ) )
		{
			device_read_buffer( tty_app, ( uint8_t * )&rx_byte, 1 );
			draw_text_printk( rx_posx, 300, "%c", rx_byte );
			rx_posx += 12;
			if( rx_posx > 200 ) rx_posx = 10;

			DEBUG_PRINTK( "[%c]\r\n", rx_byte );
		}

		if( debug_read_input( &debug_char ) )
		{
			draw_text_printk( key_posx, 280, "%c", debug_char );
			key_posx += 12;
			if( key_posx > 200 ) key_posx = 10;

			DEBUG_PRINTK( "[%c]\r\n", debug_char );

			if( debug_char == 97 )
			{
				while(1) {}
			}
		}
        
        //console_process();
		SYSTEM_EVENTS();
    }
}
#endif

#if 1
void app_main( void )
{
	device_t * dev_telnet;
	timer_data_t timer_ms;
	uint32_t counter;

	timer_Start( &timer_ms, 1000 );
	dev_telnet = NULL;
	counter = 0;
    
    graphics_init();
    
    efs_init();

	console_init();
	console_connect_uart( DEV_TTY_COMMAND );
	console_listen_telnet( 12222 );

	//webserver_init( 4545 );

	DEBUG_PRINTK( "Console test\r\n" );

    while( 1 )
    {
		if( timer_Check( &timer_ms ) )
		{
			timer_Reload( &timer_ms );
			DEBUG_PRINTK( "counter = %d\r\n", counter++ );
		}

		console_process();
		//webserver_process();
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
