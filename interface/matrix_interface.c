#include "system.h"
#include "tima_libc.h"
#include "matrix_interface.h"

///////////////////////////////////////////////////////////////

static device_t * dev_matrix;

typedef struct _matrix_data_t_
{
    uint8_t * bitmask;
    
    uint8_t matrix_width;
    
    uint8_t keyboard_height;
    uint8_t matrix_height;
    
    uint8_t curr_posx;
    
} matrix_data_t;

///////////////////////////////////////////////////////////////

void * matrix_kbd_init( uint8_t width )
{
    matrix_data_t * matrix_data;
    matrix_data = ( matrix_data_t * )MMALLOC( sizeof( matrix_data_t ) );
    if( matrix_data == NULL ) return NULL;
    
    dev_matrix = device_open( "/dev/matrix" );
    
    if( width > 15 ) width = 15;
    
    matrix_data->bitmask = NULL;
    matrix_data->matrix_width = width;
    matrix_data->curr_posx = 0xff;
    
    return matrix_data;
}

void matrix_kbd_set_bitmask( void * p_data, uint8_t * mask, uint8_t height )
{
    matrix_data_t * matrix_data = ( matrix_data_t * )p_data;
    if( dev_matrix == NULL ) return;
    
    matrix_data->bitmask = mask;
    matrix_data->matrix_height = height;
}

void matrix_kbd_set_keyboard( void * p_data, uint8_t height )
{
    matrix_data_t * matrix_data = ( matrix_data_t * )p_data;
    if( dev_matrix == NULL ) return;
    
    matrix_data->keyboard_height = height;
}

void matrix_kbd_process( void * p_data )
{
    //matrix_data_t * matrix_data = ( matrix_data_t * )p_data;
    //if( dev_matrix == NULL ) return;
    
}

uint16_t matrix_kdb_read( void * p_data )
{
    uint32_t pio_out;
    uint32_t pio_in;
    uint32_t cnt;
    uint16_t ret;
    
    __IO int i;
    
    matrix_data_t * matrix_data = ( matrix_data_t * )p_data;
    if( dev_matrix == NULL ) return 0;
    
    if( ++matrix_data->curr_posx >= matrix_data->matrix_width )
    {
        matrix_data->curr_posx = 0;
    }
    
    ret = 0;
    pio_out = 1 << matrix_data->curr_posx;
    device_write_buffer( dev_matrix, ( uint8_t * )&pio_out, sizeof(pio_out) );
    
    for( i = 0; i < 10; i++ ) {}
    
    device_read_buffer( dev_matrix, ( uint8_t * )&pio_in, sizeof(pio_in) );
    pio_in ^= NEG_U32;
    if( pio_in == 0 ) return 0;
    
    ret = matrix_data->curr_posx;
    ret <<= matrix_data->keyboard_height;
    
    for( cnt = 0; cnt < matrix_data->keyboard_height; cnt++ )
    {
        if( pio_in & ( 1 << cnt ) )
        {
            ret |= cnt;
            break;
        }
    }
    
    return ret;
}

///////////////////////////////////////////////////////////////



