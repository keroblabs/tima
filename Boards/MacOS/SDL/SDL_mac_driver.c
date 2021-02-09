#include "SDL_internal.h"
#include "device.h"
#include "system.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

static void mac_drv_init( uint32_t index )
{
}

static void mac_drv_close( uint32_t index )
{
}

static uint32_t mac_drv_ioctl( uint32_t index, uint32_t param, void * value )
{
    return 0;
}

static pipe_data_t * mac_get_rx_pipe( uint32_t index )
{
    instance_data_t * instance = sdl_get_current_instance();
    if( instance == NULL ) return NULL;
    
    return &instance->send_pipe;
}

static pipe_data_t * mac_get_tx_pipe( uint32_t index )
{
    instance_data_t * instance = sdl_get_current_instance();
    if( instance == NULL ) return NULL;

    return &instance->receive_pipe;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const device_data_t mac_device =
{
    "mac",
    &_in_use,
    
    mac_drv_init,
    mac_drv_close,
    NULL,
    NULL,
    NULL,
    mac_drv_ioctl,
    NULL,
    
    DEV_MODE_PIPE,
    
    mac_get_rx_pipe, // can_read_buffer,
    mac_get_tx_pipe, // can_write_buffer,
    
};

DECLARE_DEVICE_SECTION( mac_device );
