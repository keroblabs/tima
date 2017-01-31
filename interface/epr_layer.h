#ifndef __epr_layer_h__
#define __epr_layer_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

typedef struct epr_data_t_
{
	uint64_t addr_offset;
	uint32_t dev_lun;
	uint32_t data_size;
	uint8_t * buffer;

} epr_data_t;

///////////////////////////////////////////////////////////////

epr_data_t * epr_layer_init( const char * dev, uint32_t data_size, uint32_t offset );

void * epr_layer_read( epr_data_t * p_data );
void epr_layer_flush( epr_data_t * p_data );

void epr_layer_close( epr_data_t * p_data );

///////////////////////////////////////////////////////////////

#endif // __epr_layer_h__
