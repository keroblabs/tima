#ifndef _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_NODE_FSM_NODE_FSM_OUTPUT_H_
#define _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_NODE_FSM_NODE_FSM_OUTPUT_H_

////////////////////////////////////////////////////////////////////

#include "system.h"
#include "fsm.h"
#include "fsm_signals.h"
#include "node_fsm_definition.h"

////////////////////////////////////////////////////////////////////

#define NODE_FSM_PID        fsm_pid_from_name( "node_fsm" )
#define NODE_FSM_GET_DATA() fsm_data_from_name( "node_fsm" )

////////////////////////////////////////////////////////////////////

#define Idle_state_node_fsm 0

////////////////////////////////////////////////////////////////////

static void node_fsm_init( node_fsm_t * fsm_data );
static void node_fsm_exit( node_fsm_t * fsm_data );

static void Idle_transition_Received_Frame_indication( node_fsm_t * fsm_data, Received_Frame_indication_t * signal );

static const fsm_transition_t Idle_transitions[] =
{
    FSM_SIGNAL( Received_Frame_indication_id, Idle_transition_Received_Frame_indication ),
};

static const fsm_state_t node_fsm_state_list[] =
{
    FSM_STATE( Idle_state_node_fsm, Idle_transitions, NULL ),
};

static const char * node_fsm_debug_states[] =
{
    "Idle_state_node_fsm",
};

////////////////////////////////////////////////////////////////////

DECLARE_STATIC_FSM_SECTION( node_fsm_t, node_fsm )
{
    "node_fsm",
    node_fsm_debug_states,
    FSM_CALL_HANDLER( node_fsm_init ),
    FSM_CALL_HANDLER( node_fsm_exit ),
    FSM_STATE_TABLE( node_fsm_state_list ),
    FSM_STATE_EMPTY(),
    NULL,
    NULL,
};

////////////////////////////////////////////////////////////////////

#endif // _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_NODE_FSM_NODE_FSM_OUTPUT_H_

