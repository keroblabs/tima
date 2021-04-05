#include "system.h"
#include "tima_libc.h"

///////////////////////////////////////////////////////////////

#ifdef WANT_MULTI_INSTANCES
#include "SDL_internal.h"
#else
static device_t __dev_debug;
#endif

///////////////////////////////////////////////////////////////

static device_t * get_dev_debug( void )
{
#ifdef WANT_MULTI_INSTANCES
    instance_data_t * data = sdl_get_current_instance();
    return ( data != NULL ) ? &data->dev_printf : NULL;
#else
    return &__dev_debug;
#endif
}


void debug_init( void )
{
    device_t * dev_debug = get_dev_debug();
	uint32_t baud = 115200;

    if( device_open_ex( DEV_PRINTK, dev_debug ) == TRUE )
    {
    	device_ioctrl( dev_debug, DEVICE_SET_CONFIG_DATA, &baud );
    }
}

int debug_printk( const char *fmt, ... )
{
    va_list args;
    int printed_len;
    device_t * dev_debug = get_dev_debug();

    if( dev_debug == NULL ) return 0;
    
    /* Emit the output into the temporary buffer */
    va_start(args, fmt);
    printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args);
    va_end(args);
    
    return device_write_buffer( dev_debug, ( uint8_t * )vsf_printk_buf, printed_len );
}

bool_t debug_read_input( char * out )
{
    device_t * dev_debug = get_dev_debug();

    if( dev_debug == NULL ) return FALSE;
	if( device_is_rx_pending( dev_debug ) )
	{
		device_read_buffer( dev_debug, ( uint8_t * )out, 1 );
		return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////

DECLARE_INIT_DEBUG_SECTION( debug_init );


