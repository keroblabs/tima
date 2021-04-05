#ifndef APPLICATIONS_MESH_RADIO_SCAN_FSM_SCAN_FSM_DEFINITION_H_
#define APPLICATIONS_MESH_RADIO_SCAN_FSM_SCAN_FSM_DEFINITION_H_

////////////////////////////////////////////////////////////////////

#include "fsm.h"

////////////////////////////////////////////////////////////////////

#undef PROCESS_NAME
#define PROCESS_NAME            scan_fsm

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
        SIGNAL(Scan_Start_request)
ENDSTATES

///////////////////////////////////////////////////////////////////

typedef struct FSM_DATA scan_fsm_t_
{
    uint8_t var1;
    
} scan_fsm_t;

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_MESH_RADIO_SCAN_FSM_SCAN_FSM_DEFINITION_H_ */
