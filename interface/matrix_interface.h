#ifndef __matrix_interface_h__
#define __matrix_interface_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

void * matrix_kbd_init( uint8_t width );

void matrix_kbd_set_bitmask( void * p_data, uint8_t * mask, uint8_t height );

void matrix_kbd_set_keyboard( void * p_data, uint8_t height );

void matrix_kbd_process( void * p_data );

uint16_t matrix_kdb_read( void * p_data );

///////////////////////////////////////////////////////////////

#endif // __matrix_interface_h__
