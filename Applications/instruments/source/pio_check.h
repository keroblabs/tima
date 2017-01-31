#ifndef _PIO_CHECK_H_
#define _PIO_CHECK_H_

///////////////////////////////////////////////////////////////

#include "types.h"
#include "device.h"

///////////////////////////////////////////////////////////////

enum
{
	PIO_HANDBRAKE_PULLED,
	PIO_HANDBRAKE_RELEASED,
	PIO_HANDBRAKE_MOVING
};

enum
{
	MESSAGE_MODE_ON			= 0x1000,
	MESSAGE_MODE_OFF		= 0x1001,
	MESSAGE_BUTTON_SHORT	= 0x1002,
	MESSAGE_BUTTON_LONG		= 0x1003,
};

///////////////////////////////////////////////////////////////

void pio_check_init( void );
void pio_check_process( void );

void pio_check_set_obd_warning( uint32_t mask );

uint8_t pio_check_get_handbrake_state( void );
bool_t pio_check_get_oil_pressure( void );
bool_t pio_check_get_fullbeam( void );
bool_t pio_check_get_brake_level_state( void );
bool_t pio_check_setup_button( void );
bool_t pio_check_mode_button( void );
bool_t pio_check_engine_run_state( void );
bool_t pio_check_power_down_flag( void );

uint16_t pio_check_get_message( void );
void * pio_check_reset_message( void );

device_t * pio_check_open_output( const char * dev_name, uint8_t state );
device_t * pio_check_open_input( const char * dev_name, uint8_t * pio_var );

uint8_t pio_check_get_battery_state( void );

uint8_t pio_check_read_input( device_t * p_dev, bool_t invert );
void pio_check_set_output( device_t * p_dev, uint8_t state );

void * pio_check_event_open_read_port( const char * dev_name, void * event_handler, uint16_t id, bool_t is_invert, uint32_t interval );
bool_t pio_check_event_read_port( void * p_data, bool_t * state );
bool_t pio_check_event_get_state( void * p_event );

///////////////////////////////////////////////////////////////

#endif // _PIO_CHECK_H_
