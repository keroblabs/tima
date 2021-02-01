#include "filesystem_client.h"
#include "tima_link_data.h"
#include "datalink.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

static datalink_node_t tima_node;

////////////////////////////////////////////////////////////////////

void tima_link_filesystem_process( void )
{
    uint8_t * buffer;
    uint16_t size;

    buffer = datalink_waitfor_node( &tima_node, &size );
    if( buffer != NULL )
    {
        DEBUG_PRINTK( "-> 0x%02x, %d\r\n", buffer[0], buffer[1] );
        MFREE( buffer );
    }
}

void tima_link_filesystem_client_init( void )
{
    device_t * p_device = ( device_t * )tima_link_get_device();
    datalink_setup_node( &tima_node, ( device_t * )p_device, TIMA_INSTRUMENTS_FILESYSTEM_CLIENT );
}

void tima_link_filesystem_transaction_open_file( const char * filename, const char * mode )
{
    int pos;
    uint16_t size = strlen( filename ) + strlen( mode ) + 3;
    uint8_t * buffer = ( uint8_t * )MMALLOC( size );
    if( buffer == NULL ) return;
    memset( buffer, 0x00, size );

    pos = 0;

    buffer[pos++] = TIMA_LINK_FILESYSTEM_OPEN;
    memcpy( &buffer[pos], filename, strlen( filename ) );
    pos += strlen( filename ) + 1;

    memcpy( &buffer[pos], mode, strlen( mode ) );

    datalink_send_data( &tima_node, TIMA_INSTRUMENTS_FILESYSTEM_SERVER, buffer, size );
}

void tima_link_filesystem_checkout( void )
{

}
