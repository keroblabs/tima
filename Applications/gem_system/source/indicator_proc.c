#include "system.h"
#include "timer.h"
#include "pio_check.h"
#include "power_control.h"

//#include "debug.h"

#include "indicator_proc.h"

///////////////////////////////////////////////////////////////

#define INDICATOR_ON_MS					600
#define INDICATOR_OFF_MS				600

#define INDICATOR_MIN					2

///////////////////////////////////////////////////////////////

static void * indicator_left_dev;
static void * indicator_right_dev;
static void * hazard_dev;

static device_t * indicator_out_left_dev;
static device_t * indicator_out_right_dev;
static device_t * hazard_out_dev;

static timer_data_t indicator_flash_timer;

static bool_t indicator_left_pio;
static bool_t indicator_right_pio;

static bool_t indicator_left_flag;
static bool_t indicator_right_flag;
static bool_t hazard_flag;

static bool_t indicator_state;

static uint8_t indicator_min_cnt;

///////////////////////////////////////////////////////////////

void indicator_proc_init( void )
{
	indicator_left_dev = pio_check_event_open_read_port( "/dev/gpio26", NULL, 0, TRUE, 50 );
	indicator_right_dev = pio_check_event_open_read_port( "/dev/gpio27", NULL, 0, TRUE, 50 );

	hazard_dev = pio_check_event_open_read_port( "/dev/gpio34", NULL, 0, TRUE, 10 );

	indicator_out_left_dev = pio_check_open_output( "/dev/gpio2", FALSE );
	indicator_out_right_dev = pio_check_open_output( "/dev/gpio3", FALSE );

	hazard_out_dev = pio_check_open_output( "/dev/gpio21", FALSE );

	indicator_left_pio = FALSE;
	indicator_right_pio = FALSE;

	indicator_left_flag = FALSE;
	indicator_right_flag = FALSE;

	hazard_flag = FALSE;

	timer_Stop( &indicator_flash_timer );

	indicator_state = FALSE;
}

void indicator_proc_process( void )
{
	uint8_t read_pio;

	if( !indicator_right_dev || !indicator_left_dev ) return;

	if( pio_check_event_read_port( indicator_left_dev, &indicator_left_pio ) )
	{
		if( indicator_left_pio )
		{
			if( hazard_flag == FALSE )
			{
				timer_Expire( &indicator_flash_timer );
				indicator_min_cnt = INDICATOR_MIN;
				indicator_state = FALSE;
			}

			if( ( indicator_right_flag == TRUE ) && ( hazard_flag == FALSE ) )
			{
				pio_check_set_output( indicator_out_right_dev, FALSE );
			}

			indicator_left_flag = TRUE;
			indicator_right_flag = FALSE;
		}

		DEBUG_PRINTK( "Indicator left = %d\r\n", indicator_left_pio );
	}

	if( pio_check_event_read_port( indicator_right_dev, &indicator_right_pio ) )
	{
		if( indicator_right_pio )
		{
			if( hazard_flag == FALSE )
			{
				timer_Expire( &indicator_flash_timer );
				indicator_min_cnt = INDICATOR_MIN;
				indicator_state = FALSE;
			}

			if( ( indicator_left_flag == TRUE ) && ( hazard_flag == FALSE ) )
			{
				pio_check_set_output( indicator_out_left_dev, FALSE );
			}

			indicator_left_flag = FALSE;
			indicator_right_flag = TRUE;
		}

		DEBUG_PRINTK( "Indicator right = %d\r\n", indicator_right_pio );
	}

	if( pio_check_event_read_port( hazard_dev, &read_pio ) )
	{
		if( read_pio == TRUE )
		{
			hazard_flag = ( hazard_flag == TRUE ) ? FALSE : TRUE;
		}

		power_set_hazard_state( hazard_flag );

		if( hazard_flag && read_pio )
		{
			if( ( indicator_left_flag == FALSE ) && ( indicator_right_flag == FALSE ) )
			{
				timer_Expire( &indicator_flash_timer );
				indicator_min_cnt = INDICATOR_MIN;
				indicator_state = FALSE;
			}
		}

		DEBUG_PRINTK( "Hazard = %d\r\n", hazard_flag );
	}

	if( timer_Check( &indicator_flash_timer ) )
	{
		if( indicator_state == TRUE )
		{
			indicator_state = FALSE;
			timer_Start( &indicator_flash_timer, INDICATOR_OFF_MS );

			pio_check_set_output( indicator_out_right_dev, FALSE );
			pio_check_set_output( indicator_out_left_dev, FALSE );
			pio_check_set_output( hazard_out_dev, FALSE );

			if( indicator_min_cnt ) indicator_min_cnt--;

			if( ( indicator_right_pio == 0 ) && ( indicator_left_pio == 0 ) && ( hazard_flag == FALSE ) && ( indicator_min_cnt == 0 ) )
			{
				timer_Stop( &indicator_flash_timer );
				indicator_right_flag = FALSE;
				indicator_left_flag = FALSE;
			}
		}
		else
		{
			indicator_state = TRUE;
			timer_Start( &indicator_flash_timer, INDICATOR_ON_MS );

			if( indicator_right_flag || hazard_flag ) pio_check_set_output( indicator_out_right_dev, TRUE );
			if( indicator_left_flag || hazard_flag ) pio_check_set_output( indicator_out_left_dev, TRUE );

			if( hazard_flag ) pio_check_set_output( hazard_out_dev, TRUE );
		}
	}
}
