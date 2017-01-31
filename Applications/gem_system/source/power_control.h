#ifndef _POWER_CONTROL_H_
#define _POWER_CONTROL_H_

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

void power_control_init( void );
void power_control_process( void );

void power_set_hazard_state( bool_t state );
void power_set_doors_state( bool_t state );
void power_set_instruments_state( bool_t state );

bool_t power_get_ignition_input( void );
bool_t power_get_engine_run_state( void );
bool_t power_get_headlights_state( void );

///////////////////////////////////////////////////////////////

#endif // _POWER_CONTROL_H_
