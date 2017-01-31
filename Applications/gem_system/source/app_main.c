#include "system.h"
#include "tima_libc.h"
#include "timer.h"
#include "pipe.h"
#include "debug.h"

//#include "keypad.h"
//#include "wifi_click.h"

#include "power_control.h"
#include "pio_check.h"
#include "indicator_proc.h"
#include "odometer_control.h"
#include "nvram_control.h"
#include "syscomm.h"
#include "doors_proc.h"
//#include "wiper_control.h"

#include "graphics.h"
#include "draw.h"

#define DONT_ENABLE_KEYPAD
#define DONT_ENABLE_WIFI_CLICK
//#define ENABLE_OBD

/////////////////////////////////////////////////////////////////////////////////////////

#define KEYPAD_BUFFER_SIZE		30

/* speed sensor
 *
hz	km/h	speed		m/s							km
12	13.5	15.1875		4.21875		0.3515625	0.0003515625	2844	2843997156
30	33.3	37.4625		10.40625	0.346875	0.000346875				4294967296
40	44.4	49.95		13.875		0.346875	0.000346875
46	51		57.375		15.9375		0.346467	0.000346467
									0.0003479449728				999999	2874014795
																		4294967296


one pulse = 0.3516 m
one pulse every 250ms (1/4s) = 1.4 m/s = 5.06 km/h

200km/h = 55.5 m/s = 13.8 m (1/4s) ... 39 pulses in 250 ms
39 pulses in 1/4s = 156 pulses/s (156 hz) = 197 km/h
39 pulses in 1/4s = 1 pulse is 6.4 ms

1 pulse in 6 ms -> 166 pulses/s ... 166 x 1000 / 2844 = 58m/s = 210 km/h
1 pulse in 7 ms -> 142 pulses/s ... 142 ------------- = 50m/s = 180 km/h

200 km/h =  6320 us / pulse
199 km/h = 	6360 us / pulse
100 km/h = 12650 us / pulse
99  km/h = 12780 us / pulse

km/h = ( 1000000 / us ) * ( 1000 / 2844 ) * 3.6

*/

/////////////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_KEYPAD
static char keypad_buffer[KEYPAD_BUFFER_SIZE+1];
#endif

/////////////////////////////////////////////////////////////////////////////////////////

#define DISPLAY_LINE(x) ( 150 + ( 8 * ( x ) ) )

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void app_show_display( void )
{
    graphics_fill_box( 10, 10, 50, 50, APP_RGB(255,0,0) );
    draw_font_init( FONT_8x8 );
    draw_text( 10, 70, "Tima Test", APP_RGB(255,255,255), APP_RGB(0,0,0) );
}

void app_gpio_spi_test( void )
{
	device_t * dev;
	uint8_t data;

    dev = device_open( "/dev/gpio80" );
    if( dev != NULL )
    {
    	device_read_buffer( dev, &data, sizeof( uint8_t ) );
    	DEBUG_PRINTK( "/dev/gpio80 = %d\r\n", data );
    }
}

void app_main( void )
{
    timer_data_t timer_sec;
    bool_t lcd_initiated;
    //uint32_t obd_mask;
    uint32_t prev_val;
    uint32_t new_val;
    
    #ifdef ENABLE_KEYPAD
    char char_rx;
    #endif
    
    #ifdef ENABLE_WIFI_CLICK
    bool_t wifi_ready;
    #endif
    
    lcd_initiated = FALSE;
    
    prev_val = NEG_U32;

    app_gpio_spi_test();

    DEBUG_PRINTK( "Tima Test\r\n" );

    timer_Start(&timer_sec, 1000);
    
    syscomm_init();
    nvram_control_init();
    power_control_init();
    //obd_send_init();
    pio_check_init();
    indicator_proc_init();
    odometer_control_init();
    doors_proc_init();
    //wiper_control_init();

    #ifdef ENABLE_KEYPAD
    keypad_init();
    memset( keypad_buffer, 0x20, sizeof( keypad_buffer ) );
    keypad_buffer[KEYPAD_BUFFER_SIZE] = 0;
    #endif
    
    graphics_init();

    if( graphics_driver_ready() == TRUE )
    {
    	lcd_initiated = TRUE;
    	app_show_display();
    }

    #ifdef ENABLE_WIFI_CLICK
    wifi_click_init();
    wifi_ready = FALSE;
    #endif
    
    while( 1 )
    {
        if( graphics_driver_ready() == TRUE )
        {
        	if( lcd_initiated == FALSE )
        	{
            	lcd_initiated = TRUE;
            	app_show_display();
        	}
        }

    	if( timer_Check(&timer_sec) == TRUE )
        {
        	timer_Reload( &timer_sec );

    		new_val = odometer_control_get();
        	if( new_val != prev_val )
        	{
        		prev_val = new_val;

        		DEBUG_PRINTK( "Odometer = %d\r\n", new_val );
        	}
        }

    	/*
    	*/

        //obd_send_process();
    	syscomm_process();
        power_control_process();
        indicator_proc_process();
        odometer_control_process();
        doors_proc_process();
        //wiper_control_process();

        #ifdef ENABLE_KEYPAD
        if( keypad_hit() )
        {
        	strcpy( keypad_buffer, &keypad_buffer[1] );
        	char_rx = (char)keypad_read();

        	if( ( char_rx >= 0x20 ) && ( char_rx <= 0x7F ) )
        	{
            	keypad_buffer[KEYPAD_BUFFER_SIZE-1] = char_rx;
            	draw_text_printk( 10, 100, "%s", keypad_buffer );
        	}
        }
        #endif

        #ifdef ENABLE_WIFI_CLICK
        if( wifi_ready == FALSE )
        {
            wifi_ready = TRUE;
            //wifi_click_reset();
            //wifi_click_get_version();
        }
        
        wifi_click_process();
        
        #endif
        
        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main )
