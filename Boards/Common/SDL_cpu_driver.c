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

#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

#define SYSTEM_HEAP_SIZE        (256*1024)

uint8_t system_heap[SYSTEM_HEAP_SIZE];
uint32_t system_heap_size = SYSTEM_HEAP_SIZE;

static pthread_mutex_t cpu_mutex;

int _fe_main( void * args );

////////////////////////////////////////////////////////////////////////////////

static void cpu_init( void )
{
    pthread_mutex_init( &cpu_mutex, NULL );
}

#ifdef WANT_MULTI_INSTANCES
static void sdl_start_app_thread( void * data )
{
    SDL_Thread *thread;
    int         threadReturnValue;

    printf("Simple SDL_CreateThread test:\n");

    /* Simply create a thread */
    thread = SDL_CreateThread(_fe_main, "TestThread", (void *)data);

    if (NULL == thread)
    {
        printf("SDL_CreateThread failed: %s\n", SDL_GetError());
    }
    else
    {
        SDL_WaitThread(thread, &threadReturnValue);
        printf("Thread returned value: %d\n", threadReturnValue);
    }
}
#endif

#if defined __MACH__
int SDL_main( int argc, char* argv[] )
#else
int main( int argc, char* argv[] )
#endif
{
#ifdef WANT_MULTI_INSTANCES
    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        return 0;
    }
    
    sdl_start_app_thread(NULL);
#else
    _fe_main(NULL);
#endif
    return 0;
}

void _cpu_disable_irq( void )
{
    pthread_mutex_lock( &cpu_mutex );
}

void _cpu_enable_irq( void )
{
    pthread_mutex_unlock( &cpu_mutex );
}

void _cpu_pio( bool_t state )
{
}

void CPU_pio_reset( bool_t state )
{
}

DECLARE_INIT_CRITICAL_SECTION( cpu_init );
