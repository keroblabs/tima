#include "system.h"
#include "tima_libc.h"

#ifndef DEV_PRINTK
#define DEV_PRINTK                "/dev/tty90"
#endif

///////////////////////////////////////////////////////////////

static device_t * dev_debug;

///////////////////////////////////////////////////////////////


void debug_init( void )
{
	uint32_t baud = 115200;
    dev_debug = NULL;
    dev_debug = device_open( DEV_PRINTK );

    if( dev_debug != NULL )
    	device_ioctrl( dev_debug, DEVICE_SET_CONFIG_DATA, &baud );
}

int debug_printk( const char *fmt, ... )
{
    va_list args;
    int printed_len;
    if( dev_debug == NULL ) return 0;
    
    /* Emit the output into the temporary buffer */
    va_start(args, fmt);
    printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args);
    va_end(args);
    
    return device_write_buffer( dev_debug, ( uint8_t * )vsf_printk_buf, printed_len );
}

bool_t debug_read_input( char * out )
{
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


