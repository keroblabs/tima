#ifndef __setup_data_h__
#define __setup_data_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

typedef struct _setup_data_t_
{
	uint16_t header;

	bool_t is_mph;

	uint32_t fuel_tank_adc_min;
	uint32_t fuel_tank_adc_max;

	uint64_t speed_setup;
	uint64_t odometer_main;
	uint64_t odometer_aux;

	uint64_t trip1;
	uint64_t trip2;
	uint64_t trip3;

} setup_data_t;

///////////////////////////////////////////////////////////////

void setup_data_init( void );
void setup_data_write( void );

setup_data_t * setup_data_get( void );

///////////////////////////////////////////////////////////////

#endif // __setup_data_h__
