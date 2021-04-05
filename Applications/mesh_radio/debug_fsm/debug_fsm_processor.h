#ifndef _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_DEBUG_FSM_DEBUG_FSM_OUTPUT_H_
#define _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_DEBUG_FSM_DEBUG_FSM_OUTPUT_H_

////////////////////////////////////////////////////////////////////

#include "system.h"
#include "fsm.h"
#include "fsm_signals.h"
#include "debug_fsm_definition.h"

////////////////////////////////////////////////////////////////////

#define DEBUG_FSM_PID        fsm_pid_from_name( "debug_fsm" )
#define DEBUG_FSM_GET_DATA() fsm_data_from_name( "debug_fsm" )

////////////////////////////////////////////////////////////////////

#define Idle_state_debug_fsm 0

////////////////////////////////////////////////////////////////////

static void debug_fsm_init( debug_fsm_t * fsm_data );
static void debug_fsm_exit( debug_fsm_t * fsm_data );

static void Idle_transition_Debug_Print_indication( debug_fsm_t * fsm_data, Debug_Print_indication_t * signal );

static const fsm_transition_t Idle_transitions[] =
{
    FSM_SIGNAL( Debug_Print_indication_id, Idle_transition_Debug_Print_indication ),
};

static const fsm_state_t debug_fsm_state_list[] =
{
    FSM_STATE( Idle_state_debug_fsm, Idle_transitions, NULL ),
};

static const char * debug_fsm_debug_states[] =
{
    "Idle_state_debug_fsm",
};

////////////////////////////////////////////////////////////////////

DECLARE_STATIC_FSM_SECTION( debug_fsm_t, debug_fsm )
{
    "debug_fsm",
    debug_fsm_debug_states,
    FSM_CALL_HANDLER( debug_fsm_init ),
    FSM_CALL_HANDLER( debug_fsm_exit ),
    FSM_STATE_TABLE( debug_fsm_state_list ),
    FSM_STATE_EMPTY(),
    NULL,
    NULL,
};

////////////////////////////////////////////////////////////////////

#endif // _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_DEBUG_FSM_DEBUG_FSM_OUTPUT_H_

