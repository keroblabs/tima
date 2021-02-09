#include "SDL_internal.h"
#include "types.h"
#include "device.h"
#include "system.h"
#include "lcd.h"
#include "pipe.h"

///////////////////////////////////////////////////////////////

#define VSYNC_RATE_HZ		50
#define VSYNC_PERIOD_MS		( 1000 / VSYNC_RATE_HZ )
#define VSYNC_PERIOD(r)		( 1000 / r )

#define SDL_SCREEN_WIDTH		_USE_SCREEN_WIDTH
#define SDL_SCREEN_HEIGHT		_USE_SCREEN_HEIGHT

///////////////////////////////////////////////////////////////

SDL_Window* window = NULL;
SDL_Surface* screenSurface = NULL;
SDL_Renderer * screenRenderer = NULL;
SDL_Texture * screenTexture = NULL;

Uint32 vsync_ms;
SDL_bool is_updated;

static bool_t v_sync;

///////////////////////////////////////////////////////////////

static void sdl_update_window( void )
{
    if( screenSurface == NULL ) return;
    
    v_sync = TRUE;
    
    SDL_UpdateTexture( screenTexture, NULL, screenSurface->pixels, screenSurface->pitch );
    SDL_RenderClear( screenRenderer );
    SDL_RenderCopy( screenRenderer, screenTexture, NULL, NULL );
    SDL_RenderPresent( screenRenderer );
}

uint32_t ms_timer_hw_get_ms( void )
{
    return SDL_GetTicks();
}

uint16_t * sdl_GetTarget( void )
{
    return ( uint16_t * )screenSurface->pixels;
}

void sdl_lcd_init( void )
{
    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
    }
}

void sdl_lcd_stop( void )
{
    SDL_ShowCursor(SDL_ENABLE);
    
    //Destroy window
    SDL_DestroyWindow( window );
    
    //Quit SDL subsystems
    SDL_Quit();
}

void sdl_vsync_event( void )
{
    if( SDL_GetTicks() > vsync_ms )
    {
        vsync_ms = SDL_GetTicks() + VSYNC_PERIOD_MS;
        sdl_update_window();
    }
}

int create_window( void )
{
	Uint32 r_mask, g_mask, b_mask, a_mask;
	int bpp;

	SDL_PixelFormatEnumToMasks( SDL_PIXELFORMAT_RGB565, &bpp, &r_mask, &g_mask, &b_mask, &a_mask );

    //Create window
    window = SDL_CreateWindow( "SDL Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT, 0 );
    if( window == NULL )
    {
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
    }

	// create renderer
	screenRenderer = SDL_CreateRenderer( window, -1, 0 );
	if( screenRenderer == NULL )
	{
        printf( "Renderer could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}

    //Get window surface
	screenSurface = SDL_CreateRGBSurface(0,SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT, bpp, r_mask, g_mask, b_mask, a_mask);
	if( screenSurface == NULL )
	{
        printf( "Surface could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}

	screenTexture = SDL_CreateTexture( screenRenderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT);
	if( screenSurface == NULL )
	{
        printf( "Texture could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}

	vsync_ms = SDL_GetTicks() + VSYNC_PERIOD_MS;

	return 0;
}

static void sdl_init_internal( void * param )
{
	v_sync = FALSE;

	// create window
	if( create_window() == -1 )
	{
        printf( "Texture could not be created! SDL_Error: %s\n", SDL_GetError() );
		return;
	}

	// uncomment the line below to enable touchscreen mode
	// comment the line below to enable mouse mode
	//SDL_ShowCursor(SDL_DISABLE);

    //Fill the surface white
    SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0, 0, 0 ) );
	is_updated = SDL_TRUE;

    //Update the surface
    SDL_UpdateWindowSurface( window );

}

static void sdl_init( void )
{
    sdl_init_internal( NULL );
}

static void sdl_init_safe( void )
{
    sdl_instance_send_message( SDL_INTERNAL_DELEGATE, sdl_init_internal, NULL );
}

static bool_t sdl_lcd_init_process( void )
{
    return FALSE;
}

void sdl_clear_screen( void )
{
}

void sdl_set_contrast( uint8_t level )
{
}

void sdl_set_backlight( bool_t state )
{
}

///////////////////////////////////////////////////////////////

static const lcd_driver_t sdl_lcd_driver = 
{
	SDL_SCREEN_WIDTH,
	SDL_SCREEN_HEIGHT,

#ifdef WANT_MULTI_INSTANCES
    sdl_init_safe,
#else
	sdl_init,
#endif
    
	sdl_clear_screen,
	sdl_lcd_stop,
	sdl_update_window,
	sdl_lcd_init_process,

	sdl_set_contrast,
	sdl_set_backlight,

	sdl_GetTarget,

	NULL,

	NULL,
	NULL,
	NULL,
	NULL,

	sdl_read_mouse
};

///////////////////////////////////////////////////////////////

#if defined TIMA_OS
SYSTEM_DRIVER_DEFINE( lcd_driver_t, sdl_lcd_driver, lcd, "lcd", NULL );
#else
lcd_driver_t * lcd_driver = ( lcd_driver_t * )&sdl_lcd_driver;
#endif
