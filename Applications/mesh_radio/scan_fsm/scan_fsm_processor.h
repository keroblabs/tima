#ifndef _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_SCAN_FSM_SCAN_FSM_OUTPUT_H_
#define _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_SCAN_FSM_SCAN_FSM_OUTPUT_H_

////////////////////////////////////////////////////////////////////

#include "system.h"
#include "fsm.h"
#include "fsm_signals.h"
#include "scan_fsm_definition.h"

////////////////////////////////////////////////////////////////////

#define SCAN_FSM_PID        fsm_pid_from_name( "scan_fsm" )
#define SCAN_FSM_GET_DATA() fsm_data_from_name( "scan_fsm" )

////////////////////////////////////////////////////////////////////

#define Idle_state_scan_fsm 0

////////////////////////////////////////////////////////////////////

static void scan_fsm_init( scan_fsm_t * fsm_data );
static void scan_fsm_exit( scan_fsm_t * fsm_data );

static void Idle_transition_Scan_Start_request( scan_fsm_t * fsm_data, Scan_Start_request_t * signal );

static const fsm_transition_t Idle_transitions[] =
{
    FSM_SIGNAL( Scan_Start_request_id, Idle_transition_Scan_Start_request ),
};

static const fsm_state_t scan_fsm_state_list[] =
{
    FSM_STATE( Idle_state_scan_fsm, Idle_transitions, NULL ),
};

static const char * scan_fsm_debug_states[] =
{
    "Idle_state_scan_fsm",
};

////////////////////////////////////////////////////////////////////

DECLARE_STATIC_FSM_SECTION( scan_fsm_t, scan_fsm )
{
    "scan_fsm",
    scan_fsm_debug_states,
    FSM_CALL_HANDLER( scan_fsm_init ),
    FSM_CALL_HANDLER( scan_fsm_exit ),
    FSM_STATE_TABLE( scan_fsm_state_list ),
    FSM_STATE_EMPTY(),
    NULL,
    NULL,
};

////////////////////////////////////////////////////////////////////

#endif // _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_SCAN_FSM_SCAN_FSM_OUTPUT_H_

