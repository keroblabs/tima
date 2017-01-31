#include "types.h"
#include "device.h"
#include "system.h"
#include "timer.h"
#include "lcd.h"

//#include "debug.h"

///////////////////////////////////////////////////////////////////////////

#define LCD_WIDTH				240
#define LCD_HEIGHT				320
#define LCD_API_LEVEL			4
#define LCD_COMMAND_TIMEOUT		200
#define LCD_INIT_DELAY			3500

#define LCD_COMMAND_RESP		0x06

#define LCD_BAUD_115200			13

#define LCD_TPI_SET				0xFF38, 1, 0
#define LCD_TPI_READ			0xFF37, 1, 0
#define LCD_BACK_COLOUR			0xFFA4, 1, 2
#define LCD_CLEAR_SCREEN		0xFFCD, 0, 0
#define LCD_BAUD_RATE			0x0026, 1, 0
#define LCD_MEDIA_INIT			0xFF89, 0, 1
#define LCD_PUT_PIXEL			0xFFC1, 3, 0
#define LCD_FILL_RECT			0xFFC4, 5, 0

#define LCD_SET_FONT			0xFFE5, 1, 2
#define LCD_SET_POSITION		0xFFE9, 2, 0
#define LCD_SHOW_CHARACTER		0xFFFE, 1, 0
#define LCD_SET_FORE_COLOUR		0xFFE7, 1, 2
#define LCD_SET_BACK_COLOUR		0xFFE6, 1, 2

#define MEDIA_CMD_INIT			0xFF89, 0, 1
#define MEDIA_READ_SECTOR		0x0016, 0, 512
#define MEDIA_WRITE_SECTOR		0x0017, 512, 1
#define MEDIA_SET_ADDRESS		0xFF92, 2, 0
#define MEDIA_CMD_FLUSH			0xFF8A, 0, 2

///////////////////////////////////////////////////////////////////////////

static uint16_t lcd_command[32];
static uint8_t lcd_comm_buffer[520];
//static uint8_t lcd_resp_data[512];

static uint8_t sdcard_state;
static bool_t is_driver_ready = FALSE;
static bool_t is_driver_init = FALSE;

static device_t * lcd_hdl;
static timer_data_t timer_sec;

////////////////////////////////////////////////////////////////////////

bool_t lcd_send_command( uint16_t * param, uint8_t * resp_p, uint16_t command, uint32_t size, uint32_t resp_size )
{
    uint32_t i;
    uint32_t cmd_len;
    uint8_t * cmd_p;
    uint8_t resp;
    
    if( size >= 32 ) return FALSE;
    
    // add command
    lcd_comm_buffer[0] = ( uint8_t )( command >> 8 );
    lcd_comm_buffer[1] = ( uint8_t )( command & 0x0FF );
    
    if( size > 0 )
    {
        // add parameters
        cmd_p = &lcd_comm_buffer[2];

        for( i = 0; i < size; i++ )
        {
            *(cmd_p++) = ( uint8_t )( param[i] >> 8 );
            *(cmd_p++) = ( uint8_t )( param[i] & 0x0FF );
        }
    }
    
    // prepare to send
    cmd_len = 2 + ( size << 1 );
    timer_Start( &timer_sec, LCD_COMMAND_TIMEOUT );
    
    if( lcd_hdl == NULL ) return FALSE;
    device_write_buffer( lcd_hdl, ( uint8_t * )lcd_comm_buffer, cmd_len );
    
    //DEBUG_PRINTK( "LCD command %04x send\r\n", command );

    // wait for response
	while( device_is_rx_pending(lcd_hdl) == 0 )
    {
		if( timer_Check( &timer_sec ) ==  TRUE ) return FALSE;
		SYSTEM_EVENTS();
	}

	// check response
	device_read_buffer( lcd_hdl, (uint8_t *)&resp, 1 );
	//DEBUG_PRINTK( "LCD response %02x\r\n", resp );

    if( resp != LCD_COMMAND_RESP ) return FALSE;

    if( resp_size > 0 )
    {
        // receive additional response
        i = 0;
        timer_Start( &timer_sec, LCD_COMMAND_TIMEOUT );

        while( i < resp_size )
        {
            if( timer_Check( &timer_sec ) ==  TRUE ) return FALSE;
			if( device_read_buffer( lcd_hdl, (uint8_t *)&resp, 1 ) )
			{
                if( resp_p != NULL ) *(resp_p++) = resp;
                i++;
			}

			SYSTEM_EVENTS();
        }
    }
    
    // success
    return TRUE;   
}

