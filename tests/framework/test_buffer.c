#include <assert.h>
#include <stdlib.h>

#include "buffer.h"

typedef void (*heap_handler_t)(uint8_t cause);
typedef struct _heap_t
{
    uint8_t *memory;
    uint32_t size;
    uint32_t used;
    heap_handler_t error_hdl;
} heap_t;

void heap_init(heap_t *heap_data, uint8_t *buffer, uint32_t size);
uint32_t heap_in_use(heap_t *heap_data);
void *heap_malloc(heap_t *heap_data, uint32_t size);
void heap_free(heap_t *heap_data, void *ptr);

static unsigned int critical_depth;
static unsigned int disable_calls;
static unsigned int enable_calls;
static heap_t test_heap;
static union
{
    uint32_t alignment;
    uint8_t data[192 * 1024];
} test_heap_memory;

void test_disable_irq(void)
{
    critical_depth++;
    disable_calls++;
}

void test_enable_irq(void)
{
    assert(critical_depth > 0);
    critical_depth--;
    enable_calls++;
}

void *test_malloc(uint32_t size)
{
    return heap_malloc(&test_heap, size);
}

void test_free(void *ptr)
{
    if(ptr != NULL)
    {
        heap_free(&test_heap, ptr);
    }
}

static void reset_irq_counts(void)
{
    assert(critical_depth == 0);
    disable_calls = 0;
    enable_calls = 0;
}

