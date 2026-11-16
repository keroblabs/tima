#include "pool.h"

void pool_create(pool_data_t * pool_data, uint32_t element_size, void * buffer, uint32_t buffer_size)
{
    uint8_t * temp_buffer = (uint8_t *)buffer;
    uint32_t temp_index = 0;

    pool_data->element_size = element_size;
    pool_data->free_pool = NULL;
    pool_data->last_free = NULL;
    pool_data->active.queue = NULL;

    while (temp_index < buffer_size)
    {
        pool_element_t * element = (pool_element_t *)&temp_buffer[temp_index];

        element->p_next = pool_data->free_pool;
        pool_data->free_pool = element;

        if (pool_data->last_free == NULL)
        {
            pool_data->last_free = element;
        }

        temp_index += (element_size + sizeof(pool_element_t));
    }
}

void * pool_malloc_element(pool_data_t * pool_data)
{
    void * ret = NULL;

    if (pool_data->free_pool != NULL)
    {
        pool_element_t * element = pool_data->free_pool;
        pool_data->free_pool = element->p_next;

        if (pool_data->free_pool == NULL)
        {
            pool_data->last_free = NULL;
        }

        element->p_next = element;
        ret = element->data;
    }

    DO_ASSERT(ret != NULL);

    return ret;
}

void * pool_peek_head(pool_data_t * pool_data, queue_data_t * queue)
{
    queue_data_t * this_queue = queue ? queue : &pool_data->active;
    return this_queue->queue ? this_queue->queue->data : NULL;
}

void * pool_get_from_head(pool_data_t * pool_data, queue_data_t * queue)
{
    void * ret = pool_peek_head(pool_data, queue);

    if (ret != NULL)
    {
        pool_remove_from_queue(pool_data, queue, ret);
    }

    return ret;
}

void pool_add_to_queue(pool_data_t * pool_data, void * p_element, queue_data_t * queue, pool_search_hdl add_hdl)
{
    queue_data_t   * this_queue     = queue ? queue : &pool_data->active;
    pool_element_t * this_element   = (pool_element_t *)((void *)((n_ptr_t)p_element - sizeof(pool_element_t)));
    pool_element_t * search_element = NULL;
    pool_element_t * prev_element   = NULL;

    if (this_element->p_next != this_element)
    {
        pool_remove_from_queue(pool_data, this_queue, p_element);
    }

    this_element->p_next = NULL;

    /* Read queue head AFTER the possible remove above: if p_element was the
       head, this_queue->queue now points at the new head, not at the stale
       just-removed element. Capturing this earlier corrupts insertion. */
    search_element = this_queue->queue;

    while (search_element)
    {
        if (add_hdl && add_hdl(search_element->data, p_element))
        {
            if (prev_element != NULL)
            {
                this_element->p_next = prev_element->p_next;
                prev_element->p_next = this_element;
            }
            else
            {
                this_element->p_next = this_queue->queue;
                this_queue->queue = this_element;
            }

            break;
        }
        prev_element = search_element;
        search_element = search_element->p_next;
    }

    /* Fell off the end without a match -> append at tail (or set as head
       when the queue was empty). search_element is NULL only in this case;
       when the loop broke after insertion it still points at the element
       we inserted before. */
    if (search_element == NULL)
    {
        if (prev_element != NULL)
        {
            prev_element->p_next = this_element;
        }
        else
        {
            this_queue->queue = this_element;
        }
    }
}

void * pool_get_chain_element(pool_data_t * pool_data, queue_data_t * queue, void * p_element)
{
    pool_element_t * this_element = NULL;
    void * ret = NULL;

    if (p_element == NULL)
    {
        ret = pool_peek_head(pool_data, queue);
    }
    else
    {
        this_element = (pool_element_t *)((void *)((n_ptr_t)p_element - sizeof(pool_element_t)));

        if (this_element->p_next != this_element)
        {
            ret = this_element->p_next->data;
        }
    }

    return ret;
}

void * pool_locate_element(pool_data_t * pool_data, queue_data_t * queue, void * p_element, pool_search_hdl search_hdl)
{
    queue_data_t   * this_queue     = queue ? queue : &pool_data->active;
    pool_element_t * search_element = this_queue->queue;

    while (search_element && search_hdl)
    {
        if (search_hdl(search_element->data, p_element))
        {
            return search_element->data;
        }

        search_element = search_element->p_next;
    }

    return NULL;
}

void pool_remove_from_queue(pool_data_t * pool_data, queue_data_t * queue, void * p_element)
{
    queue_data_t   * this_queue     = queue ? queue : &pool_data->active;
    pool_element_t * this_element   = (pool_element_t *)((void *)((n_ptr_t)p_element - sizeof(pool_element_t)));
    pool_element_t * search_element = this_queue->queue;
    pool_element_t * prev_element   = NULL;

    if (this_element->p_next == this_element)
    {
        return;
    }

    while (search_element)
    {
        if (search_element->data == p_element)
        {
            if (prev_element != NULL)
            {
                prev_element->p_next = search_element->p_next;
            }
            else
            {
                this_queue->queue = this_element->p_next;
            }

            this_element->p_next = this_element;

            break;
        }
        prev_element = search_element;
        search_element = search_element->p_next;
    }
}

void pool_free_element(pool_data_t * pool_data, void ** p_element)
{
    pool_element_t * this_element   = (pool_element_t *)((void *)((n_ptr_t)(*p_element) - sizeof(pool_element_t)));
    pool_element_t * prev_element   = NULL;

    DO_ASSERT(this_element->p_next == this_element);

    if (pool_data->last_free != NULL)
    {
        pool_data->last_free->p_next = this_element;
    }
    else
    {
        pool_data->free_pool = this_element;
    }

    pool_data->last_free = this_element;

    *p_element = NULL;
}

