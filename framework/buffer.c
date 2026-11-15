#include "buffer.h"

////////////////////////////////////////////////////////////////////////////////////////

#define MASK_ON_HEAP                0x02

////////////////////////////////////////////////////////////////////////////////////////

static bool_t _buffer_is_valid( buffer_data_t * buffer_data )
{
    return ( buffer_data != NULL ) &&
           ( buffer_data->buffer != NULL ) &&
           ( buffer_data->size >= 2 ) &&
           ( buffer_data->pointer_in < buffer_data->size ) &&
           ( buffer_data->pointer_out < buffer_data->size );
}

void buffer_init( buffer_data_t * buffer_data, uint8_t * buffer_ptr, uint32_t size )
{
    if( buffer_data == NULL ) return;

    // init buffer
    buffer_data->buffer = NULL;
    buffer_data->size = 0;
    buffer_data->pointer_in = 0;
    buffer_data->pointer_out = 0;
    buffer_data->mask = 0;
    buffer_data->user_count = 1;
    buffer_data->fill_size = 0;

    if( ( buffer_ptr == NULL ) || ( size < 2 ) ) return;

    buffer_data->buffer = buffer_ptr;
    buffer_data->size = size;
}

buffer_data_t * buffer_create( uint32_t size )
{
    if( size < 2 ) return NULL;

    buffer_data_t * buffer_data = ( buffer_data_t * )MMALLOC( sizeof( buffer_data_t ) );
    if( buffer_data == NULL )
    {
        return NULL;
    }

    uint8_t * buffer_ptr = ( uint8_t * )MMALLOC( size );
    if( buffer_ptr == NULL )
    {
        MFREE( buffer_data );
        return NULL;
    }

    buffer_init( buffer_data, buffer_ptr, size );
    buffer_data->mask |= MASK_ON_HEAP;

    return buffer_data;
}

void buffer_destroy( buffer_data_t * buffer_data )
{
	if( buffer_data == NULL ) return;

	DI();

    if( buffer_data->user_count > 0 ) buffer_data->user_count--;

    if( ( buffer_data->mask & MASK_ON_HEAP ) && ( buffer_data->user_count == 0 ) )
    {
        MFREE( buffer_data->buffer );
        MFREE( buffer_data );
    }

    EI();
}

void buffer_reset( buffer_data_t * buffer_data )
{
	if( buffer_data == NULL ) return;

	DI();
    buffer_data->pointer_in   = 0;
    buffer_data->pointer_out  = 0;
    EI();
}

void buffer_set_buffer( buffer_data_t * buffer_data, uint8_t * buffer_ptr )
{
	if( ( buffer_data == NULL ) || ( buffer_ptr == NULL ) ) return;

	DI();
    buffer_data->buffer = buffer_ptr;
    EI();
}

void buffer_reset_read( buffer_data_t * buffer_data )
{
	if( buffer_data == NULL ) return;

	DI();
    buffer_data->pointer_out = 0;
    EI();
}

void buffer_read_set_offset( buffer_data_t * buffer_data, uint32_t offset )
{
    if( !_buffer_is_valid( buffer_data ) ) return;

    buffer_data->pointer_out = MIN( offset, buffer_data->pointer_in );
}

uint32_t buffer_get_current_index( buffer_data_t * buffer_data )
{
	uint32_t ret;
	if( buffer_data == NULL ) return 0;

	DI();

    ret = _buffer_is_valid( buffer_data ) ? buffer_data->pointer_out : 0;

    EI();

    return ret;
}

uint32_t buffer_get_max_index( buffer_data_t * buffer_data )
{
	uint32_t ret;
	if( buffer_data == NULL ) return 0;

	DI();

    ret = _buffer_is_valid( buffer_data ) ? buffer_data->pointer_in : 0;

    EI();

    return ret;
}

void buffer_resize( buffer_data_t * buffer_data, uint32_t new_size )
{
	if( ( buffer_data == NULL ) || ( new_size < 2 ) ) return;
	if( !( buffer_data->mask & MASK_ON_HEAP ) ) return;

	DI();
	if( !_buffer_is_valid( buffer_data ) )
	{
	    EI();
	    return;
	}

    uint8_t * new_buffer = ( uint8_t * )MMALLOC( new_size );
    if( new_buffer != NULL )
    {
        uint32_t old_size = buffer_data->size;
        uint32_t unread_size = 0;
        uint32_t copy_size;
        uint32_t source_index;

        if( ( old_size > 0 ) && ( buffer_data->buffer != NULL ) )
        {
            if( buffer_data->pointer_in >= buffer_data->pointer_out )
            {
                unread_size = buffer_data->pointer_in - buffer_data->pointer_out;
            }
            else
            {
                unread_size = ( old_size - buffer_data->pointer_out ) + buffer_data->pointer_in;
            }
        }

        copy_size = MIN( unread_size, new_size - 1 );
        source_index = buffer_data->pointer_out;

        // On shrink, retain the newest unread bytes, matching overwrite-oldest writes.
        for( uint32_t skip = unread_size - copy_size; skip > 0; skip-- )
        {
            source_index++;
            if( source_index == old_size ) source_index = 0;
        }

        for( uint32_t index = 0; index < copy_size; index++ )
        {
            new_buffer[ index ] = buffer_data->buffer[ source_index ];
            source_index++;
            if( source_index == old_size ) source_index = 0;
        }

        MFREE( buffer_data->buffer );

        buffer_data->buffer = new_buffer;
        buffer_data->size   = new_size;
        buffer_data->pointer_out = 0;
        buffer_data->pointer_in = copy_size;
    }

    EI();
}

void buffer_add_user( buffer_data_t * buffer_data )
{
	if( buffer_data == NULL ) return;

	DI();

	buffer_data->user_count++;

	EI();
}