void hw_lcd_set_pixel( uint16_t x, uint16_t y, uint16_t colour )
{
    if( is_driver_ready == FALSE ) return;
    lcd_command[0] = x;
    lcd_command[1] = y;
    lcd_command[2] = colour;
    
    lcd_send_command( lcd_command, NULL, LCD_PUT_PIXEL );
}

void hw_lcd_4d_set_pos( uint16_t posx, uint16_t posy )
{
    if( is_driver_ready == FALSE ) return;
    lcd_command[0] = posx;
    lcd_command[1] = posy;

    lcd_send_command( lcd_command, NULL, LCD_SET_POSITION );
}

void hw_lcd_4d_show_char( char char_out )
{
    if( is_driver_ready == FALSE ) return;
    lcd_command[0] = ( uint16_t )char_out;
    lcd_send_command( lcd_command, NULL, LCD_SHOW_CHARACTER );
}

void hw_lcd_4d_init_text( void )
{
	// set system font
    lcd_command[0] = 0;
    lcd_send_command( lcd_command, NULL, LCD_SET_FONT );

    // set background colour
    lcd_command[0] = 0x0000;
    lcd_send_command( lcd_command, NULL, LCD_SET_BACK_COLOUR );

    // set foreground colour
    lcd_command[0] = 0xFFFF;
    lcd_send_command( lcd_command, NULL, LCD_SET_FORE_COLOUR );
}

#if 0
void lcd_init( void )
{
    uint32_t baud;
    
    if( is_driver_ready == TRUE ) return;

	lcd_hdl = device_open( DEV_4DLCD );
	if( lcd_hdl == NULL ) return;

	sdcard_state = 0;
    
    baud = 9600;
    device_ioctrl( lcd_hdl, DEVICE_SET_CONFIG_DATA, &baud );

    // wait for initialization
    timer_Start( &timer_sec, LCD_INIT_DELAY );
	while( timer_Check( &timer_sec ) == 0 )
	{
		SYSTEM_EVENTS();
	}

    // set background colout
	lcd_command[0] = 0x0000;
    if( lcd_send_command( lcd_command, NULL, LCD_BACK_COLOUR ) == FALSE ) return;
        
    // clear screen
    if( lcd_send_command( NULL, NULL, LCD_CLEAR_SCREEN ) == FALSE ) return;
        
    // init memory card
    if( lcd_send_command( NULL, &sdcard_state, LCD_MEDIA_INIT ) == FALSE ) return;
        
    // change baud rate
    //lcd_command[0] = LCD_BAUD_115200;
    //if( lcd_send_command( lcd_command, NULL, LCD_BAUD_RATE ) == FALSE ) return;
    //baud = 115200;
    //device_ioctrl( lcd_hdl, DEVICE_SET_CONFIG_DATA, &baud );

	// enable touch screen
	lcd_command[0] = 0;
    if( lcd_send_command( lcd_command, NULL, LCD_TPI_SET ) == FALSE ) return;
        
	// reset touch screen
	lcd_command[0] = 2;
    if( lcd_send_command( lcd_command, NULL, LCD_TPI_SET ) == FALSE ) return;

	is_driver_ready = TRUE;
}

bool_t lcd_init_process( void )
{
	return TRUE;
}

#else
void lcd_init( void )
{
    uint32_t baud;

    if( is_driver_init == TRUE ) return;

	lcd_hdl = device_open( DEV_4DLCD );
	if( lcd_hdl == NULL ) return;

	sdcard_state = 0;

    baud = 9600;
    device_ioctrl( lcd_hdl, DEVICE_SET_CONFIG_DATA, &baud );

    // wait for initialization
    timer_Start( &timer_sec, LCD_INIT_DELAY );

    DEBUG_PRINTK( "LCD Init OK\r\n" );
    is_driver_init = TRUE;
    is_driver_ready = FALSE;
}

