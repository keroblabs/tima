#include "system.h"
#include "types.h"

#include "regsrtc.h"
#include "regsdigctl.h"

///////////////////////////////////////////////////////////////

void ms_timer_hw_init( void )
{
}

uint32_t ms_timer_get_ms( void )
{
	uint32_t ret = HW_RTC_MILLISECONDS_RD();
	return ret;
}

uint32_t ms_timer_get_us( void )
{
	uint32_t ret = HW_DIGCTL_MICROSECONDS_RD();
	return ret;
}

uint32_t ms_timer_get_secs( void )
{
	uint32_t ret = HW_RTC_SECONDS_RD();
	return ret;
}

void ms_timer_set_ms( uint32_t ms )
{
	HW_RTC_MILLISECONDS_SET( ms );
}

void ms_timer_set_us( uint32_t ms )
{
	HW_DIGCTL_MICROSECONDS_SET( ms );
}

void ms_timer_set_secs( uint32_t secs)
{
	HW_RTC_SECONDS_SET( secs );
}


///////////////////////////////////////////////////////////////

void ms_timer_drv_init( uint32_t index )
{
}

void ms_timer_drv_close( uint32_t index )
{
}

uint32_t ms_timer_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
    return 0;
}

uint32_t ms_timer_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
    if( size >= sizeof( uint32_t ) )
	{
		uint32_t * p_ms_value = ( uint32_t * )buffer;
    	if( index == 0 ) *p_ms_value = ms_timer_get_ms();
    	else if( index == 1 ) *p_ms_value = ms_timer_get_us();
    	else if( index == 2 ) *p_ms_value = ms_timer_get_secs();
	}

    return sizeof( uint32_t );
}

static uint32_t ms_timer_ioctrl( uint32_t index, uint32_t param, void * value )
{
	uint32_t * p_ms_value = ( uint32_t * )value;

	if( param == DEVICE_SET_RUNTIME_VALUE )
	{
    	if( index == 0 ) ms_timer_set_ms( *p_ms_value );
    	else if( index == 1 ) ms_timer_set_us( *p_ms_value );
    	else if( index == 2 ) ms_timer_set_secs( *p_ms_value );
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////

static bool_t ms_timer_validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    if( index == 1 ) return TRUE;
    if( index == 2 ) return TRUE;
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
    ms_timer_ioctrl,
    NULL,

    DEV_MODE_CHANNEL,

    ms_timer_drv_input,
    ms_timer_drv_output,

};

DECLARE_DEVICE_SECTION( ms_timer_device );
