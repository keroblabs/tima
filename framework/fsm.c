#include "fsm.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

#ifdef WANT_MULTI_INSTANCES
#include "SDL_internal.h"
#else
static fsm_instance_t _local_fsm_instance;
#endif

///////////////////////////////////////////////////////////////

static fsm_instance_t * get_instance( void )
{
#ifdef WANT_MULTI_INSTANCES
    instance_data_t * data = sdl_get_current_instance();
    return ( data != NULL ) ? &data->fsm_instance : NULL;
#else
    return &_local_fsm_instance;
#endif
}

static const fsm_state_t * fsm_get_states( const fsm_state_t * fsm_state, uint32_t state_count, uint32_t state_id )
{
    uint32_t count;
    
    for( count = 0; count < state_count; count++ )
    {
        if( fsm_state[count].state_id == state_id ) return &fsm_state[count];
    }
    
    return NULL;
}

static const fsm_transition_t * fsm_get_transition( const fsm_transition_t * fsm_transition, uint32_t transition_count, uint32_t signal_id )
{
    uint32_t count;
    
    for( count = 0; count < transition_count; count++ )
    {
        if( ( fsm_transition[count].signal_id == _ANY_SIGNAL ) || ( fsm_transition[count].signal_id == signal_id ) ) return &fsm_transition[count];
    }
    
    return NULL;
}

static const fsm_state_t * fsm_get_state_any( const fsm_state_t * fsm_state, uint32_t state_count, uint32_t signal_id )
{
    uint32_t count;
    
    for( count = 0; count < state_count; count++ )
    {
        if( fsm_get_transition( fsm_state[count].transitions, fsm_state[count].count, signal_id ) )
        {
            return &fsm_state[count];
        }
    }
    
    return NULL;
}

static fsm_process_t * fsm_from_pid( fsm_instance_t * instance, uint32_t pid )
{
    fsm_process_t * search = NULL;
    if( ( pid == INVALID_PID ) || ( pid == MAIN_THREAD_PID ) ) return NULL;
    
    while( ( search = list_get_next( &instance->processes, search ? &search->node : NULL ) ) != NULL )
    {
        if( search->pid == pid ) return search;
    }
    
    return NULL;
}

static void delete_signal( list_node_t * node )
{
    fsm_signal_t * signal = ( fsm_signal_t * )node;
    if( signal->data ) MFREE( signal->data );
    MFREE( signal );
}

static void delete_process( list_node_t * node )
{
    fsm_process_t * process = ( fsm_process_t * )node;
    list_clear( &process->saved_signals );
    MFREE( process );
}

static void fsm_timer_check( fsm_instance_t * instance )
{
    fsm_timer_t * search = NULL;
    
    while( ( search = ( fsm_timer_t * )list_get_next( &instance->timers, search ? &search->node : NULL ) ) != NULL )
    {
        if( timer_Check( &search->timer ) == TRUE )
        {
            fsm_signal_t * signal = ( fsm_signal_t * )MMALLOC( sizeof( fsm_signal_t ) );
            if( signal != NULL )
            {
                signal->source = search->pid_creator;
                signal->dest = search->pid_creator;
                signal->signal_id = search->signal_id;
                signal->data = NULL;
                
                fsm_send_signal( search->pid_creator, search->signal_id, signal );
                list_remove( &instance->timers, &search->node );
            }
        }
    }
}

static void fsm_run_background( fsm_instance_t * instance )
{
    fsm_process_t * process = NULL;
    
    while( ( process = ( fsm_process_t * )list_get_next( &instance->processes, process ? &process->node : NULL ) ) != NULL )
    {
        /* find a valid state - guaranteed we will find it */
        const fsm_state_t * fsm_state = fsm_get_states( process->fsm_data->state_list, process->fsm_data->state_count, process->curr_state_id );
        if( fsm_state->background != NULL )
        {
            instance->curr_process = process;
            fsm_state->background( process->data );
            instance->curr_process = NULL;
        }
    }
}

