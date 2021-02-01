#include "ignition_ctrl.h"
#include "system.h"
#include "timer.h"
#include "pio_check.h"
#include "security_client.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////

#define START_DELAY_MS          350
#define SAFE_TIME_MS            500

////////////////////////////////////////////////////////////////////

enum
{
    IgnitionMode_Idle,
    IgnitionMode_Acc,
    IgnitionMode_On,
    IgnitionMode_Start,
    IgnitionMode_Done,
};

////////////////////////////////////////////////////////////////////

static uint8_t ignition_mode;
static void * pio_start_button;
static void * pio_start_relay;
static void * pio_ignition_relay;
static void * pio_acc_relay;
static timer_data_t start_timer;
static timer_data_t safe_start_timer;

////////////////////////////////////////////////////////////////////

static void ignition_control_process( void )
{
    uint8_t button_start;

    if( timer_Check( &safe_start_timer ) )
    {
        if( pio_check_event_read_port( pio_start_button, &button_start ) )
        {
            if( button_start )
            {
                if( ignition_mode == IgnitionMode_Acc )
                {
                    ignition_mode = IgnitionMode_On;
                    pio_check_event_set_output( pio_ignition_relay, TRUE );
                    timer_Start( &start_timer, START_DELAY_MS );
                }
                else if( ignition_mode == IgnitionMode_On )
                {
                    ignition_mode = IgnitionMode_Acc;
                    pio_check_event_set_output( pio_ignition_relay, FALSE );
                }
                else if( ignition_mode == IgnitionMode_Done )
                {
                    ignition_mode = IgnitionMode_Acc;
                    pio_check_event_set_output( pio_ignition_relay, FALSE );
                    timer_Start( &start_timer, START_DELAY_MS );
                }
            }
            else
            {
                timer_Stop( &start_timer );
                timer_Expire( &safe_start_timer );

                if( ignition_mode == IgnitionMode_Start )
                {
                    pio_check_event_set_output( pio_start_relay, FALSE );
                    pio_check_event_set_output( pio_ignition_relay, TRUE );
                    pio_check_event_set_output( pio_acc_relay, TRUE );
                    ignition_mode = IgnitionMode_Done;
                }
                else if( ignition_mode == IgnitionMode_Acc )
                {
                }
            }
        }
        else if( timer_Check( &start_timer ) )
        {
            if( ignition_mode == IgnitionMode_On )
            {
                timer_Stop( &start_timer );
                timer_Start( &safe_start_timer, SAFE_TIME_MS );

                ignition_mode = IgnitionMode_Start;
                pio_check_event_set_output( pio_start_relay, TRUE );
                pio_check_event_set_output( pio_acc_relay, FALSE );
            }
            else if( ignition_mode == IgnitionMode_Acc )
            {
                ignition_mode = IgnitionMode_On;
                pio_check_event_set_output( pio_ignition_relay, TRUE );
                timer_Start( &start_timer, START_DELAY_MS );
            }
        }
    }
}

void ignition_key_state( bool_t state )
{
    if( ( ignition_mode == IgnitionMode_Idle ) && ( state == TRUE ) )
    {
        ignition_mode = IgnitionMode_Acc;
        pio_check_event_set_output( pio_acc_relay, TRUE );
    }
    else if( ( ignition_mode == IgnitionMode_Acc ) && ( state == FALSE ) )
    {
        ignition_mode = IgnitionMode_Idle;
        pio_check_event_set_output( pio_acc_relay, FALSE );
    }
}

bool_t ignition_remove_key_allowed( void )
{
    if( ignition_mode >= IgnitionMode_On ) return FALSE;
    return TRUE;
}

bool_t ignition_idle_state( void )
{
    if( ignition_mode == IgnitionMode_Idle ) return TRUE;
    return FALSE;
}

void ignition_control_init( void )
{
    pio_start_button = pio_check_event_open_read_port( "/dev/gpio3", NULL, 0, TRUE, 50 );

    pio_acc_relay      = pio_check_event_open_output( "/dev/gpio1", TRUE, FALSE );
    pio_ignition_relay = pio_check_event_open_output( "/dev/gpio2", TRUE, FALSE );
    pio_start_relay    = pio_check_event_open_output( "/dev/gpio5", TRUE, FALSE );

    timer_Stop( &start_timer );
    timer_Expire( &safe_start_timer );

    ignition_mode = IgnitionMode_Idle;
}

DECLARE_PROCESS_SECTION(ignition_control_process);
