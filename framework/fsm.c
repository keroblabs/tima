#include "fsm.h"
#include "heap.h"
#include "timer.h"

////////////////////////////////////////////////////////////////////////////////////////

typedef struct fsm_timer_signal_t_
{
    FSM_SIGNAL_TYPE;

    uint32_t time_ms;

} fsm_timer_signal_t;


typedef struct _fsm_process_t
{
	list_node_t list_node;

	list_data_t saved_events_list;

    uint32_t pid;
    uint32_t state;

    void * p_data;
    const fsm_config_t * p_config;

} fsm_process_t;

////////////////////////////////////////////////////////////////////////////////////////

static fsm_process_t * current_process = NULL;
static list_data_t fsm_list;
static list_data_t signal_list;
static list_data_t timer_list;

////////////////////////////////////////////////////////////////////////////////////////

static fsm_process_t * fsm_find_process( uint32_t pid )
{
    fsm_process_t * search = NULL;

    while( ( search = ( fsm_process_t * )list_get_next( &fsm_list, &search->list_node ) ) != NULL )
    {
        if( search->pid == pid ) return search;
    }

    return NULL;
}

static uint32_t fsm_get_new_pid( void )
{
    uint32_t search_pid = 1;
    fsm_process_t * process = NULL;

    while( ( process = ( fsm_process_t * )list_get_next( &fsm_list, &process->list_node ) ) != NULL )
    {
        if( search_pid != process->pid ) break;
        search_pid++;
    }

    return search_pid;
}

static const fsm_transition_t * fsm_get_transition( const fsm_transition_t * transitions, uint32_t total_transitions, uint32_t signal )
{
    uint32_t cnt;

    for( cnt = 0; cnt < total_transitions; cnt++ )
    {
        if( ( transitions[cnt].signal == signal ) || ( transitions[cnt].signal == FSM_ANY_SIGNAL ) )
        {
            return &transitions[cnt];
        }
    }

    return NULL;
}

static fsm_signal_t * fsm_get_signal( list_data_t * list )
{
    fsm_signal_t * signal = ( fsm_signal_t * )list_get_head( &signal_list );
    if( signal == NULL ) return NULL;

    list_unlink( list, &signal->list_node );

    return signal;
}

static fsm_signal_t * fsm_timer_get_expired( void )
{
    fsm_timer_signal_t * search = ( fsm_timer_signal_t * )list_get_head( &timer_list );
    uint32_t time_now = timer_get_MS();

    if( ( search != NULL ) && ( search->time_ms >= time_now ) )
    {
    	list_unlink( &timer_list, &search->fsm_signal.list_node );
        return &search->fsm_signal;
    }

    return NULL;
}

static void fsm_delete_signal( list_node_t *node )
{
	fsm_signal_t * signal = ( fsm_signal_t * )node;

    if( signal->buffer != NULL ) buffer_destroy( signal->buffer );
    MFREE( signal );
}

static void fsm_delete_process( list_node_t * node )
{
	fsm_process_t * process = ( fsm_process_t * )node;

	if( process->p_data != NULL ) MFREE( process->p_data );
	MFREE( process );
}

////////////////////////////////////////////////////////////////////////////////////////

void fsm_process_signal( void )
{
    fsm_handler_t handler = NULL;
    fsm_signal_t * signal;
    const fsm_transition_t * process_transition;
    uint32_t current_state;

    /* look for timer first */
    signal = fsm_timer_get_expired();
    if( signal == NULL )
    {
        /* get pending signal, if any */
        signal = fsm_get_signal( &signal_list );
    }

    /* no signal to process, exit */
    if( signal == NULL ) return;

    /* find destination */
    current_process = fsm_find_process( signal->destination_pid );
    if( current_process == NULL ) return;

    while( signal != NULL )
    {
        /* save state to detect change */
        current_state = current_process->state;

        /* get handler for the current process state */
        process_transition = fsm_get_transition( current_process->p_config->fsm_states[current_process->state].state_transition,
                                                 current_process->p_config->fsm_states[current_process->state].transition_count, signal->id );
        if( process_transition != NULL )
        {
            handler = process_transition->handler;
        }

        if( handler == NULL )
        {
            /* transition not found in the current state, try common */
            process_transition = fsm_get_transition( current_process->p_config->common_transition, current_process->p_config->common_count, signal->id );
            if( process_transition != NULL )
            {
                handler = process_transition->handler;
            }
        }

        if( handler == NULL )
        {
            /* transition not in common, check to save */
            const fsm_state_t * process_state = &current_process->p_config->fsm_states[current_process->state];
            if( process_state->is_save == TRUE )
            {
                /* save for the next transition */
                handler = fsm_save_event;
            }
        }

        if( handler == NULL )
        {
            /* not to save, send to invalid */
        	handler = current_process->p_config->invalid_handler;
        }

        if( handler == NULL )
        {
            /* invalid handler not found, call default */
            handler = fsm_ignore_event;
        }

        handler( current_process->p_data, signal );

        /* This will free the memory of the signal */
        fsm_delete_signal( &signal->list_node );

        /* no change of state or empty saved signals, exit the loop */
        if( ( current_process->state == current_state ) ||
            ( list_size( &current_process->saved_events_list ) == 0 ) )
        {
        	break;
        }

        /* get a save signal and let it loop */
        signal = fsm_get_signal( &current_process->saved_events_list );
    }

    current_process = NULL;
}