static void test_circular_buffer_overwrites_oldest_entry(void)
{
    uint8_t storage[3] = { 0, 0, 0 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    reset_irq_counts();

    assert(buffer_write(&buffer, 0x11) == TRUE);
    assert(buffer_write(&buffer, 0x22) == TRUE);
    assert(buffer_write(&buffer, 0x33) == TRUE);
    assert(buffer_write(&buffer, 0x44) == TRUE);

    assert(critical_depth == 0);
    assert(disable_calls == 4);
    assert(enable_calls == 4);
    assert(buffer.pointer_in == 1);
    assert(buffer.pointer_out == 2);
    assert(storage[0] == 0x44);
    assert(storage[1] == 0x22);
    assert(storage[2] == 0x33);

    assert(buffer_get_size(&buffer) == 2);
    assert(buffer_read(&buffer) == 0x33);
    assert(buffer_read(&buffer) == 0x44);
    assert(buffer_is_empty(&buffer) == TRUE);
}

static void test_created_buffer_uses_circular_behavior(void)
{
    buffer_data_t *buffer = buffer_create(3);

    assert(buffer != NULL);
    assert(buffer_write(buffer, 0x11) == TRUE);
    assert(buffer_write(buffer, 0x22) == TRUE);
    assert(buffer_write(buffer, 0x33) == TRUE);
    assert(buffer->pointer_in == 0);
    assert(buffer->pointer_out == 1);
    assert(buffer_read(buffer) == 0x22);
    assert(buffer_read(buffer) == 0x33);
    assert(buffer_is_empty(buffer) == TRUE);

    buffer_destroy(buffer);
    assert(critical_depth == 0);
}

static void test_direct_and_bulk_write_apis_remain_writable(void)
{
    uint8_t storage[6] = { 0, 0, 0, 0, 0, 0 };
    uint8_t source[2] = { 0x51, 0x52 };
    buffer_data_t buffer;
    buffer_data_t input;

    buffer_init(&buffer, storage, sizeof(storage));
    buffer_init(&input, source, sizeof(source));
    input.pointer_in = sizeof(source);

    assert(buffer_write(&buffer, 0x50) == TRUE);
    assert(buffer_write_buffer(&buffer, source, sizeof(source)) == 2);
    assert(buffer_write_buffer_ex(&buffer, &input) == 2);

    assert(buffer.pointer_in == 5);
    assert(buffer.pointer_out == 0);
    assert(storage[0] == 0x50);
    assert(storage[1] == 0x51);
    assert(storage[2] == 0x52);
    assert(storage[3] == 0x51);
    assert(storage[4] == 0x52);
}

static void test_wrapped_writes_preserve_outer_critical_section(void)
{
    uint8_t storage[3] = { 0, 0, 0 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    reset_irq_counts();

    test_disable_irq();
    assert(buffer_write(&buffer, 0x11) == TRUE);
    assert(buffer_write(&buffer, 0x22) == TRUE);
    assert(buffer_write(&buffer, 0x33) == TRUE);
    assert(buffer_write(&buffer, 0x44) == TRUE);

    assert(critical_depth == 1);
    assert(disable_calls == 5);
    assert(enable_calls == 4);
    test_enable_irq();
    assert(critical_depth == 0);
    assert(enable_calls == 5);
    assert(buffer.pointer_in == 1);
    assert(buffer.pointer_out == 2);
}

static void test_resize_preserves_newest_unread_wrapped_bytes(void)
{
    buffer_data_t *buffer = buffer_create(5);

    assert(buffer != NULL);
    assert(buffer_write(buffer, 1) == TRUE);
    assert(buffer_write(buffer, 2) == TRUE);
    assert(buffer_write(buffer, 3) == TRUE);
    assert(buffer_write(buffer, 4) == TRUE);
    assert(buffer_read(buffer) == 1);
    assert(buffer_read(buffer) == 2);
    assert(buffer_write(buffer, 5) == TRUE);
    assert(buffer_write(buffer, 6) == TRUE);
    assert(buffer_get_size(buffer) == 4);

    buffer_resize(buffer, 7);

    assert(buffer->size == 7);
    assert(buffer->pointer_out == 0);
    assert(buffer->pointer_in == 4);
    assert(buffer_get_size(buffer) == 4);

    buffer_resize(buffer, 3);

    assert(buffer->size == 3);
    assert(buffer->pointer_out == 0);
    assert(buffer->pointer_in == 2);
    assert(buffer_get_size(buffer) == 2);
    assert(buffer_read(buffer) == 5);
    assert(buffer_read(buffer) == 6);
    assert(buffer_is_empty(buffer) == TRUE);

    buffer_destroy(buffer);
}

static void test_buffer_sizes_and_bulk_counts_exceed_uint16(void)
{
    const uint32_t count = 65540;
    uint8_t *source = (uint8_t *)malloc(count);
    uint8_t *output = (uint8_t *)malloc(count);
    buffer_data_t *buffer = buffer_create(70000);

    assert(source != NULL);
    assert(output != NULL);
    assert(buffer != NULL);
    for(uint32_t index = 0; index < count; index++)
    {
        source[index] = (uint8_t)index;
    }

    assert(buffer->size == 70000);
    assert(buffer_write_buffer(buffer, source, count) == count);
    assert(buffer->pointer_in == count);
    assert(buffer_get_max_index(buffer) == count);
    assert(buffer_get_size(buffer) == count);

    buffer_resize(buffer, 70010);

    assert(buffer->size == 70010);
    assert(buffer->pointer_in == count);
    assert(buffer_get_size(buffer) == count);
    assert(buffer_read_buffer(buffer, output, count) == count);
    for(uint32_t index = 0; index < count; index++)
    {
        assert(output[index] == source[index]);
    }
    assert(buffer->pointer_out == count);

    buffer_destroy(buffer);
    free(output);
    free(source);
}

static void test_max_size_allocation_fails_without_truncation_or_leak(void)
{
    uint32_t heap_usage_before = heap_in_use(&test_heap);
    buffer_data_t *buffer;
    uint8_t *original_storage;

    assert(buffer_create(UINT32_MAX) == NULL);
    assert(heap_in_use(&test_heap) == heap_usage_before);

    buffer = buffer_create(4);
    assert(buffer != NULL);
    assert(buffer_write(buffer, 0x61) == TRUE);
    assert(buffer_write(buffer, 0x62) == TRUE);
    original_storage = buffer->buffer;
    heap_usage_before = heap_in_use(&test_heap);

    buffer_resize(buffer, UINT32_MAX);

    assert(buffer->buffer == original_storage);
    assert(buffer->size == 4);
    assert(buffer_get_size(buffer) == 2);
    assert(heap_in_use(&test_heap) == heap_usage_before);
    assert(buffer_read(buffer) == 0x61);
    assert(buffer_read(buffer) == 0x62);
    buffer_destroy(buffer);
}

static void test_available_capacity_matches_overwrite_ring(void)
{
    uint8_t storage[4] = { 0, 0, 0, 0 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));

    assert(buffer_get_size(&buffer) == 0);
    assert(buffer_get_available_size(&buffer) == 3);
    assert(buffer_is_full(&buffer) == FALSE);

    assert(buffer_write(&buffer, 0x11) == TRUE);
    assert(buffer_get_size(&buffer) == 1);
    assert(buffer_get_available_size(&buffer) == 2);
    assert(buffer_is_full(&buffer) == FALSE);

    assert(buffer_write(&buffer, 0x22) == TRUE);
    assert(buffer_get_size(&buffer) == 2);
    assert(buffer_get_available_size(&buffer) == 1);
    assert(buffer_is_full(&buffer) == FALSE);

    assert(buffer_write(&buffer, 0x33) == TRUE);
    assert(buffer_get_size(&buffer) == 3);
    assert(buffer_get_available_size(&buffer) == 0);
    assert(buffer_is_full(&buffer) == TRUE);

    assert(buffer_write(&buffer, 0x44) == TRUE);
    assert(buffer_get_size(&buffer) == 3);
    assert(buffer_get_available_size(&buffer) == 0);
    assert(buffer_is_full(&buffer) == TRUE);
    assert(buffer_read(&buffer) == 0x22);
    assert(buffer_read(&buffer) == 0x33);
    assert(buffer_read(&buffer) == 0x44);
}

static void test_small_ring_has_one_byte_usable_capacity(void)
{
    uint8_t storage[2] = { 0, 0 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    assert(buffer_get_available_size(&buffer) == 1);
    assert(buffer_is_full(&buffer) == FALSE);

    assert(buffer_write(&buffer, 0x51) == TRUE);
    assert(buffer_get_available_size(&buffer) == 0);
    assert(buffer_is_full(&buffer) == TRUE);
    assert(buffer_write(&buffer, 0x52) == TRUE);
    assert(buffer_get_size(&buffer) == 1);
    assert(buffer_read(&buffer) == 0x52);
    assert(buffer_get_available_size(&buffer) == 1);
    assert(buffer_is_full(&buffer) == FALSE);
}

static void test_invalid_initialization_leaves_an_inert_buffer(void)
{
    uint8_t storage[4] = { 0, 0, 0, 0 };
    uint8_t output[2] = { 0, 0 };
    buffer_data_t buffer;

    buffer_init(NULL, storage, sizeof(storage));

    buffer_init(&buffer, NULL, sizeof(storage));
    assert(buffer.buffer == NULL);
    assert(buffer.size == 0);
    assert(buffer.pointer_in == 0);
    assert(buffer.pointer_out == 0);
    assert(buffer_read(&buffer) == 0);
    assert(buffer_write(&buffer, 0x71) == FALSE);
    assert(buffer_write_buffer(&buffer, storage, 1) == 0);
    assert(buffer_read_buffer(&buffer, output, sizeof(output)) == 0);
    assert(buffer_get_size(&buffer) == 0);
    assert(buffer_get_available_size(&buffer) == 0);
    assert(buffer_get_current_index(&buffer) == 0);
    assert(buffer_get_max_index(&buffer) == 0);
    assert(buffer_get_pointer(&buffer) == NULL);
    assert(buffer_is_empty(&buffer) == TRUE);
    assert(buffer_is_full(&buffer) == FALSE);

    buffer_init(&buffer, storage, 0);
    assert(buffer.buffer == NULL);
    assert(buffer.size == 0);
    assert(buffer_write(&buffer, 0x72) == FALSE);

    buffer_init(&buffer, storage, 1);
    assert(buffer.buffer == NULL);
    assert(buffer.size == 0);
    assert(buffer_write(&buffer, 0x73) == FALSE);
    buffer_destroy(&buffer);
}

static void test_init_initializes_every_remaining_member(void)
{
    uint8_t storage[3] = { 0, 0, 0 };
    buffer_data_t buffer;

    memset(&buffer, 0xA5, sizeof(buffer));
    buffer_init(&buffer, storage, sizeof(storage));

    assert(buffer.buffer == storage);
    assert(buffer.size == sizeof(storage));
    assert(buffer.pointer_in == 0);
    assert(buffer.pointer_out == 0);
    assert(buffer.mask == 0);
    assert(buffer.user_count == 1);
}

static void test_invalid_sizes_and_null_arguments_are_rejected(void)
{
    uint8_t storage[4] = { 0, 0, 0, 0 };
    uint8_t output[2] = { 0, 0 };
    buffer_data_t buffer;
    buffer_data_t *minimum_buffer;

    assert(buffer_create(0) == NULL);
    assert(buffer_create(1) == NULL);
    assert(buffer_create(UINT32_MAX) == NULL);

    minimum_buffer = buffer_create(2);
    assert(minimum_buffer != NULL);
    assert(minimum_buffer->size == 2);
    assert(buffer_write(minimum_buffer, 0x75) == TRUE);
    buffer_resize(minimum_buffer, 1);
    assert(minimum_buffer->size == 2);
    assert(buffer_read(minimum_buffer) == 0x75);
    buffer_destroy(minimum_buffer);

    buffer_init(&buffer, storage, sizeof(storage));
    buffer_reset(NULL);
    buffer_set_buffer(NULL, storage);
    buffer_set_buffer(&buffer, NULL);
    assert(buffer.buffer == storage);
    buffer_reset_read(NULL);
    buffer_read_set_offset(NULL, 0);
    assert(buffer_get_current_index(NULL) == 0);
    assert(buffer_get_max_index(NULL) == 0);
    buffer_resize(NULL, 4);
    buffer_add_user(NULL);
    buffer_destroy(NULL);
    assert(buffer_get_pointer_at(NULL, 0) == NULL);
    assert(buffer_get_pointer(NULL) == NULL);
    assert(buffer_read(NULL) == 0);
    assert(buffer_write(NULL, 0x74) == FALSE);
    assert(buffer_write_buffer(NULL, storage, 1) == 0);
    assert(buffer_write_buffer(&buffer, NULL, 1) == 0);
    assert(buffer_write_buffer(&buffer, NULL, 0) == 0);
    assert(buffer_write_buffer_ex(NULL, &buffer) == 0);
    assert(buffer_write_buffer_ex(&buffer, NULL) == 0);
    assert(buffer_read_buffer(NULL, output, 1) == 0);
    assert(buffer_read_buffer(&buffer, NULL, 1) == 0);
    assert(buffer_read_buffer(&buffer, NULL, 0) == 0);
    assert(buffer_get_size(NULL) == 0);
    assert(buffer_get_available_size(NULL) == 0);
    assert(buffer_is_full(NULL) == FALSE);
    assert(buffer_is_empty(NULL) == TRUE);
}

int main(void)
{
    heap_init(&test_heap, test_heap_memory.data, sizeof(test_heap_memory.data));
    test_circular_buffer_overwrites_oldest_entry();
    test_created_buffer_uses_circular_behavior();
    test_direct_and_bulk_write_apis_remain_writable();
    test_wrapped_writes_preserve_outer_critical_section();
    test_resize_preserves_newest_unread_wrapped_bytes();
    test_buffer_sizes_and_bulk_counts_exceed_uint16();
    test_max_size_allocation_fails_without_truncation_or_leak();
    test_available_capacity_matches_overwrite_ring();
    test_small_ring_has_one_byte_usable_capacity();
    test_invalid_initialization_leaves_an_inert_buffer();
    test_invalid_sizes_and_null_arguments_are_rejected();
    test_init_initializes_every_remaining_member();
    return 0;
}
