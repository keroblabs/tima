#include "system.h"
#include "timer.h"

#include "pio_check.h"
#include "power_control.h"
#include "nvram_control.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

#define PIO_CHECK_MS			300
#define POWERDOWN_TIMER_MS		3000
#define POWERSAFE_TIMER_MS		10000
#define POWERDOWN_CUTOFF_MS		5000
#define HEADLIGHTS_ON_MS		2000

#define ALLOW_POWER_DOWN		0x00

#define POWER_FLAG_DOORS		0x01
#define POWER_FLAG_HAZARD		0x02
#define POWER_FLAG_IGNITION		0x04
#define POWER_FLAG_INSTRUMENTS	0x08

///////////////////////////////////////////////////////////////

static device_t * pio_power_relay;
static device_t * pio_headlights;

static void * pio_ign_input;
static void * pio_engine_run;

static bool_t in_powerdown;
static bool_t do_powerdown;
static bool_t enable_nvram;
static bool_t is_cutoff;

static uint8_t power_flag;

static bool_t engine_run_state;
static bool_t headlights_state;

static timer_data_t system_powerdown_timer;
static timer_data_t power_safe_timer;
static timer_data_t power_cutoff_timer;
static timer_data_t headlights_timer;

///////////////////////////////////////////////////////////////

void power_control_init( void )
{
	in_powerdown = FALSE;
	do_powerdown = FALSE;
	enable_nvram = FALSE;
	is_cutoff = FALSE;

	power_flag = 0x00;

	engine_run_state = FALSE;
	headlights_state = TRUE;

	timer_Stop( &system_powerdown_timer );
	timer_Start( &power_safe_timer, POWERSAFE_TIMER_MS );
	timer_Stop( &power_cutoff_timer );
	timer_Stop( &headlights_timer );

	// power relay control
	pio_power_relay = pio_check_open_output( "/dev/gpio1", TRUE );
	pio_headlights = pio_check_open_output( "/dev/gpio4", FALSE );

	// ignition detector
	pio_ign_input = pio_check_event_open_read_port( "/dev/gpio33", NULL, 0, TRUE, 50 );
	pio_engine_run = pio_check_event_open_read_port( "/dev/gpio15", NULL, 0, TRUE, 50 );
}

void power_control_process( void )
{
	uint8_t read_pio;

	if( timer_Check( &power_safe_timer ) )
	{
		timer_Stop( &power_safe_timer );
		enable_nvram = TRUE;
	}

	if( timer_Check( &headlights_timer ) )
	{
		timer_Stop( &headlights_timer );
		pio_check_set_output( pio_headlights, TRUE );
		headlights_state = TRUE;
	}

	if( pio_check_event_read_port ( pio_engine_run, &engine_run_state ) )
	{
		DEBUG_PRINTK( "Engine run state = %d\r\n", engine_run_state );
		if( engine_run_state == FALSE )
		{
			timer_Stop( &headlights_timer );
			pio_check_set_output( pio_headlights, FALSE );
			headlights_state = FALSE;
		}
		else
		{
			timer_Start( &headlights_timer, HEADLIGHTS_ON_MS );
		}
	}

	if( pio_check_event_read_port( pio_ign_input, &read_pio ) )
	{
		DEBUG_PRINTK( "Ignition state = %d\r\n", read_pio );

		if( read_pio )
		{
			power_flag |= POWER_FLAG_IGNITION;
			timer_Stop( &power_cutoff_timer );
			is_cutoff = FALSE;
		}
		else
		{
			power_flag &= ~POWER_FLAG_IGNITION;
			timer_Start( &power_cutoff_timer, POWERDOWN_CUTOFF_MS );
		}

	}

	if( in_powerdown )
	{
		if( ( power_flag != ALLOW_POWER_DOWN ) && ( is_cutoff == FALSE ) )
		{
			timer_Stop( &system_powerdown_timer );
			in_powerdown = FALSE;
			pio_check_set_output( pio_power_relay, TRUE );

			DEBUG_PRINTK( "Stop powerdown timer\r\n" );
		}

		if( timer_Check( &system_powerdown_timer ) )
		{
			if( is_cutoff == TRUE )
			{
				DEBUG_PRINTK( "Do cutoff\r\n" );
			}

			timer_Stop( &system_powerdown_timer );
			pio_check_set_output( pio_power_relay, FALSE );

			DEBUG_PRINTK( "Do powerdown\r\n" );
		}
	}
	else
	{
		if( ( power_flag == ALLOW_POWER_DOWN ) || ( is_cutoff == TRUE ) )
		{
			in_powerdown = TRUE;
			timer_Start( &system_powerdown_timer, POWERDOWN_TIMER_MS );

			if( enable_nvram ) nvram_control_flush();

			if( is_cutoff == TRUE )
			{
				DEBUG_PRINTK( "Start powerdown timer with cutoff\r\n" );

			}
			else
			{
				DEBUG_PRINTK( "Start powerdown timer\r\n" );
			}
		}
	}

	if( timer_Check( &power_cutoff_timer ) )
	{
		timer_Stop( &power_cutoff_timer );
		is_cutoff = TRUE;
	}
}

void power_set_instruments_state( bool_t state )
{
	if( state ) power_flag |= POWER_FLAG_INSTRUMENTS;
	else power_flag &= ~POWER_FLAG_INSTRUMENTS;
}

void power_set_hazard_state( bool_t state )
{
	if( state ) power_flag |= POWER_FLAG_HAZARD;
	else power_flag &= ~POWER_FLAG_HAZARD;
}

void power_set_doors_state( bool_t state )
{
	if( state ) power_flag |= POWER_FLAG_DOORS;
	else power_flag &= ~POWER_FLAG_DOORS;
}

bool_t power_get_ignition_input( void )
{
	return ( power_flag & POWER_FLAG_IGNITION ) ? TRUE : FALSE;
}

bool_t power_get_engine_run_state( void )
{
	return engine_run_state;
}

bool_t power_get_headlights_state( void )
{
	return headlights_state;
}
