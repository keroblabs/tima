#ifndef __MBULK_H__
#define __MBULK_H__

///////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

typedef struct _mbulk_buffer_t_
{
	uint8_t * buffer;

	uint32_t max_size;
	uint32_t curr_size;
	uint32_t add_size;

	uint32_t users;

	uint32_t mask;

} bulk_buffer_t;

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

bulk_buffer_t * bulk_buffer_create( uint32_t size );

bulk_buffer_t * bulk_buffer_create_with_buffer( uint8_t * buffer, uint32_t size );

bulk_buffer_t * bulk_buffer_duplicate( bulk_buffer_t * bulk_buffer );

bulk_buffer_t * bulk_buffer_convert_from_buffer( uint8_t * buffer, uint32_t size );

void bulk_buffer_create_with_buffer_ex( bulk_buffer_t * bulk_buffer, uint8_t * buffer, uint32_t size );

void bulk_buffer_destroy( bulk_buffer_t * bulk_buffer );

void bulk_buffer_add_user( bulk_buffer_t * bulk_buffer );

void bulk_buffer_set_add_size( bulk_buffer_t * bulk_buffer, uint32_t value );

void bulk_buffer_add_data( bulk_buffer_t * bulk_buffer, uint8_t * data, uint32_t size );

void bulk_buffer_extract_data( bulk_buffer_t * bulk_buffer, uint32_t offset, uint8_t * data, uint32_t size );

uint8_t * bulk_buffer_pointer( bulk_buffer_t * bulk_buffer );

uint8_t * bulk_buffer_pointer_at( bulk_buffer_t * bulk_buffer, uint32_t offset );

uint32_t bulk_buffer_current_size( bulk_buffer_t * bulk_buffer );

uint32_t bulk_buffer_max_size( bulk_buffer_t * bulk_buffer );

///////////////////////////////////////////////////////////////////////////////////////

#endif // __MBULK_H__

