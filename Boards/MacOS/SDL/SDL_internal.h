#ifndef BOARDS_MACOS_SDL_SDL_INTERNAL_H_
#define BOARDS_MACOS_SDL_SDL_INTERNAL_H_

////////////////////////////////////////////////////////////////////

#include "types.h"

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

#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>


#include "pipe.h"

////////////////////////////////////////////////////////////////////

#define MAX_INSTANCES           10

#define SDL_INTERNAL_DELEGATE   1

////////////////////////////////////////////////////////////////////

typedef void ( *sdl_delegate_t )( void * data );

////////////////////////////////////////////////////////////////////

typedef struct thread_list_t_
{
    SDL_Thread * thread;
    SDL_threadID id;
    
    uint8_t current_channel;
    pipe_data_t send_pipe;
    pipe_data_t receive_pipe;
    bool_t last_tx_good;
    
    const void * rom_data;
    
} instance_data_t;

////////////////////////////////////////////////////////////////////

void sdl_instance_init( void );
int sdl_get_thread_index( void );

instance_data_t * sdl_get_current_instance( void );
instance_data_t * sdl_get_instance_from_index( int index );
instance_data_t * sdl_alloc_instance( void );

void sdl_mouse_init( void );
void sdl_mouse_event( uint32_t type, SDL_MouseButtonEvent event );
int sdl_read_mouse( uint16_t * posx, uint16_t * posy, uint16_t * posz );

void sdl_text_input_event( uint8_t keycode );
void sdl_keypad_down_event( SDL_Keycode sym );
int sdl_library_kbhit( void );
void sdl_input_event( void );

void sdl_vsync_event( void );
void sdl_lcd_stop( void );
void sdl_lcd_init( void );

void sdl_instance_send_message( int message_code, void * data1, void * data2 );

////////////////////////////////////////////////////////////////////

#endif /* BOARDS_MACOS_SDL_SDL_INTERNAL_H_ */
