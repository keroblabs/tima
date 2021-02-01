#include "system.h"
#include "tima_link.h"
#include "tima_link_data.h"
#include "datalink.h"

////////////////////////////////////////////////////////////////////

#define TIMA_LINK_TEST_MESSAGE ( uint8_t * )"Tima Link\r\n"

////////////////////////////////////////////////////////////////////

static device_t * tima_device;
static datalink_t tima_link;
static bool_t is_init = FALSE;

////////////////////////////////////////////////////////////////////

void * tima_link_get_device( void )
{
    if( is_init == FALSE )
    {
        uint32_t baud = 115200;
        tima_device = NULL;
        tima_device = device_open( "/dev/tty3" );

        if( tima_device != NULL )
            device_ioctrl( tima_device, DEVICE_SET_CONFIG_DATA, &baud );

        datalink_init( &tima_link, tima_device, TIMA_LINK_HEADER, TIMA_LINK_FOOTER, TIMA_LINK_CRC );
        is_init = TRUE;
    }

    return ( tima_device != NULL ) ? tima_device : NULL;
}

