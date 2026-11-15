#include "system.h"
#include "tima_libc.h"
#include "timer.h"
#include "debug.h"
#include "pio_check.h"
#include "can_comms.h"
#include "output_ctrl.h"

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
    timer_data_t timer_sec;
    void * led_red;
    void * led_green;

    device_t * adc_output;
    uint32_t new_value;
    uint16_t adc_value;
    bool_t output_state;

    DEBUG_PRINTK( "BCM 02\r\n" );

    can_comms_init();
    console_init();
    output_ctrl_init();

    //console_connect_uart( DEV_PRINTK );

    timer_Start(&timer_sec, 250);
    new_value = 0;

    led_red =    pio_check_event_open_output( "/dev/gpio6", FALSE, FALSE );
    led_green =  pio_check_event_open_output( "/dev/gpio7", FALSE, FALSE );

    adc_output = device_open( "/dev/adc86" );

    while( 1 )
    {
    	if( timer_Check(&timer_sec) == TRUE )
        {
            pio_check_event_set_output( led_red,   (new_value & 0x01) ? FALSE : TRUE  );
            pio_check_event_set_output( led_green, (new_value & 0x01) ? TRUE  : FALSE );

            //output_state = (new_value & 0x08) ? TRUE : FALSE;
            //outp_u7_out5( output_state );
            //device_read_buffer( adc_output, ( uint8_t * )&adc_value, 2 );
            //DEBUG_PRINTK( "ADC = %d\n", adc_value );

            //outl7_ctrl( (new_value & 0x04) ? TRUE : FALSE );

            timer_Reload( &timer_sec );

            if ((new_value & 0x03) == 0)
    	    {
                DEBUG_PRINTK( "Counter [%c] = %d\r\n", '+', new_value >> 2 );
    	    }
            new_value++;
        }

    	output_ctrl_process();
        console_process();
        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main );
