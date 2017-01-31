#ifndef __adc_engine_h__
#define __adc_engine_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

#define PIPE_ADC_TRIGGER_NAME	"ADC_TRIGGER"
#define PIPE_ADC_VALUE_NAME		"ADC_VALUE"
#define MAX_ADC_INDEX			4

///////////////////////////////////////////////////////////////

typedef struct _adc_read_t_
{
	uint32_t sender;
	uint16_t index;
	uint16_t value;

} adc_read_t;

///////////////////////////////////////////////////////////////

void adc_engine_process( void );
void adc_engine_init( void );

void adc_conversion_start( adc_read_t * adc_data, uint16_t index );
int adc_conversion_is_complete( adc_read_t * adc_data );

///////////////////////////////////////////////////////////////

#endif // __adc_engine_h__
