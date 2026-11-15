#include "timer_imx233.h"
#include "system_imx233.h"

///////////////////////////////////////////////////////////////

#define MS_TIMER_SCALE          (1000)

///////////////////////////////////////////////////////////////

imx233_timer_fn_t timer_fns[4];

#define define_timer_irq(nr) \
void INT_TIMER##nr(void) \
{ \
    if(timer_fns[nr]) \
        timer_fns[nr](); \
    __REG_CLR(HW_TIMROT_TIMCTRL(nr)) = HW_TIMROT_TIMCTRL__IRQ; \
}

define_timer_irq(0)
define_timer_irq(1)
define_timer_irq(2)
define_timer_irq(3)

///////////////////////////////////////////////////////////////

static __IO uint32_t _ms_timer;

///////////////////////////////////////////////////////////////

static void tick_timer_1000ms(void)
{
    /* Run through the list of tick tasks */
    _ms_timer += MS_TIMER_SCALE;
    HW_DIGCTL_MICROSECONDS = 0;
}

static void imx233_setup_timer(unsigned timer_nr, bool_t reload, unsigned count,
    unsigned src, unsigned prescale, bool_t polarity, imx233_timer_fn_t fn)
{
    timer_fns[timer_nr] = fn;
    
    HW_TIMROT_TIMCTRL(timer_nr) = src | prescale;
    if(polarity)
        __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__POLARITY;
    if(reload)
    {
        __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__RELOAD;
        /* manual says count - 1 for reload timers */
        HW_TIMROT_TIMCOUNT(timer_nr) = count - 1;
    }
    else
        HW_TIMROT_TIMCOUNT(timer_nr) = count;
    /* only enable interrupt if function is set */
    if(fn != NULL)
    {
        /* enable interrupt */
        imx233_enable_interrupt(INT_SRC_TIMER(timer_nr), TRUE);
        /* clear irq bit and enable */
        __REG_CLR(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__IRQ;
        __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__IRQ_EN;
    }
    else
        imx233_enable_interrupt(INT_SRC_TIMER(timer_nr), FALSE);
    /* finally update */
    __REG_SET(HW_TIMROT_TIMCTRL(timer_nr)) = HW_TIMROT_TIMCTRL__UPDATE;
}

static void imx233_enable_timrot_xtal_clk32k(bool_t enable)
{
    if(enable)
        __REG_CLR(HW_CLKCTRL_XTAL) = HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE;
    else
        __REG_SET(HW_CLKCTRL_XTAL) = HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE;
}

static void imx233_timrot_init(void)
{
    __REG_CLR(HW_TIMROT_ROTCTRL) = __BLOCK_CLKGATE;
    __REG_CLR(HW_TIMROT_ROTCTRL) = __BLOCK_SFTRST;
    
    /* enable xtal path to timrot */
    imx233_enable_timrot_xtal_clk32k(TRUE);
}

void tick_start(unsigned int interval_in_ms)
{
    /* use the 1-kHz XTAL clock source */
    imx233_setup_timer(0, TRUE, interval_in_ms,
        HW_TIMROT_TIMCTRL__SELECT_1KHZ_XTAL, HW_TIMROT_TIMCTRL__PRESCALE_1,
        FALSE, &tick_timer_1000ms);
}

void imx233_get_us( uint64_t * p_timer )
{
    uint32_t curr_ms;
    DI(); curr_ms = _ms_timer; EI();
    *p_timer = ( curr_ms * 1000000 ) + HW_DIGCTL_MICROSECONDS;
}

uint32_t imx233_get_ms( void )
{
    uint32_t curr_ms;
    DI(); curr_ms = _ms_timer; EI();
    return ( curr_ms * 1000 ) + ( HW_DIGCTL_MICROSECONDS / 1000 );
}

void imx233_timer_init( void )
{
    imx233_timrot_init();
    tick_start( MS_TIMER_SCALE );
}
