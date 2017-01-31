#include "buffer.h"

////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////

void buffer_init( buffer_data_t * buffer_data, uint8_t * buffer_ptr, uint16_t size )
{
    // init buffer
    buffer_data->buffer = buffer_ptr;
    buffer_data->size = size;
    buffer_data->pointer_in = 0;
    buffer_data->pointer_out = 0;
}

void buffer_reset( buffer_data_t * buffer_data )
{
	if( buffer_data == NULL ) return;
    buffer_data->pointer_in = 0;
    buffer_data->pointer_out = 0;
}

void buffer_set_buffer( buffer_data_t * buffer_data, uint8_t * buffer_ptr )
{
    buffer_data->buffer = buffer_ptr;
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
	if( buffer_data->pointer_out == buffer_data->size ) buffer_data->pointer_out = 0;
    
    EI();
    
	return( ret_value );
}

void buffer_write( buffer_data_t * buffer_data, uint8_t in_data )
{	
	if( buffer_data == NULL ) return;
	
    DI();
    
    // insert data into the buffer
	buffer_data->buffer[ buffer_data->pointer_in ] = in_data;
	buffer_data->pointer_in++;
    
    // update pointers and control overflow
	if( buffer_data->pointer_in == buffer_data->size ) buffer_data->pointer_in = 0;
	if( buffer_data->pointer_in == buffer_data->pointer_out ) buffer_data->pointer_out++;
	if( buffer_data->pointer_out == buffer_data->size ) buffer_data->pointer_out = 0;
    
    EI();
}

void buffer_write_buffer( buffer_data_t * buffer_data, uint8_t * in_data, uint16_t size )
{
	uint16_t counter;
	
    //DI();
    
	for( counter = 0; counter < size; counter++ )
	{
		buffer_write( buffer_data, in_data[ counter ] );
	}
    
    //EI();
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

