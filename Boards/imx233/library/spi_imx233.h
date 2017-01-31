#ifndef __SERIAL_DBG_IMX233__
#define __SERIAL_DBG_IMX233__

#define HW_SERIALDBG_BASE           		0x80070000

#define HW_SERIALDBG_DR           			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x0))
#define HW_SERIALDBG_ERR           			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x4))
#define HW_SERIALDBG_FLAGS         			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x18))
#define HW_SERIALDBG_GILPR         			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x20))
#define HW_SERIALDBG_INTBAUD       			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x24))
#define HW_SERIALDBG_FRACBAUD      			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x28))
#define HW_SERIALDBG_LINECTRL      			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x2c))
#define HW_SERIALDBG_UARTCTRL      			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x30))
#define HW_SERIALDBG_FIFOINTR      			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x34))
#define HW_SERIALDBG_INTRMASK      			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x38))
#define HW_SERIALDBG_RAWISTATUS    			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x3C))
#define HW_SERIALDBG_MASKISTATUS   			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x40))
#define HW_SERIALDBG_CLEARINTR   			(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x44))
#define HW_SERIALDBG_DMACTRL   				(*(volatile uint32_t *)(HW_SERIALDBG_BASE + 0x48))

#define HW_SERIALDBG_DR__OVERRUN_ERR		(1<<11)
#define HW_SERIALDBG_DR__BREAK_ERR			(1<<10)
#define HW_SERIALDBG_DR__PARITY_ERR			(1<<9)
#define HW_SERIALDBG_DR__FRAMING_ERR		(1<<8)

#define HW_SERIALDBG_ERR__CLEAR_ERROR		(1<<4)
#define HW_SERIALDBG_ERR__OVERRUN_ERR		(1<<3)
#define HW_SERIALDBG_ERR__BREAK_ERR			(1<<2)
#define HW_SERIALDBG_ERR__PARITY_ERR		(1<<1)
#define HW_SERIALDBG_ERR__FRAMING_ERR		(1<<0)

#define HW_SERIALDBG_FLAGS__RINGING			(1<<8)
#define HW_SERIALDBG_FLAGS__TXFIFOEMPTY		(1<<7)
#define HW_SERIALDBG_FLAGS__RXFIFOFULL		(1<<6)
#define HW_SERIALDBG_FLAGS__TXFIFOFULL		(1<<5)
#define HW_SERIALDBG_FLAGS__RXFIFOEMPTY		(1<<4)
#define HW_SERIALDBG_FLAGS__UARTBUSY		(1<<3)
#define HW_SERIALDBG_FLAGS__DCD				(1<<2)
#define HW_SERIALDBG_FLAGS__DSR				(1<<1)
#define HW_SERIALDBG_FLAGS__CTS				(1<<0)

#define HW_SERIALDBG_LINECTRL__SPS			(1<<7)
#define HW_SERIALDBG_LINECTRL__WLEN_8		(3<<5)
#define HW_SERIALDBG_LINECTRL__WLEN_7		(2<<5)
#define HW_SERIALDBG_LINECTRL__WLEN_6		(1<<5)
#define HW_SERIALDBG_LINECTRL__WLEN_5		(0<<5)

#define HW_SERIALDBG_LINECTRL__FEN			(1<<4)
#define HW_SERIALDBG_LINECTRL__2STOP		(1<<3)
#define HW_SERIALDBG_LINECTRL__EPS			(1<<2)
#define HW_SERIALDBG_LINECTRL__PEN			(1<<1)
#define HW_SERIALDBG_LINECTRL__BRK			(1<<0)

#define HW_SERIALDBG_UARTCTRL__RXE			(1<<9)
#define HW_SERIALDBG_UARTCTRL__TXE			(1<<8)
#define HW_SERIALDBG_UARTCTRL__LOOPBACK		(1<<7)
#define HW_SERIALDBG_UARTCTRL__UARTEN		(1<<0)

#define HW_SERIALDBG__OEIM					(1<<10)
#define HW_SERIALDBG__BEIM					(1<<9)
#define HW_SERIALDBG__PEIM					(1<<8)
#define HW_SERIALDBG__FEIM					(1<<7)
#define HW_SERIALDBG__RTIM					(1<<6)
#define HW_SERIALDBG__TXIM					(1<<5)
#define HW_SERIALDBG__RXIM					(1<<4)




#ifndef __ASSEMBLY__
/*
 * We can use a combined structure for PL010 and PL011, because they overlap
 * only in common registers.
 */
struct pl01x_regs {
	uint32_t	dr;		/* 0x00 Data register */
	uint32_t	ecr;		/* 0x04 Error clear register (Write) */
	uint32_t	pl010_lcrh;	/* 0x08 Line control register, high byte */
	uint32_t	pl010_lcrm;	/* 0x0C Line control register, middle byte */
	uint32_t	pl010_lcrl;	/* 0x10 Line control register, low byte */
	uint32_t	pl010_cr;	/* 0x14 Control register */
	uint32_t	fr;		/* 0x18 Flag register (Read only) */
#ifdef CONFIG_PL011_SERIAL_RLCR
	uint32_t	pl011_rlcr;	/* 0x1c Receive line control register */
#else
	uint32_t	reserved;
#endif
	uint32_t	ilpr;		/* 0x20 IrDA low-power counter register */
	uint32_t	pl011_ibrd;	/* 0x24 Integer baud rate register */
	uint32_t	pl011_fbrd;	/* 0x28 Fractional baud rate register */
	uint32_t	pl011_lcrh;	/* 0x2C Line control register */
	uint32_t	pl011_cr;	/* 0x30 Control register */
	uint32_t    pl011_fls;  /* 0x34 Interrupt FIFO */
	uint32_t    intr_mask;  /* 0x38 Interrupt mask */
	uint32_t    intr_status;/* 0x3C Interrupt status */
	uint32_t    intr_masked;/* 0x40 Interrupt masked */
	uint32_t    intr_clear; /* 0x44 Interupt clear */
};
#endif

