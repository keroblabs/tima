#include "system.h"
#include "graphics.h"

#include "types.h"

#include "regsclkctrl.h"
#include "lcdif-imx233.h"
#include "pinctrl-imx233.h"

///////////////////////////////////////////////////////////////////////////

#define LCD_WIDTH			    	_USE_SCREEN_WIDTH
#define LCD_HEIGHT  		    	_USE_SCREEN_HEIGHT

#if LCD_WIDTH == 1024
#define LCD_WIDTH_BACK_PORTION		156
#define LCD_HEIGHT_BACK_PORTION		16
#define LCD_HSYNC_PULSE_WIDTH		6
#define LCD_VSYNC_PULSE_WIDTH		10
#elif LCD_WIDTH == 800
#define LCD_WIDTH_BACK_PORTION		20
#define LCD_HEIGHT_BACK_PORTION		16
#define LCD_HSYNC_PULSE_WIDTH		6
#define LCD_VSYNC_PULSE_WIDTH		10
#else
#error LCD not defined
#endif

///////////////////////////////////////////////////////////////////////////

static __IO uint16_t video_buffer[LCD_WIDTH*LCD_HEIGHT];

///////////////////////////////////////////////////////////////////////////

void imx233_lcdif_enable_bus_master(bool_t enable)
{
    if(enable)
        __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__LCDIF_MASTER;
    else
        __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__LCDIF_MASTER;
}

void imx233_lcdif_enable(bool_t enable)
{
    if(enable)
        __REG_CLR(HW_LCDIF_CTRL) = __BLOCK_CLKGATE;
    else
        __REG_SET(HW_LCDIF_CTRL) = __BLOCK_CLKGATE;
}

void imx233_lcdif_run(bool_t enable)
{
    if(enable)
        __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__RUN;
    else
        __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__RUN;
}

