#include "system_imx233.h"

#include "timer_imx233.h"
#include "dma_imx233.h"
#include "pinctrl-imx233.h"

////////////////////////////////////////////////////////////////////////////////////////

#define default_interrupt(name) \
    extern __attribute__((weak, alias("UIRQ"))) void name(void)

static void UIRQ (void) __attribute__((interrupt ("IRQ")));
void irq_handler(void) __attribute__((interrupt("IRQ")));
void fiq_handler(void) __attribute__((interrupt("FIQ")));

default_interrupt(INT_UART_DEBUG);
default_interrupt(INT_USB_CTRL);
default_interrupt(INT_TIMER0);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_TIMER3);
default_interrupt(INT_LCDIF_DMA);
default_interrupt(INT_LCDIF_ERROR);
default_interrupt(INT_SSP1_DMA);
default_interrupt(INT_SSP1_ERROR);
default_interrupt(INT_SSP2_DMA);
default_interrupt(INT_SSP2_ERROR);
default_interrupt(INT_I2C_DMA);
default_interrupt(INT_I2C_ERROR);
default_interrupt(INT_GPIO0);
default_interrupt(INT_GPIO1);
default_interrupt(INT_GPIO2);

typedef void (*isr_t)(void);

static isr_t isr_table[INT_SRC_NR_SOURCES] =
{
	[INT_SRC_UART_DEBUG] = INT_UART_DEBUG,
    [INT_SRC_USB_CTRL] = INT_USB_CTRL,
    [INT_SRC_TIMER(0)] = INT_TIMER0,
    [INT_SRC_TIMER(1)] = INT_TIMER1,
    [INT_SRC_TIMER(2)] = INT_TIMER2,
    [INT_SRC_TIMER(3)] = INT_TIMER3,
    [INT_SRC_LCDIF_DMA] = INT_LCDIF_DMA,
    [INT_SRC_LCDIF_ERROR] = INT_LCDIF_ERROR,
    [INT_SRC_SSP1_DMA] = INT_SSP1_DMA,
    [INT_SRC_SSP1_ERROR] = INT_SSP1_ERROR,
    [INT_SRC_SSP2_DMA] = INT_SSP2_DMA,
    [INT_SRC_SSP2_ERROR] = INT_SSP2_ERROR,
    [INT_SRC_I2C_DMA] = INT_I2C_DMA,
    [INT_SRC_I2C_ERROR] = INT_I2C_ERROR,
    [INT_SRC_GPIO0] = INT_GPIO0,
    [INT_SRC_GPIO1] = INT_GPIO1,
    [INT_SRC_GPIO2] = INT_GPIO2,
};

////////////////////////////////////////////////////////////////////////////////////////

static void UIRQ(void)
{
}

void irq_handler(void)
{
    HW_ICOLL_VECTOR = HW_ICOLL_VECTOR; /* notify icoll that we entered ISR */
    (*(isr_t *)HW_ICOLL_VECTOR)();
    
    /* acknowledge completion of IRQ (all use the same priority 0 */
    HW_ICOLL_LEVELACK = HW_ICOLL_LEVELACK__LEVEL0;
}

void fiq_handler(void)
{
}

void imx233_enable_interrupt(int src, bool_t enable)
{
    if(enable)
        __REG_SET(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__ENABLE;
    else
        __REG_CLR(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__ENABLE;
}

void imx233_softirq(int src, bool_t enable)
{
    if(enable)
        __REG_SET(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__SOFTIRQ;
    else
        __REG_CLR(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__SOFTIRQ;
}

void _cpu_disable_irq( void )
{
}

void _cpu_enable_irq( void )
{
}

void system_init(void)
{
    int i;
    
    /* disable all interrupts */
    for(i = 0; i < INT_SRC_NR_SOURCES; i++)
    {
        /* priority = 0, disable, disable fiq */
        HW_ICOLL_INTERRUPT(i) = 0;
    }
    
    /* setup vbase as isr_table */
    HW_ICOLL_VBASE = (uint32_t)&isr_table;
    
    /* enable final irq bit */
    __REG_SET(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__IRQ_FINAL_ENABLE;

    imx233_timer_init();
    imx233_dma_init();
    imx233_pinctrl_init();
    
    /*
    imx233_ssp_init();
    */
}
