#ifndef _TIMER_H_
#define _TIMER_H_

////////////////////////////////////////////////////////////////////////

#include "types.h"

////////////////////////////////////////////////////////////////////////

typedef struct
{
	uint32_t reload_ms;
    uint32_t expire_ms;
    uint32_t status;
    
} timer_data_t;

//#if !defined _MSC_VER
//typedef long int time_t;
//#endif

////////////////////////////////////////////////////////////////////////

#define TIMER_INIT      0x00
#define TIMER_OFF       0x00
#define TIMER_RUN       0x01
#define TIMER_STOPPED   0x02
#define TIMER_EXPIRED   0x04

////////////////////////////////////////////////////////////////////////

time_t timer_Now( void );
void timer_SetRTC( const time_t time );

uint32_t timer_get_MS( void );
uint64_t timer_get_US( void );

void timer_Init( void );
void timer_Start( timer_data_t * timer_entry, uint32_t period );
void timer_Reload( timer_data_t * timer_entry );
void timer_Stop( timer_data_t * timer_entry );
void timer_Expire( timer_data_t * timer_entry );
bool_t timer_Check( timer_data_t * timer_entry );
uint32_t timer_Get_Counter( timer_data_t * timer_entry );

////////////////////////////////////////////////////////////////////////

#endif // _TIMER_H_

