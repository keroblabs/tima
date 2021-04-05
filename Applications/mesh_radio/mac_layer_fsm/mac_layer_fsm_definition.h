#ifndef APPLICATIONS_MESH_RADIO_MAC_LAYER_FSM_MAC_LAYER_FSM_DEFINITION_H_
#define APPLICATIONS_MESH_RADIO_MAC_LAYER_FSM_MAC_LAYER_FSM_DEFINITION_H_

////////////////////////////////////////////////////////////////////

#include "fsm.h"
#include "device.h"
#include "mac.h"

////////////////////////////////////////////////////////////////////

#define MAX_NUM_CLIENTS         20

////////////////////////////////////////////////////////////////////

#undef PROCESS_NAME
#define PROCESS_NAME            mac_layer_fsm

#undef FSM_INIT_HANDLER
#define FSM_INIT_HANDLER        1

#undef FSM_EXIT_HANDLER
#define FSM_EXIT_HANDLER        1

#undef FSM_UNHANDLED_HANDLER
#define FSM_UNHANDLED_HANDLER   1

#undef FSM_UNKNOWN_HANDLER
#define FSM_UNKNOWN_HANDLER     0

#undef FSM_TYPE_STATIC
#define FSM_TYPE_STATIC         1

////////////////////////////////////////////////////////////////////

STATES
    STATE(Idle)
        SIGNAL(Set_Channel_request)
        SIGNAL(Transmit_Frame_request)
        INVALID(ANY_SIGNAL)
    STATE(on_channel) BACKGROUND
        SIGNAL(Set_Channel_request)
        SIGNAL(Transmit_Frame_request)
        SIGNAL(Unset_Channel_request)
        SIGNAL(Listen_Timeout_indication)
    STATE(scheduling) BACKGROUND
        SAVE(ANY_SIGNAL)
    STATE(tx_frame) BACKGROUND
        SAVE(ANY_SIGNAL)

    UNHANDLED(Set_Channel_request, Set_Channel_confirm)
    UNHANDLED(Transmit_Frame_request, Transmit_Done_indication)
    UNHANDLED(Unset_Channel_request, Listen_Done_indication)
ENDSTATES

///////////////////////////////////////////////////////////////////

typedef struct client_data_t_
{
    uint8_t channel;
    uint32_t client;
    uint32_t listen_time;
    
    uint16_t tx_flags;
} client_data_t;

typedef struct FSM_DATA fsm_mac_layer_t_
{
    device_t mac_device;
    client_data_t client;    
    mac_driver_t * mac_driver;
    
} fsm_mac_layer_t;

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_MESH_RADIO_MAC_LAYER_FSM_MAC_LAYER_FSM_DEFINITION_H_ */
