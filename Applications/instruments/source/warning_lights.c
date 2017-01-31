#include "timer.h"
#include "instruments.h"
#include "warning_lights.h"

#include "icons.h"

///////////////////////////////////////////////////////////////

#define TIMED_WARNING_PROC( mask, timer, cnt )		if( timer_Check( &timer ) )															\
													{	timer_Reload( &timer ); cnt++;													\
														if( cnt & 0x01 )																\
														{	instruments_update_warning_light_index( mask, ( 1 << mask ), TRUE ); }		\
														else { instruments_update_warning_light_index( mask, 0, TRUE ); }				\
													}

#define TIMED_WARNING( mask, timer, ms )	if( ( curr_warning_mask & ( 1 << mask ) ) &&			\
												!( prev_warning_mask & ( 1 << mask ) ) )			\
											{ timer_Start( &timer, ms ); }							\
											else if( !( curr_warning_mask & ( 1 << mask ) ) &&		\
													  ( prev_warning_mask & ( 1 << mask ) ) )		\
											{	timer_Stop( &timer );	}

///////////////////////////////////////////////////////////////

#define TEMP_FLASHING_MS		800
#define BRAKE_FLASHING_MS		300

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static uint32_t curr_warning_mask;
static uint32_t prev_warning_mask;

static timer_data_t handbrake_moving_timer;
static uint8_t handbrake_moving_cnt;

static timer_data_t temperature_timer;
static uint8_t temperature_cnt;

///////////////////////////////////////////////////////////////

void warning_lights_update( uint32_t input )
{
	uint32_t cnt_search;

	curr_warning_mask = input;

	if( curr_warning_mask != prev_warning_mask )
	{
		for( cnt_search = 0; cnt_search < 32; cnt_search++ )
		{
			if( ( curr_warning_mask & ( 1 << cnt_search ) ) != ( prev_warning_mask & ( 1 << cnt_search ) ) )
			{
				instruments_update_warning_light_index( cnt_search, curr_warning_mask & ( 1 << cnt_search ), FALSE );
			}
		}

		if( curr_warning_mask & ( 1 << WARNING_HANDBRAKE_STATE ) )
		{
			instruments_update_warning_light_index( WARNING_HANDBRAKE_STATE, ( 1 << WARNING_HANDBRAKE_STATE ), FALSE );
		}
		else
		{
			TIMED_WARNING( WARNING_HANDBRAKE_MOVING, handbrake_moving_timer, BRAKE_FLASHING_MS );
		}

		TIMED_WARNING( WARNING_TEMPERATURE, temperature_timer, TEMP_FLASHING_MS );

		prev_warning_mask = curr_warning_mask;
	}
}

void warning_lights_process( void )
{
	if( curr_warning_mask & ( 1 << WARNING_HANDBRAKE_STATE ) )
	{
	}
	else
	{
		TIMED_WARNING_PROC( WARNING_HANDBRAKE_MOVING, handbrake_moving_timer, handbrake_moving_cnt );
	}

	TIMED_WARNING_PROC( WARNING_TEMPERATURE, temperature_timer, temperature_cnt );
}

void warning_lights_init( void )
{
	prev_warning_mask = 0;
	curr_warning_mask = 0;

	timer_Stop( &handbrake_moving_timer );
	handbrake_moving_cnt = 0;

	timer_Stop( &temperature_timer );
	temperature_cnt = 0;
}
