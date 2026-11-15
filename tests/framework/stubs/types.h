#ifndef TEST_FRAMEWORK_TYPES_H
#define TEST_FRAMEWORK_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef unsigned char bool_t;

#define TRUE 1
#define FALSE 0
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define DI test_disable_irq
#define EI test_enable_irq
#define MMALLOC test_malloc
#define MFREE test_free

void test_disable_irq(void);
void test_enable_irq(void);
void *test_malloc(uint32_t size);
void test_free(void *ptr);

#endif
