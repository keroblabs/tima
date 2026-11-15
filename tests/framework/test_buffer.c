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

static void test_full_rejection_balances_critical_section(void)
{
    uint8_t storage[3] = { 0xAA, 0xBB, 0xCC };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    buffer_set_no_roll_over(&buffer, TRUE);
    reset_irq_counts();

    assert(buffer_write(&buffer, 0x11) == TRUE);
    assert(buffer_write(&buffer, 0x22) == TRUE);
    assert(buffer_write(&buffer, 0x33) == FALSE);

    assert(critical_depth == 0);
    assert(disable_calls == 3);
    assert(enable_calls == 3);
    assert(buffer.pointer_in == 2);
    assert(buffer.pointer_out == 0);
    assert(storage[0] == 0x11);
    assert(storage[1] == 0x22);
    assert(storage[2] == 0xCC);
}

static void test_full_rejection_preserves_outer_critical_section(void)
{
    uint8_t storage[3] = { 0, 0, 0 };
    buffer_data_t buffer;

    buffer_init(&buffer, storage, sizeof(storage));
    buffer_set_no_roll_over(&buffer, TRUE);
    reset_irq_counts();

    test_disable_irq();
    assert(buffer_write(&buffer, 0x11) == TRUE);
    assert(buffer_write(&buffer, 0x22) == TRUE);
    assert(buffer_write(&buffer, 0x33) == FALSE);

    assert(critical_depth == 1);
    assert(disable_calls == 4);
    assert(enable_calls == 3);
    test_enable_irq();
    assert(critical_depth == 0);
    assert(enable_calls == 4);
}

int main(void)
{
    test_full_rejection_balances_critical_section();
    test_full_rejection_preserves_outer_critical_section();
    return 0;
}
