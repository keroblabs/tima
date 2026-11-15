#include "buffer.h"

////////////////////////////////////////////////////////////////////////////////////////

#define MASK_ON_HEAP                0x02
#define MASK_READ_ONLY				0x04

////////////////////////////////////////////////////////////////////////////////////////

void buffer_init( buffer_data_t * buffer_data, uint8_t * buffer_ptr, uint16_t size )
{
    // init buffer
    buffer_data->buffer = buffer_ptr;
    buffer_data->size = size;
    buffer_data->pointer_in = 0;
    buffer_data->pointer_out = 0;
    buffer_data->mask           = 0;
    buffer_data->user_count     = 1;
}

buffer_data_t * buffer_create( uint16_t size )
{
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
	DI();

    if( buffer_data->user_count > 0 ) buffer_data->user_count--;

    if( ( buffer_data->mask & MASK_ON_HEAP ) && ( buffer_data->user_count == 0 ) )
    {
        MFREE( buffer_data->buffer );
        MFREE( buffer_data );
    }

    EI();
}

void buffer_set_read_only( buffer_data_t * buffer_data, bool_t state )
{
    if( state == TRUE ) buffer_data->mask |= MASK_READ_ONLY;
    else buffer_data->mask &= ~MASK_READ_ONLY;
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
	DI();
    buffer_data->buffer = buffer_ptr;
    EI();
}

void buffer_reset_read( buffer_data_t * buffer_data )
{
	DI();
    buffer_data->pointer_out = 0;
    EI();
}

void buffer_read_set_offset( buffer_data_t * buffer_data, uint32_t offset )
{
    buffer_data->pointer_out = MIN( offset, buffer_data->pointer_in );
}

uint32_t buffer_get_current_index( buffer_data_t * buffer_data )
{
	uint32_t ret;

	DI();

    ret =  buffer_data->pointer_out;

    EI();

    return ret;
}

uint32_t buffer_get_max_index( buffer_data_t * buffer_data )
{
	uint32_t ret;

	DI();

    ret =  buffer_data->pointer_in;

    EI();

    return ret;
}

void buffer_resize( buffer_data_t * buffer_data, uint32_t new_size )
{
	if( !( buffer_data->mask & MASK_ON_HEAP ) ) return;

	DI();

    uint8_t * new_buffer = ( uint8_t * )MMALLOC( new_size );
    if( new_buffer != NULL )
    {
        memcpy( new_buffer, buffer_data->buffer, MIN( new_size, buffer_data->size ) );
        MFREE( buffer_data->buffer );

        buffer_data->buffer = new_buffer;
        buffer_data->size   = new_size;
    }

    EI();
}

void buffer_add_user( buffer_data_t * buffer_data )
{
	DI();

	buffer_data->user_count++;

	EI();
}

const uint8_t * buffer_get_pointer_at( buffer_data_t * buffer_data, uint32_t offset )
{
	uint8_t * ret;

	DI();

    ret = &buffer_data->buffer[ MIN( offset, buffer_data->pointer_in ) ];

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

uint16_t buffer_write_buffer( buffer_data_t * buffer_data, uint8_t * in_data, uint16_t size )
{
	uint16_t counter;
	
    //DI();
    
	for( counter = 0; counter < size; counter++ )
	{
		if( buffer_write( buffer_data, in_data[ counter ] ) == FALSE ) break;
	}
    
    //EI();

	return counter;
}

uint16_t buffer_write_buffer_ex( buffer_data_t * buffer_data, buffer_data_t * buffer_input )
{
    return buffer_write_buffer( buffer_data, buffer_input->buffer, buffer_input->pointer_in );
}

uint16_t buffer_read_buffer( buffer_data_t * buffer_data, uint8_t * out_data, uint16_t size )
{
	uint16_t counter;
	
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

uint16_t buffer_get_size( buffer_data_t * buffer_data )
{
    uint16_t size;

	if( buffer_data == NULL ) return 0;
    
    DI();
    
    if( buffer_data->pointer_in >= buffer_data->pointer_out )
    {
        // in and out at same side
        size = buffer_data->pointer_in - buffer_data->pointer_out;
    }
    else
    {
        // in turned around
        size = ( buffer_data->size - buffer_data->pointer_out ) + buffer_data->pointer_in;
    }

    EI();
    
    return( size );
}

uint16_t buffer_get_available_size( buffer_data_t * buffer_data )
{
    return buffer_data->size - buffer_get_size( buffer_data );
}

bool_t buffer_is_full( buffer_data_t * buffer_data )
{        
    bool_t ret = FALSE;
    
	if( buffer_data == NULL ) return FALSE;
	
    DI();
    
    // input is just before output
	if( buffer_data->pointer_in == ( buffer_data->pointer_out - 1 ) ) ret = TRUE;

    // input is in the tail, and output is in the head
	else if( ( buffer_data->pointer_in == ( buffer_data->size - 1 ) ) &&
             ( buffer_data->pointer_out == 0 ) ) ret = TRUE;

    EI();

    return ret;
}

bool_t buffer_is_empty( buffer_data_t * buffer_data )
{
    bool_t ret = FALSE;
    
	if( buffer_data == NULL ) return TRUE;

    DI();

	if( buffer_data->pointer_in == buffer_data->pointer_out ) ret = TRUE;

    EI();
    
    return ret;
}

