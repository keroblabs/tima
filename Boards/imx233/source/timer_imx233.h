#ifndef __timer_imx233_h__
#define __timer_imx233_h__

///////////////////////////////////////////////////////////////

#include "types.h"
#include "cpu.h"
#include "regsclkctrl.h"
#include "regstimrot.h"

///////////////////////////////////////////////////////////////

#define HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE     (1 << 26)

#define HW_DIGCTL_BASE                          0x8001C000
#define HW_DIGCTL_MICROSECONDS                  (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0xC0))

#define HW_TIMROT_BASE                          0x80068000
#define HW_TIMROT_TIMCTRL(i)                    (*(volatile uint32_t *)(HW_TIMROT_BASE + 0x20 + (i) * 0x20))
#define HW_TIMROT_TIMCTRL__IRQ                  (1 << 15)
#define HW_TIMROT_TIMCTRL__IRQ_EN               (1 << 14)
#define HW_TIMROT_TIMCTRL__POLARITY             (1 << 8)
#define HW_TIMROT_TIMCTRL__UPDATE               (1 << 7)
#define HW_TIMROT_TIMCTRL__RELOAD               (1 << 6)
#define HW_TIMROT_TIMCTRL__PRESCALE_1           (0 << 4)
#define HW_TIMROT_TIMCTRL__PRESCALE_2           (1 << 4)
#define HW_TIMROT_TIMCTRL__PRESCALE_4           (2 << 4)
#define HW_TIMROT_TIMCTRL__PRESCALE_8           (3 << 4)
#define HW_TIMROT_TIMCTRL__SELECT_32KHZ_XTAL    8
#define HW_TIMROT_TIMCTRL__SELECT_8KHZ_XTAL     9
#define HW_TIMROT_TIMCTRL__SELECT_4KHZ_XTAL     10
#define HW_TIMROT_TIMCTRL__SELECT_1KHZ_XTAL     11
#define HW_TIMROT_TIMCTRL__SELECT_TICK_ALWAYS   12
#define HW_TIMROT_TIMCOUNT(i)                   (*(volatile uint32_t *)(HW_TIMROT_BASE + 0x30 + (i) * 0x20))

///////////////////////////////////////////////////////////////

typedef void (*imx233_timer_fn_t)(void);

///////////////////////////////////////////////////////////////

void imx233_get_us( uint64_t * p_timer );
void imx233_timer_init( void );

uint32_t imx233_get_ms( void );
    
///////////////////////////////////////////////////////////////

#endif // __timer_imx233_h__