const uint8_t * buffer_get_pointer_at( buffer_data_t * buffer_data, uint32_t offset )
{
	const uint8_t * ret = NULL;
	if( buffer_data == NULL ) return NULL;

	DI();

    if( _buffer_is_valid( buffer_data ) )
    {
        ret = &buffer_data->buffer[ MIN( offset, buffer_data->pointer_in ) ];
    }

    EI();

    return ret;
}

const uint8_t * buffer_get_pointer( buffer_data_t * buffer_data )
{
    return buffer_get_pointer_at( buffer_data, 0 );
}

////////////////////////////////////////////////////////////////////////////////

uint8_t buffer_read( buffer_data_t * buffer_data )
{
	uint8_t ret_value = 0;

	if( buffer_data == NULL ) return 0;

    DI();

    if( !_buffer_is_valid( buffer_data ) )
    {
        EI();
        return 0;
    }

    // buffer is empty
	if( buffer_data->pointer_in == buffer_data->pointer_out )
    {
        EI();
        return( 0 );
    }

    // get byte from buffer
	ret_value = buffer_data->buffer[ buffer_data->pointer_out ];

    // move out-pointer
	buffer_data->pointer_out++;
	if( buffer_data->pointer_out == buffer_data->size )
	{
	    buffer_data->pointer_out = 0;
	}
    
    EI();
    
	return( ret_value );
}

bool_t buffer_write( buffer_data_t * buffer_data, uint8_t in_data )
{	
	if( buffer_data == NULL ) return FALSE;

    DI();

    if( !_buffer_is_valid( buffer_data ) )
    {
        EI();
        return FALSE;
    }

    // insert data into the circular buffer
	buffer_data->buffer[ buffer_data->pointer_in ] = in_data;
	buffer_data->pointer_in++;

    // update pointers and discard the oldest entry on overflow
	if( buffer_data->pointer_in == buffer_data->size ) buffer_data->pointer_in = 0;
	if( buffer_data->pointer_in == buffer_data->pointer_out ) buffer_data->pointer_out++;
	if( buffer_data->pointer_out == buffer_data->size ) buffer_data->pointer_out = 0;

    EI();

    return TRUE;
}

uint32_t buffer_write_buffer( buffer_data_t * buffer_data, uint8_t * in_data, uint32_t size )
{
	uint32_t counter;
	if( ( size > 0 ) && ( in_data == NULL ) ) return 0;
	
    //DI();
    
	for( counter = 0; counter < size; counter++ )
	{
		if( buffer_write( buffer_data, in_data[ counter ] ) == FALSE ) break;
	}
    
    //EI();

	return counter;
}

uint32_t buffer_write_buffer_ex( buffer_data_t * buffer_data, buffer_data_t * buffer_input )
{
    if( ( buffer_input == NULL ) || ( buffer_input->buffer == NULL ) ||
        ( buffer_input->size < 2 ) || ( buffer_input->pointer_in > buffer_input->size ) ||
        ( buffer_input->pointer_out >= buffer_input->size ) ) return 0;

    return buffer_write_buffer( buffer_data, buffer_input->buffer, buffer_input->pointer_in );
}

uint32_t buffer_read_buffer( buffer_data_t * buffer_data, uint8_t * out_data, uint32_t size )
{
	uint32_t counter;
	if( ( buffer_data == NULL ) || ( ( size > 0 ) && ( out_data == NULL ) ) ||
	    !_buffer_is_valid( buffer_data ) ) return 0;
	
    //DI();
    
	for( counter = 0; counter < size; counter++ )
	{
		// buffer is empty
		if( buffer_data->pointer_in == buffer_data->pointer_out ) break;
	
		out_data[ counter ] = buffer_read( buffer_data );
	}
	
    //EI();
    
	return counter;
}

static uint32_t _buffer_get_size_unlocked( buffer_data_t * buffer_data )
{
    if( buffer_data->pointer_in >= buffer_data->pointer_out )
    {
        // in and out at same side
        return buffer_data->pointer_in - buffer_data->pointer_out;
    }

    // in turned around
    return ( buffer_data->size - buffer_data->pointer_out ) + buffer_data->pointer_in;
}

uint32_t buffer_get_size( buffer_data_t * buffer_data )
{
    uint32_t size;

	if( buffer_data == NULL ) return 0;

    DI();
    size = _buffer_is_valid( buffer_data ) ? _buffer_get_size_unlocked( buffer_data ) : 0;
    EI();

    return( size );
}

uint32_t buffer_get_available_size( buffer_data_t * buffer_data )
{
    uint32_t available_size = 0;
	if( buffer_data == NULL ) return 0;

    DI();

    if( _buffer_is_valid( buffer_data ) )
    {
        uint32_t max_size = buffer_data->size - 1;
        uint32_t current_size = _buffer_get_size_unlocked( buffer_data );

        if( current_size < max_size ) available_size = max_size - current_size;
    }

    EI();

    return available_size;
}

bool_t buffer_is_full( buffer_data_t * buffer_data )
{        
    bool_t ret = FALSE;
    uint32_t current_size;

	if( buffer_data == NULL ) return FALSE;

    DI();

    if( _buffer_is_valid( buffer_data ) )
    {
        current_size = _buffer_get_size_unlocked( buffer_data );
        if( current_size >= ( buffer_data->size - 1 ) ) ret = TRUE;
    }

    EI();

    return ret;
}

bool_t buffer_is_empty( buffer_data_t * buffer_data )
{
    bool_t ret = FALSE;
    
	if( buffer_data == NULL ) return TRUE;

    DI();

	if( !_buffer_is_valid( buffer_data ) ||
	    ( buffer_data->pointer_in == buffer_data->pointer_out ) ) ret = TRUE;

    EI();
    
    return ret;
}

