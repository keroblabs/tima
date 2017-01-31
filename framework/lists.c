#include "lists.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

list_t * list_create( void )
{
	list_t * ret = ( list_t * )MMALLOC( sizeof( list_t ) );
	return ret;
}

void list_destroy( list_t * list )
{
	MFREE( list );
}

void list_add( list_t * list, void * p_item )
{
	list_add_with_id( list, p_item, 0 );
}

void list_add_with_id( list_t * list, void * p_item, uint32_t id )
{
	element_t * p_element = ( element_t * )p_item;
	p_element->id = id;

	if( list->p_first == NULL )
	{
		list->p_first = p_element;
	}
	else
	{
		list->p_last->p_next = p_element;
	}

	list->p_last = p_element;
}

void * list_locate( list_t * list, uint32_t id )
{
	element_t * p_search = list->p_first;

	while( p_search != NULL )
	{
		if( p_search->id == id ) return p_search;
		p_search = (element_t *)p_search->p_next;
	}

	return NULL;
}

void * list_locate_by_index( list_t * list, uint32_t index )
{
	element_t * p_search = list->p_first;
	uint32_t cnt = 0;

	if( index == 0 ) return list->p_first;

	while( p_search != NULL )
	{
		if( cnt == index ) return p_search;
		p_search = (element_t *)p_search->p_next;
		cnt++;
	}

	return NULL;
}

void list_remove_by_pointer( list_t * list, void * p_item )
{
	element_t * p_search = list->p_first;
	element_t * p_prev = NULL;

	while( p_search != NULL )
	{
		if( p_search == p_item )
		{
			if( p_prev != NULL )
			{
				p_prev->p_next = p_search->p_next;
				if( p_prev->p_next == NULL ) list->p_last = p_prev;
			}
			else
			{
				list->p_first = (element_t *)p_search->p_next;
			}

			if( list->p_first == NULL ) list->p_last = NULL;

			break;
		}

		p_prev = p_search;
		p_search = (element_t *)p_search->p_next;
	}
}

void list_remove_by_id( list_t * list, uint32_t id )
{
	element_t * p_search = list->p_first;
	element_t * p_prev = NULL;

	while( p_search != NULL )
	{
		if( p_search->id == id )
		{
			if( p_prev != NULL )
			{
				p_prev->p_next = p_search->p_next;
				if( p_prev->p_next == NULL ) list->p_last = p_prev;
			}
			else
			{
				list->p_first = (element_t *)p_search->p_next;
			}

			if( list->p_first == NULL ) list->p_last = NULL;

			break;
		}

		p_prev = p_search;
		p_search = (element_t *)p_search->p_next;
	}
}

bool_t list_is_empty( list_t * list )
{
	if( list->p_first == NULL ) return TRUE;
	return FALSE;
}

