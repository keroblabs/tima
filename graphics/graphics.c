#include "system.h"
#include "graphics.h"
#include "timer.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

enum
{
	LCD_MODE_DISABLED,
	LCD_MODE_API,
	LCD_MODE_FRAMEBUFFER,
};

enum
{
	LCD_VIEW_LANDSCAPE,
	LCD_VIEW_PORTRAIT,
};

///////////////////////////////////////////////////////////////

#define VSYNC_TIME_MS		50

///////////////////////////////////////////////////////////////

static lcd_driver_t * lcd_driver;
static bool_t lcd_ready = FALSE;
static bool_t lcd_init_done = FALSE;
static timer_data_t vsync_timer;

///////////////////////////////////////////////////////////////

static uint8_t lcd_mode;
static uint8_t lcd_view;

///////////////////////////////////////////////////////////////

void graphics_init( void )
{
	lcd_mode = LCD_MODE_DISABLED;

	lcd_ready = FALSE;
	lcd_init_done = FALSE;

    lcd_driver = device_get_api( "/dev/lcd" );
	if( lcd_driver == NULL )
	{
		DEBUG_PRINTK( "LCD failed\r\n" );
		return;
	}

	lcd_driver->init();
	lcd_driver->clear_lcd();
	lcd_driver->set_backlight( TRUE );

	lcd_view = LCD_VIEW_LANDSCAPE;

	if( lcd_driver->get_buffer != NULL )
	{
		/* use framebuffer mode */
		lcd_mode = LCD_MODE_FRAMEBUFFER;
	}
	else
	{
		/* use api mode */
		lcd_mode = LCD_MODE_API;
	}

	lcd_ready = TRUE;

	timer_Start( &vsync_timer, VSYNC_TIME_MS );
	timer_Stop( &vsync_timer );
}

void graphics_init_auto_vsync( void )
{
	timer_Start( &vsync_timer, VSYNC_TIME_MS );
}

void graphics_process( void )
{
	if( timer_Check( &vsync_timer ) )
	{
		timer_Reload( &vsync_timer );

		graphics_flip_frame();
	}
}

bool_t graphics_driver_ready( void )
{
	if( lcd_driver->ready == NULL ) return TRUE;
	return lcd_driver->ready();
}

void graphics_set_landscape( void )
{
	lcd_view = LCD_VIEW_LANDSCAPE;
}

void graphics_set_portrait( void )
{
	lcd_view = LCD_VIEW_PORTRAIT;
}

void graphics_flip_frame( void )
{
	if( !lcd_ready ) return;
	if( lcd_driver->update != NULL )
	{
		lcd_driver->update();
	}
}

uint16_t * graphics_get_target( void )
{
	if( !lcd_ready ) return NULL;
	return lcd_driver->get_buffer();
}

uint16_t graphics_width( void )
{
	if( !lcd_ready ) return 0;
	if( lcd_view == LCD_VIEW_PORTRAIT )
	{
		return lcd_driver->height;
	}

	return lcd_driver->width;
}

uint16_t graphics_height( void )
{
	if( !lcd_ready ) return 0;
	if( lcd_view == LCD_VIEW_PORTRAIT )
	{
		return lcd_driver->width;
	}

	return lcd_driver->height;
}

void graphics_clear_screen( void )
{
    if( lcd_mode == LCD_MODE_API )
    {
        lcd_driver->clear_lcd();
    }
    else if( lcd_mode == LCD_MODE_FRAMEBUFFER )
    {
        // memset( lcd_driver->get_buffer(), 0x00, lcd_driver->height * lcd_driver->height * sizeof( uint16_t ) );
        graphics_fill_box(0, 0, lcd_driver->width, lcd_driver->height, 0);
    }
}

void graphics_set_pixel( uint16_t posx, uint16_t posy, uint16_t colour )
{
	uint16_t tx;
	uint16_t * p_pixel;
	uint32_t index;

	if( !lcd_ready ) return;
	if( lcd_mode == LCD_MODE_API )
	{
		lcd_driver->set_pixel( posx, posy, colour );
	}
	else if( lcd_mode == LCD_MODE_FRAMEBUFFER )
	{
		if( lcd_view == LCD_VIEW_PORTRAIT )
		{
			tx = posx;
			posx = lcd_driver->width - posy;
			posy = tx;
		}

		if( posx >= lcd_driver->width ) return;
		if( posy >= lcd_driver->height ) return;

		p_pixel = ( uint16_t * )lcd_driver->get_buffer();
		index = ((uint32_t)posy * lcd_driver->width) + (uint32_t)posx;

		p_pixel[index] = colour;
	}
}

