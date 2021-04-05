#include "system.h"
#include "debug.h"
#include "tima_libc.h"
#include "device.h"
#include "timer.h"
#include "graphics.h"
#include "draw.h"
#include "config_reader.h"
#include "fsm.h"

/////////////////////////////////////////////////////////////////////////////////////////

typedef struct config_data_t_
{
    uint16_t node_id;
    uint16_t signal_level;
    const char node_name[32];
    
} config_data_t;

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void app_main_thread_fsm( void * data, fsm_signal_t * signal )
{
    
}

void app_main( void )
{
    config_data_t config;
    
    config_reader_load( "/rom", ( uint8_t * )&config, sizeof( config ) );

    fsm_global_init( app_main_thread_fsm, NULL );
    
    graphics_init();
    draw_line( 0, 10, 100, 10, APP_RGB(255, 255, 255));
    
	DEBUG_PRINTK( "Console test %d\r\n", config.node_id );
    
    system_start_sub_applications( TIMA_SUB_APPLICATION_FSM );
    
    while( 1 )
    {
        fsm_process();
        
        SYSTEM_EVENTS();
    }
}

DECLARE_APPLICATION_SECTION( app_main );

