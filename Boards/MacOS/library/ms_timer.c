#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "system.h"
#include "types.h"

uint32_t ms_timer_hw_get_ms( void );

uint32_t ms_timer_get_ms( void )
{
    return  ms_timer_hw_get_ms();
}

static void ms_timer_drv_init( uint32_t index )
{
}

static void ms_timer_drv_close( uint32_t index )
{
}

static uint32_t ms_timer_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
    return 0;
}

static uint32_t ms_timer_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
    if( size >= sizeof( uint32_t ) )
	{
		uint32_t * p_ms_value = ( uint32_t * )buffer;
		*p_ms_value = ms_timer_get_ms();
	}

    return sizeof( uint32_t );
}

////////////////////////////////////////////////////////////////////////

static bool_t ms_timer_validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

static bool_t _in_use;

const static device_data_t ms_timer_device =
{
    "timer*",
    &_in_use,

    ms_timer_drv_init,
    ms_timer_drv_close,
    NULL,
    ms_timer_validate,
    NULL,
    NULL,
    NULL,

    DEV_MODE_CHANNEL,

    ms_timer_drv_input,
    ms_timer_drv_output,

};

DECLARE_DEVICE_SECTION( ms_timer_device );

