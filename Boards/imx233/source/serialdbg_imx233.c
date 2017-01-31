#include "serialdbg_imx233.h"
#include "system_imx233.h"
#include "regsuartdbg.h"

#define CONFIG_BAUDRATE						115200
#define CONFIG_CONS_INDEX 					0
#define CONSOLE_PORT 						CONFIG_CONS_INDEX
#define CONFIG_PL01x_PORTS					{ (void *)HW_UARTDBGDR_ADDR }

static volatile unsigned char *const port[] = REGS_UARTDBG_BASE;
#define NUM_PORTS (sizeof(port)/sizeof(port[0]))

static void pl01x_putc (int portnum, char c);
static int pl01x_getc (int portnum);
static int pl01x_tstc (int portnum);


static struct pl01x_regs *pl01x_get_regs(int portnum)
{
	return (struct pl01x_regs *) port[portnum];
}


static void pl01x_putc (int portnum, char c)
{
	struct pl01x_regs *regs = pl01x_get_regs(portnum);

	/* Wait until there is space in the FIFO */
	while (readl(&regs->fr) & UART_PL01x_FR_TXFF)
		WATCHDOG_RESET();

	/* Send the character */
	writel(c, &regs->dr);
}

static int pl01x_getc (int portnum)
{
	struct pl01x_regs *regs = pl01x_get_regs(portnum);
	unsigned int data;

	/* Wait until there is data in the FIFO */
	while (readl(&regs->fr) & UART_PL01x_FR_RXFE)
		WATCHDOG_RESET();

	data = readl(&regs->dr);

	/* Check for an error flag */
	if (data & 0xFFFFFF00) {
		/* Clear the error */
		writel(0xFFFFFFFF, &regs->ecr);
		return -1;
	}

	return (int) data;
}

static int pl01x_tstc (int portnum)
{
	struct pl01x_regs *regs = pl01x_get_regs(portnum);

	WATCHDOG_RESET();
	return !(readl(&regs->fr) & UART_PL01x_FR_RXFE);
}

int imx233_serial_init( uint32_t baudrate )
{
	struct pl01x_regs *regs = pl01x_get_regs(CONSOLE_PORT);
	unsigned int divisor;

	/* First, disable everything */
	writel(0, &regs->pl010_cr);

	/* Set baud rate */
	switch (baudrate) {
	case 9600:
		divisor = UART_PL010_BAUD_9600;
		break;

	case 19200:
		divisor = UART_PL010_BAUD_9600;
		break;

	case 38400:
		divisor = UART_PL010_BAUD_38400;
		break;

	case 57600:
		divisor = UART_PL010_BAUD_57600;
		break;

	case 115200:
		divisor = UART_PL010_BAUD_115200;
		break;

	default:
		divisor = UART_PL010_BAUD_38400;
	}

	writel((divisor & 0xf00) >> 8, &regs->pl010_lcrm);
	writel(divisor & 0xff, &regs->pl010_lcrl);

	/* Set the UART to be 8 bits, 1 stop bit, no parity, fifo enabled */
	writel(UART_PL010_LCRH_WLEN_8 | UART_PL010_LCRH_FEN, &regs->pl010_lcrh);

	/* Finally, enable the UART */
	writel(UART_PL010_CR_UARTEN, &regs->pl010_cr);

	return 0;
}

void imx233_serial_putc(const char c)
{
	if (c == '\n')
		pl01x_putc (CONSOLE_PORT, '\r');

	pl01x_putc (CONSOLE_PORT, c);
}

int imx233_serial_getc(void)
{
	return pl01x_getc (CONSOLE_PORT);
}

int imx233_serial_rx_check(void)
{
	return pl01x_tstc (CONSOLE_PORT);
}

void imx233_serial_setbrg(void)
{
	struct pl01x_regs *regs = pl01x_get_regs(CONSOLE_PORT);

	while (!(readl(&regs->fr) & UART_PL01x_FR_TXFE))
	{
	}
	while (readl(&regs->fr) & UART_PL01x_FR_BUSY)
	{
	}

	imx233_serial_init( CONFIG_BAUDRATE );
}
