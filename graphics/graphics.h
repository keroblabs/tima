#ifndef __graphics_h__
#define __graphics_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t * buffer;

    uint16_t width;
    uint16_t height;

    uint16_t background_colour;

} bitmap_t;

///////////////////////////////////////////////////////////////

void graphics_init( void );
void graphics_set_landscape( void );
void graphics_set_portrait( void );
void graphics_init_auto_vsync( void );

bool_t graphics_driver_ready( void );

void graphics_process( void );

uint16_t graphics_get_pixel( uint16_t posx, uint16_t posy );

void graphics_show_bitmap( uint16_t posx, uint16_t posy, const bitmap_t * bitmap, bool_t use_transparent );
void graphics_set_pixel( uint16_t posx, uint16_t posy, uint16_t colour );
void graphics_set_bitmap_rgb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t * data, int i_transparent );
void graphics_get_bitmap_rgb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t * data );
void graphics_fill_box( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t colour );

int graphics_read_ts( uint16_t * posx, uint16_t * posy, uint16_t * posz );

void graphics_flip_frame( void );

uint16_t * graphics_get_target( void );

uint16_t graphics_width( void );
uint16_t graphics_height( void );

void graphics_bitmap_1bb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t heigth, 
					      uint8_t *input, uint16_t back_colour, uint16_t fore_colour );

///////////////////////////////////////////////////////////////

#endif // __graphics_h__
