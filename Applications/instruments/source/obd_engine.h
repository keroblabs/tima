#ifndef __odb_engine_h__
#define __odb_engine_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

typedef struct _obd_data_t
{
	uint8_t command;
	char resp[30];
} obd_data_t;

///////////////////////////////////////////////////////////////

enum
{
	OBD_STATUS_WAITING,
	OBD_STATUS_SEARCHING,
	OBD_STATUS_DONE,
	OBD_STATUS_NO_RESPONSE,
	OBD_STATUS_NO_DATA,
	OBD_STATUS_OK,
	OBD_STATUS_IDLE,
	OBD_STATUS_ERROR,
};

enum
{
	OBD_RESPONSE_FLUSH,
	OBD_RESPONSE_INIT,
	OBD_RESPONSE_ECHO_OFF,
	OBD_RESPONSE_INIT_END,
	OBD_RESPONSE_READY,
};

enum
{
	OBD_COMMAND_RPM,
	OBD_COMMAND_SPEED,
	OBD_COMMAND_TEMPERATURE,
	OBD_COMMAND_FUEL_PRESSURE,
	OBD_COMMAND_ODOMETER,
	OBD_COMMAND_ENGINE_LOAD,
	OBD_COMMAND_INTAKE_PRESSURE,
	OBD_COMMAND_INTAKE_TEMPERATURE,

	OBD_COMMAND_END
};

#define OBD_PIPE_NAME "OBD_PIPE"

///////////////////////////////////////////////////////////////

uint8_t obd_engine_get_response( int * value );

void obd_engine_autoupdate( bool_t state );

void obd_engine_request( uint8_t cmd );
void obd_engine_process( void );
void obd_engine_init( void );

bool_t obd_is_data_ready( void );

///////////////////////////////////////////////////////////////

#endif // __odb_engine_h__
