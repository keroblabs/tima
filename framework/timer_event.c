#include "lists.h"
#include "timer.h"
#include "timer_event.h"
#include "runtime.h"

///////////////////////////////////////////////////////////////

typedef struct _timer_event_t_
{
	LIST_HEADER;
	timer_data_t timer;
	bool_t is_terminate;
	ptr_event_t handler;
	void * data;

} timer_event_t;

///////////////////////////////////////////////////////////////

static list_t * timer_event_list;
static uint32_t master_timer_id;
static uint32_t timer_event_pid;
static uint32_t timer_event_cnt;

///////////////////////////////////////////////////////////////

void timer_event_proc( void )
{
	timer_event_t * p_event;

	if( list_is_empty( timer_event_list ) == TRUE )
	{
		runtime_terminate( timer_event_pid );
		timer_event_pid = 0;
		return;
	}

	if( ( p_event = ( timer_event_t * )list_locate_by_index( timer_event_list, timer_event_cnt ) ) == NULL )
	{
		timer_event_cnt = 0;
		return;
	}

	if( p_event->is_terminate == TRUE )
	{
		list_remove_by_pointer( timer_event_list, p_event );
		MFREE( p_event );
	}
	else if( timer_Check( &p_event->timer ) )
	{
		p_event->handler( p_event, p_event->data );
		timer_Reload( &p_event->timer );
	}

	timer_event_cnt++;
}

void timer_event_init( void )
{
	master_timer_id = 0;
	timer_event_list = list_create();
	timer_event_pid = 0;
}

void timer_event_destroy( void )
{
	runtime_terminate( timer_event_pid );
	list_destroy( timer_event_list );
}

uint32_t timer_event_start( uint32_t interval, ptr_event_t handler, void * data )
{
	timer_event_t * timer_event;
	if( timer_event_list == NULL ) return 0;

	timer_event = ( timer_event_t * )MMALLOC( sizeof( timer_event_t ) );
	if( timer_event == NULL ) return 0;

	timer_Start( &timer_event->timer, interval );
	timer_event->is_terminate = FALSE;
	timer_event->handler = handler;
	timer_event->data = data;

	master_timer_id++;
	list_add_with_id( timer_event_list, timer_event, master_timer_id );

	if( timer_event_pid == 0 )
	{
		timer_event_pid = runtime_add( NULL, timer_event_proc );
	}

	return master_timer_id;
}

void timer_event_set_interval( uint32_t id, uint32_t interval )
{
	timer_event_t * timer_event = ( timer_event_t * )list_locate( timer_event_list, id );
	if( timer_event == NULL ) return;

	timer_Start( &timer_event->timer, interval );
	timer_event->is_terminate = FALSE;
}

void timer_event_stop( uint32_t id )
{
	timer_event_t * timer_event = ( timer_event_t * )list_locate( timer_event_list, id );
	if( timer_event == NULL ) return;

	timer_event->is_terminate = TRUE;
}
