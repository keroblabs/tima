#ifndef _DRAW_H
#define _DRAW_H

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct _font_data_t
{
	uint16_t width;
	uint16_t height;
	uint16_t offset;
	uint16_t size;

	const uint8_t * data;

} font_data_t;

//////////////////////////////////////////////////////////////////////////////////////

enum
{
	FONT_4x6,
	FONT_5x12,
	FONT_6x8,
	FONT_8x8,
	FONT_10x18,
	FONT_12x16,
};

//////////////////////////////////////////////////////////////////////////////////////

extern const font_data_t font_12x16;
extern const font_data_t font_8x8;

//////////////////////////////////////////////////////////////////////////////////////

void draw_font_init( uint8_t index );

void draw_line( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t colour );
void draw_text( uint16_t posx, uint16_t posy, char * text, uint16_t fore_colour, uint16_t back_colour );

void draw_text_vertical( uint16_t posx, uint16_t posy, char * text, uint16_t fore_colour, uint16_t back_colour );

uint32_t draw_get_text_height( char * text );
uint32_t draw_get_text_width( char * text );

void draw_circle( uint16_t posx, uint16_t posy, int ratio, uint16_t colour );
void draw_circle_arc( uint16_t posx, uint16_t posy, int ratio, int start, int end, uint16_t colour );

int draw_text_printk( uint16_t posx, uint16_t posy, const char *fmt, ... );

uint32_t draw_restore_line_data( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t * buffer, uint32_t max_size );
uint32_t draw_get_line_data( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t * buffer, uint32_t max_size );

//////////////////////////////////////////////////////////////////////////////////////

#endif