void fsm_save_event( void * p_data, fsm_signal_t * signal )
{
    list_insert_tail( &current_process->saved_events_list, &signal->list_node );
}

void fsm_ignore_event( void * p_data, fsm_signal_t * signal )
{
	fsm_delete_signal( &signal->list_node );
}

fsm_signal_t * fsm_build_event( uint32_t signal_id, buffer_data_t * buffer )
{
    fsm_signal_t * signal = ( fsm_signal_t * )MMALLOC( sizeof( fsm_signal_t ) );
    if( signal != NULL )
    {
        signal->source_pid      = ( current_process != NULL ) ? current_process->pid : FSM_PID_EXTERNAL;
        signal->destination_pid = FSM_PID_INVALID;
        signal->buffer          = buffer;
        signal->id              = signal_id;
    }

    return signal;
}

fsm_signal_t * fsm_clone_event( fsm_signal_t * event )
{
    fsm_signal_t * clone = fsm_build_event( event->id, event->buffer );
    if( clone == NULL ) return NULL;

    if( ( event->buffer != NULL ) && ( buffer_add_user( event->buffer ) == FALSE ) )
    {
        MFREE( clone );
        return NULL;
    }

    return clone;
}

void fsm_send_event( fsm_signal_t * signal, uint32_t destination_pid )
{
	if( signal != NULL )
	{
		signal->destination_pid = destination_pid;
	    list_insert_tail( &signal_list, &signal->list_node );
	}
}

void fsm_set_state( uint32_t state )
{
    if( current_process != NULL ) current_process->state = state;
}

uint32_t fsm_get_state( void )
{
    return ( current_process != NULL ) ? current_process->state : FSM_PID_INVALID;
}

uint32_t fsm_start_process( const fsm_config_t * fsm_config, void * init_param )
{
    fsm_process_t * process = ( fsm_process_t * )MMALLOC( sizeof( fsm_process_t ) );
    if( process == NULL ) return FSM_PID_INVALID;
    memset( process, 0x00, sizeof( fsm_process_t ) );

    process->p_config   = fsm_config;
    process->pid        = fsm_get_new_pid();
    process->p_data     = NULL;

    list_init( &process->saved_events_list, fsm_delete_signal );
    list_insert_tail( &fsm_list, &process->list_node );

    if( fsm_config->fsm_start != NULL )
    {
        fsm_process_t * process_saved = current_process;
        current_process = process;
        fsm_config->fsm_start( init_param );
        current_process = process_saved;
    }

    return process->pid;
}

void fsm_terminate_process( uint32_t pid )
{
    const fsm_config_t * fsm_config;
    fsm_process_t * process = fsm_find_process( pid );
    if( process == NULL ) return;

    fsm_config = ( const fsm_config_t * )process->p_config;

    if( fsm_config->fsm_exit != NULL )
    {
        fsm_process_t * process_saved = current_process;
        current_process = process;
        fsm_config->fsm_exit();
        current_process = process_saved;
    }

    list_clear( &process->saved_events_list );

    fsm_delete_process( &process->list_node );
}

void * fsm_create_internal_data( uint32_t size )
{
    if( current_process == NULL ) return NULL;

    current_process->p_data = MMALLOC( size );
    return current_process->p_data;
}

uint32_t fsm_self( void )
{
    return current_process ? current_process->pid : FSM_PID_INVALID;
}

void fsm_set_timer( uint32_t signal_id, uint32_t time_ms )
{
    fsm_timer_signal_t * search = NULL;
    fsm_timer_signal_t * before = NULL;
    fsm_timer_signal_t * new_timer;

    if( current_process == NULL ) return;

    new_timer = ( fsm_timer_signal_t * )MMALLOC( sizeof( fsm_timer_signal_t ) );
    if( new_timer == NULL ) return;
    memset( new_timer, 0x00, sizeof( fsm_timer_signal_t ) );

    new_timer->time_ms                    = time_ms;
    new_timer->fsm_signal.destination_pid = current_process->pid;
    new_timer->fsm_signal.source_pid      = current_process->pid;
    new_timer->fsm_signal.id              = signal_id;
    new_timer->fsm_signal.buffer          = NULL;

    while( ( search = ( fsm_timer_signal_t * )list_get_next( &timer_list, &search->fsm_signal.list_node ) ) != NULL )
    {
        if( search->time_ms > time_ms )
        {
        	if( before == NULL )
        	{
        		list_insert_head( &timer_list, &new_timer->fsm_signal.list_node );
        	}
        	else
        	{
        		list_insert_after( &timer_list, &before->fsm_signal.list_node, &new_timer->fsm_signal.list_node );
        	}
            break;
        }

        before = search;
    }
}

void fsm_kill_timer( uint32_t signal_id )
{
    fsm_timer_signal_t * search = NULL;

    while( ( search = ( fsm_timer_signal_t * )list_get_next( &timer_list, &search->fsm_signal.list_node ) ) != NULL )
    {
        if( search->fsm_signal.id == signal_id )
        {
            tima_list_unlink( &timer_list, &search->fsm_signal.list_node );
            break;
        }
    }
}

uint32_t fsm_now( void )
{
    return timer_get_MS();
}

void fsm_init( void )
{
    list_init( &fsm_list, fsm_delete_process );
    list_init( &signal_list, fsm_delete_signal );
    list_init( &timer_list, fsm_delete_signal );
}
