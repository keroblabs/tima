#ifndef __datalink_h__
#define __datalink_h__

///////////////////////////////////////////////////////////////

#include "types.h"
#include "device.h"
#include "pipe.h"
#include "timer.h"

///////////////////////////////////////////////////////////////

typedef struct _datalink_t_
{
	uint8_t * buffer;
	device_t * comm_dev;

	uint8_t state;
	uint16_t size;
	uint16_t pointer;

	uint8_t header;
	uint8_t footer;

	timer_data_t rx_timeout;
	pipe_data_t pipe;

} datalink_t;

///////////////////////////////////////////////////////////////

void datalink_init( device_t * p_dev, uint8_t header, uint8_t footer, datalink_t * p_data );
void datalink_process( datalink_t * p_data );
void datalink_send_data( datalink_t * p_data, uint8_t * buffer, uint32_t size );

///////////////////////////////////////////////////////////////

#endif // __datalink_h__
