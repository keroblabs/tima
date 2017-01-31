#ifndef __MBULK_H__
#define __MBULK_H__

///////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

typedef struct _mbulk_t_
{
	uint16_t sender;
	uint16_t target;
	uint16_t message;
	uint16_t size;

	uint8_t * data;

	struct _mbulk_t_ * p_next;

} mbulk_t;

typedef struct _mbulk_pipe_t_
{
	mbulk_t * base_mbulk;
	mbulk_t * last_mbulk;

} mbulk_pipe_t;

typedef struct _mbulk_data_t
{
	uint16_t my_id;
	mbulk_t * curr_mbulk;
	mbulk_t * rx_mbulk;

	mbulk_pipe_t * pipe;

	struct _mbulk_data_t * p_next;

} mbulk_data_t;


///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

void mbulk_pipe_init( mbulk_pipe_t * pipe );

mbulk_data_t * mbulk_init( mbulk_data_t * pipe, uint16_t my_id );

void mbulk_destroy( mbulk_data_t * p_data );

uint8_t * mbulk_create( mbulk_data_t * p_data, uint32_t size );

void mbulk_send( mbulk_data_t * p_data, uint16_t message, uint16_t dest );

uint8_t * mbulk_receive( mbulk_data_t * p_data );
uint8_t * mbulk_receive_ex( mbulk_data_t * mbulk_info, uint16_t message );

uint16_t mbulk_get_sender( mbulk_data_t * p_data );
uint16_t mbulk_get_message( mbulk_data_t * p_data );
uint32_t mbulk_get_size( mbulk_data_t * p_data );

void mbulk_release( mbulk_data_t * p_data );
void mbulk_cancel( mbulk_data_t * p_data );

///////////////////////////////////////////////////////////////////////////////////////

#endif // __MBULK_H__

