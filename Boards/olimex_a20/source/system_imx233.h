#ifndef __INTERRUPT_IMX233_H__
#define __INTERRUPT_IMX233_H__

////////////////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "cpu.h"

////////////////////////////////////////////////////////////////////////////////////////

#define HW_ICOLL_BASE                           0x80000000
#define HW_ICOLL_VECTOR                         (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x0))
#define HW_ICOLL_LEVELACK                       (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x10))
#define HW_ICOLL_LEVELACK__LEVEL0               0x1
#define HW_ICOLL_VBASE                          (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x40))
#define HW_ICOLL_INTERRUPT(i)                   (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x120 + (i) * 0x10))
#define HW_ICOLL_INTERRUPT__PRIORITY_BM         0x3
#define HW_ICOLL_INTERRUPT__ENABLE              0x4
#define HW_ICOLL_INTERRUPT__SOFTIRQ             0x8
#define HW_ICOLL_INTERRUPT__ENFIQ               0x10
#define HW_ICOLL_CTRL                           (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x20))
#define HW_ICOLL_CTRL__IRQ_FINAL_ENABLE         (1 << 16)
#define HW_ICOLL_CTRL__ARM_RSE_MODE             (1 << 18)

#define INT_SRC_UART_DEBUG	0
#define INT_SRC_SSP2_ERROR  2
#define INT_SRC_USB_CTRL    11
#define INT_SRC_SSP1_DMA    14
#define INT_SRC_SSP1_ERROR  15
#define INT_SRC_GPIO0       16
#define INT_SRC_GPIO1       17
#define INT_SRC_GPIO2       18
#define INT_SRC_GPIO(i)     (INT_SRC_GPIO0 + (i))
#define INT_SRC_SSP2_DMA    20
#define INT_SRC_I2C_DMA     26
#define INT_SRC_I2C_ERROR   27
#define INT_SRC_TIMER(nr)   (28 + (nr))
#define INT_SRC_LCDIF_DMA   45
#define INT_SRC_LCDIF_ERROR 46
#define INT_SRC_NR_SOURCES  66

////////////////////////////////////////////////////////////////////////////////////////

#define readw(addr)		(*(volatile u16 *) (addr))
#define readl(addr)		(*(volatile u32 *) (addr))
#define writew(b,addr)		((*(volatile u16 *) (addr)) = (b))
#define writel(b,addr)		((*(volatile u32 *) (addr)) = (b))

////////////////////////////////////////////////////////////////////////////////////////

void imx233_enable_interrupt(int src, bool_t enable);
void imx233_softirq(int src, bool_t enable);
void system_init(void);

////////////////////////////////////////////////////////////////////////////////////////

#endif // __INTERRUPT_IMX233_H__