static void fsm_process_exit_check( fsm_instance_t * instance )
{
    fsm_process_t * search = NULL;
    
    while( ( search = ( fsm_process_t * )list_get_next( &instance->processes, search ? &search->node : NULL ) ) != NULL )
    {
        if( search->terminate_pending == TRUE )
        {
            fsm_terminate_process( search->pid );
        }
    }
}

void fsm_set_timer( uint32_t signal_id, uint32_t time_ms )
{
    fsm_instance_t * instance = get_instance();
    fsm_timer_t * fsm_timer;
    
    fsm_timer = list_find_value( &instance->timers, fsm_timer_t, signal_id, sizeof(signal_id), &signal_id );
    if( fsm_timer == NULL )
    {
        fsm_timer = ( fsm_timer_t * )MMALLOC( sizeof( fsm_timer_t ) );
        list_insert_tail( &instance->timers, &fsm_timer->node );
    }
    
    if( fsm_timer != NULL )
    {
        fsm_timer->pid_creator = fsm_self();
        fsm_timer->signal_id   = signal_id;

        timer_Start( &fsm_timer->timer, time_ms );
        DEBUG_PRINTK( "[%8d] fsm_set_timer - pid_source %d, signal_id %d, when %d\r\n", timer_get_MS(), fsm_timer->pid_creator, signal_id, timer_get_MS() + time_ms );
    }
}

void fsm_reset_timer( uint32_t signal_id )
{
    fsm_instance_t * instance = get_instance();
    fsm_timer_t * fsm_timer = list_find_value( &instance->timers, fsm_timer_t, signal_id, sizeof(signal_id), &signal_id );
    if( fsm_timer != NULL )
    {
        DEBUG_PRINTK( "[%8d] fsm_reset_timer - pid_source %d, signal_id %d\r\n", timer_get_MS(), fsm_timer->pid_creator, signal_id );
        list_remove( &instance->timers, &fsm_timer->node );
    }
}

void fsm_send_signal( uint32_t dest, uint32_t signal_id, void * data )
{
    fsm_instance_t * instance = get_instance();
    fsm_process_t * process = instance->curr_process;
    fsm_signal_t * signal = ( fsm_signal_t * )MMALLOC( sizeof( fsm_signal_t ) );
    
    if( signal )
    {
        signal->source      = process ? process->pid : MAIN_THREAD_PID;
        signal->dest        = dest;
        signal->signal_id   = signal_id;
        signal->data        = data;

        DEBUG_PRINTK( "[%8d] fsm_send_signal - pid_source %d, pid_dest = %d, signal_id %d, %s\r\n", timer_get_MS(), signal->source, dest, signal_id, data ? "with_data" : "no_data" );
        list_insert_tail( &instance->signals, &signal->node );
    }
}

void fsm_terminate_process( uint32_t pid )
{
    fsm_instance_t * instance = get_instance();
    fsm_process_t * process = fsm_from_pid( instance, pid );
    
    if( process )
    {
        if( process == instance->curr_process )
        {
            process->terminate_pending = TRUE;
        }
        else
        {
            if( process->fsm_data->exit )
            {
                instance->curr_process = process;
                process->fsm_data->exit( process->data );
                instance->curr_process = NULL;
            }
            
            DEBUG_PRINTK( "[%8d] fsm_terminate_process - pid %d\r\n", timer_get_MS(), process->pid );
            list_remove( &instance->processes, &process->node );
        }
    }
}

void fsm_exit( void )
{
    fsm_instance_t * instance = get_instance();
    fsm_process_t * process = instance->curr_process;

    if( process )
    {
        process->terminate_pending = TRUE;
    }
}

uint32_t fsm_self( void )
{
    fsm_instance_t * instance = get_instance();
    fsm_process_t * process = instance->curr_process;
    
    return ( process != NULL ) ? process->pid : INVALID_PID;
}

uint32_t fsm_pid_from_name( const char * name )
{
    fsm_process_t * search = NULL;
    fsm_instance_t * instance = get_instance();

    while( ( search = list_get_next( &instance->processes, search ? &search->node : NULL ) ) != NULL )
    {
        if( !strcmp( search->fsm_data->name, name ) ) return search->pid;
    }
    
    return INVALID_PID;
}