uint16_t graphics_get_pixel( uint16_t posx, uint16_t posy )
{
	uint16_t tx;
	uint16_t * p_pixel;
	uint32_t index;

	if( !lcd_ready ) return 0;
	if( ( lcd_mode == LCD_MODE_API ) && ( lcd_driver->get_pixel != NULL ) )
	{
		return lcd_driver->get_pixel( posx, posy );
	}
	else if( lcd_mode == LCD_MODE_FRAMEBUFFER )
	{
		if( lcd_view == LCD_VIEW_PORTRAIT )
		{
			tx = posx;
			posx = lcd_driver->width - posy;
			posy = tx;
		}

		if( posx >= lcd_driver->width ) return 0;
		if( posy >= lcd_driver->height ) return 0;

		p_pixel = ( uint16_t * )lcd_driver->get_buffer();
		index = ((uint32_t)posy * lcd_driver->width) + (uint32_t)posx;

		return p_pixel[index];
	}

	return 0;
}

void graphics_show_bitmap( uint16_t posx, uint16_t posy, const bitmap_t * bitmap, bool_t use_transparent )
{
	int transparent;
	if( !lcd_ready ) return;
	transparent = -1;
	if( use_transparent ) transparent = (int)bitmap->background_colour;
	graphics_set_bitmap_rgb( posx, posy, bitmap->width, bitmap->height, bitmap->buffer, transparent );
}

void graphics_set_bitmap_rgb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t * data, int i_transparent )
{
	uint16_t x,y;
	uint16_t * p_pixel;
	uint32_t index;
	uint32_t counter;
	uint16_t transparent = 0;
	bool_t is_transparent = FALSE;

	uint16_t tx;

	if( !lcd_ready ) return;
	if( lcd_view == LCD_VIEW_PORTRAIT )
	{
		tx = posx;
		posx = lcd_driver->width - posy;
		posy = tx;
	}

	if( lcd_mode == LCD_MODE_API )
	{
		lcd_driver->set_bitmap_rgb( posx, posy, width, height, data, i_transparent );
	}
	else if( lcd_mode == LCD_MODE_FRAMEBUFFER )
	{
		if( i_transparent != -1 )
		{
			is_transparent = TRUE;
			transparent = ( uint16_t )i_transparent;
		}

		p_pixel = ( uint16_t * )lcd_driver->get_buffer();

		if( lcd_view == LCD_VIEW_PORTRAIT )
		{
			tx = posx;
			posx = lcd_driver->width - posy;
			posy = tx;

			if( posx >= lcd_driver->width ) return;
			if( posy >= lcd_driver->height ) return;

			for( y = 0; y < height; y++ )
			{
				counter = 0;
				index = ( ( ( uint32_t )( posy + y ) ) * lcd_driver->width ) + ( ( uint32_t )( posx ) );

				for( x = 0; x < width; x++ )
				{
					if( ( is_transparent == FALSE ) || ( transparent != data[counter] ) )
						p_pixel[index] = data[counter];

					counter += height;
					index--;
				}
			}
		}
		else
		{
			/*
			if( posx >= lcd_driver->width ) return;
			if( posy >= lcd_driver->height ) return;

			for( y = 0; y < height; y++ )
			{
				counter = 0;
				index = ( ( ( uint32_t )( posy + y ) ) * lcd_driver->width ) + ( ( uint32_t )( posx ) );

				for( x = 0; x < width; x++ )
				{
					if( ( is_transparent == FALSE ) || ( transparent != data[counter] ) )
						p_pixel[index] = data[counter];

					counter++;
					index++;
				}
			}
			*/
			uint16_t * lcd_buffer_margin = ( uint16_t * )p_pixel;
		    uint16_t * lcd_ptr;
			uint16_t counter;

			lcd_buffer_margin += ( uint32_t )( posy * lcd_driver->width );
			lcd_buffer_margin += ( uint32_t )( posx );

			while( height-- )
			{
		        lcd_ptr = lcd_buffer_margin;

		        for( counter = 0; counter < width; counter++ )
		        {
		            if( ( data[ counter ] != transparent ) ||
		                ( is_transparent == FALSE ) )
		                *lcd_ptr = data[ counter ];

		            lcd_ptr++;
		        }

				lcd_buffer_margin += ( lcd_driver->width );
				data += ( width );
			}
		}
	}
}

