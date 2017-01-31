#include "system.h"
#include "timer.h"

#include "draw.h"
#include "debug.h"

#include "keypad.h"

///////////////////////////////////////////////////////////////

#define ESC_CHAR 0x1B

///////////////////////////////////////////////////////////////

static device_t * tty_keypad;

///////////////////////////////////////////////////////////////

void keypad_init( void )
{
	uint32_t baud = 9600;
	tty_keypad = device_open( DEV_KEYPAD );
	if( tty_keypad != NULL )
		device_ioctrl( tty_keypad, DEVICE_SET_CONFIG_DATA, &baud );
}

bool_t keypad_hit( void )
{
	if( tty_keypad == NULL ) return FALSE;
	if( device_is_rx_pending( tty_keypad ) ) return TRUE;
	return FALSE;
}

uint8_t keypad_read( void )
{
	uint8_t ret;
	uint8_t key_ret;
	timer_data_t wait_ms;

	ret = 0;

	if( keypad_hit() )
	{
		device_read_buffer( tty_keypad, &key_ret, 1 );
		if( key_ret == ESC_CHAR )
		{
			timer_Start( &wait_ms, 10 );
			while( !timer_Check( &wait_ms ) || keypad_hit() )
			{
				device_read_buffer( tty_keypad, &key_ret, 1 );
				timer_Reload( &wait_ms );
			}
		}
		else
		{
			ret = key_ret;
		}
	}

	return ret;
}