void * fsm_data_from_name( const char * name )
{
    fsm_process_t * search = NULL;
    fsm_instance_t * instance = get_instance();
    
    while( ( search = list_get_next( &instance->processes, search ? &search->node : NULL ) ) != NULL )
    {
        if( !strcmp( search->fsm_data->name, name ) ) return search->data;
    }
    
    return NULL;
}

void * fsm_data_from_pid( uint32_t pid )
{
    fsm_process_t * search = NULL;
    fsm_instance_t * instance = get_instance();
    
    while( ( search = list_get_next( &instance->processes, search ? &search->node : NULL ) ) != NULL )
    {
        if( search->pid == pid ) return search->data;
    }
    
    return NULL;
}

void fsm_set_state( uint32_t state_id )
{
    fsm_instance_t * instance = get_instance();
    fsm_process_t * process = instance->curr_process;
    
    if( process != NULL )
    {
        const fsm_state_t * fsm_state = fsm_get_states( process->fsm_data->state_list, process->fsm_data->state_count, state_id );
        if( fsm_state != NULL )
        {
            DEBUG_PRINTK( "[%8d] fsm_set_state - pid %d, old_state %d, new_state %d\r\n", timer_get_MS(), process->pid, process->curr_state_id, state_id );
            process->curr_state_id = state_id;
            process->changed_state = TRUE;
        }
    }
}

uint32_t fsm_get_state( void )
{
    uint32_t state = INVALID_STATE;
    fsm_instance_t * instance = get_instance();
    fsm_process_t * process = instance->curr_process;
    
    if( process != NULL )
    {
        state = process->curr_state_id;
    }
    
    return state;
}

void fsm_save_signal_ex( void * data, fsm_signal_t * signal )
{
    fsm_instance_t * instance = get_instance();
    if( instance != NULL )
    {
        fsm_process_t * process = instance->curr_process;
        list_insert_tail( &process->saved_signals, &signal->node );
        DEBUG_PRINTK( "[%8d] fsm_saved - pid %d, dest %d, state_id %d, signal_id %d\r\n", timer_get_MS(), signal->source, signal->dest, process->curr_state_id, signal->signal_id );
    }
}

void fsm_invalid_signal_ex( void * data, fsm_signal_t * signal )
{
    fsm_instance_t * instance = get_instance();
    if( ( instance != NULL ) && ( instance->curr_process != NULL ) && ( instance->curr_process->fsm_data->unhandled != NULL ) )
    {
        fsm_process_t * process = instance->curr_process;
        process->fsm_data->unhandled( process->data, signal );
        DEBUG_PRINTK( "[%8d] fsm_unhandled - pid %d, dest %d, state_id %d, signal_id %d\r\n", timer_get_MS(), signal->source, signal->dest, process->curr_state_id, signal->signal_id );
        delete_signal( &signal->node );
    }
    else
    {
        fsm_unknown_signal_ex( data, signal );
    }
}

void fsm_unknown_signal_ex( void * data, fsm_signal_t * signal )
{
    fsm_instance_t * instance = get_instance();
    if( ( instance != NULL ) && ( instance->curr_process != NULL ) && ( instance->curr_process->fsm_data->unknown != NULL ) )
    {
        fsm_process_t * process = instance->curr_process;
        process->fsm_data->unknown( process->data, signal );
        DEBUG_PRINTK( "[%8d] fsm_unknown - pid %d, dest %d, state_id %d, signal_id %d\r\n", timer_get_MS(), signal->source, signal->dest, process->curr_state_id, signal->signal_id );
        delete_signal( &signal->node );
    }
    else
    {
        delete_signal( &signal->node );
    }
}

