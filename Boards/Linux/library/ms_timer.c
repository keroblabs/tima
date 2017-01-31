#include "system.h"
#include "types.h"

#include <sys/time.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

static bool_t is_init = FALSE;

void ms_timer_hw_init( void )
{
	if( is_init == TRUE ) return;
	is_init = TRUE;

    gettimeofday( &ms_start, NULL );
}

uint32_t ms_timer_get_ms( void )
{
    struct timeval ms_value;
    long mtime, seconds, useconds;
    
    gettimeofday( &ms_value, NULL );
    
    seconds = ms_value.tv_sec - ms_start.tv_sec;
    useconds = ms_value.tv_usec - ms_start.tv_usec;
    mtime = ( seconds * 1000 ) + ( useconds / 1000 );
    
    return ( uint32_t )mtime;
}

///////////////////////////////////////////////////////////////////////////

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
		*p_ms_value = ms_timer_get_ms();
	}
    
    return sizeof( uint32_t );
}

////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

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
