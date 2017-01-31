#ifndef __CMD_COMMON_H__
#define __CMD_COMMON_H__

///////////////////////////////////////////////////////////////

#include "types.h"
#include "system.h"
#include "file.h"
#include "fs.h"
#include "commands.h"

///////////////////////////////////////////////////////////////

void console_show_mounted_devices( device_t * dev );
char * console_concatenate_path( char * input, bool_t * is_found, console_instance_t * p_instance );

///////////////////////////////////////////////////////////////

#endif // __CMD_COMMON_H__
