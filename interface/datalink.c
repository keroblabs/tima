#include "system.h"
#include "datalink.h"

///////////////////////////////////////////////////////////////

enum
{
	DATALINK_IDLE,
	DATALINK_WAIT_HEADER,
	DATALINK_WAIT_SIZE,
	DATALINK_RX_PAYLOAD,
	DATALINK_WAIT_FOOTER,
};

///////////////////////////////////////////////////////////////

#define DATALINK_MAX_PAYLOAD		2048
#define DATALINK_RX_TIMEOUT			300

///////////////////////////////////////////////////////////////

void datalink_init( device_t * p_dev, uint8_t header, uint8_t footer, datalink_t * p_data )
{
	p_data->comm_dev = p_dev;
	p_data->buffer = NULL;
	p_data->header = header;
	p_data->footer = footer;
	pipe_init( &p_data->pipe, "PIPE_NO_NAME", 16 );
	p_data->pointer = 0;
	p_data->size = 0;
	timer_Stop( &p_data->rx_timeout );
}

void datalink_send_data( datalink_t * p_data, uint8_t * buffer, uint32_t size )
{
	device_write_buffer( p_data->comm_dev, buffer, size );
}

void datalink_process( datalink_t * p_data )
{
	uint8_t rx_char;
	uint16_t rx_size;

	if( p_data->comm_dev == NULL ) return;

	if( timer_Check( &p_data->rx_timeout ) )
	{
		timer_Stop( &p_data->rx_timeout );
		p_data->state = DATALINK_IDLE;
	}

	switch( p_data->state )
	{
		case DATALINK_IDLE:
			if( device_is_rx_pending( p_data->comm_dev ) == 0 ) break;
			device_read_buffer( p_data->comm_dev, &rx_char, 1 );
			if( rx_char != p_data->header ) break;

			// got header
			p_data->state = DATALINK_WAIT_SIZE;
			p_data->size = 0;
			timer_Start( &p_data->rx_timeout, DATALINK_RX_TIMEOUT );
			// fall to next case

		case DATALINK_WAIT_SIZE:
			if( device_is_rx_pending( p_data->comm_dev ) < 2 ) break;
			device_read_buffer( p_data->comm_dev, ( uint8_t * )&p_data->size, 2 );

			if( p_data->size > DATALINK_MAX_PAYLOAD )
			{
				// header size exceeds
				timer_Stop( &p_data->rx_timeout );
				p_data->state = DATALINK_IDLE;
				break;
			}

			p_data->buffer = pipe_alloc_buffer( p_data->size );
			if( p_data->buffer == NULL )
			{
				// failed to mmalloc
				timer_Stop( &p_data->rx_timeout );
				p_data->state = DATALINK_IDLE;
				break;
			}

			// valid size, now get the payload
			p_data->state = DATALINK_WAIT_SIZE;
			p_data->pointer = 0;
			timer_Start( &p_data->rx_timeout, DATALINK_RX_TIMEOUT );
			// fall to next case

		case DATALINK_RX_PAYLOAD:
			while( device_is_rx_pending( p_data->comm_dev ) )
			{
				rx_size = ( uint16_t )device_is_rx_pending( p_data->comm_dev );
				if( rx_size > p_data->size ) rx_size = p_data->size;
				if( p_data->pointer < p_data->size )
				{
					device_read_buffer( p_data->comm_dev, &p_data->buffer[p_data->pointer], rx_size );
					p_data->pointer += rx_size;
					timer_Start( &p_data->rx_timeout, DATALINK_RX_TIMEOUT );
				}

				if( p_data->pointer >= p_data->size )
				{
					// finished payload, get footer
					device_read_buffer( p_data->comm_dev, &rx_char, 1 );
					if( rx_char == p_data->footer )
					{
						pipe_write( &p_data->pipe, p_data->buffer, p_data->size );
					}
					else
					{
						pipe_release_buffer( p_data->buffer );
						p_data->buffer = NULL;
					}

					timer_Stop( &p_data->rx_timeout );
					p_data->state = DATALINK_IDLE;
					break;
				}
			}
			break;
	}
}

