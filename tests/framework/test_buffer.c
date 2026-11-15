#include <assert.h>
#include <stdlib.h>

#include "buffer.h"

static unsigned int critical_depth;
static unsigned int disable_calls;
static unsigned int enable_calls;

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
    return malloc(size);
}

void test_free(void *ptr)
{
    free(ptr);
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

int main(void)
{
    test_circular_buffer_overwrites_oldest_entry();
    test_created_buffer_uses_circular_behavior();
    test_direct_and_bulk_write_apis_remain_writable();
    test_wrapped_writes_preserve_outer_critical_section();
    return 0;
}
