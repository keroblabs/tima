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
static bool_t consume_on_next_irq_disable;
static buffer_data_t *interleaved_read_buffer;
static uint8_t interleaved_read_value;
static heap_t test_heap;
static union
{
    uint32_t alignment;
    uint8_t data[192 * 1024];
} test_heap_memory;

void test_disable_irq(void)
{
    if(consume_on_next_irq_disable)
    {
        consume_on_next_irq_disable = FALSE;
        assert(interleaved_read_buffer != NULL);
        interleaved_read_value = buffer_read(interleaved_read_buffer);
    }

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
    assert(critical_depth == 0);
    return heap_malloc(&test_heap, size);
}

void test_free(void *ptr)
{
    if(ptr != NULL)
    {
        assert(critical_depth == 0);
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

static void test_try_read_distinguishes_empty_from_zero_byte(void)
{
    uint8_t storage[3] = { 0, 0, 0 };
    buffer_data_t buffer;
    uint8_t value = 0xA5;

    buffer_init(&buffer, storage, sizeof(storage));

    assert(buffer_try_read(NULL, &value) == FALSE);
    assert(value == 0xA5);
    assert(buffer_try_read(&buffer, NULL) == FALSE);
    assert(buffer_try_read(&buffer, &value) == FALSE);
    assert(value == 0xA5);

    assert(buffer_write(&buffer, 0x00) == TRUE);
    assert(buffer_try_read(&buffer, &value) == TRUE);
    assert(value == 0x00);
    assert(buffer_is_empty(&buffer) == TRUE);

    assert(buffer_write(&buffer, 0x00) == TRUE);
    assert(buffer_read(&buffer) == 0x00);
    assert(buffer_is_empty(&buffer) == TRUE);
}

static void test_read_offset_setter_is_protected_and_clamped(void)
{
    uint8_t storage[5] = { 0, 0, 0, 0, 0 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    assert(buffer_write(&buffer, 0x11) == TRUE);
    assert(buffer_write(&buffer, 0x22) == TRUE);
    assert(buffer_write(&buffer, 0x33) == TRUE);

    reset_irq_counts();
    test_disable_irq();
    buffer_read_set_offset(&buffer, 2);
    assert(critical_depth == 1);
    assert(disable_calls == 2);
    assert(enable_calls == 1);
    test_enable_irq();
    assert(critical_depth == 0);
    assert(enable_calls == 2);
    assert(buffer.pointer_out == 2);

    reset_irq_counts();
    buffer_read_set_offset(&buffer, UINT32_MAX);
    assert(buffer.pointer_out == buffer.pointer_in);
    assert(critical_depth == 0);
    assert(disable_calls == 1);
    assert(enable_calls == 1);
}

static void test_buffer_user_count_rejects_overflow(void)
{
    uint8_t storage[2] = { 0, 0 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    assert(buffer.user_count == 1);

    reset_irq_counts();
    assert(buffer_add_user(&buffer) == TRUE);
    assert(buffer.user_count == 2);
    assert(critical_depth == 0);
    assert(disable_calls == 1);
    assert(enable_calls == 1);

    buffer.user_count = UINT8_MAX;
    reset_irq_counts();
    assert(buffer_add_user(&buffer) == FALSE);
    assert(buffer.user_count == UINT8_MAX);
    assert(critical_depth == 0);
    assert(disable_calls == 1);
    assert(enable_calls == 1);
    assert(buffer_add_user(NULL) == FALSE);
}

static void test_buffer_user_references_release_in_order(void)
{
    uint32_t heap_usage_before = heap_in_use(&test_heap);
    buffer_data_t *buffer = buffer_create(4);

    assert(buffer != NULL);
    assert(buffer_add_user(buffer) == TRUE);
    assert(buffer->user_count == 2);

    buffer_destroy(buffer);
    assert(buffer->user_count == 1);
    assert(heap_in_use(&test_heap) > heap_usage_before);

    buffer_destroy(buffer);
    assert(heap_in_use(&test_heap) == heap_usage_before);
}

static void test_bulk_read_returns_partial_count_and_preserves_zero_bytes(void)
{
    uint8_t storage[4] = { 0, 0, 0, 0 };
    uint8_t input[2] = { 0x00, 0x22 };
    uint8_t output[4] = { 0xA5, 0xA5, 0xA5, 0xA5 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    assert(buffer_write_buffer(&buffer, input, sizeof(input)) == sizeof(input));

    reset_irq_counts();
    assert(buffer_read_buffer(&buffer, output, sizeof(output)) == sizeof(input));
    assert(output[0] == 0x00);
    assert(output[1] == 0x22);
    assert(output[2] == 0xA5);
    assert(output[3] == 0xA5);
    assert(critical_depth == 0);
    assert(disable_calls == 3);
    assert(enable_calls == 3);
    assert(buffer_is_empty(&buffer) == TRUE);
}

static void test_bulk_read_handles_an_interleaved_consumer(void)
{
    uint8_t storage[3] = { 0, 0, 0 };
    uint8_t output = 0xA5;
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    assert(buffer_write(&buffer, 0x31) == TRUE);
    reset_irq_counts();

    interleaved_read_buffer = &buffer;
    interleaved_read_value = 0xFF;
    consume_on_next_irq_disable = TRUE;

    assert(buffer_read_buffer(&buffer, &output, 1) == 0);
    assert(consume_on_next_irq_disable == FALSE);
    assert(interleaved_read_value == 0x31);
    assert(output == 0xA5);
    assert(critical_depth == 0);
    assert(disable_calls == 2);
    assert(enable_calls == 2);
    assert(buffer_is_empty(&buffer) == TRUE);

    interleaved_read_buffer = NULL;
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
    uint8_t source[3] = { 0x51, 0x52, 0x00 };
    const uint32_t source_count = 2;
    buffer_data_t buffer;
    buffer_data_t input;

    buffer_init(&buffer, storage, sizeof(storage));
    buffer_init(&input, source, sizeof(source));
    input.pointer_in = source_count;

    assert(buffer_write(&buffer, 0x50) == TRUE);
    assert(buffer_write_buffer(&buffer, source, source_count) == source_count);
    assert(buffer_write_buffer_ex(&buffer, &input) == source_count);

    assert(buffer.pointer_in == 5);
    assert(buffer.pointer_out == 0);
    assert(storage[0] == 0x50);
    assert(storage[1] == 0x51);
    assert(storage[2] == 0x52);
    assert(storage[3] == 0x51);
    assert(storage[4] == 0x52);
}

static void test_buffer_copy_uses_unread_wrapped_bytes_without_consuming_source(void)
{
    uint8_t source_storage[5] = { 0, 0, 0, 0, 0 };
    uint8_t destination_storage[6] = { 0, 0, 0, 0, 0, 0 };
    uint8_t initial_data[4] = { 1, 2, 3, 4 };
    uint8_t wrapped_data[3] = { 5, 6, 7 };
    uint8_t expected[4] = { 4, 5, 6, 7 };
    uint8_t destination_output[4] = { 0, 0, 0, 0 };
    uint8_t source_output[4] = { 0, 0, 0, 0 };
    buffer_data_t source;
    buffer_data_t destination;

    buffer_init(&source, source_storage, sizeof(source_storage));
    buffer_init(&destination, destination_storage, sizeof(destination_storage));

    assert(buffer_write_buffer(&source, initial_data, sizeof(initial_data)) == sizeof(initial_data));
    assert(buffer_read(&source) == 1);
    assert(buffer_read(&source) == 2);
    assert(buffer_read(&source) == 3);
    assert(buffer_write_buffer(&source, wrapped_data, sizeof(wrapped_data)) == sizeof(wrapped_data));
    assert(buffer_get_size(&source) == sizeof(expected));

    assert(buffer_write_buffer_ex(&destination, &source) == sizeof(expected));
    assert(buffer_read_buffer(&destination, destination_output, sizeof(destination_output)) == sizeof(expected));
    assert(memcmp(destination_output, expected, sizeof(expected)) == 0);

    assert(buffer_get_size(&source) == sizeof(expected));
    assert(buffer_read_buffer(&source, source_output, sizeof(source_output)) == sizeof(expected));
    assert(memcmp(source_output, expected, sizeof(expected)) == 0);
}

static void test_buffer_copy_handles_empty_and_self_source(void)
{
    uint8_t storage[4] = { 0, 0, 0, 0 };
    buffer_data_t source;
    buffer_data_t destination;

    buffer_init(&source, storage, sizeof(storage));
    buffer_init(&destination, storage, sizeof(storage));
    assert(buffer_write_buffer_ex(&destination, &source) == 0);

    assert(buffer_write(&source, 0x41) == TRUE);
    assert(buffer_write_buffer_ex(&destination, &source) == 0);
    assert(buffer_write_buffer_ex(&source, &source) == 0);
    assert(buffer_get_size(&source) == 1);
    assert(buffer_read(&source) == 0x41);
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

    reset_irq_counts();
    buffer_resize(buffer, 7);
    assert(critical_depth == 0);
    assert(disable_calls == 2);
    assert(enable_calls == 2);

    assert(buffer->size == 7);
    assert(buffer->pointer_out == 0);
    assert(buffer->pointer_in == 4);
    assert(buffer_get_size(buffer) == 4);

    reset_irq_counts();
    buffer_resize(buffer, 3);
    assert(critical_depth == 0);
    assert(disable_calls == 2);
    assert(enable_calls == 2);

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
    assert(buffer.buffer == storage);
    buffer_reset_read(NULL);
    buffer_read_set_offset(NULL, 0);
    assert(buffer_get_current_index(NULL) == 0);
    assert(buffer_get_max_index(NULL) == 0);
    buffer_resize(NULL, 4);
    assert(buffer_add_user(NULL) == FALSE);
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
    test_try_read_distinguishes_empty_from_zero_byte();
    test_read_offset_setter_is_protected_and_clamped();
    test_buffer_user_count_rejects_overflow();
    test_buffer_user_references_release_in_order();
    test_bulk_read_returns_partial_count_and_preserves_zero_bytes();
    test_bulk_read_handles_an_interleaved_consumer();
    test_created_buffer_uses_circular_behavior();
    test_direct_and_bulk_write_apis_remain_writable();
    test_buffer_copy_uses_unread_wrapped_bytes_without_consuming_source();
    test_buffer_copy_handles_empty_and_self_source();
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
