#include "mmc_layer.h"
#include "system_imx233.h"
#include "pinctrl-imx233.h"
#include "clkctrl-imx233.h"
#include "dma_imx233.h"

#include "types.h"
#include "system.h"

////////////////////////////////////////////////////////////////////////

/* Used for DMA */
struct ssp_dma_command_t
{
    struct apb_dma_command_t dma;
    /* PIO words */
    uint32_t ctrl0;
    uint32_t cmd0;
    uint32_t cmd1;
};

#define MMC_BLOCK_SIZE		512
#define MMC_BUS_WIDTH		4

////////////////////////////////////////////////////////////////////////

static struct ssp_dma_command_t ssp_dma_cmd[2];

////////////////////////////////////////////////////////////////////////

void INT_SSP(int ssp)
{
    /* reset dma channel on error */
    if(imx233_dma_is_channel_error_irq(APB_SSP(ssp)))
        imx233_dma_reset_channel(APB_SSP(ssp));

    /* clear irq flags */
    imx233_dma_clear_channel_interrupt(APB_SSP(ssp));
}

void INT_SSP1_DMA(void)
{
    INT_SSP(1);
}

void INT_SSP2_DMA(void)
{
    INT_SSP(2);
}

void INT_SSP1_ERROR(void)
{
    //panicf("ssp1 error");
	while(1) {}
}

void INT_SSP2_ERROR(void)
{
    //panicf("ssp2 error");
	while(1) {}
}

void imx233_sdcard_init(void)
{
	int ssp = 0;

    /* Gate block */
    __REG_CLR(HW_SSP_CTRL0(ssp)) = __BLOCK_CLKGATE;
    while(HW_SSP_CTRL0(ssp) & __BLOCK_CLKGATE){}

    /* Gate dma channel */
    imx233_dma_clkgate_channel(APB_SSP(ssp), TRUE);

    /* If first block to start, start SSP clock */
    /** 2.3.1: the clk_ssp maximum frequency is 102.858 MHz */
    /* fracdiv = 18 => clk_io = pll = 480Mhz
     * intdiv = 5 => clk_ssp = 96Mhz */
    imx233_set_fractional_divisor(CLK_IO, 18);
    imx233_enable_clock(CLK_SSP, FALSE);
    imx233_set_clock_divisor(CLK_SSP, 5);
    imx233_set_bypass_pll(CLK_SSP, FALSE); /* use IO */
    imx233_enable_clock(CLK_SSP, TRUE);

    /* clear mode and word length */
    __REG_CLR(HW_SSP_CTRL1(ssp)) = HW_SSP_CTRL1__SSP_MODE_BM | HW_SSP_CTRL1__WORD_LENGTH_BM;
    /* set mode, set word length to 8-bit, polarity and enable dma */
    __REG_SET(HW_SSP_CTRL1(ssp)) = HW_SSP_CTRL1__SSP_MODE__SD_MMC |
        HW_SSP_CTRL1__WORD_LENGTH__EIGHT_BITS << HW_SSP_CTRL1__WORD_LENGTH_BP |
        HW_SSP_CTRL1__POLARITY | HW_SSP_CTRL1__DMA_ENABLE;
}

void imx233_ssp_set_timings(int ssp, int divide, int rate, int timeout)
{
    HW_SSP_TIMING(ssp) = divide << HW_SSP_TIMING__CLOCK_DIVIDE_BP | rate |
        timeout << HW_SSP_TIMING__CLOCK_TIMEOUT_BP;
}

