#include "system.h"

#include "regsrtc.h"
#include "regsdigctl.h"

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

static time_t ms_timer_get_secs( void )
{
	uint32_t ret = HW_RTC_SECONDS_RD();
	return ( time_t )ret;
}

static void ms_timer_set_secs( time_t secs)
{
	HW_RTC_SECONDS_SET( ( uint32_t )secs );
}

static void rtc_driver_init( uint32_t index )
{
}

static void rtc_driver_close( uint32_t index )
{
}

static uint32_t rtc_driver_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
    time_t * p_value = ( time_t * )buffer;
    
    if( size >= sizeof( time_t ) )
    {
    	*p_value = ms_timer_get_secs();
    }
    
    return sizeof( time_t );
}

static uint32_t rtc_driver_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
    time_t * p_value = ( time_t * )buffer;
    
    if( size >= sizeof( time_t ) )
    {
    	ms_timer_set_secs( *p_value );
    }
    
    return sizeof( time_t );
}

///////////////////////////////////////////////////////////////////////////

static bool_t rtc_validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t rtc_device =
{
    "rtc*",
    &_in_use,

    rtc_driver_init,
    rtc_driver_close,
    NULL,
    rtc_validate,
    NULL,
    NULL,
    NULL,

    DEV_MODE_CHANNEL,

    rtc_driver_input,
    rtc_driver_output,

};

DECLARE_DEVICE_SECTION( rtc_device );



