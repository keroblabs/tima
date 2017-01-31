#ifndef __warning_lights_h__
#define __warning_lights_h__

///////////////////////////////////////////////////////////////

#include "types.h"
#include "graphics.h"

///////////////////////////////////////////////////////////////
/*

warnings:

headlamp/full beam/fog
indicators/hazard
engine/epc
battery
oil
handbrake/brake


*/
///////////////////////////////////////////////////////////////

enum
{
	WARNING_NONE,

	WARNING_HEADLAMP,
	WARNING_INDLEFT,
	WARNING_INDRIGHT,
	WARNING_HAZARD,
	WARNING_FOGLIGHT,
	WARNING_ENGINE,
	WARNING_SYSFAIL,
	WARNING_CRUISE,
	WARNING_DOORLEFT,
	WARNING_DOORRIGHT,
    WARNING_POWERDOWN,


	WARNING_FULLBEAM,
	WARNING_LOWWASHER,
	WARNING_HANDBRAKE_STATE,
	WARNING_BRAKELEVEL,
	WARNING_HANDBRAKE_MOVING,
	WARNING_OILPRESSURE,
	WARNING_BATTERY,
	WARNING_TEMPERATURE,
	WARNING_BONNET,
	WARNING_BOOT,
    
	WARNING_MAX
};

typedef struct
{
	uint16_t posx;
	uint16_t posy;

	const bitmap_t * p_icon;
	uint32_t mask;
	bool_t timed;

} warning_data_t;

///////////////////////////////////////////////////////////////

void warning_lights_update( uint32_t input );
void warning_lights_process( void );
void warning_lights_init( void );

///////////////////////////////////////////////////////////////

#endif // __warning_lights_h__
