#include "system.h"
#include "timer.h"

#include "pio_check.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

typedef struct _gpio_event_t_
{
	bool_t pio_states[3];
	uint16_t pio_id;
	bool_t is_invert;
	timer_data_t interval;
	uint8_t pio_flag;
	device_t dev;

	void (*event_handler)( void * event, uint16_t id );

	struct _gpio_event_t_ * p_next;

} gpio_event_t;

///////////////////////////////////////////////////////////////

static uint16_t pio_state;
//static gpio_event_t * gpio_event;
//static gpio_event_t * gpio_event_last;

///////////////////////////////////////////////////////////////

bool_t pio_check_event_read_port( void * p_data, bool_t * state )
{
	gpio_event_t * port_data = ( gpio_event_t * )p_data;
	bool_t ret = FALSE;

	if( timer_Check( &port_data->interval ) == FALSE ) return FALSE;
	timer_Reload( &port_data->interval );

	port_data->pio_states[1] = port_data->pio_states[2];
	port_data->pio_states[2] = pio_check_read_input( &port_data->dev, port_data->is_invert );

	if( ( port_data->pio_states[1] == port_data->pio_states[2] ) && ( port_data->pio_states[0] != port_data->pio_states[1] ) )
	{
		port_data->pio_states[0] = port_data->pio_states[1];
		if( ( port_data->pio_states[2] ) && ( port_data->pio_flag == FALSE ) )
		{
			port_data->pio_flag = TRUE;
			ret = TRUE;
		}
		else if( ( !port_data->pio_states[2] ) && ( port_data->pio_flag == TRUE ) )
		{
			port_data->pio_flag = FALSE;
			ret = TRUE;
		}

		//DEBUG_PRINTK( "Door left = %d\r\n", pio_door_left[2] );
	}

	if( ( ret == TRUE ) && ( state != NULL ) )
	{
		*state = port_data->pio_flag;
	}

	return ret;
}

void * pio_check_event_open_read_port( const char * dev_name, void * event_handler, uint16_t id, bool_t is_invert, uint32_t interval )
{
	gpio_event_t * port_data;

	port_data = ( gpio_event_t * )MMALLOC( sizeof( gpio_event_t ) );
	if( port_data == NULL ) return NULL;

	device_open_ex( dev_name, &port_data->dev );
	if( device_open_ex( dev_name, &port_data->dev ) != FALSE )
	{
		DEBUG_PRINTK( "%s OK\r\n", dev_name );

		if( port_data != NULL )
		{
			port_data->event_handler = event_handler;
			port_data->pio_id = id;
			port_data->is_invert = is_invert;
			memset( port_data->pio_states, 0x00, sizeof( port_data->pio_states ) );

			port_data->pio_flag = pio_check_read_input( &port_data->dev, port_data->is_invert ) ? FALSE : TRUE;
			port_data->pio_states[0] = port_data->pio_states[1] = port_data->pio_states[2] = port_data->pio_flag;

			timer_Start( &port_data->interval, interval );
		}
	}
	else
	{
		MFREE( port_data );
	}

	return port_data;
}

void * pio_check_event_open_output( const char * dev_name, bool_t is_invert, bool_t initial )
{
    gpio_event_t * port_data;

    port_data = ( gpio_event_t * )MMALLOC( sizeof( gpio_event_t ) );
    if( port_data == NULL ) return NULL;

    device_open_ex( dev_name, &port_data->dev );
    if( device_open_ex( dev_name, &port_data->dev ) != FALSE )
    {
        //DEBUG_PRINTK( "%s OK\r\n", dev_name );

        if( port_data != NULL )
        {
            port_data->is_invert = is_invert;
            pio_check_event_set_output( port_data, initial );
        }
    }
    else
    {
        MFREE( port_data );
    }

    return port_data;
}