uint32_t fsm_start_process( const fsm_init_data_t * fsm_data, void * data )
{
    uint32_t pid;
    fsm_instance_t * instance = get_instance();
    
    if( ( pid = fsm_pid_from_name( fsm_data->name ) ) == INVALID_PID )
    {
        fsm_process_t * new_process = MMALLOC( sizeof( fsm_process_t ) );
        if( new_process == NULL ) return INVALID_PID;
        
        list_init( &new_process->saved_signals, delete_signal );
        new_process->fsm_data      = fsm_data;
        new_process->data          = data;
        new_process->pid           = instance->pid_counter++;
        
        list_insert_tail( &instance->processes, &new_process->node );
        DEBUG_PRINTK( "[%8d] fsm_start_process - pid %d, new_state %d\r\n", timer_get_MS(), new_process->pid, new_process->curr_state_id );

        if( fsm_data->init )
        {
            instance->curr_process = new_process;
            fsm_data->init( data );
            instance->curr_process = NULL;
        }
    }
    
    return pid;
}

void fsm_global_init( fsm_handler_t main_thread, void * data )
{
    fsm_instance_t * instance = get_instance();
    list_init( &instance->processes, delete_process );
    list_init( &instance->signals, delete_signal );
    list_init( &instance->timers, NULL );
    
    instance->main_hander = main_thread;
    instance->main_data = data;
    
    instance->pid_counter = 1;
}

void fsm_process( void )
{
    fsm_instance_t * instance = get_instance();
    if( list_size( &instance->processes ) == 0 ) return;
    
    while( list_size( &instance->signals ) != 0 )
    {
        fsm_timer_check( instance );
        fsm_process_exit_check( instance );
        fsm_run_background( instance );
        
        fsm_signal_t * signal = ( fsm_signal_t * )list_get_head( &instance->signals );
        fsm_process_t * process = NULL;
        
        if( signal->dest == MAIN_THREAD_PID )
        {
            if( instance->main_hander ) instance->main_hander( instance->main_data, signal );
            delete_signal( &signal->node );
        }
        else
        {
            process = ( fsm_process_t * )fsm_from_pid( instance, signal->dest );
        }

        while( process != NULL )
        {
            const fsm_transition_t * transition = NULL;
            const fsm_state_t * fsm_state;
            bool_t can_save = FALSE;
            
            /* find a valid state - guaranteed we will find it */
            fsm_state = fsm_get_states( process->fsm_data->state_list, process->fsm_data->state_count, process->curr_state_id );

            /* Find transition in the current state */
            transition = fsm_get_transition( fsm_state->transitions, fsm_state->count, signal->signal_id );
            if( transition == NULL )
            {
                /* Not in the current state, look in common */
                transition = fsm_get_transition( process->fsm_data->common, process->fsm_data->common_count, signal->signal_id );
            }
            
            if( ( transition == NULL ) && ( fsm_get_state_any( process->fsm_data->state_list, process->fsm_data->state_count, signal->signal_id ) != NULL ) )
            {
                /* not in common, but somewhere else */
                can_save = TRUE;
            }

            instance->curr_process = process;
            
            if( transition )
            {
                /* found the state or in common */
                fsm_handler_t handler = transition->handler;
                DEBUG_PRINTK( "[%8d] fsm_transition - pid %d, dest %d, state_id %d, signal_id %d\r\n", timer_get_MS(), signal->source, signal->dest, process->curr_state_id, signal->signal_id );
                
                handler( process->data, signal );
                delete_signal( &signal->node );
            }
            else if( can_save )
            {
                /* Signal found in some other state, and this state doesn't want to save, go to unhandled, if exists, otherwise unknown  */
                fsm_invalid_signal_ex( NULL, signal );
            }
            else
            {
                /* not found anywere, it's unknown */
                fsm_unknown_signal_ex( NULL, signal );
            }
            
            instance->curr_process = NULL;

            if( process->changed_state )
            {
                /* handle saved signals if state changed */
                process->changed_state = FALSE;
                
                if( list_size( &process->saved_signals ) )
                {
                    /* there is a signal in the saved list to handle */
                    signal = ( fsm_signal_t * )list_get_head( &process->saved_signals );
                }
                else
                {
                    /* no saved signals, finished with this process */
                    process = NULL;
                }
            }
            else
            {
                /* no change in state, finished with this process */
                process = NULL;
            }
        }
    }
}

