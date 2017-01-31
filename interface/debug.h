#ifndef __DEBUG_INTERFACE_h__
#define __DEBUG_INTERFACE_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

#define DEBUG_ENABLED

///////////////////////////////////////////////////////////////

int debug_printk( const char *fmt, ... );
bool_t debug_read_input( char * out );

#ifndef _NO_DEBUG

#ifdef DEBUG_PRINTK
#undef DEBUG_PRINTK
#endif

#define DEBUG_PRINTK debug_printk
#endif

#define TIMA_DEBUG

///////////////////////////////////////////////////////////////

#endif // __DEBUG_INTERFACE_h__
