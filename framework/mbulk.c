#include "mbulk.h"

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

uint8_t * _mbulk_receive_local( mbulk_data_t * mbulk_info, uint16_t message, bool_t check_message )
{
	mbulk_t * p_search;
	mbulk_t * p_prev;

	p_search = mbulk_info->pipe->base_mbulk;
	p_prev = NULL;

	while( p_search != NULL )
	{
		if( ( p_search->target == mbulk_info->my_id ) &&
		    ( ( p_search->message == message ) || ( check_message == FALSE ) ) )
		{
			if( p_prev == NULL ) mbulk_info->pipe->base_mbulk = p_search->p_next;
			else p_prev->p_next = p_search->p_next;

			if( p_prev->p_next == NULL ) mbulk_info->pipe->last_mbulk = p_prev;

			mbulk_info->rx_mbulk = p_search;
			return p_search->data;
		}

		p_prev = p_search;
		p_search = p_search->p_next;
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////

void mbulk_pipe_init( mbulk_pipe_t * pipe )
{
	memset( pipe, 0x00, sizeof( mbulk_pipe_t ) );
}

mbulk_data_t * mbulk_init( mbulk_data_t * pipe, uint16_t my_id )
{
	mbulk_data_t * mbulk_data;

	mbulk_data = ( mbulk_data_t * )MMALLOC( sizeof( mbulk_data_t ) );
	if( mbulk_data == NULL ) return NULL;
	memset( mbulk_data, 0x00, sizeof( mbulk_data_t ) );

	mbulk_data->my_id = my_id;
	mbulk_data->pipe = pipe;

	return mbulk_data;
}

void mbulk_destroy( mbulk_data_t * mbulk_info )
{
	if( mbulk_info->curr_mbulk != NULL )
	{
		MFREE( mbulk_info->curr_mbulk->data );
		MFREE( mbulk_info->curr_mbulk );
	}

	if( mbulk_info->rx_mbulk != NULL )
	{
		MFREE( mbulk_info->rx_mbulk->data );
		MFREE( mbulk_info->rx_mbulk );
	}

	MFREE( mbulk_info );
}

uint8_t * mbulk_create( mbulk_data_t * mbulk_info, uint32_t size )
{
	mbulk_info->curr_mbulk = ( mbulk_t * )MMALLOC( sizeof( mbulk_t ) );
	if( mbulk_info->curr_mbulk == NULL ) return NULL;
	memset( mbulk_info->curr_mbulk, 0x00, sizeof( mbulk_t ) );

	mbulk_info->curr_mbulk->data = ( uint8_t * )MMALLOC( size );
	if( mbulk_info->curr_mbulk->data == NULL )
	{
		MFREE( mbulk_info->curr_mbulk );
		return NULL;
	}

	memset( mbulk_info->curr_mbulk->data, 0x00, size );

	mbulk_info->curr_mbulk->size = size;
	mbulk_info->curr_mbulk->sender = mbulk_info->my_id;

	return mbulk_info->curr_mbulk->data;
}

void mbulk_send( mbulk_data_t * mbulk_info, uint16_t message, uint16_t dest )
{
	mbulk_info->curr_mbulk->target = dest;
	mbulk_info->curr_mbulk->message = message;

	if( mbulk_info->pipe->base_mbulk == NULL ) mbulk_info->pipe->base_mbulk = mbulk_info->curr_mbulk;
	else mbulk_info->pipe->last_mbulk->p_next = mbulk_info->curr_mbulk;

	mbulk_info->pipe->last_mbulk = mbulk_info->curr_mbulk;
}

uint8_t * mbulk_receive( mbulk_data_t * mbulk_info )
{
	return _mbulk_receive_local( mbulk_info, 0, FALSE );
}

uint8_t * mbulk_receive_ex( mbulk_data_t * mbulk_info, uint16_t message )
{
	return _mbulk_receive_local( mbulk_info, message, TRUE );
}

uint16_t mbulk_get_sender( mbulk_data_t * mbulk_info )
{
	return mbulk_info->rx_mbulk->sender;
}

uint32_t mbulk_get_size( mbulk_data_t * mbulk_info )
{
	return mbulk_info->rx_mbulk->size;
}

uint16_t mbulk_get_message( mbulk_data_t * mbulk_info )
{
	return mbulk_info->rx_mbulk->message;
}

void mbulk_release( mbulk_data_t * mbulk_info )
{
	if( mbulk_info->rx_mbulk != NULL )
	{
		MFREE( mbulk_info->rx_mbulk->data );
		MFREE( mbulk_info->rx_mbulk );
	}
}

void mbulk_cancel( mbulk_data_t * mbulk_info )
{
	if( mbulk_info->curr_mbulk != NULL )
	{
		MFREE( mbulk_info->curr_mbulk->data );
		MFREE( mbulk_info->curr_mbulk );
	}
}

