#include "system.h"
#include "timer.h"

#include "gem_simulator.h"
#include "elm327_simulator.h"
#include "syscomm.h"
#include "debug.h"

#define ENABLE_GEM_SIMULATOR
#define ENABLE_ELM327_SIMULATOR

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

static uint32_t nvram_size = 256;
static uint8_t local_nvram[256];
static uint32_t odometer_value;

/////////////////////////////////////////////////////////////////////////////////////////

uint32_t app_odometer_value( void )
{
    return odometer_value;
}

void nvram_control_read( uint32_t address, void * data, uint32_t size )
{
    uint32_t cnt;
    uint8_t * p_data = ( uint8_t * )data;
    
    if( nvram_size == 0 ) return;
    if( size == 0 ) return;
    
    for( cnt = address; cnt < nvram_size; cnt++ )
    {
        *p_data = local_nvram[cnt];
        p_data++;
        if( --size == 0 ) break;
    }
}

void nvram_control_write( uint32_t address, void * data, uint32_t size )
{
    uint32_t cnt;
    uint8_t * p_data = ( uint8_t * )data;
    
    if( nvram_size == 0 ) return;
    if( size == 0 ) return;
    
    for( cnt = address; cnt < nvram_size; cnt++ )
    {
        local_nvram[cnt] = *p_data;
        p_data++;
        if( --size == 0 ) break;
    }
}

void app_main( void )
{
    timer_data_t timer_odometer;
    timer_data_t syscomm_timer;
    bool_t is_syscomm;
    
    is_syscomm = FALSE;
    odometer_value = 0;
    timer_Start( &timer_odometer, 500 );
    timer_Start( &syscomm_timer, 5000 );
    
    memset( local_nvram, 0x00, sizeof( local_nvram ) );
    
    #ifdef ENABLE_GEM_SIMULATOR
    //DEBUG_PRINTK( "GEM Simulator\r\n" );
    //syscomm_init();
    #endif
    
    #ifdef ENABLE_ELM327_SIMULATOR
    DEBUG_PRINTK( "ELM327 Simulator\r\n" );
    elm327_simulator_init();
    #endif
    
    while( 1 )
    {
        if( timer_Check( &timer_odometer ) )
        {
            timer_Reload( &timer_odometer );
            odometer_value += 100;
        }
        
        #ifdef ENABLE_GEM_SIMULATOR
        if( timer_Check( &syscomm_timer ) )
        {
            timer_Stop( &syscomm_timer );
            is_syscomm = TRUE;
  
            DEBUG_PRINTK( "GEM Simulator\r\n" );
            syscomm_init();
        }
        #endif
        
        #ifdef ENABLE_GEM_SIMULATOR
        if( is_syscomm == TRUE )
            syscomm_process();
        #endif
        
        #ifdef ENABLE_ELM327_SIMULATOR
        elm327_simulator_process();
        #endif
        
        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main )
