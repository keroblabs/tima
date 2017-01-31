#include "system.h"
#include "types.h"

#include "regslradc.h"
#include "pinctrl-imx233.h"
#include "system_imx233.h"

//#include "debug.h"

//////////////////////////////////////////////////////////////////////////////

#define ADC_NB_SAMPLES      1
#define ADC_DMA_SIZE        (uint32_t)( ADC_NB_CHANNELS )
#define ADC_TOTAL_INDEX		4
#define ADC_NOT_IN_USE		0xFF

//////////////////////////////////////////////////////////////////////////////

enum
{
	ADC_STATE_IDLE,
	ADC_STATE_PENDING,
	ADC_STATE_CONVERTING,
	ADC_STATE_READY
};

//////////////////////////////////////////////////////////////////////////////

static bool_t adc_init = FALSE;

static uint32_t imx233_adc_mask;

static uint8_t adc_busy;
static uint8_t adc_index;
static uint8_t adc_state[ADC_TOTAL_INDEX];
static uint16_t adc_value[ADC_TOTAL_INDEX];

//////////////////////////////////////////////////////////////////////////////
    
void hw_adc_start( uint32_t index )
{
	if( index >= ADC_TOTAL_INDEX )
	{
		imx233_adc_mask = 0;
		return;
	}

	imx233_adc_mask = ( 1 << index );
	HW_LRADC_CTRL0_SET( imx233_adc_mask );

	if( !( HW_LRADC_CTRL0_RD() & ( 1 << index ) ) )
	{
		DEBUG_PRINTK( "ADC%d failed\r\n", index );
	}
}

bool_t hw_adc_check( uint16_t * value, uint32_t index )
{
	if( imx233_adc_mask == 0 ) return FALSE;
	if( index >= ADC_TOTAL_INDEX ) return FALSE;
	if( !( imx233_adc_mask & ( 1 << index ) ) ) return FALSE;

	if( !( HW_LRADC_CTRL0_RD() & ( 1 << index ) ) )
	{
		*value = ( HW_LRADC_CHn_RD(index) & 0x03FFFF );
		return TRUE;
	}

	return FALSE;
}

void hw_adc_init( void )
{
	__IO int i;

	// reset adc
	HW_LRADC_CTRL0_SET(BM_LRADC_CTRL0_SFTRST | BM_LRADC_CTRL0_CLKGATE);
	for( i = 0; i < 1000; i++ ) {}
	HW_LRADC_CTRL0_CLR(BM_LRADC_CTRL0_SFTRST | BM_LRADC_CTRL0_CLKGATE);
}

///////////////////////////////////////////////////////////////

static void adc_process( void )
{
	uint16_t adc_result = 0;
	int i;

	if( adc_busy != ADC_NOT_IN_USE )
	{
		if( hw_adc_check( &adc_result, adc_busy ) )
		{
			if( adc_state[ adc_busy ] == ADC_STATE_CONVERTING )
			{
				DEBUG_PRINTK( "ADC end %d (%d)\r\n", adc_busy, adc_result );
				adc_state[adc_busy] = ADC_STATE_READY;
				//if( adc_busy != 0 ) adc_result >>= 4;
				adc_value[adc_busy] = adc_result;
			}

			adc_busy = ADC_NOT_IN_USE;
		}
	}
	else
	{
		if( adc_state[ adc_index ] == ADC_STATE_PENDING )
		{
			DEBUG_PRINTK( "ADC start %d\r\n", adc_index );
			adc_state[ adc_index ] = ADC_STATE_CONVERTING;
			hw_adc_start( adc_index );
			adc_busy = adc_index;
		}

		adc_index++;
		if( adc_index >= ADC_TOTAL_INDEX ) adc_index = 0;
	}
}

static uint32_t adc_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
	int ret = 0;

	return ret;
}

static uint32_t adc_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	uint32_t ret = 0;
	uint32_t ret_value;
	uint16_t * p_adc = ( uint16_t * )buffer;

	if( adc_state[index] == ADC_STATE_READY )
	{
		// if adc conversion has finished
		ret = sizeof( uint16_t );

		if( buffer != NULL )
		{
			adc_state[index] = ADC_STATE_IDLE;
			*p_adc = adc_value[index];
		}
	}

    return ret;
}

static void adc_drv_init( uint32_t index )
{
	int i;

	if( adc_init == TRUE ) return;

	for( i = 0; i < ADC_TOTAL_INDEX; i++ )
	{
		adc_value[i] = 0;
		adc_state[i] = ADC_STATE_IDLE;
	}

	adc_busy = ADC_NOT_IN_USE;
	adc_index = 0;

	hw_adc_init();

	adc_init = TRUE;
}

static void adc_drv_close( uint32_t index )
{
}

static uint32_t adc_drv_ioctl( uint32_t index, uint32_t param, void * value )
{
	//uint32_t * p_value = ( uint32_t * )value;

	switch( param )
	{
		case DEVICE_SET_RUNTIME_VALUE:
			// trigger adc
			adc_state[index] = ADC_STATE_PENDING;
			DEBUG_PRINTK( "ADC trigger %d\r\n", index );
			break;
	}

	return 0;
}

static bool_t adc_validate( uint32_t index )
{
    if( index < ADC_TOTAL_INDEX ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t adc_device =
{
    "adc*",
    &_in_use,

    adc_drv_init,
    adc_drv_close,
    NULL,
    adc_validate,
    NULL,
    adc_drv_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

    adc_input,
    adc_output,

};

DECLARE_DEVICE_SECTION( adc_device );
DECLARE_PROCESS_SECTION( adc_process );
