#include "system.h"

#include "gem_simulator.h"
#include "elm327_simulator.h"

#include "debug.h"

//#define ENABLE_GEM_SIMULATOR
#define ENABLE_ELM327_SIMULATOR

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void app_main( void )
{
    #ifdef ENABLE_GEM_SIMULATOR
    DEBUG_PRINTK( "GEM Simulator\r\n" );
    gem_simulator_init();
    #endif
    
    #ifdef ENABLE_ELM327_SIMULATOR
    DEBUG_PRINTK( "ELM327 Simulator\r\n" );
    elm327_simulator_init();
    #endif
    
    while( 1 )
    {
        #ifdef ENABLE_GEM_SIMULATOR
        gem_simulator_process();
        #endif
        
        #ifdef ENABLE_ELM327_SIMULATOR
        elm327_simulator_process();
        #endif
        
        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main )
