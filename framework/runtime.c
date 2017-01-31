#include "system.h"
#include "runtime.h"
#include "lists.h"

///////////////////////////////////////////////////////////////

#define RUNTIME_READY			0x0000
#define RUNTIME_RUNNING			0x0001
#define RUNTIME_PAUSE			0x0002
#define RUNTIME_ONE				0x0004
#define RUNTIME_TERMINATE		0x0008

///////////////////////////////////////////////////////////////

typedef struct _runtime_t
{
	LIST_HEADER;

	uint32_t mask;

	ptr_module_t init;
	ptr_module_t proc;

} runtime_t;

///////////////////////////////////////////////////////////////

static uint32_t master_pid;
static uint32_t runtime_cnt;

static list_t * runtime_list;

///////////////////////////////////////////////////////////////

void runtime_init( void )
{
	master_pid = 0;
	runtime_list = list_create();
	runtime_cnt = 0;
}

uint32_t runtime_add( void * init, void * proc )
{
	runtime_t * p_run;

	if( runtime_list == NULL ) return 0;

	p_run = ( runtime_t * )MMALLOC( sizeof( runtime_t ) );
	if( p_run == NULL ) return 0;

	memset( p_run, 0x00, sizeof( runtime_t ) );
	master_pid++;

	p_run->init = ( ptr_module_t )init;
	p_run->proc = ( ptr_module_t )proc;
	p_run->mask = RUNTIME_READY;

	list_add_with_id( runtime_list, p_run, master_pid );

	return master_pid;
}

void runtime_pause( uint32_t pid )
{
	runtime_t * p_run;

	p_run = ( runtime_t * )list_locate( runtime_list, pid );
	if( p_run == NULL ) return;
	p_run->mask |= RUNTIME_PAUSE;
}

void runtime_resume( uint32_t pid )
{
	runtime_t * p_run;

	p_run = ( runtime_t * )list_locate( runtime_list, pid );
	if( p_run == NULL ) return;
	p_run->mask &= ~RUNTIME_PAUSE;
}

void runtime_terminate( uint32_t pid )
{
	runtime_t * p_run;

	p_run = ( runtime_t * )list_locate( runtime_list, pid );
	if( p_run == NULL ) return;
	p_run->mask |= RUNTIME_TERMINATE;
}

void runtime_proc( void )
{
	runtime_t * p_run;

	if( list_is_empty( runtime_list ) ) return;

	// get a runtime process
	p_run = ( runtime_t * )list_locate_by_index( runtime_list, runtime_cnt++ );
	if( p_run == NULL )
	{
		runtime_cnt = 0;
	}

	// if it's for termination, remove from list
	else if( p_run->mask & RUNTIME_TERMINATE )
	{
		list_remove_by_pointer( runtime_list, p_run );
		MFREE( p_run );
	}
	
	// if it's paused, do nothing
	else if( p_run->mask & RUNTIME_PAUSE )
	{
	}

	// if it's not running, call init
	else if( !( p_run->mask & RUNTIME_RUNNING ) )
	{
		if( p_run->init != NULL ) p_run->init();
		p_run->mask |= RUNTIME_RUNNING;
	}

	// it's good to execute
	else
	{
		p_run->proc();
	}
}

///////////////////////////////////////////////////////////////

#if 0 // def WANT_RUNTIME_INTERFACE

DECLARE_INIT_SECTION( runtime_init );
DECLARE_PROCESS_SECTION( runtime_proc );

#endif
