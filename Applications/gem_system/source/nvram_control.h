#ifndef _NVRAM_CONTROL_H_
#define _NVRAM_CONTROL_H_

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

#define NVRAM_ADDR_NVRAM_ID		0x00
#define NVRAM_ADDR_ODOMETER		0x04
#define NVRAM_ADDR_TRIP			0x08
#define NVRAM_ADDR_BITMASK1     0x14
#define NVRAM_ADDR_BITMASK2     0x18

///////////////////////////////////////////////////////////////

void nvram_control_init( void );

void nvram_control_read( uint32_t address, void * data, uint32_t size );
void nvram_control_write( uint32_t address, void * data, uint32_t size );

void nvram_control_flush( void );

///////////////////////////////////////////////////////////////

#endif // _FUEL_H_