void imx233_lcdif_reset(void)
{
    //imx233_reset_block(&HW_LCDIF_CTRL);// doesn't work
    while(HW_LCDIF_CTRL & __BLOCK_CLKGATE)
        HW_LCDIF_CTRL &= ~__BLOCK_CLKGATE;
    while(!(HW_LCDIF_CTRL & __BLOCK_SFTRST))
        HW_LCDIF_CTRL |= __BLOCK_SFTRST;
    while(HW_LCDIF_CTRL & __BLOCK_CLKGATE)
        HW_LCDIF_CTRL &= ~__BLOCK_CLKGATE;
    while(HW_LCDIF_CTRL & __BLOCK_SFTRST)
        HW_LCDIF_CTRL &= ~__BLOCK_SFTRST;
    while(HW_LCDIF_CTRL & __BLOCK_CLKGATE)
        HW_LCDIF_CTRL &= ~__BLOCK_CLKGATE;
    __REG_SET(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__RESET;
}

unsigned imx233_lcdif_enable_irqs(unsigned irq_bm)
{
    unsigned old_msk = (HW_LCDIF_CTRL1 & HW_LCDIF_CTRL1__IRQ_EN_BM) >>HW_LCDIF_CTRL1__IRQ_EN_BP ;
    /* clear irq status */
    __REG_CLR(HW_LCDIF_CTRL1) = irq_bm << HW_LCDIF_CTRL1__IRQ_BP;
    /* disable irqs */
    __REG_CLR(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__IRQ_EN_BM;
    /* enable irqs */
    __REG_SET(HW_LCDIF_CTRL1) = irq_bm << HW_LCDIF_CTRL1__IRQ_EN_BP;

    return old_msk;
}

void imx233_lcdif_dma_send(void *buf, unsigned width, unsigned height)
{
    HW_LCDIF_CUR_BUF = (uint32_t)buf;
    HW_LCDIF_NEXT_BUF = (uint32_t)buf;
    HW_LCDIF_TRANSFER_COUNT = 0;
    HW_LCDIF_TRANSFER_COUNT = (height << 16) | width;
}

void imx233_lcdif_init_pio( void )
{
	//imx233_set_pin_function( 1, 18, PINCTRL_FUNCTION_GPIO );
	//imx233_enable_gpio_output( 1, 18, TRUE );
	//imx233_set_gpio_output( 1, 18, FALSE );

	imx233_set_pin_function( 0, 7, PINCTRL_FUNCTION_ALT1 );
	imx233_set_pin_function( 0, 6, PINCTRL_FUNCTION_ALT1 );
	imx233_set_pin_function( 0, 5, PINCTRL_FUNCTION_ALT1 );
	imx233_set_pin_function( 0, 4, PINCTRL_FUNCTION_ALT1 );
	imx233_set_pin_function( 0, 3, PINCTRL_FUNCTION_ALT1 );
	imx233_set_pin_function( 0, 2, PINCTRL_FUNCTION_ALT1 );
	imx233_set_pin_function( 0, 1, PINCTRL_FUNCTION_ALT1 );
	imx233_set_pin_function( 0, 0, PINCTRL_FUNCTION_ALT1 );

	imx233_set_pin_function( 1, 7, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 6, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 5, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 4, PINCTRL_FUNCTION_MAIN );
	//imx233_set_pin_function( 1, 3, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 2, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 1, PINCTRL_FUNCTION_MAIN );
	//imx233_set_pin_function( 1, 0, PINCTRL_FUNCTION_MAIN );

	imx233_set_pin_function( 1, 25, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 24, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 23, PINCTRL_FUNCTION_MAIN );
	imx233_set_pin_function( 1, 22, PINCTRL_FUNCTION_MAIN );
}

void imx233_lcdif_init_clock( void )
{
	// disable clockgate
	HW_CLKCTRL_PIX_SET(BM_CLKCTRL_PIX_CLKGATE);

	// set pixel divider
	HW_CLKCTRL_PIX_WR(1);

	// reenable clockgate
	HW_CLKCTRL_PIX_CLR(BM_CLKCTRL_PIX_CLKGATE);
}

void imx233_lcdif_regs_init( void )
{
	HW_LCDIF_CTRL = HW_LCDIF_CTRL__DOT_CLOCK_MODE |
					HW_LCDIF_CTRL__LCDIF_MASTER |
					HW_LCDIF_CTRL__BYPASS_CNT; //  |
					// HW_LCDIF_CTRL__DATA_SELECT;

	HW_LCDIF_CTRL1 = HW_LCDIF_CTRL1__BYTE_PACKING_FORMAT_BM;

	HW_LCDIF_VDCTRL0 =  HW_LCDIF_VDCTRL0__VSYNC_PERIOD_UNIT_LINES |
						HW_LCDIF_VDCTRL0__VSYNC_PULSE_WIDTH_UNIT_LINES |
						HW_LCDIF_VDCTRL0__ENABLE_PRESENT |
						HW_LCDIF_VDCTRL0__ENABLE_ACTIVE_HIGH |
						HW_LCDIF_VDCTRL0__DOTCLK_FALLING |
						HW_LCDIF_VDCTRL0__VSYNC_PULSE_WIDTH(LCD_VSYNC_PULSE_WIDTH);

	HW_LCDIF_VDCTRL1 = LCD_HEIGHT + ( LCD_HEIGHT_BACK_PORTION << 1 ) + LCD_VSYNC_PULSE_WIDTH;

	HW_LCDIF_VDCTRL2 = 	HW_LCDIF_VDCTRL2__HSYNC_PULSE_WIDTH(LCD_HSYNC_PULSE_WIDTH) |
						HW_LCDIF_VDCTRL2__HSYNC_PERIOD(LCD_WIDTH+(LCD_WIDTH_BACK_PORTION<<1)+LCD_HSYNC_PULSE_WIDTH);

	HW_LCDIF_VDCTRL3 = 	HW_LCDIF_VDCTRL3__HORIZONTAL_WAIT_CNT(LCD_WIDTH_BACK_PORTION+LCD_HSYNC_PULSE_WIDTH) |
						HW_LCDIF_VDCTRL3__VERTICAL_WAIT_CNT(LCD_HEIGHT_BACK_PORTION+LCD_VSYNC_PULSE_WIDTH);

	HW_LCDIF_VDCTRL4 =  HW_LCDIF_VDCTRL4__SYNC_SIGNALS_ON |
						HW_LCDIF_VDCTRL4__DOTCLK_H_VALID_DATA_CNT(LCD_WIDTH);

}

void imx_233_lcdif_init( void )
{
	imx233_lcdif_reset();
	imx233_lcdif_init_clock();

	imx233_lcdif_run(FALSE);

	imx233_lcdif_init_pio();
	imx233_lcdif_regs_init();

	imx233_lcdif_dma_send((uint8_t *)video_buffer, LCD_WIDTH, LCD_HEIGHT);

	imx233_lcdif_enable(TRUE);
	imx233_lcdif_run(TRUE);

	//imx233_set_gpio_output( 1, 18, TRUE );
}


///////////////////////////////////////////////////////////////////////////

void lcd_flip_screen( void )
{
}

void lcd_init( void )
{
	imx_233_lcdif_init();
}

void lcd_clear( void )
{
	memset( ( uint8_t * )video_buffer, 0x00, LCD_WIDTH*LCD_HEIGHT*2 );
}

void lcd_finish_LCD( void )
{

}


void lcd_set_contrast(uint8_t nContrast)
{
}

void lcd_set_backlight( bool_t state )
{
}

uint16_t * lcd_get_buffer( void )
{
	return ( uint16_t * )video_buffer;
}

///////////////////////////////////////////////////////////////////////////

static const lcd_driver_t imx233_lcdif_api =
{
    LCD_WIDTH,
    LCD_HEIGHT,

	lcd_init,
	lcd_clear,
	lcd_finish_LCD,
	lcd_flip_screen,
	NULL,

    lcd_set_contrast,
    lcd_set_backlight,

	lcd_get_buffer,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,

    NULL,
};

SYSTEM_DRIVER_DEFINE( lcd_driver_t, imx233_lcdif_api, lcd, "lcd", NULL );

