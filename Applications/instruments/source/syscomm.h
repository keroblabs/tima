#ifndef _SYSCOMM_H_
#define _SYSCOMM_H_

///////////////////////////////////////////////////////////////

#include "types.h"
#include "pipe.h"

///////////////////////////////////////////////////////////////


enum
{
    SYSCOMM_PIPE_NO_REQUEST,
    
    SYSCOMM_PIPE_POWERUP,
    SYSCOMM_PIPE_TRIP_READ,
    SYSCOMM_PIPE_SETUP_READ,
    SYSCOMM_PIPE_TRIP_WRITE,
    SYSCOMM_PIPE_SETUP_WRITE,
    
    SYSCOMM_PIPE_ODOMETER_READ,
    SYSCOMM_PIPE_BITMASK_READ,
    SYSCOMM_PIPE_BITMASK_WRITE,
    SYSCOMM_PIPE_POWERDOWN,
    
    SYSCOMM_PIPE_TIMEOUT,
};

enum
{
	GEM_FLAG_ENGINE_RUN =    ( 1 << 4 ),
	GEM_FLAG_HEADLIGHTS =    ( 1 << 8 ),
    GEM_FLAG_ACC_ON =        ( 1 << 11 ),
    GEM_FLAG_MAX
};

///////////////////////////////////////////////////////////////

void syscomm_init( void );
void syscomm_process( void );


pipe_data_t * syscomm_get_pipe_request( void );
pipe_data_t * syscomm_get_pipe_response( void );


///////////////////////////////////////////////////////////////

#endif // _SYSCOMM_H_