bool_t lcd_init_process( void )
{
	if( is_driver_ready == TRUE ) return TRUE;
	if( timer_Check( &timer_sec ) == FALSE ) return FALSE;
	timer_Stop( &timer_sec );

	DEBUG_PRINTK( "LCD Process\r\n" );

    // set background colour
	lcd_command[0] = 0x0000;
    if( lcd_send_command( lcd_command, NULL, LCD_BACK_COLOUR ) == FALSE ) return FALSE;
    DEBUG_PRINTK( "1" );

    // clear screen
    if( lcd_send_command( NULL, NULL, LCD_CLEAR_SCREEN ) == FALSE ) return FALSE;
    DEBUG_PRINTK( "2" );

    // init memory card
    //if( lcd_send_command( NULL, &sdcard_state, LCD_MEDIA_INIT ) == FALSE ) return FALSE;
    //DEBUG_PRINTK( "3" );

    // change baud rate
    //lcd_command[0] = LCD_BAUD_115200;
    //if( lcd_send_command( lcd_command, NULL, LCD_BAUD_RATE ) == FALSE ) return;
    //baud = 115200;
    //device_ioctrl( lcd_hdl, DEVICE_SET_CONFIG_DATA, &baud );

	// enable touch screen
	lcd_command[0] = 0;
    if( lcd_send_command( lcd_command, NULL, LCD_TPI_SET ) == FALSE ) return FALSE;
    DEBUG_PRINTK( "4" );

	// reset touch screen
	lcd_command[0] = 2;
    if( lcd_send_command( lcd_command, NULL, LCD_TPI_SET ) == FALSE ) return FALSE;
    DEBUG_PRINTK( "5" );

    DEBUG_PRINTK( "LCD Done\r\n" );
	is_driver_ready = TRUE;
	return TRUE;
}
#endif

void lcd_clear( void )
{
    if( is_driver_ready == FALSE ) return;
    lcd_send_command( NULL, NULL, LCD_CLEAR_SCREEN );
}

void lcd_fill_lcd_color( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t colour )
{
    if( is_driver_ready == FALSE ) return;
    lcd_command[0] = posx;
    lcd_command[1] = posy;
    lcd_command[2] = posx + width;
    lcd_command[3] = posy + height;
    lcd_command[4] = colour;
    
    lcd_send_command( lcd_command, NULL, LCD_FILL_RECT );
}

void lcd_show_bitmap_rgb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t * bitmap, int use_transparent )
{
    uint16_t x, y;
    uint32_t cnt = 0;
    uint16_t colout16;
	uint16_t transparent;

    if( is_driver_ready == FALSE ) return;
	if( use_transparent != -1 )
	{
		transparent = ( uint16_t )use_transparent;
	}
    
    for( y = 0; y < height; y++ )
    {
        for( x = 0; x < width; x++ )
        {
            colout16 = bitmap[cnt];
            
            if( ( colout16 != transparent ) || ( use_transparent == -1 ) )
            {
                hw_lcd_set_pixel( posx + x, posy + y, colout16 );
            }
            
            cnt++;
        }
    }    
}

void lcd_set_contrast(uint8_t nContrast)
{
}

void lcd_set_pixel( uint16_t posx, uint16_t posy, uint16_t colour )
{
	hw_lcd_set_pixel( posx, posy, colour );
}

void lcd_set_backlight( uint8_t state )
{
}

void lcd_finish_LCD( void )
{
}

int lcd_read_tpi( uint16_t * posx, uint16_t * posy, uint16_t * posz )
{
	uint16_t resp;

    if( is_driver_ready == FALSE ) return 0;

	// get touch screen status
	lcd_command[0] = 0;
    if( lcd_send_command( lcd_command, ( uint8_t * )&resp, LCD_TPI_READ ) == FALSE ) return 0;
	if( resp == 0 ) return 0;

	*posz = 0;
	if( ( resp == 1 ) || ( resp == 3 ) ) *posz = 100;

	// get posx
	lcd_command[0] = 1;
    if( lcd_send_command( lcd_command, ( uint8_t * )posx, LCD_TPI_READ ) == FALSE ) return 0;

	// get posy
	lcd_command[0] = 2;
    if( lcd_send_command( lcd_command, ( uint8_t * )posy, LCD_TPI_READ ) == FALSE ) return 0;

    return 1;
}

