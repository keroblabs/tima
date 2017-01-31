#include "system.h"
#include "timer.h"
#include "pio_check.h"
#include "power_control.h"

//#include "debug.h"

#include "doors_proc.h"

///////////////////////////////////////////////////////////////

#define PIO_CHECK_MS					50
#define DEBOUNCING_CNT					2

#define LOCK_STATE_LOCKED				0x00
#define LOCK_STATE_UNLOCK_LEFT			0x01
#define LOCK_STATE_UNLOCK_RIGHT			0x02

///////////////////////////////////////////////////////////////

static device_t * motor_positive_dev;
static device_t * motor_negative_dev;

static void * door_left_pio;
static void * door_right_pio;
static void * door_left_open_pio;

static uint8_t lock_state;

static bool_t button_state;
static bool_t door_state;

///////////////////////////////////////////////////////////////

void doors_proc_init( void )
{
	door_left_pio = pio_check_event_open_read_port( "/dev/gpio35", NULL, 0, TRUE, 10 );
	door_right_pio = pio_check_event_open_read_port( "/dev/gpio36", NULL, 0, TRUE, 10 );

	door_left_open_pio = pio_check_event_open_read_port( "/dev/gpio31", NULL, 0, TRUE, 50 );

	motor_positive_dev = pio_check_open_output( "/dev/gpio18", FALSE );
	motor_negative_dev = pio_check_open_input( "/dev/gpio19", FALSE );

	pio_check_set_output( motor_positive_dev, FALSE );
	pio_check_set_output( motor_negative_dev, FALSE );

	door_state = FALSE;
	button_state = FALSE;

	lock_state = LOCK_STATE_LOCKED;
}

void doors_proc_process( void )
{
	bool_t read_pio = FALSE;

	if( pio_check_event_read_port( door_left_pio, &read_pio ) )
	{
		DEBUG_PRINTK( "Door left = %d\r\n", read_pio );

		pio_check_set_output( motor_positive_dev, FALSE );
		pio_check_set_output( motor_negative_dev, FALSE );

		button_state = FALSE;

		if( read_pio ) lock_state |= LOCK_STATE_UNLOCK_LEFT;
		else lock_state &= ~LOCK_STATE_UNLOCK_LEFT;

		// if doors are locked, indicate that doors are false and allow powerdown
		if( lock_state == LOCK_STATE_LOCKED )
		{
			power_set_doors_state( FALSE );
			DEBUG_PRINTK( "All doors locked\r\n" );
		}
		else
		{
			power_set_doors_state( TRUE );
			DEBUG_PRINTK( "All doors unlocked\r\n" );
		}
	}

#if 0
	if( pio_check_event_read_port( door_right_pio, &read_pio ) )
	{
		DEBUG_PRINTK( "Door right = %d\r\n", read_pio );

		if( !read_pio ) lock_state |= LOCK_STATE_UNLOCK_RIGHT;
		else lock_state &= ~LOCK_STATE_UNLOCK_RIGHT;

		// if doors are locked, indicate that doors are false and allow powerdown
		if( lock_state == LOCK_STATE_LOCKED ) power_set_doors_state( FALSE );
		else power_set_doors_state( TRUE );
	}
#endif

	if( pio_check_event_read_port( door_left_open_pio, &read_pio ) )
	{
		DEBUG_PRINTK( "Door left open = %d\r\n", read_pio );
		door_state = read_pio;
	}
}
