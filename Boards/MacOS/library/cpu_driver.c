#include "system.h"

////////////////////////////////////////////////////////////////////////////////

#define SYSTEM_HEAP_SIZE 		(256*1024)

uint8_t system_heap[SYSTEM_HEAP_SIZE];
uint32_t system_heap_size = SYSTEM_HEAP_SIZE;

////////////////////////////////////////////////////////////////////////////////

void _cpu_disable_irq( void )
{
}

void _cpu_enable_irq( void )
{
}

void _cpu_pio( bool_t state )
{
}

void CPU_pio_reset( bool_t state )
{
}