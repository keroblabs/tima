#ifndef _PIO_CHECK_H_
#define _PIO_CHECK_H_

///////////////////////////////////////////////////////////////

#include "types.h"
#include "device.h"

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

	WARNING_MAX
};

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

void pio_check_init( void );
void pio_check_process( void );

void * pio_check_event_open_read_port( const char * dev_name, void * event_handler, uint16_t id, bool_t is_invert, uint32_t interval );
bool_t pio_check_event_read_port( void * p_data, bool_t * state );
bool_t pio_check_event_get_state( void * p_event );

void pio_check_set_bit( uint16_t bit, bool_t state );

uint16_t pio_check_read( void );

device_t * pio_check_open_output( const char * dev_name, uint8_t state );
device_t * pio_check_open_input( const char * dev_name, uint8_t * pio_var );

uint8_t pio_check_read_input( device_t * p_dev, bool_t invert );
void pio_check_set_output( device_t * p_dev, uint8_t state );

///////////////////////////////////////////////////////////////

#endif // _PIO_CHECK_H_
