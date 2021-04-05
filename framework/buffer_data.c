#include "buffer_data.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

buffer_t * buffer_data_alloc( uint32_t size )
{
    buffer_t * ret = ( buffer_t * )MMALLOC( sizeof( buffer_t ) );
    if( ret == NULL ) return NULL;

    ret->max_size = size;
    ret->size = 0;
    ret->data = NULL;

    return ret;
}

buffer_t * buffer_data_create( void * data, uint32_t size )
{
    buffer_t * ret = buffer_data_alloc( size );
    if( ret == NULL ) return NULL;
    
    buffer_data_append( ret, data, size );
    return ret;
}

void buffer_data_append( buffer_t * buffer_data, void * data, uint32_t size )
{
    void * new_data;

    if( ( buffer_data->size + size ) > buffer_data->max_size ) return;

    new_data = MMALLOC( buffer_data->size + size );
    if( new_data == NULL ) return;

    if( buffer_data->data != NULL )
    {
        memcpy( new_data, buffer_data->data, buffer_data->size );
        MFREE( buffer_data->data );
    }

    memcpy( new_data + buffer_data->size, data, size );
    buffer_data->data = data;
    buffer_data->size += size;
}

void buffer_data_clear( buffer_t * buffer_data )
{
    if( buffer_data->data != NULL )
    {
        MFREE( buffer_data->data );
        buffer_data->data = NULL;
    }

    buffer_data->size = 0;
}

void buffer_data_destroy( buffer_t * buffer_data )
{
    buffer_data_clear( buffer_data );
    MFREE( buffer_data );
}

void * buffer_data_get_rw_at( buffer_t * buffer_data, uint32_t pos )
{
    if( pos >= buffer_data->size ) return NULL;
    return buffer_data->data + pos;
}

void * buffer_data_get_rw( buffer_t * buffer_data )
{
    return buffer_data_get_rw_at( buffer_data, 0 );
}

uint32_t buffer_data_size( buffer_t * buffer_data )
{
    return buffer_data->size;
}
