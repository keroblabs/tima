#include "system.h"
#include "types.h"
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////

static uint32_t console_output( uint32_t index, char * text, uint32_t size )
{
	printf( text );

    return size;
}

static void console_drv_init( uint32_t index )
{
}

static void console_drv_close( uint32_t index )
{
}

static bool_t validate( uint32_t index )
{
    if( index == 3 ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t console_device = 
{
    "tty*",
    &_in_use,
    
    console_drv_init,
    console_drv_close,
    NULL,
    validate,
    NULL,
    NULL,
	NULL,
    
    DEV_MODE_CHANNEL,
    
    console_input,
    console_output,
    
};

DECLARE_DEVICE_SECTION( console_device );
