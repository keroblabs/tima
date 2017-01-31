#include "types.h"
#include "system.h"
#include "system_imx233.h"

#include "timer_imx233.h"
//#include "uartdbg_imx233.h"
//#include "dma_imx233.h"
#include "pinctrl-imx233.h"

#include "regsrtc.h"
#include "platform.h"

////////////////////////////////////////////////////////////////////////////////////////

#define WATCHDOG_TIMER_MS				1000

#define SYSCOMM_FAULT_UNDEF         	0x21
#define SYSCOMM_FAULT_I_ABORT       	0x22
#define SYSCOMM_FAULT_D_ABORT       	0x23

////////////////////////////////////////////////////////////////////////////////////////

void _irq_handler_asm( void );

////////////////////////////////////////////////////////////////////////////////////////

uint8_t system_heap[SYSTEM_HEAP_SIZE];
uint32_t system_heap_size = SYSTEM_HEAP_SIZE;
__IO uint32_t intr_counter;

////////////////////////////////////////////////////////////////////////////////////////

#define default_interrupt(name) \
    extern __attribute__((weak, alias("UIRQ"))) void name(void)

static void UIRQ (void) __attribute__((interrupt ("IRQ")));
void irq_handler(void) __attribute__((interrupt("IRQ")));
void fiq_handler(void) __attribute__((interrupt("FIQ")));

void undef_handler(void);
void i_abort_handler(void);
void d_abort_handler(void);


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

static const isr_t isr_table[INT_SRC_NR_SOURCES] =
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

static const uint32_t cpu_vectors[] =
{
	0xea00000e,
	0xea00000e,
	0xea00000e,
	0xea00000e,
	0xea00000e,
	0xea00000e,
	0xea00000e,
	0xea00000e,

	0x00000074,
	(uint32_t)undef_handler,
	0x000000b0,
	(uint32_t)i_abort_handler,

	(uint32_t)d_abort_handler,
	0x00000074,
	//(uint32_t)irq_handler,
	(uint32_t)_irq_handler_asm,
	(uint32_t)fiq_handler,
};

////////////////////////////////////////////////////////////////////////////////////////

static void UIRQ(void)
{
    //while(1) {}
}

void irq_handler(void)
{
    HW_ICOLL_VECTOR = HW_ICOLL_VECTOR; /* notify icoll that we entered ISR */
    (*(isr_t *)HW_ICOLL_VECTOR)();
    
    /* acknowledge completion of IRQ (all use the same priority 0 */
    HW_ICOLL_LEVELACK = HW_ICOLL_LEVELACK__LEVEL0;

    //__REG_CLR(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__IRQ_FINAL_ENABLE;
}

void fiq_handler(void)
{
	//while(1) {}
}

void undef_handler(void)
{
}

void i_abort_handler(void)
{
}

void d_abort_handler(void)
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
    __REG_CLR(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__IRQ_FINAL_ENABLE;
}

void _cpu_enable_irq( void )
{
	__REG_SET(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__IRQ_FINAL_ENABLE;
}

void imx_233_rebuild_vectors(void)
{
	__IO uint32_t * p_addr;
	uint8_t cnt;

	p_addr = ( __IO uint32_t * )0x00000000;

	for( cnt = 0; cnt < 16; cnt++ )
	{
		p_addr[cnt] = cpu_vectors[cnt];
	}
}

void imx_233_enable_watchdog( void )
{
	HW_RTC_CTRL_SET(BM_RTC_CTRL_WATCHDOGEN);
}

void imx_233_reset_watchdog( void )
{
	HW_RTC_WATCHDOG_WR(WATCHDOG_TIMER_MS);
}

void imx_233_hardware_setup(void)
{
    int i;
    
    intr_counter = 0;

    imx_233_rebuild_vectors();

    __REG_CLR(HW_ICOLL_CTRL) = ( HW_ICOLL_CTRL__SOFT_RESET | HW_ICOLL_CTRL__CLOCK_GATE );

    /* disable all interrupts */
    for(i = 0; i < INT_SRC_NR_SOURCES; i++)
    {
        /* priority = 0, disable, disable fiq */
        HW_ICOLL_INTERRUPT(i) = 0;
    }
    
    /* setup vbase as isr_table */
    HW_ICOLL_VBASE = (uint32_t)&isr_table[0];
    
    /* enable final irq bit */
    __REG_CLR(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__FIRQ_FINAL_ENABLE;
    __REG_SET(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__IRQ_FINAL_ENABLE;

    //imx233_timer_init();
    //imx233_dma_init();
    imx233_pinctrl_init();
    //imx233_serial_init( 115200 );
    /*
    imx233_ssp_init();
    */

    imx_233_enable_watchdog();
}

DECLARE_PROCESS_SECTION( imx_233_reset_watchdog );