#define UART_PL01x_RSR_OE               0x08
#define UART_PL01x_RSR_BE               0x04
#define UART_PL01x_RSR_PE               0x02
#define UART_PL01x_RSR_FE               0x01

#define UART_PL01x_FR_TXFE              0x80
#define UART_PL01x_FR_RXFF              0x40
#define UART_PL01x_FR_TXFF              0x20
#define UART_PL01x_FR_RXFE              0x10
#define UART_PL01x_FR_BUSY              0x08
#define UART_PL01x_FR_TMSK              (UART_PL01x_FR_TXFF + UART_PL01x_FR_BUSY)

/*
 *  PL010 definitions
 *
 */
#define UART_PL010_CR_LPE               (1 << 7)
#define UART_PL010_CR_RTIE              (1 << 6)
#define UART_PL010_CR_TIE               (1 << 5)
#define UART_PL010_CR_RIE               (1 << 4)
#define UART_PL010_CR_MSIE              (1 << 3)
#define UART_PL010_CR_IIRLP             (1 << 2)
#define UART_PL010_CR_SIREN             (1 << 1)
#define UART_PL010_CR_UARTEN            (1 << 0)

#define UART_PL010_LCRH_WLEN_8          (3 << 5)
#define UART_PL010_LCRH_WLEN_7          (2 << 5)
#define UART_PL010_LCRH_WLEN_6          (1 << 5)
#define UART_PL010_LCRH_WLEN_5          (0 << 5)
#define UART_PL010_LCRH_FEN             (1 << 4)
#define UART_PL010_LCRH_STP2            (1 << 3)
#define UART_PL010_LCRH_EPS             (1 << 2)
#define UART_PL010_LCRH_PEN             (1 << 1)
#define UART_PL010_LCRH_BRK             (1 << 0)


#define UART_PL010_BAUD_460800            1
#define UART_PL010_BAUD_230400            3
#define UART_PL010_BAUD_115200            13
#define UART_PL010_BAUD_57600             15
#define UART_PL010_BAUD_38400             23
#define UART_PL010_BAUD_19200             47
#define UART_PL010_BAUD_14400             63
#define UART_PL010_BAUD_9600              156
#define UART_PL010_BAUD_4800              191
#define UART_PL010_BAUD_2400              383
#define UART_PL010_BAUD_1200              767
/*
 *  PL011 definitions
 *
 */
#define UART_PL011_LCRH_SPS             (1 << 7)
#define UART_PL011_LCRH_WLEN_8          (3 << 5)
#define UART_PL011_LCRH_WLEN_7          (2 << 5)
#define UART_PL011_LCRH_WLEN_6          (1 << 5)
#define UART_PL011_LCRH_WLEN_5          (0 << 5)
#define UART_PL011_LCRH_FEN             (1 << 4)
#define UART_PL011_LCRH_STP2            (1 << 3)
#define UART_PL011_LCRH_EPS             (1 << 2)
#define UART_PL011_LCRH_PEN             (1 << 1)
#define UART_PL011_LCRH_BRK             (1 << 0)

#define UART_PL011_CR_CTSEN             (1 << 15)
#define UART_PL011_CR_RTSEN             (1 << 14)
#define UART_PL011_CR_OUT2              (1 << 13)
#define UART_PL011_CR_OUT1              (1 << 12)
#define UART_PL011_CR_RTS               (1 << 11)
#define UART_PL011_CR_DTR               (1 << 10)
#define UART_PL011_CR_RXE               (1 << 9)
#define UART_PL011_CR_TXE               (1 << 8)
#define UART_PL011_CR_LPE               (1 << 7)
#define UART_PL011_CR_IIRLP             (1 << 2)
#define UART_PL011_CR_SIREN             (1 << 1)
#define UART_PL011_CR_UARTEN            (1 << 0)

#define UART_PL011_IMSC_OEIM            (1 << 10)
#define UART_PL011_IMSC_BEIM            (1 << 9)
#define UART_PL011_IMSC_PEIM            (1 << 8)
#define UART_PL011_IMSC_FEIM            (1 << 7)
#define UART_PL011_IMSC_RTIM            (1 << 6)
#define UART_PL011_IMSC_TXIM            (1 << 5)
#define UART_PL011_IMSC_RXIM            (1 << 4)
#define UART_PL011_IMSC_DSRMIM          (1 << 3)
#define UART_PL011_IMSC_DCDMIM          (1 << 2)
#define UART_PL011_IMSC_CTSMIM          (1 << 1)
#define UART_PL011_IMSC_RIMIM           (1 << 0)

#endif // __SERIAL_DBG_IMX233__
