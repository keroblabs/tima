#include "mbulk.h"

///////////////////////////////////////////////////////////////////////////////////////

enum
{
	BULK_READ_ONLY,
	BULK_BUFFER_ON_HEAP,
	BULK_DATA_ON_HEAP,

};

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

static bulk_buffer_t * create_bulk_buffer( bulk_buffer_t * bulk_buffer, uint8_t * buffer, uint32_t size )
{
	bulk_buffer_t * ret = ( bulk_buffer == NULL ) ? ( bulk_buffer_t * )MMALLOC( sizeof( bulk_buffer ) ) : bulk_buffer;

	if( bulk_buffer == NULL )
	{
		bulk_buffer = ( bulk_buffer_t * )MMALLOC( sizeof( bulk_buffer ) );
		if( bulk_buffer == NULL ) return NULL;

		memset( bulk_buffer, 0x00, sizeof( bulk_buffer_t ) );

		bulk_buffer->mask |= BULK_DATA_ON_HEAP;
	}
	else
	{
		memset( bulk_buffer, 0x00, sizeof( bulk_buffer_t ) );
	}

	if( buffer == NULL )
	{
		bulk_buffer->buffer = ( uint8_t * )MMALLOC( sizeof( size ) ) ;
		if( bulk_buffer->buffer == NULL )
		{
			if( bulk_buffer->mask & BULK_DATA_ON_HEAP )
			{
				MFREE( bulk_buffer );
			}
			return NULL;
		}

		bulk_buffer->mask |= BULK_BUFFER_ON_HEAP;
	}
	else
	{
		bulk_buffer->buffer = buffer;
	}

	memset( bulk_buffer->buffer, 0x00, size );

	bulk_buffer->max_size = size;
	bulk_buffer->users    = 1;
	bulk_buffer->add_size = 0;

	return bulk_buffer;
}

///////////////////////////////////////////////////////////////////////////////////////

bulk_buffer_t * bulk_buffer_create( uint32_t size )
{
	return create_bulk_buffer( NULL, NULL, size );
}

bulk_buffer_t * bulk_buffer_create_with_buffer( uint8_t * buffer, uint32_t size )
{
	return create_bulk_buffer( NULL, buffer, size );
}

bulk_buffer_t * bulk_buffer_convert_from_buffer( uint8_t * buffer, uint32_t size )
{
	return create_bulk_buffer( ( bulk_buffer_t * )buffer, &buffer[sizeof(bulk_buffer_t)], size - sizeof(bulk_buffer_t) );
}

bulk_buffer_t * bulk_buffer_duplicate( bulk_buffer_t * bulk_buffer )
{
	bulk_buffer_t * new_bulk = create_bulk_buffer( NULL, NULL, bulk_buffer->max_size );
	if( new_bulk == NULL ) return NULL;

	memcpy( new_bulk->buffer, bulk_buffer->buffer, bulk_buffer->max_size );

	return new_bulk;
}

void bulk_buffer_create_with_buffer_ex( bulk_buffer_t * bulk_buffer, uint8_t * buffer, uint32_t size )
{
	create_bulk_buffer( bulk_buffer, buffer, size );
}

void bulk_buffer_destroy( bulk_buffer_t * bulk_buffer )
{
	if( bulk_buffer->users == 0 ) return;

	if( --bulk_buffer->users == 0 )
	{
		if( bulk_buffer->mask & BULK_BUFFER_ON_HEAP )
		{
			MFREE( bulk_buffer->mask );
		}

		if( bulk_buffer->mask & BULK_DATA_ON_HEAP )
		{
			MFREE( bulk_buffer );
		}
	}
}

void bulk_buffer_add_user( bulk_buffer_t * bulk_buffer )
{
	if( bulk_buffer->users != 0 ) bulk_buffer->users++;
}

void bulk_buffer_set_add_size( bulk_buffer_t * bulk_buffer, uint32_t value )
{
	bulk_buffer->add_size = value;
}

void bulk_buffer_add_data( bulk_buffer_t * bulk_buffer, uint8_t * data, uint32_t size )
{
	if( ( bulk_buffer->curr_size + size ) > bulk_buffer->max_size )
	{
		if( !( bulk_buffer->mask & BULK_BUFFER_ON_HEAP ) ) return;

		uint32_t new_size = ( ( ( bulk_buffer->curr_size + size ) / bulk_buffer->add_size ) + 1 ) * bulk_buffer->add_size;
		uint8_t * new_buffer = ( uint8_t * )MMALLOC( new_size );
		if( new_buffer == NULL ) return;

		memcpy( new_buffer, bulk_buffer->buffer, bulk_buffer->max_size );
		MFREE( bulk_buffer->buffer );

		bulk_buffer->buffer   = new_buffer;
		bulk_buffer->max_size = new_size;
	}

	memcpy( &bulk_buffer->buffer[bulk_buffer->curr_size], data, size );
	bulk_buffer->curr_size += size;
}

void bulk_buffer_extract_data( bulk_buffer_t * bulk_buffer, uint32_t offset, uint8_t * data, uint32_t size )
{
	if( ( size + offset ) > bulk_buffer->curr_size )
	{
		size = bulk_buffer->curr_size - offset;
	}

	memcpy( data, &bulk_buffer->buffer[offset], size );
}

uint8_t * bulk_buffer_pointer( bulk_buffer_t * bulk_buffer )
{
	if( bulk_buffer->users == 0 ) return NULL;

	return bulk_buffer->buffer;
}

uint8_t * bulk_buffer_pointer_at( bulk_buffer_t * bulk_buffer, uint32_t offset )
{
	if( bulk_buffer->users == 0 ) return NULL;

	return &bulk_buffer->buffer[offset];
}

uint32_t bulk_buffer_current_size( bulk_buffer_t * bulk_buffer )
{
	if( bulk_buffer->users == 0 ) return 0;

	return bulk_buffer->curr_size;
}

uint32_t bulk_buffer_max_size( bulk_buffer_t * bulk_buffer )
{
	if( bulk_buffer->users == 0 ) return 0;

	return bulk_buffer->max_size;
}
