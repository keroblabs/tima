#ifndef APPLICATIONS_MESH_RADIO_DEBUG_FSM_H_
#define APPLICATIONS_MESH_RADIO_DEBUG_FSM_H_

////////////////////////////////////////////////////////////////////

#include "types.h"
#include "debug_fsm_processor.h"

////////////////////////////////////////////////////////////////////

#define DEBUG_ENABLED

///////////////////////////////////////////////////////////////

int debug_fsm_printk( const char *fmt, ... );

#ifndef _NO_DEBUG

#ifdef DEBUG_PRINTK
#undef DEBUG_PRINTK
#endif

#define DEBUG_PRINTK debug_fsm_printk
#endif

#define TIMA_DEBUG

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_MESH_RADIO_DEBUG_FSM_H_ */
