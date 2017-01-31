#include "system.h"
#include "timer.h"

#include "lp_filter.h"
#include "draw.h"
#include "instruments.h"

//#include "debug.h"

#include "fuel_check.h"

///////////////////////////////////////////////////////////////

#define LOW_FLAG_FLASHING_MS            400

#define ADC_FUEL_READ_INTERVAL			1000

#define ADC_FULL_TANK_VALUE				2850
#define ADC_EMPTY_TANK_VALUE			2450
#define ADC_FAILED_VALUE				4000

#define ADC_FUEL_FACTOR_VALUE			( 102400 / ( ADC_FULL_TANK_VALUE - ADC_EMPTY_TANK_VALUE ) )

#define ADC_DEVICE_PATH					"/dev/adc1"
#define FUEL_FILTER_LEN					128

///////////////////////////////////////////////////////////////

typedef struct adc_fuel_table_t_
{
	uint32_t adc_min;
	uint32_t adc_max;
} adc_fuel_table_t;

const static adc_fuel_table_t adc_fuel_table[] =
{
	{ 0,    2432 },
	{ 2432, 2510 },
	{ 2510, 2588 },
	{ 2588, 2650 },
	{ 2650, 2721 },
	{ 2721, 2768 },
	{ 2768, 2825 },
	{ 2825, 2825 },
	{ 2825, 2930 },
	{ 2930, 3010 },
	{ 3010, 4096 },
	{ 4096, 9999 },
};

///////////////////////////////////////////////////////////////

static uint32_t prev_fuel_value;
static uint32_t quick_load;
static uint32_t fuel_check_value;
static uint16_t adc_fuel_value;
static uint32_t filter_buffer[FUEL_FILTER_LEN];

static bool_t is_fast_read;
static bool_t low_flag;
static bool_t very_low_flag;
static bool_t low_flag_flashing;

static timer_data_t adc_read_delay;
static timer_data_t low_flag_timer;

#ifdef DEBUG_ENABLED
static timer_data_t adc_debug_timer;
#endif

static device_t * fuel_adc_device;

static lp_filter_t fuel_filter;

///////////////////////////////////////////////////////////////

void fuel_check_init( void )
{
    low_flag = FALSE;
    very_low_flag = FALSE;
    low_flag_flashing = FALSE;
    
	is_fast_read = TRUE;
	fuel_check_value = 0;
	quick_load = FUEL_FILTER_LEN*3;
	prev_fuel_value = NEG_U32;
    
	fuel_adc_device = device_open( ADC_DEVICE_PATH );
	if( fuel_adc_device != NULL )
	{
		DEBUG_PRINTK( "Fuel Ready\r\n", (int)fuel_adc_device );

		timer_Start( &adc_read_delay, ADC_FUEL_READ_INTERVAL );
        timer_Start( &low_flag_timer, LOW_FLAG_FLASHING_MS );

#ifdef DEBUG_ENABLED
		timer_Start( &adc_debug_timer, 2000 );
#endif

		device_ioctrl( fuel_adc_device, DEVICE_SET_RUNTIME_VALUE, NULL );

		lp_filter_init( &fuel_filter, filter_buffer, FUEL_FILTER_LEN );
	}
}

void fuel_check_process( void )
{
	uint32_t fuel_value;

	if( fuel_adc_device != NULL )
	{
#ifdef DEBUG_ENABLED
		if( timer_Check( &adc_debug_timer ) )
		{
			timer_Reload( &adc_debug_timer );
			DEBUG_PRINTK( "ADC Fuel = %d - %d\r\n", lp_filter_get_value( &fuel_filter ), fuel_check_get() );
		}
#endif
        if( timer_Check( &low_flag_timer ) )
        {
            timer_Reload( &low_flag_timer );
            
            if( ( very_low_flag == TRUE ) && ( quick_load == 0 ) )
            {
                low_flag_flashing = low_flag_flashing ? FALSE : TRUE;
                skin_motocycle_draw_fuel_gauge( fuel_check_get(), low_flag_flashing );
            }
        }
        
		if( timer_Check( &adc_read_delay ) && device_is_rx_pending( fuel_adc_device ) )
		{
			device_read_buffer( fuel_adc_device, ( uint8_t * )&adc_fuel_value, sizeof( adc_fuel_value ) );
			lp_filter_add( &fuel_filter, adc_fuel_value );

			timer_Reload( &adc_read_delay );
			device_ioctrl( fuel_adc_device, DEVICE_SET_RUNTIME_VALUE, NULL );

			if( quick_load )
			{
				quick_load--;
				timer_Expire( &adc_read_delay );
			}
			else
			{
				fuel_value = fuel_check_get();
				if( ( fuel_value != prev_fuel_value ) && ( quick_load == 0 ) )
				{
                    if( ( very_low_flag == FALSE ) && ( low_flag == TRUE ) )
                        skin_motocycle_draw_fuel_gauge( fuel_check_get(), TRUE );
                    else
                        skin_motocycle_draw_fuel_gauge( fuel_check_get(), low_flag_flashing );
                    
					prev_fuel_value = fuel_value;
				}
			}
		}
	}
}

uint32_t fuel_check_get( void )
{
	uint32_t fuel_avg;
	uint32_t fuel_value;
	uint32_t cnt;

	fuel_avg = lp_filter_get_value( &fuel_filter );
    
    if( fuel_avg > 4000 )
    {
        low_flag_flashing = FALSE;
        very_low_flag = FALSE;
        low_flag = FALSE;
        
        return 0;
    }

	for( cnt = 0; cnt < 20; cnt++ )
	{
		if( adc_fuel_table[cnt].adc_min > 4095 ) break;

		if( ( fuel_avg >= adc_fuel_table[cnt].adc_min ) && ( fuel_avg < adc_fuel_table[cnt].adc_max ) )
		{
			fuel_value = cnt * 10;
			break;
		}
	}
    
    low_flag = FALSE;
    if( fuel_value <= 8 )
    {
        very_low_flag = TRUE;
        low_flag = TRUE;
    }
    else if( fuel_value <= 12 )
    {
        very_low_flag = FALSE;
        low_flag = TRUE;
    }
    else if( fuel_value >= 15 )
    {
        low_flag_flashing = FALSE;
        very_low_flag = FALSE;
        low_flag = FALSE;
    }

	return fuel_value;
}

uint32_t fuel_check_get_adc( void )
{
	return lp_filter_get_value( &fuel_filter );
}


