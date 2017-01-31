#ifndef __lists_h__
#define __lists_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

#define LIST_HEADER			void * p_next;	uint32_t id; uint32_t cnt

///////////////////////////////////////////////////////////////

typedef struct _element_t_
{
	LIST_HEADER;

} element_t;

typedef struct _list_t_
{
	element_t * p_first;
	element_t * p_last;

} list_t;

///////////////////////////////////////////////////////////////

list_t * list_create( void );
void list_destroy( list_t * list );

void list_add( list_t * list, void * p_item );
void list_add_with_id( list_t * list, void * p_item, uint32_t id );

void * list_locate( list_t * list, uint32_t id );
void * list_locate_by_index( list_t * list, uint32_t index );

void list_remove_by_pointer( list_t * list, void * p_item );
void list_remove_by_id( list_t * list, uint32_t id );

bool_t list_is_empty( list_t * list );

///////////////////////////////////////////////////////////////

#endif // __lists_h__
