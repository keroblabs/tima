#include "timer.h"
#include "system.h"

///////////////////////////////////////////////////////////////////////////////////////

static device_t * ms_timer_device_hdl;
static device_t * rtc_device_hdl;

///////////////////////////////////////////////////////////////////////////////////////

#define _MS_TIMER ( timer_get_MS() )

///////////////////////////////////////////////////////////////////////////////////////

time_t timer_Now( void )
{
	time_t ret = 0;
    if( rtc_device_hdl != NULL )
	{
		device_read_buffer( rtc_device_hdl, ( uint8_t * )&ret, sizeof( ret ) );
	}
	return ret;
}

void timer_SetRTC( const time_t time )
{
	if( rtc_device_hdl == NULL ) return;
	device_write_buffer( rtc_device_hdl, ( uint8_t * )&time, sizeof( time ) );
}

uint64_t timer_get_US( void )
{
    uint64_t val = 0;

    if( ms_timer_device_hdl != NULL )
    {
        device_read_buffer( ms_timer_device_hdl, ( uint8_t * )&val, sizeof( val ) );
    }
    return val;
}

uint32_t timer_get_MS( void )
{
    uint32_t val = 0;

	if( ms_timer_device_hdl != NULL )
	{
		device_read_buffer( ms_timer_device_hdl, ( uint8_t * )&val, sizeof( val ) );
	}
    return val;
}

void timer_Init( void )
{
	ms_timer_device_hdl = device_open( "/dev/timer0" );
    rtc_device_hdl = device_open( "/dev/rtc0" );
}

void timer_Start( timer_data_t * timer_entry, uint32_t period )
{
	uint32_t curr_ms = _MS_TIMER;

	timer_entry->reload_ms = period;
    timer_entry->expire_ms = curr_ms + period;
    timer_entry->status = TIMER_INIT | TIMER_RUN;
}

void timer_Reload( timer_data_t * timer_entry )
{
	timer_Start( timer_entry, timer_entry->reload_ms );
}

void timer_Stop( timer_data_t * timer_entry )
{
    timer_entry->status = TIMER_STOPPED;
}

void timer_Expire( timer_data_t * timer_entry )
{
    timer_entry->status = TIMER_EXPIRED;
}

bool_t timer_Check( timer_data_t * timer_entry )
{
	uint32_t curr_ms = _MS_TIMER;

	if( timer_entry->status & TIMER_STOPPED )       return( FALSE );
	else if( timer_entry->status & TIMER_EXPIRED )	return( TRUE );

    if( timer_entry->expire_ms < curr_ms )
    {
		if( ( curr_ms - timer_entry->expire_ms ) < 0x7FFFFFFF ) return( TRUE );
    }
	else if( timer_entry->expire_ms > curr_ms )
	{
		if( ( timer_entry->expire_ms - curr_ms ) > 0x7FFFFFFF ) return( TRUE );
	}

    return( FALSE );
}

uint32_t timer_Get_Counter( timer_data_t * timer_entry )
{
	uint32_t curr_ms = _MS_TIMER;

	if( timer_entry->expire_ms <= curr_ms )
	{
		if( ( curr_ms - timer_entry->expire_ms ) > 0x7FFFFFFF )
		{
			return( ( 0xFFFFFFFF - curr_ms ) + timer_entry->expire_ms ) + 1;
		}
	}
	else
	{
		if( ( timer_entry->expire_ms - curr_ms ) < 0x7FFFFFFF )
		{
			return( timer_entry->expire_ms - curr_ms );
		}
	}

	return( 0L );

}

///////////////////////////////////////////////////////////////////////////////////////

DECLARE_INIT_CRITICAL_SECTION( timer_Init );
