#ifndef _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_MAC_LAYER_FSM_MAC_LAYER_FSM_OUTPUT_H_
#define _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_MAC_LAYER_FSM_MAC_LAYER_FSM_OUTPUT_H_

////////////////////////////////////////////////////////////////////

#include "system.h"
#include "fsm.h"
#include "fsm_signals.h"
#include "mac_layer_fsm_definition.h"

////////////////////////////////////////////////////////////////////

#define MAC_LAYER_FSM_PID        fsm_pid_from_name( "mac_layer_fsm" )
#define MAC_LAYER_FSM_GET_DATA() fsm_data_from_name( "mac_layer_fsm" )

////////////////////////////////////////////////////////////////////

#define Idle_state_mac_layer_fsm 0
#define on_channel_state_mac_layer_fsm 1
#define scheduling_state_mac_layer_fsm 2
#define tx_frame_state_mac_layer_fsm 3

////////////////////////////////////////////////////////////////////

static void mac_layer_fsm_init( fsm_mac_layer_t * fsm_data );
static void mac_layer_fsm_exit( fsm_mac_layer_t * fsm_data );

static void Idle_transition_Set_Channel_request( fsm_mac_layer_t * fsm_data, Set_Channel_request_t * signal );
static void Idle_transition_Transmit_Frame_request( fsm_mac_layer_t * fsm_data, Transmit_Frame_request_t * signal );
static void on_channel_transition_background( fsm_mac_layer_t * fsm_data );
static void on_channel_transition_Set_Channel_request( fsm_mac_layer_t * fsm_data, Set_Channel_request_t * signal );
static void on_channel_transition_Transmit_Frame_request( fsm_mac_layer_t * fsm_data, Transmit_Frame_request_t * signal );
static void on_channel_transition_Unset_Channel_request( fsm_mac_layer_t * fsm_data, Unset_Channel_request_t * signal );
static void on_channel_transition_Listen_Timeout_indication( fsm_mac_layer_t * fsm_data, Listen_Timeout_indication_t * signal );
static void scheduling_transition_background( fsm_mac_layer_t * fsm_data );
static void tx_frame_transition_background( fsm_mac_layer_t * fsm_data );

static const fsm_transition_t Idle_transitions[] =
{
    FSM_SIGNAL( Set_Channel_request_id, Idle_transition_Set_Channel_request ),
    FSM_SIGNAL( Transmit_Frame_request_id, Idle_transition_Transmit_Frame_request ),
    FSM_SIGNAL( _ANY_SIGNAL, fsm_invalid_signal_ex ),
};

static const fsm_transition_t on_channel_transitions[] =
{
    FSM_SIGNAL( Set_Channel_request_id, on_channel_transition_Set_Channel_request ),
    FSM_SIGNAL( Transmit_Frame_request_id, on_channel_transition_Transmit_Frame_request ),
    FSM_SIGNAL( Unset_Channel_request_id, on_channel_transition_Unset_Channel_request ),
    FSM_SIGNAL( Listen_Timeout_indication_id, on_channel_transition_Listen_Timeout_indication ),
};

static const fsm_transition_t scheduling_transitions[] =
{
    FSM_SIGNAL( _ANY_SIGNAL, fsm_save_signal_ex ),
};

static const fsm_transition_t tx_frame_transitions[] =
{
    FSM_SIGNAL( _ANY_SIGNAL, fsm_save_signal_ex ),
};

static const fsm_state_t mac_layer_fsm_state_list[] =
{
    FSM_STATE( Idle_state_mac_layer_fsm, Idle_transitions, NULL ),
    FSM_STATE( on_channel_state_mac_layer_fsm, on_channel_transitions, on_channel_transition_background ),
    FSM_STATE( scheduling_state_mac_layer_fsm, scheduling_transitions, scheduling_transition_background ),
    FSM_STATE( tx_frame_state_mac_layer_fsm, tx_frame_transitions, tx_frame_transition_background ),
};

static const char * mac_layer_fsm_debug_states[] =
{
    "Idle_state_mac_layer_fsm",
    "on_channel_state_mac_layer_fsm",
    "scheduling_state_mac_layer_fsm",
    "tx_frame_state_mac_layer_fsm",
};

static void mac_layer_fsm_unhandled( fsm_mac_layer_t * fsm_data, fsm_signal_t * signal )
{
    switch( signal->signal_id )
    {
        case Set_Channel_request_id:
            SEND_Set_Channel_confirm( signal->source, RESULT_invalid_state, 0 );
            break;

        case Transmit_Frame_request_id:
            SEND_Transmit_Done_indication( signal->source, RESULT_invalid_state, 0 );
            break;

        case Unset_Channel_request_id:
            SEND_Listen_Done_indication( signal->source, RESULT_invalid_state, 0 );
            break;

    }
}

////////////////////////////////////////////////////////////////////

DECLARE_STATIC_FSM_SECTION( fsm_mac_layer_t, mac_layer_fsm )
{
    "mac_layer_fsm",
    mac_layer_fsm_debug_states,
    FSM_CALL_HANDLER( mac_layer_fsm_init ),
    FSM_CALL_HANDLER( mac_layer_fsm_exit ),
    FSM_STATE_TABLE( mac_layer_fsm_state_list ),
    FSM_STATE_EMPTY(),
    FSM_CALL_TRANSITION( mac_layer_fsm_unhandled ),
    NULL,
};

////////////////////////////////////////////////////////////////////

#endif // _USERS_MARCIOCHEROBIM_PROJECTS_GIT_DEV_TIMAOS_V42_APPLICATIONS_MESH_RADIO_MAC_LAYER_FSM_MAC_LAYER_FSM_OUTPUT_H_