void graphics_get_bitmap_rgb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t * data )
{
	uint16_t x,y,i;
	uint16_t * p_pixel;
	uint16_t * d_pixel;
	uint32_t index;

	if( !lcd_ready ) return;
	if( ( lcd_mode == LCD_MODE_API ) && ( lcd_driver->get_bitmap_rgb != NULL ) )
	{
		lcd_driver->get_bitmap_rgb( posx, posy, width, height, data );
	}
	else if( lcd_mode == LCD_MODE_FRAMEBUFFER )
	{
		if( posx >= lcd_driver->width ) return;
		if( posy >= lcd_driver->height ) return;

		p_pixel = ( uint16_t * )lcd_driver->get_buffer();

		for( i = y = 0; y < height; y++ )
		{
			if( ( y + posy ) >= lcd_driver->height ) break;

			index = ((uint32_t)(posy+y) * lcd_driver->width) + (uint32_t)posx;
			d_pixel = &p_pixel[index];

			for( x = 0; x < width; x++ )
			{
				if( ( x + posx ) < lcd_driver->width )
				{
					data[i] = d_pixel[x];
				}

				i++;
			}
		}
	}
}

void graphics_fill_box( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t colour )
{
	uint16_t x,y,i;
	uint16_t * p_pixel;
	uint16_t * d_pixel;
	uint32_t index;
	uint16_t tx;

	if( !lcd_ready ) return;
	if( lcd_mode == LCD_MODE_API )
	{
		lcd_driver->set_fill_box( posx, posy, width, height, colour );
	}
	else if( lcd_mode == LCD_MODE_FRAMEBUFFER )
	{
		p_pixel = ( uint16_t * )lcd_driver->get_buffer();

		if( lcd_view == LCD_VIEW_PORTRAIT )
		{
			tx = posx;
			posx = lcd_driver->width - posy;
			posy = tx;
		}

		if( posx >= lcd_driver->width ) return;
		if( posy >= lcd_driver->height ) return;

		for( i = y = 0; y < height; y++ )
		{
			if( ( y + posy ) >= lcd_driver->height ) break;

			index = ((uint32_t)(posy+y) * lcd_driver->width) + (uint32_t)posx;
			d_pixel = &p_pixel[index];
			i = 0;

			for( x = 0; x < width; x++ )
			{
				if( ( x + posx ) < lcd_driver->width )
				{
					d_pixel[i] = colour;
				}

				i++;
			}
		}
	}
}

int graphics_read_ts( uint16_t * posx, uint16_t * posy, uint16_t * posz )
{
	int ret;
	uint16_t ty;

	if( !lcd_ready ) return 0;
	if( ( lcd_driver == NULL ) || ( lcd_driver->input_ts == NULL ) ) return 0;
	ret = lcd_driver->input_ts( posx, posy, posz );

	if( lcd_view == LCD_VIEW_PORTRAIT )
	{
		ty = *posy;
		*posy = lcd_driver->width - *posx;
		*posx = ty;
	}

	return ret;
}

void graphics_bitmap_1bb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t heigth, 
					      uint8_t *input, uint16_t back_colour, uint16_t fore_colour )
{
	uint16_t x;
	uint16_t y;
	uint16_t bit_mask;
	uint8_t * ptr_pixel;

	if( !lcd_ready ) return;

	for( y = 0; y < heigth; y++ )
	{
		if( y > lcd_driver->height ) break;

		ptr_pixel = &input[ ( ( y >> 3 ) * width ) ];
		bit_mask = 0x01 << ( y & 0x07 );
		
		for( x = 0; x < width; x++ )
		{
			if( x > lcd_driver->width ) break;

			if( ( ptr_pixel[ x ] & bit_mask ) != 0 )
			{
				graphics_set_pixel( posx + x, posy + y, fore_colour );
			}
			else
			{
                graphics_set_pixel( posx + x, posy + y, back_colour );
			}		
		}
	}
}