enum imx233_ssp_error_t imx233_ssp_sd_mmc_transfer(uint8_t cmd,
    uint32_t cmd_arg, enum imx233_ssp_resp_t resp, void *buffer,
    bool_t wait4irq, bool_t read, uint32_t *resp_ptr)
{
	unsigned xfer_size;
	enum imx233_ssp_error_t ret;
	int ssp = 1;
	unsigned block_count = 1;

    //mutex_lock(&ssp_mutex[ssp - 1]);
    /* Enable all interrupts */
    imx233_enable_interrupt(INT_SRC_SSP_DMA(ssp), TRUE);
    imx233_dma_enable_channel_interrupt(APB_SSP(ssp), TRUE);

    xfer_size = block_count * (1 << ssp_log_block_size[ssp - 1]);

    ssp_dma_cmd[ssp - 1].cmd0 = cmd | HW_SSP_CMD0__APPEND_8CYC |
    		MMC_BLOCK_SIZE << HW_SSP_CMD0__BLOCK_SIZE_BP |
        (block_count - 1) << HW_SSP_CMD0__BLOCK_COUNT_BP;
    ssp_dma_cmd[ssp - 1].cmd1 = cmd_arg;
    /* setup all flags and run */
    ssp_dma_cmd[ssp - 1].ctrl0 = xfer_size | HW_SSP_CTRL0__ENABLE |
        (buffer ?  0 : HW_SSP_CTRL0__IGNORE_CRC) |
        (wait4irq ? HW_SSP_CTRL0__WAIT_FOR_IRQ : 0) |
        (resp != SSP_NO_RESP ? HW_SSP_CTRL0__GET_RESP : 0) |
        (resp == SSP_LONG_RESP ? HW_SSP_CTRL0__LONG_RESP : 0) |
        (MMC_BUS_WIDTH << HW_SSP_CTRL0__BUS_WIDTH_BP) |
        (buffer ? HW_SSP_CTRL0__DATA_XFER : 0) |
        (read ? HW_SSP_CTRL0__READ : 0);
    /* setup the dma parameters */
    ssp_dma_cmd[ssp - 1].dma.buffer = buffer;
    ssp_dma_cmd[ssp - 1].dma.next = NULL;
    ssp_dma_cmd[ssp - 1].dma.cmd =
        (buffer == NULL ? HW_APB_CHx_CMD__COMMAND__NO_XFER :
         read ? HW_APB_CHx_CMD__COMMAND__WRITE : HW_APB_CHx_CMD__COMMAND__READ) |
        HW_APB_CHx_CMD__IRQONCMPLT | HW_APB_CHx_CMD__SEMAPHORE |
        HW_APB_CHx_CMD__WAIT4ENDCMD |
        (3 << HW_APB_CHx_CMD__CMDWORDS_BP) |
        (xfer_size << HW_APB_CHx_CMD__XFER_COUNT_BP);

    __REG_CLR(HW_SSP_CTRL1(ssp)) = HW_SSP_CTRL1__ALL_IRQ;
    imx233_dma_reset_channel(APB_SSP(ssp));
    imx233_dma_start_command(APB_SSP(ssp), &ssp_dma_cmd[ssp - 1].dma);

    /* the SSP hardware already has a timeout but we never know; 1 sec is a maximum
     * for all operations */
    // enum imx233_ssp_error_t ret;

    /*
    if(semaphore_wait(&ssp_sema[ssp - 1], HZ) == OBJ_WAIT_TIMEDOUT)
    {
        imx233_dma_reset_channel(APB_SSP(ssp));
        ret = SSP_TIMEOUT;
    }
    else
    	*/
    if((HW_SSP_CTRL1(ssp) & HW_SSP_CTRL1__ALL_IRQ) == 0)
        ret =  SSP_SUCCESS;
    else if(HW_SSP_CTRL1(ssp) & (HW_SSP_CTRL1__RESP_TIMEOUT_IRQ |
            HW_SSP_CTRL1__DATA_TIMEOUT_IRQ | HW_SSP_CTRL1__RECV_TIMEOUT_IRQ))
        ret = SSP_TIMEOUT;
    else
        ret = SSP_ERROR;

    if(resp_ptr != NULL)
    {
        if(resp != SSP_NO_RESP)
            *resp_ptr++ = HW_SSP_SDRESP0(ssp);
        if(resp == SSP_LONG_RESP)
        {
            *resp_ptr++ = HW_SSP_SDRESP1(ssp);
            *resp_ptr++ = HW_SSP_SDRESP2(ssp);
            *resp_ptr++ = HW_SSP_SDRESP3(ssp);
        }
    }
    //mutex_unlock(&ssp_mutex[ssp - 1]);
    return ret;
}



void imx233_ssp_setup_ssp1_sd_mmc_pins(void)
{
	unsigned int i;
	bool_t enable_pullups = TRUE;
	unsigned drive_strength = 1;

    /* SSP_{CMD,SCK} */
    imx233_set_pin_drive_strength(2, 0, drive_strength);
    imx233_set_pin_drive_strength(2, 6, drive_strength);
    imx233_set_pin_function(2, 0, PINCTRL_FUNCTION_MAIN);
    imx233_set_pin_function(2, 6, PINCTRL_FUNCTION_MAIN);
    imx233_enable_pin_pullup(2, 0, enable_pullups);

    /* SSP_DATA{0-3} */
    for(i = 0; i < MMC_BUS_WIDTH; i++)
    {
        imx233_set_pin_drive_strength(2, 2 + i, drive_strength);
        imx233_set_pin_function(2, 2 + i, PINCTRL_FUNCTION_MAIN);
        imx233_enable_pin_pullup(2, 2 + i, enable_pullups);
    }
}




