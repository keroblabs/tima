#ifdef WIN32
#include <SDL.h>
#include <Windows.h>
#include <conio.h>
#elif defined __MACH__
#include <SDL2/SDL.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#endif

#include "system.h"
#include "pipe.h"

#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

#define SYSTEM_HEAP_SIZE        (256*1024)
#define MAX_INSTANCES           10
#define _WANT_MULTI_INSTANCES

uint8_t system_heap[SYSTEM_HEAP_SIZE];
uint32_t system_heap_size = SYSTEM_HEAP_SIZE;

////////////////////////////////////////////////////////////////////////////////

typedef struct thread_list_t_
{
    SDL_Thread * thread;
    SDL_threadID id;
    
    uint8_t current_channel;
    pipe_data_t send_pipe;
    pipe_data_t receive_pipe;
    bool_t last_tx_good;
    
} thread_list_t;


typedef struct instance_data_t_
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
    { 12224, 3,  50, "node 03" },
    { 12225, 4, 200, "node 04" },
    {     0, 0,   0, ""        }
};

////////////////////////////////////////////////////////////////////////////////

static pthread_mutex_t cpu_mutex;
static thread_list_t thread_list[10];

////////////////////////////////////////////////////////////////////////////////

int _fe_main( void * args );

////////////////////////////////////////////////////////////////////////////////

static void cpu_init( void )
{
    pthread_mutex_init( &cpu_mutex, NULL );
}

void _cpu_disable_irq( void )
{
    pthread_mutex_lock( &cpu_mutex );
}

void _cpu_enable_irq( void )
{
    pthread_mutex_unlock( &cpu_mutex );
}

DECLARE_INIT_CRITICAL_SECTION( cpu_init );

////////////////////////////////////////////////////////////////////////////////

static int cpu_get_id( void )
{
    SDL_threadID thread_id = SDL_GetThreadID( NULL );
    
    for( int i = 0; i < MAX_INSTANCES; i++ )
    {
        if( thread_list[i].thread == NULL ) break;
        if( thread_list[i].id == thread_id ) return i;
    }
    
    return -1;
}

#ifdef WANT_MULTI_INSTANCES
static void cpu_send_frame_trigger( void * p_pipe )
{
    int cnt;
    
    int curr_id = cpu_get_id();
    if( curr_id == -1 ) return;
    
    uint8_t * frame;
    uint16_t size;
    
    pipe_data_t * pipe = ( pipe_data_t * )p_pipe;
    frame = pipe_read( pipe, &size );
    
    for( cnt = 0; cnt < MAX_INSTANCES; cnt++ )
    {
        if( thread_list[cnt].thread == NULL ) break;
        
        if( ( thread_list[cnt].id != curr_id ) && ( thread_list[cnt].current_channel == thread_list[curr_id].current_channel ) )
        {
            pipe_send_buffer( &thread_list[cnt].send_pipe, frame, size );

            thread_list[curr_id].last_tx_good = TRUE;
        }
    }
}

static SDL_Thread* sdl_start_app_thread( void * data )
{
    SDL_Thread *thread;
    SDL_threadID thread_id;

    printf("Simple SDL_CreateThread test:\n");

    /* Simply create a thread */
    thread = SDL_CreateThread(_fe_main, "TestThread", (void *)data);
    thread_id = SDL_GetThreadID( thread );

    if (NULL == thread)
    {
        printf("SDL_CreateThread failed: %s\n", SDL_GetError());
    }
    
    return thread;
}
#endif

#if defined __MACH__
int SDL_main( int argc, char* argv[] )
#else
int main( int argc, char* argv[] )
#endif
{
#ifdef WANT_MULTI_INSTANCES
    int status;

    memset( thread_list, 0x00, sizeof( thread_list ) );
    
    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        return 0;
    }
    
    for( int i = 0; i < MAX_INSTANCES; i++ )
    {
        if( rom_data[i].socket_port == 0 ) break;
        
        thread_list[i].thread   = sdl_start_app_thread( NULL );
        thread_list[i].id       = SDL_GetThreadID( thread_list[i].thread );
        
        pipe_init( &thread_list[i].send_pipe, "SEND_PIPE", 16 );
        pipe_init( &thread_list[i].receive_pipe, "RECEIVE_PIPE", 16 );
        
        pipe_set_tx_trigger( &thread_list[i].send_pipe, cpu_send_frame_trigger );
        pipe_set_tx_trigger( &thread_list[i].receive_pipe, cpu_send_frame_trigger );
    }
    
    if( thread_list[0].thread != NULL )
    {
        SDL_WaitThread( thread_list[0].thread, &status );
    }
#else
    _fe_main(NULL);
#endif
    return 0;
}

uint8_t * cpu_get_rom_data( uint16_t * size )
{
    SDL_threadID thread_id = SDL_GetThreadID( NULL );
    
    for( int i = 0; i < MAX_INSTANCES; i++ )
    {
        if( thread_list[i].thread == NULL ) break;
        if( thread_list[i].id == thread_id )
        {
            *size = ( uint16_t )sizeof( rom_data_t );
            return ( uint8_t * )&rom_data[i];
        }
    }
    
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

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
    int curr_id = cpu_get_id();
    if( curr_id == -1 ) return NULL;

    return &thread_list[curr_id].send_pipe;
}

static pipe_data_t * mac_get_tx_pipe( uint32_t index )
{
    int curr_id = cpu_get_id();
    if( curr_id == -1 ) return NULL;
    
    return &thread_list[curr_id].receive_pipe;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const device_data_t can_bus_device =
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

DECLARE_DEVICE_SECTION( can_bus_device );
