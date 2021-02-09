#include "SDL_internal.h"
#include "device.h"
#include "system.h"

////////////////////////////////////////////////////////////////////

typedef struct rom_data_t_
{
    uint16_t socket_port;
    uint16_t node_id;
    uint16_t signal_level;
    const char node_name[32];
    
} rom_data_t;

static const rom_data_t rom_data[] =
{
    { 12222, 1, 200, "node 01" },
    { 12223, 2, 100, "node 02" },
    //{ 12224, 3,  50, "node 03" },
    //{ 12225, 4, 200, "node 04" },
    {     0, 0,   0, ""        }
};

////////////////////////////////////////////////////////////////////

static instance_data_t instance_list[MAX_INSTANCES];

int _fe_main( void * args );

////////////////////////////////////////////////////////////////////

void sdl_instance_init( void )
{
    memset( instance_list, 0x00, sizeof( instance_list ) );
}

int sdl_get_thread_index( void )
{
    SDL_threadID thread_id = SDL_ThreadID();
    
    for( int i = 0; i < MAX_INSTANCES; i++ )
    {
        if( instance_list[i].thread == NULL ) break;
        if( instance_list[i].id == thread_id ) return i;
    }
    
    return -1;
}

window_data_t * sdl_get_window_data( void )
{
    instance_data_t * data = sdl_get_current_instance();
    if( data == NULL ) return NULL;
    
    return &data->window_data;
}

instance_data_t * sdl_get_current_instance( void )
{
    SDL_threadID thread_id = SDL_ThreadID();
    
    for( int i = 0; i < MAX_INSTANCES; i++ )
    {
        if( instance_list[i].thread == NULL ) break;
        if( instance_list[i].id == thread_id )
        {
            return &instance_list[i];
        }
    }
    
    return NULL;
}

instance_data_t * sdl_get_instance_from_index( int index )
{
    if( instance_list[index].thread == NULL ) return NULL;
    return &instance_list[index];
}

instance_data_t * sdl_alloc_instance( void )
{
    for( int i = 0; i < MAX_INSTANCES; i++ )
    {
        if( rom_data[i].socket_port == 0 ) break;
        
        if( instance_list[i].thread == NULL )
        {
            instance_list[i].rom_data = &rom_data[i];
            return &instance_list[i];
        }
    }
    
    return NULL;
}

const void * sdl_get_rom_data( uint16_t * size )
{
    instance_data_t * data = sdl_get_current_instance();
    if( data == NULL ) return NULL;
    
    if( size ) *size = ( uint16_t )sizeof( rom_data_t );
    return data;
}

