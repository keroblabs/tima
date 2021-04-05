#ifndef FRAMEWORK_FSM_H_
#define FRAMEWORK_FSM_H_

////////////////////////////////////////////////////////////////////

#include "types.h"
#include "lists.h"
#include "timer.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

#define FSM_SIGNAL( signal, handler )               { signal, ( fsm_handler_t )handler }
#define FSM_STATE_TABLE( signals )                  signals, sizeof(signals)/sizeof(fsm_transition_t)
#define FSM_STATE_EMPTY()                           NULL, 0
#define FSM_STATE( state, transition, bground )     { state, FSM_STATE_TABLE(transition), (fsm_call_t)bground }

////////////////////////////////////////////////////////////////////

#define INVALID_PID                                 (NEG_U32)
#define INVALID_STATE                               ((NEG_U32)-1)
#define MAIN_THREAD_PID                             ((NEG_U32)-2)
#define _ANY_SIGNAL                                 ((NEG_U32)-3)

////////////////////////////////////////////////////////////////////

#define FSM_DATA
#define SIGNALS
#define STATES
#define ENDSTATES
#define STATE(s)
#define COMMON()
#define SIGNAL(s)
#define PARAM(p)
#define WITH_DATA
#define WITH_MASK
#define SIGNALS_DONE
#define ENUM(n)
#define ENUMS_DONE
#define SAVE(s)
#define INVALID(s)
#define ANY_SIGNAL
#define BACKGROUND
#define UNHANDLED(i,o)

////////////////////////////////////////////////////////////////////

#define fsm_save_signal(data, signal)           fsm_save_signal_ex(data, (fsm_signal_t *)signal)
#define fsm_invalid_signal(data, signal)        fsm_invalid_signal_ex(data, (fsm_signal_t *)signal)
#define fsm_unknown_signal(data, signal)        fsm_unknown_signal_ex(data, (fsm_signal_t *)signal)

#define DECLARE_STATIC_FSM_SECTION(type, data)      static const fsm_init_data_t _init_##data; \
                                                    static void _create_fsm_##data##_init( void ) \
                                                    { \
                                                        type * fsm_data = ( type * )MMALLOC( sizeof( type ) ); \
                                                        if( fsm_data != NULL ) \
                                                        { \
                                                            fsm_start_process( &_init_##data, fsm_data ); \
                                                        } \
                                                    } \
                                                    DECLARE_SUB_APPLICATION_SECTION( _create_fsm_##data##_init, TIMA_SUB_APPLICATION_FSM ); \
                                                    static const fsm_init_data_t _init_##data =

#define DECLARE_DYNAMIC_FSM_SECTION(type, data)     static const fsm_init_data_t _init_##data; \
                                                    void _start_fsm##data##_init( void ) \
                                                    { \
                                                        type * fsm_data = ( type * )MMALLOC( sizeof( type ) ); \
                                                        if( fsm_data != NULL ) \
                                                        { \
                                                            fsm_start_process( &_init_##data, fsm_data ); \
                                                        } \
                                                    } \
                                                    static const fsm_init_data_t _init_##data =

#define START_DYNAMIC_FSM(data)                     do { void _start_fsm##data##_init( void ); \
                                                    _start_fsm##data##_init(); } while(0)

////////////////////////////////////////////////////////////////////

typedef void ( *fsm_call_t )( void * data );

typedef struct fsm_signal_t_
{
    list_node_t node;

    uint32_t source;
    uint32_t dest;
    uint32_t signal_id;
    
    void * data;
    
} fsm_signal_t;

typedef struct fsm_timer_t_
{
    list_node_t node;
    
    timer_data_t timer;
    uint32_t pid_creator;
    uint32_t signal_id;
    
} fsm_timer_t;

typedef void ( *fsm_handler_t )( void * data, fsm_signal_t * signal );

typedef struct fsm_transitions_t_
{
    uint32_t signal_id;
    fsm_handler_t handler;
    
} fsm_transition_t;

typedef struct fsm_state_t_
{
    uint32_t state_id;
    const fsm_transition_t * transitions;
    uint32_t count;
    fsm_call_t background;
    
} fsm_state_t;

#define FSM_CALL_HANDLER(a)     ((fsm_call_t)a)
#define FSM_CALL_TRANSITION(a)  ((fsm_handler_t)a)

typedef struct fsm_init_data_t_
{
    const char * name;
    const char ** debug_states;
    
    fsm_call_t init;
    fsm_call_t exit;
    
    const fsm_state_t * state_list;  // states and transitions in each state
    uint32_t state_count;

    const fsm_transition_t * common; // transitions for any state
    uint32_t common_count;

    fsm_handler_t unhandled;         // wrong state, not in common, and not save in current state
    fsm_handler_t unknown;           // not in any state

} fsm_init_data_t;

typedef struct fsm_process_t_
{
    list_node_t node;
    
    uint32_t pid;
    void * data;
    uint32_t curr_state_id;
    
    bool_t changed_state;
    bool_t terminate_pending;
    
    list_t saved_signals;
    
    const fsm_init_data_t * fsm_data;
    
} fsm_process_t;

typedef struct fsm_background_t_
{
    list_node_t node;

    fsm_handler_t main_hander;
    void * main_data;

} fsm_background_t;

typedef struct fsm_instance_t_
{
    list_t processes;
    list_t signals;
    list_t timers;

    fsm_handler_t main_hander;
    void * main_data;

    uint32_t pid_counter;
    fsm_process_t * curr_process;
    
} fsm_instance_t;

////////////////////////////////////////////////////////////////////

void fsm_send_signal( uint32_t dest, uint32_t signal, void * data );
void fsm_set_timer( uint32_t signal_id, uint32_t time_ms );
void fsm_reset_timer( uint32_t signal_id );

uint32_t fsm_start_process( const fsm_init_data_t * fsm_data, void * data );
void fsm_terminate_process( uint32_t pid );

uint32_t fsm_self( void );
uint32_t fsm_pid_from_name( const char * name );
void * fsm_data_from_name( const char * name );
void * fsm_data_from_pid( uint32_t pid );

void fsm_save_signal_ex( void * data, fsm_signal_t * signal );
void fsm_invalid_signal_ex( void * data, fsm_signal_t * signal );
void fsm_unknown_signal_ex( void * data, fsm_signal_t * signal );

void fsm_exit( void );
void fsm_set_state( uint32_t state );
uint32_t fsm_get_state( void );

void fsm_global_init( fsm_handler_t main_thread, void * data );
void fsm_process( void );

////////////////////////////////////////////////////////////////////

#endif /* FRAMEWORK_FSM_H_ */
