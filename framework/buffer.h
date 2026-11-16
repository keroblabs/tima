#ifndef __BUFFER_H__
#define __BUFFER_H__

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint32_t pointer_in;
    uint32_t pointer_out;

    uint8_t mask;
    uint8_t user_count;

    uint32_t size;
    uint8_t *buffer;

} buffer_data_t;

//////////////////////////////////////////////////////////////////////////////////////

// Circular storage needs at least two bytes (one usable slot plus the sentinel).
// Backing storage and ownership are fixed by buffer_init() or buffer_create();
// changing buffer_data->buffer directly is unsupported. Do not reinitialize
// a heap-created descriptor with buffer_init().
// Callers of buffer_init() must provide at least size accessible backing bytes.
// Invalid initialization leaves an inert descriptor; buffer_create() returns NULL.
// Data operations treat invalid buffers as empty: reads and size/count getters
// return zero, writes return FALSE, pointer getters return NULL, and invalid
// buffers are empty and not full. Null descriptors and nonzero bulk operations
// with null data pointers return empty/failure results or no-op.
void buffer_init( buffer_data_t * buffer_data, uint8_t * buffer_ptr, uint32_t size );

buffer_data_t * buffer_create( uint32_t size );

// Buffer lifecycle operations may use the heap and are not ISR-safe. Call them
// from task context. buffer_resize() and final buffer_destroy() require
// exclusive access to the buffer. Callers must also stop using pointers
// returned by buffer_get_pointer[_at]() before resizing or destroying the
// final reference.
void buffer_add_user( buffer_data_t * buffer_data );
void buffer_destroy( buffer_data_t * buffer_data );
void buffer_reset( buffer_data_t * buffer_data );
void buffer_reset_read( buffer_data_t * buffer_data );
void buffer_read_set_offset( buffer_data_t * buffer_data, uint32_t offset );

uint32_t buffer_get_current_index( buffer_data_t * buffer_data );
uint32_t buffer_get_max_index( buffer_data_t * buffer_data );

void buffer_resize( buffer_data_t * buffer_data, uint32_t new_size );

const uint8_t * buffer_get_pointer_at( buffer_data_t * buffer_data, uint32_t offset );
const uint8_t * buffer_get_pointer( buffer_data_t * buffer_data );

////////////////////////////////////////////////////////////////////////////////

// Returns TRUE and consumes one byte, including 0x00. Returns FALSE for an
// empty/invalid buffer or null arguments, leaving out_data unchanged.
bool_t buffer_try_read( buffer_data_t * buffer_data, uint8_t * out_data );

// Compatibility helper: returns 0 for both an empty buffer and a stored 0x00.
uint8_t buffer_read( buffer_data_t * buffer_data );

bool_t buffer_write( buffer_data_t * buffer_data, uint8_t in_data );

// Circular storage reserves one slot: usable capacity is size - 1. Writes continue
// when full and overwrite the oldest unread byte; available size reports space
// before that overwrite occurs.
// Bulk reads and writes are partial, non-atomic batches: each byte is processed
// separately, and concurrent buffer operations may interleave between bytes.
uint32_t buffer_write_buffer( buffer_data_t * buffer_data, uint8_t * in_data, uint32_t size );
uint32_t buffer_write_buffer_ex( buffer_data_t * buffer_data, buffer_data_t * buffer_input );
uint32_t buffer_read_buffer( buffer_data_t * buffer_data, uint8_t * out_data, uint32_t size );
uint32_t buffer_get_size( buffer_data_t * buffer_data );
uint32_t buffer_get_available_size( buffer_data_t * buffer_data );

bool_t buffer_is_full( buffer_data_t * buffer_data );
bool_t buffer_is_empty( buffer_data_t * buffer_data );

//////////////////////////////////////////////////////////////////////////////////////

#endif // __BUFFER_H__

