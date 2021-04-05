#ifndef APPLICATIONS_MESH_RADIO_DEBUG_FSM_DEBUG_FSM_DEFINITION_H_
#define APPLICATIONS_MESH_RADIO_DEBUG_FSM_DEBUG_FSM_DEFINITION_H_

////////////////////////////////////////////////////////////////////

#include "fsm.h"
#include "device.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

#undef PROCESS_NAME
#define PROCESS_NAME            debug_fsm

#undef FSM_INIT_HANDLER
#define FSM_INIT_HANDLER        1

#undef FSM_EXIT_HANDLER
#define FSM_EXIT_HANDLER        1

#undef FSM_UNHANDLED_HANDLER
#define FSM_UNHANDLED_HANDLER   0

#undef FSM_UNKNOWN_HANDLER
#define FSM_UNKNOWN_HANDLER     0

#undef FSM_TYPE_STATIC
#define FSM_TYPE_STATIC         1

////////////////////////////////////////////////////////////////////

STATES
    STATE(Idle)
        SIGNAL(Debug_Print_indication)
ENDSTATES

///////////////////////////////////////////////////////////////////

typedef struct FSM_DATA debug_fsm_t_
{
    device_t debug_device;
    
} debug_fsm_t;

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_MESH_RADIO_DEBUG_FSM_DEBUG_FSM_DEFINITION_H_ */
