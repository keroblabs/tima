#include "SDL_internal.h"
#include "system.h"

////////////////////////////////////////////////////////////////////

int _fe_main( void * args );

////////////////////////////////////////////////////////////////////

static Uint32 user_defined_type;

static SDL_mutex *user_signal_lock;
static SDL_cond  *user_signal_cond;
static bool_t user_signal_flag;

////////////////////////////////////////////////////////////////////

static void sdl_event_init( void )
{
    user_defined_type = SDL_RegisterEvents(1);
    
    user_signal_lock = SDL_CreateMutex();
    user_signal_cond = SDL_CreateCond();
}

static void run_delegate( void * data1, void * data2 )
{
    sdl_delegate_t delegate = ( sdl_delegate_t )data1;
    delegate( data2 );
}

static int sdl_event_loop( void )
{
    //Event handler
    SDL_Event e;
    
    // console keyboard
    sdl_input_event();
    
    // vsync
    sdl_vsync_event();
    
    //Handle events on queue
    while( SDL_PollEvent( &e ) != 0 )
    {
        if( e.type == user_defined_type )
        {
            SDL_LockMutex( user_signal_lock );
            
            switch( e.user.code )
            {
                case SDL_INTERNAL_DELEGATE:
                    run_delegate( e.user.data1, e.user.data2 );
                    break;
            }
            
            user_signal_flag = TRUE;
            SDL_CondSignal( user_signal_cond );
            SDL_UnlockMutex( user_signal_lock );
        }
        else
        {
            switch( e.type )
            {
                case SDL_TEXTINPUT:
                    sdl_text_input_event( ( uint8_t )e.text.text[0] );
                    break;
                    
                case SDL_KEYDOWN:
                    sdl_keypad_down_event( e.key.keysym.sym );
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION:
                    sdl_mouse_event( e.type, e.button );
                    break;
                    
                case SDL_QUIT:
                    System_PowerDown();
                    break;
            }
        }
    }
    
    return 1;
}

static void cpu_send_frame_trigger( void * p_pipe )
{
    int cnt;
    
    instance_data_t * curr_instance = sdl_get_current_instance();
    if( curr_instance == NULL ) return;
    
    uint8_t * frame;
    uint16_t size;
    
    pipe_data_t * pipe = ( pipe_data_t * )p_pipe;
    frame = pipe_read( pipe, &size );
    
    for( cnt = 0; cnt < MAX_INSTANCES; cnt++ )
    {
        instance_data_t * instance = sdl_get_instance_from_index( cnt );
        if( instance != NULL )
        {
            if( ( instance->current_channel == curr_instance->current_channel ) && ( instance->id != curr_instance->id ) )
            {
                pipe_send_buffer( &instance->send_pipe, frame, size );
                
                curr_instance->last_tx_good = TRUE;
            }
        }
    }
}

static SDL_Thread * sdl_start_app_thread( void * data )
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

void sdl_instance_send_message( int message_code, void * data1, void * data2 )
{
    if (user_defined_type != ((Uint32)-1))
    {
        SDL_Event event;
        SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */
        event.type = user_defined_type;
        event.user.code = message_code;
        event.user.data1 = data1;
        event.user.data2 = data2;
        SDL_PushEvent(&event);
        
        user_signal_flag = FALSE;
        SDL_LockMutex( user_signal_lock );
        while( user_signal_flag == FALSE )
        {
            SDL_CondWait( user_signal_cond, user_signal_lock );
        }
        SDL_UnlockMutex( user_signal_lock );
    }
}

#if defined __MACH__
int SDL_main( int argc, char* argv[] )
#else
int main( int argc, char* argv[] )
#endif
{
#ifdef WANT_MULTI_INSTANCES
    sdl_instance_init();
    sdl_mouse_init();
    sdl_event_init();
    sdl_lcd_init();
    
    for( int i = 0; i < MAX_INSTANCES; i++ )
    {
        instance_data_t * instance = sdl_alloc_instance();
        if( instance == NULL ) break;
        
        instance->thread   = sdl_start_app_thread( NULL );
        instance->id       = SDL_GetThreadID( instance->thread );
        
        pipe_init( &instance->send_pipe,    "SEND_PIPE", 16 );
        pipe_init( &instance->receive_pipe, "RECEIVE_PIPE", 16 );
        
        pipe_set_tx_trigger( &instance->send_pipe, cpu_send_frame_trigger );
        pipe_set_tx_trigger( &instance->receive_pipe, cpu_send_frame_trigger );
    }
    
    while( 1 )
    {
        sdl_event_loop();
    }
    /*
     if( thread_list[0].thread != NULL )
     {
     SDL_WaitThread( thread_list[0].thread, &status );
     }
     */
#else
    _fe_main(NULL);
#endif
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
