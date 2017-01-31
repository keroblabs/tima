#include "system.h"
#include "device.h"
#include "types.h"

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

static uint32_t console_output( uint32_t index, char * text, uint32_t size )
{
    return size;
}

static uint32_t console_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	return 0;
}

void console_drv_close( uint32_t index )
{
}

void console_drv_init( uint32_t index )
{
}

static bool_t validate( uint32_t index )
{
    if( ( index >= 90 ) && ( index <= 99 ) ) return TRUE;
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
