#ifndef FRAMEWORK_BUFFER_DATA_H_
#define FRAMEWORK_BUFFER_DATA_H_

////////////////////////////////////////////////////////////////////

#include "types.h"

////////////////////////////////////////////////////////////////////

typedef struct buffer_t_
{
    uint32_t size;
    uint32_t max_size;
    void * data;

} buffer_t;

////////////////////////////////////////////////////////////////////

buffer_t * buffer_data_alloc( uint32_t size );
buffer_t * buffer_data_create( void * data, uint32_t size );

void buffer_data_append( buffer_t * buffer_data, void * data, uint32_t size );
void buffer_data_clear( buffer_t * buffer_data );
void buffer_data_destroy( buffer_t * buffer_data );

void * buffer_data_get_rw_at( buffer_t * buffer_data, uint32_t pos );
void * buffer_data_get_rw( buffer_t * buffer_data );

uint32_t buffer_data_size( buffer_t * buffer_data );

////////////////////////////////////////////////////////////////////

#endif /* FRAMEWORK_BUFFER_DATA_H_ */
