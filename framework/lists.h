#ifndef _LISTS_LIB_C_
#define _LISTS_LIB_C_

//////////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////////

#define LIST_HEADER            void * p_next

// MUST be the first element of a data structure
typedef struct _list_node_t
{
    struct _list_node_t *next;

} list_node_t;

typedef void (*delete_cb_t)(list_node_t *node);

typedef struct _list_data_t
{
    uint16_t       count;
    list_node_t   *head;
    list_node_t   *tail;
    delete_cb_t    delete_cb;

} list_data_t;

//////////////////////////////////////////////////////////////////////////////////////////

#define list_find_value(list, type, typemember, size, value) (type *)list_find(list, offsetof(type, typemember), (uint16_t) size, value)

// example of search
// #define find_entry(list, value) list_find_value(list, <entry_struct_type>, <struct_member>, sizeof(<member_type>), (value))

/*
 static <entry_struct_type> *create_entry(list_t *list, <entry parameters to find and build>)
 {
 <entry_struct_type> *ret = find_entry(list, <searching parameter>);
 if (ret == NULL)
 {
 ret = (<entry_struct_type> *)mmalloc(sizeof(<entry_struct_type>));
 }

 if (ret != NULL)
 {
 list_insert_tail(list, &ret->node);
 <initialise entry>
 }


 return ret;
 }
 */

//////////////////////////////////////////////////////////////////////////////////////////

void list_init(list_data_t *list, delete_cb_t delete_cb);
void list_clear(list_data_t *list);
void list_insert_head(list_data_t *list, list_node_t *node);
void list_insert_tail(list_data_t *list, list_node_t *node);
void list_insert_after(list_data_t *list, list_node_t *existingnode, list_node_t *node);
void list_remove_tail(list_data_t *list);

bool_t list_unlink(list_data_t *list, list_node_t *node);
bool_t list_remove(list_data_t *list, list_node_t *node);
bool_t list_is_empty(const list_data_t * const list);

list_node_t *list_find(list_data_t *list, size_t offset, uint16_t size, const void *value);

void * list_get_head(const list_data_t * const list);
void * list_get_tail(const list_data_t * const list);
void * list_get_next(const list_data_t * const list, list_node_t * node);
void * list_get_prev(const list_data_t * const list, list_node_t * curr);

uint16_t list_size(const list_data_t * const list);

//////////////////////////////////////////////////////////////////////////////////////////

#endif // _LIST_LIB_C_
