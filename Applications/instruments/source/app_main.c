#include "system.h"
#include "debug.h"
#include "tima_libc.h"

#include "graphics.h"
#include "draw.h"
#include "timer.h"

#include "instruments.h"
#include "pio_check.h"

#ifdef USE_BATTERY_CHECK_ADC
#include "battery_check.h"
#endif

//#include "setup_data.h"
#include "obd_engine.h"
#include "obd_control.h"
#include "gui_setup.h"
#include "fuel_check.h"
#include "syscomm.h"
#include "odometer_control.h"

/////////////////////////////////////////////////////////////////////////////////////////

#define USE_LOCAL_OBD

/////////////////////////////////////////////////////////////////////////////////////////

#define OBD_MESSAGE_MAX_SIZE		50
#define OBD_MESSAGE_PAYLOAD_SIZE	11
#define OBD_MESSAGE_HEADER			0xF8
#define OBD_MESSAGE_FOOTER			0xA8
#define OBD_MESSAGE_ACK             0x9A

#define OBD_TIMEOUT_MS				100

/////////////////////////////////////////////////////////////////////////////////////////

enum
{
	OBD_MESSAGE_STATE_IDLE,
	OBD_MESSAGE_STATE_PAYLOAD,
	OBD_MESSAGE_STATE_PARSE
};

typedef struct _obd_message_
{
	uint8_t header;
	uint8_t speed;
	uint16_t rpm;
	uint32_t odometer;
	uint16_t mask;
	uint8_t temp;
	uint8_t footer;

} obd_message_t;

/////////////////////////////////////////////////////////////////////////////////////////

static char debug_string[128];

//static uint8_t obd_message[OBD_MESSAGE_MAX_SIZE];
static uint16_t obd_msg_index;
static bool_t obd_msg_state;

static timer_data_t obd_timeout;

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
	//char rx_chr;

	uint32_t counter;
	uint32_t prev_rpm_cnt;
	timer_data_t debug_timer;
	timer_data_t rpm_timer;

	bool_t obd_ready;

    syscomm_init();
    //setup_data_init();

#ifdef USE_BATTERY_CHECK_ADC
	battery_init();
#endif

	obd_msg_state= OBD_MESSAGE_STATE_IDLE;
	obd_msg_index = 0;

	graphics_init();
	graphics_init_auto_vsync();

#ifdef USE_LOCAL_OBD
	//obd_engine_init();
    obd_control_init();
#endif

	fuel_check_init();
	instruments_init();
	pio_check_init();
	gui_setup_init();
	odometer_control_init();

	DEBUG_PRINTK( "Tima Instruments\r\n" );

	timer_Start( &rpm_timer, 200 );
	timer_Start( &debug_timer, 1000 );

	obd_ready = FALSE;
	counter = 0;
	prev_rpm_cnt = 0;

	timer_Stop( &obd_timeout );
    
    while( 1 )
    {
		//if( debug_read_input( &rx_chr ) )
		//{
		//	DEBUG_PRINTK( "[%c] HW_UARTAPP_STAT_RD(1) = 0x%08x\r\n", rx_chr, HW_UARTAPP_STAT_RD(1) );
		//}
    	/*
		if( timer_Check( &rpm_timer ) )
    	{
    		timer_Reload( &rpm_timer );
    	}
    	*/
    	if( timer_Check( &debug_timer ) )
    	{
    		timer_Reload( &debug_timer );

			sprintk( debug_string, "Fuel=%d (%d)   ", (int)fuel_check_get(), (int)fuel_check_get_adc() );
			instruments_show_information( debug_string, SETUP_PARAM_DISPLAY2_LINE3 );
    	}
        
#ifdef USE_LOCAL_OBD
#if 0
        if( ( obd_engine_get_response( NULL ) == OBD_STATUS_IDLE ) && ( obd_ready == FALSE ) )
        {
    		DEBUG_PRINTK( "Start Auto\r\n" );

        	obd_engine_autoupdate( TRUE );
            obd_ready = TRUE;
        }
		obd_engine_process();
#else
        obd_control_process();
#endif
#endif

#ifdef USE_BATTERY_CHECK_ADC
		battery_process();
#endif

		fuel_check_process();
		instruments_process();
		pio_check_process();
		gui_setup_process();
		graphics_process();
		syscomm_process();
		odometer_control_process();

		SYSTEM_EVENTS();
    }
}

DECLARE_APPLICATION_SECTION( app_main );