void pio_check_event_set_output( void * p_event, bool_t state )
{
    gpio_event_t * event = ( gpio_event_t * )p_event;
    if( event == NULL ) return;

    pio_check_set_output( &event->dev, state ? ( event->is_invert ? FALSE : TRUE ) : ( event->is_invert ? TRUE : FALSE ) );
}

bool_t pio_check_event_get_state( void * p_event )
{
	gpio_event_t * event = ( gpio_event_t * )p_event;
	return pio_check_read_input( &event->dev, event->is_invert );
}


#if 0
static void pio_check_read_input_event( gpio_event_t * event )
{
	if( !timer_Check( &event->interval ) ) return;

	timer_Reload( &event->interval );

	event->pio_states[1] = event->pio_states[2];
	event->pio_states[2] = pio_check_read_input( event->dev, event->is_invert );

	if( ( event->pio_states[1] == event->pio_states[2] ) && ( event->pio_states[0] != event->pio_states[1] ) )
	{
		event->pio_states[0] = event->pio_states[1];
		if( event->event_handler ) event->event_handler( event, event->pio_id );
	}
}


void * pio_check_open_input_event( const char * dev_name, void * event_handler, uint16_t id, bool_t is_invert, uint32_t interval )
{
	gpio_event_t * event;

	event = ( gpio_event_t * )MMALLOC( sizeof( event ) );
	if( event == NULL ) return NULL;

	event->dev = device_open( dev_name );
	if( event->dev != NULL )
	{
		DEBUG_PRINTK( "%s OK\r\n", dev_name );

		if( event != NULL )
		{
			event->event_handler = event_handler;
			event->pio_id = id;
			event->is_invert = is_invert;
			memset( event->pio_states, 0x00, sizeof( event->pio_states ) );

			timer_Start( &event->interval, interval );
		}

		if( gpio_event == NULL )
		{
			gpio_event = event;
		}
		else
		{
			gpio_event->p_next = event;
		}

		gpio_event_last = event;
	}
	else
	{
		MFREE( event );
	}

	return event;
}
#endif

void pio_check_process( void )
{
#if 0
	gpio_event_t * curr_event = gpio_event;

	while( gpio_event != NULL )
	{
		pio_check_read_input_event( gpio_event );
		gpio_event = gpio_event->p_next;
	}
#endif
}

device_t * pio_check_open_output( const char * dev_name, uint8_t state )
{
	device_t * dev_ret;

	dev_ret = device_open( dev_name );
	if( dev_ret != NULL )
	{
		DEBUG_PRINTK( "%s OK\r\n", dev_name );
		device_write_buffer( dev_ret, &state, sizeof( state ) );
	}

	return dev_ret;
}

device_t * pio_check_open_input( const char * dev_name, uint8_t * pio_var )
{
	device_t * dev_ret;

	dev_ret = device_open( dev_name );
	if( dev_ret != NULL )
	{
		//DEBUG_PRINTK( "%s OK\r\n", dev_name );

		if( pio_var != NULL )
			device_read_buffer( dev_ret, pio_var, sizeof( uint8_t ) );
	}

	return dev_ret;
}

uint8_t pio_check_read_input( device_t * p_dev, bool_t invert )
{
	uint8_t pio_var = FALSE;

	if( p_dev != NULL )
	{
		device_read_buffer( p_dev, &pio_var, sizeof( uint8_t ) );
		if( invert ) pio_var = pio_var ? FALSE : TRUE;
	}

	return pio_var;
}

void pio_check_set_output( device_t * p_dev, uint8_t state )
{
	if( p_dev != NULL )
	{
		device_write_buffer( p_dev, &state, sizeof( state ) );
	}
}

void pio_check_init( void )
{
	pio_state = 0;
	//gpio_event = NULL;
}

void pio_check_set_bit( uint16_t bit, bool_t state )
{
	if( state ) pio_state |= ( 1 << bit );
	else pio_state &= ~( 1 << bit );
}

uint16_t pio_check_read( void )
{
	return pio_state;
}

