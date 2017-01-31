#include "system.h"
#include "timer.h"

#if 1 // defined MAC_OS_EMULATOR
#include "debug.h"
#endif

#include "graphics.h"
#include "draw.h"
#include "instruments.h"
#include "message.h"
#include "tima_libc.h"
#include "pio_check.h"

#include "gui_setup.h"

///////////////////////////////////////////////////////////////

enum
{
	SETUP_FORM_IDLE,
	SETUP_FORM_CONFIG,
	SETUP_FORM_CONFIG_ITEMS,
	SETUP_FORM_RESET,
};

///////////////////////////////////////////////////////////////

#define TIMER_FLASHING_MS			400
#define TIMER_SHORT_PRESS_MS		100
#define TIMER_LONG_PRESS_MS			1500
#define CONFIG_MAX_ITEM				5

///////////////////////////////////////////////////////////////

static timer_data_t boot_time;
static message_data_t * p_message;

static bool_t is_skip;

///////////////////////////////////////////////////////////////

void gui_setup_decr_time( char * timer_str, uint8_t index )
{
	if( index == 0 )
	{
		if( ( timer_str[0] == '0' ) || ( timer_str[0] == ' ' ) )
		{
			timer_str[0] = '2';
		}
		else
		{
			timer_str[0]--;
		}
	}
	else if( index == 1 )
	{
		if( timer_str[1] == '0' )
		{
			if( ( timer_str[0] == '0' ) || ( timer_str[0] == ' ' ) )
			{
				timer_str[1] = '3';
			}
			else
			{
				timer_str[1] = '9';
			}
		}
		else
		{
			timer_str[1]--;
		}
	}
	else if( index == 2 )
	{
		if( timer_str[3] == '0' )
		{
			timer_str[3] = '5';
		}
		else
		{
			timer_str[3]--;
		}
	}
	else if( index == 3 )
	{
		if( timer_str[4] == '0' )
		{
			timer_str[4] = '9';
		}
		else
		{
			timer_str[4]--;
		}
	}
}

void gui_setup_incr_time( char * timer_str, uint8_t index )
{
	if( index == 0 )
	{
		if( timer_str[0] == '2' )
		{
			timer_str[0] = '0';
		}
		else
		{
			timer_str[0]++;
		}

		if( ( timer_str[0] == '2' ) && ( timer_str[1] > '3' ) )
		{
			timer_str[1] = '3';
		}
	}
	else if( index == 1 )
	{
		if( ( timer_str[0] == '2' ) && ( timer_str[1] == '3' ) )
		{
			timer_str[1] = '0';
		}
		else if( timer_str[1] == '9' )
		{
			timer_str[1] = '0';
		}
		else
		{
			timer_str[1]++;
		}
	}
	else if( index == 2 )
	{
		if( timer_str[3] == '5' )
		{
			timer_str[3] = '0';
		}
		else
		{
			timer_str[3]++;
		}
	}
	else if( index == 3 )
	{
		if( timer_str[4] == '9' )
		{
			timer_str[4] = '0';
		}
		else
		{
			timer_str[4]++;
		}
	}
}

uint32_t gui_setup_string_to_time( char * time_str, date_time_t * date_now )
{
	uint32_t ret;
	char local_time_str[10];

	strcpy( local_time_str, time_str );
	local_time_str[2] = 0;

	date_now->tm_hour = ( int )tima_atoi( &local_time_str[0] );
	date_now->tm_min = ( int )tima_atoi( &local_time_str[3] );

	return ret;
}

void gui_setup_init( void )
{
	p_message = ( message_data_t * )pio_check_reset_message();
	timer_Start( &boot_time, 1000 );
	is_skip = TRUE;
}

void gui_setup_process( void )
{
	uint16_t message;

	if( message_Read( p_message ) != Message_NoMessage )
	{
		message = message_Get( p_message );
		if( !is_skip )
		{
			if( message == MESSAGE_MODE_ON )
			{
				DEBUG_PRINTK( "MESSAGE_MODE_ON\r\n" );
			}

			if( message == MESSAGE_BUTTON_SHORT )
			{
				DEBUG_PRINTK( "MESSAGE_BUTTON_SHORT\r\n" );
				instruments_show_information( NULL, SETUP_PARAM_INCR_TRIP_INDEX );
			}
			if( message == MESSAGE_BUTTON_LONG )
			{
				DEBUG_PRINTK( "MESSAGE_BUTTON_LONG\r\n" );
				instruments_show_information( NULL, SETUP_PARAM_RESET_TRIP );
			}
		}
	}

	if( timer_Check( &boot_time ) )
	{
		timer_Stop( &boot_time );
		is_skip = FALSE;
	}
}

