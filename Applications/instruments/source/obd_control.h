#ifndef _OBD_CONTROL_H_
#define _OBD_CONTROL_H_

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////


enum
{
    OBD_REQUEST_NONE,
    
    OBD_REQUEST_SPEED,
    OBD_REQUEST_RPM,
    OBD_REQUEST_TEMPERATURE,
    OBD_REQUEST_ENGINE_LOAD,
    OBD_REQUEST_INTAKE_PRESSURE,
    OBD_REQUEST_INTAKE_TEMP,
    OBD_REQUEST_ECHO_OFF,
    
    OBD_REQUEST_FLUSH,
    
    OBD_REQUEST_MAX
};

enum
{
    OBD_RESULT_NONE,
    
    OBD_RESULT_OK,
    OBD_RESULT_ECHO_OFF_OK,
    
    OBD_RESULT_NO_DATA,
    OBD_RESULT_NO_ANSWER,
    OBD_RESULT_DATA_ERROR,
    OBD_RESULT_TIMEOUT,
    
    OBD_RESULT_MAX
};

///////////////////////////////////////////////////////////////

typedef struct _obd_response_t
{
    uint8_t request;
    uint8_t response;
    uint32_t timestamp;
    int value;
    
} obd_resp_t;

///////////////////////////////////////////////////////////////

void obd_control_init( void );
void obd_control_process( void );

obd_resp_t * obd_control_get_response( void );

void obd_control_remove_response( void );
void obd_control_send_request( uint8_t req );

///////////////////////////////////////////////////////////////

#endif // _OBD_CONTROL_H_