///////////////////////////////////////////////////////////////////////////

static bool_t remote_media_set_sector_addr( uint32_t addr )
{
	lcd_command[0] = ( addr >> 16 );
	lcd_command[1] = ( addr & 0xFFFF );
    if( lcd_send_command( lcd_command, NULL, MEDIA_SET_ADDRESS ) == FALSE ) return FALSE;
	return TRUE;
}

static uint32_t remote_media_read_sector( uint8_t * buffer, uint32_t size )
{
	if( size != 512 ) return 0;
	lcd_send_command( NULL, buffer, MEDIA_READ_SECTOR );
	return size;
}

static uint32_t remote_media_write_sector( uint8_t * buffer, uint32_t size )
{
	if( size != 512 ) return 0;
	lcd_send_command( ( uint16_t * )buffer, NULL, MEDIA_WRITE_SECTOR );
	return size;
}

static bool_t remote_media_init( void )
{
	uint16_t status = 0;

	if( lcd_send_command( lcd_command, ( uint8_t * )&status, MEDIA_CMD_INIT ) == FALSE ) return FALSE;
	if( status == 1 ) return TRUE;

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static int _sdcard_init( void )
{
	if( remote_media_init() == FALSE ) return MAL_FAIL;
	return MAL_OK;
}

static int _sdcard_read_block( uint32_t addr, uint8_t * buffer, uint16_t size )
{
	uint32_t sd_addr = addr >> 9;
	if( remote_media_set_sector_addr( sd_addr ) == FALSE ) return 0;
	if( remote_media_read_sector( buffer, size ) == 0 ) return 0;

	return (int)size;
}

static int _sdcard_write_block( uint32_t addr, uint8_t * buffer, uint16_t size )
{
	uint32_t sd_addr = addr >> 9;
	if( remote_media_set_sector_addr( sd_addr ) == FALSE ) return 0;
	if( remote_media_write_sector( buffer, size ) == 0 ) return 0;

	return (int)size;
}

static uint32_t _sdcard_get_image_size( void )
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////

static const lcd_driver_t lcd_api = 
{
    LCD_WIDTH,
    LCD_HEIGHT,

    lcd_init,
    lcd_clear,
    lcd_finish_LCD,
	NULL,
	lcd_init_process,

    lcd_set_contrast,
    lcd_set_backlight,

	NULL,

	NULL,

    lcd_set_pixel,
    lcd_show_bitmap_rgb,
    NULL,
    lcd_fill_lcd_color,
    
    lcd_read_tpi,
};

///////////////////////////////////////////////////////////////////////////

#if defined TIMA_OS
//SYSTEM_DRIVER_DEFINE( lcd_driver_t, lcd_api, lcd, "lcd", NULL );
// type, api, name, asc name
//#define SYSTEM_DRIVER_DEFINE(t,a,n,p,i)

uint16_t _lcd_get_api( uint32_t index, lcd_driver_t ** api, uint16_t size )
{
    if( api != NULL ) *api = ( lcd_driver_t * )&lcd_api;
    return sizeof( uint32_t );
}

uint32_t _lcd_get_api2( uint32_t index, uint32_t param, void * data )
{
    lcd_driver_t ** p_api = ( lcd_driver_t ** )data;
    //lcd_driver_t * pp_api;
    
    if( param == DEVICE_GET_API )
    {
        //pp_api = ( lcd_driver_t * )&lcd_api;
        //*p_api = pp_api;
        *p_api = ( lcd_driver_t * )&lcd_api;
    }
    return 0;
}

static bool_t _device_in_use = FALSE;

const static device_data_t lcd_device =
{
    "lcd",
    &_device_in_use,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    _lcd_get_api2,
    NULL,
    DEV_MODE_CHANNEL,
    _lcd_get_api,
    NULL
};
DECLARE_DEVICE_SECTION( lcd_device )
#else
lcd_driver_t * lcd_driver = ( lcd_driver_t * )&sdl_lcd_driver;
#endif
