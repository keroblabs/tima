#ifndef fsm_h
#define fsm_h

////////////////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "lists.h"
#include "buffer.h"

////////////////////////////////////////////////////////////////////////////////////////

#define FSM_SIGNAL_TYPE         fsm_signal_t fsm_signal;

#define FSM_ANY_SIGNAL          (NEG_U32-3)

#define FSM_PID_EXTERNAL        (NEG_U32-2)
#define FSM_PID_INVALID         (NEG_U32-1)
#define PID_DEFAULT             (NEG_U32)

#define FSM_HANDLER(id, hdl)    id, ( fsm_handler_t )hdl
#define FSM_TRANSITION(a)       sizeof( a ) / sizeof( fsm_transition_t ), a
#define FSM_STATES(a)           sizeof( a ) / sizeof( fsm_state_t ), a
#define FSM_TRANSITION_NULL     0, NULL

////////////////////////////////////////////////////////////////////////////////////////

#define fsm_create_data(t)      ( t * )fsm_set_internal_data( fsm_self(), sizeof( t ) );

////////////////////////////////////////////////////////////////////////////////////////

typedef void ( *fsm_call_t )( void * p_data );
typedef void ( *fsm_exit_t )( void );

typedef struct _fsm_signal_t
{
	list_node_t list_node;

    uint32_t source_pid;
    uint32_t destination_pid;
    uint32_t id;

    buffer_data_t * buffer;

} fsm_signal_t;

typedef void ( *fsm_handler_t )( void * p_data, fsm_signal_t * signal );

typedef struct _fsm_transition_t
{
    uint32_t signal;
    
    fsm_handler_t handler;
    
} fsm_transition_t;

typedef struct _fsm_states_t
{
    bool_t is_save;

    uint32_t transition_count;
    const fsm_transition_t * state_transition;
    
} fsm_state_t;

typedef struct _fsm_config_t
{
    uint32_t type;

    uint32_t state_count;
    const fsm_state_t * fsm_states;

    uint32_t common_count;
    const fsm_transition_t * common_transition;

    fsm_handler_t invalid_handler;

    fsm_call_t fsm_start;
    fsm_call_t fsm_process;
    fsm_exit_t fsm_exit;

} fsm_config_t;

////////////////////////////////////////////////////////////////////////////////////////

void fsm_save_event( void * p_data, fsm_signal_t * signal );
void fsm_ignore_event( void * p_data, fsm_signal_t * signal );
void fsm_set_state( uint32_t state );

fsm_signal_t * fsm_build_event( uint32_t signal_id, buffer_data_t * buffer );
void fsm_send_event( fsm_signal_t * signal, uint32_t destination_pid );

uint32_t fsm_get_state( void );
uint32_t fsm_start_process( const fsm_config_t * fsm_config, void * init_param );

void fsm_terminate_process( uint32_t pid );

void * fsm_create_internal_data( uint32_t size );

uint32_t fsm_self( void );

void fsm_set_timer( uint32_t signal_id, uint32_t time_ms );
void fsm_kill_timer( uint32_t signal_id );
void fsm_init( void );

uint32_t fsm_now( void );

////////////////////////////////////////////////////////////////////////////////////////

#endif /* fsm_h */
