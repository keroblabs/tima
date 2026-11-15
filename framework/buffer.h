#ifndef __BUFFER_H__
#define __BUFFER_H__

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t pointer_in;
    uint16_t pointer_out;

    uint8_t mask;
    uint8_t user_count;

    uint16_t size;
    uint16_t fill_size;

    uint8_t *buffer;

} buffer_data_t;

//////////////////////////////////////////////////////////////////////////////////////

void buffer_init( buffer_data_t * buffer_data, uint8_t * buffer_ptr, uint16_t size );

buffer_data_t * buffer_create( uint16_t size );

void buffer_add_user( buffer_data_t * buffer_data );
void buffer_destroy( buffer_data_t * buffer_data );
void buffer_reset( buffer_data_t * buffer_data );
void buffer_reset_read( buffer_data_t * buffer_data );
void buffer_read_set_offset( buffer_data_t * buffer_data, uint32_t offset );

uint32_t buffer_get_current_index( buffer_data_t * buffer_data );
uint32_t buffer_get_max_index( buffer_data_t * buffer_data );

void buffer_set_buffer( buffer_data_t * buffer_data, uint8_t * buffer_ptr );
void buffer_resize( buffer_data_t * buffer_data, uint32_t new_size );

const uint8_t * buffer_get_pointer_at( buffer_data_t * buffer_data, uint32_t offset );
const uint8_t * buffer_get_pointer( buffer_data_t * buffer_data );

////////////////////////////////////////////////////////////////////////////////

uint8_t buffer_read( buffer_data_t * buffer_data );

bool_t buffer_write( buffer_data_t * buffer_data, uint8_t in_data );

uint16_t buffer_write_buffer( buffer_data_t * buffer_data, uint8_t * in_data, uint16_t size );
uint16_t buffer_write_buffer_ex( buffer_data_t * buffer_data, buffer_data_t * buffer_input );
uint16_t buffer_read_buffer( buffer_data_t * buffer_data, uint8_t * out_data, uint16_t size );
uint16_t buffer_get_size( buffer_data_t * buffer_data );
uint16_t buffer_get_available_size( buffer_data_t * buffer_data );

bool_t buffer_is_full( buffer_data_t * buffer_data );
bool_t buffer_is_empty( buffer_data_t * buffer_data );

//////////////////////////////////////////////////////////////////////////////////////

#endif // __BUFFER_H__

