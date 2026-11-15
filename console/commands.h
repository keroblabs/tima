#ifndef __commands_h__
#define __commands_h__

///////////////////////////////////////////////////////////////

#include "types.h"
#include "system.h"
#include "fs.h"

///////////////////////////////////////////////////////////////

typedef struct _console_instance_t_
{
	char command_buffer[128];
	uint16_t command_pointer;

	bool_t is_open;
	bool_t echo_mode;
	bool_t command_mode;
	bool_t carriage_returned;
	uint16_t telnet_port;

	device_t * dev_console_tty;

	char cmd_curr_dir[MAX_LONG_FILENAME_LENGHT]; 

	struct _console_instance_t_ * p_next;

} console_instance_t;

typedef struct _command_data_t_
{
	char * name;
	void (* init )( void );
	int (* handler )( console_instance_t * p_instance, char * input );

} command_data_t;

///////////////////////////////////////////////////////////////

//#define CONSOLE_PRINTK(...) file_fprintk( FILE_CONSOLE_HDL, __VA_ARGS__ )
//#define CONSOLE_FLUSH()     file_flush( FILE_CONSOLE_HDL )
#if defined DEBUG_PRINTK
#undef DEBUG_PRINTK
#endif

#define DEBUG_PRINTK(...)       console_flaged_printk( __VA_ARGS__ )
#define CONSOLE_PRINTK(...)		console_printk( __VA_ARGS__ )
#define CONSOLE_FLUSH()			

///////////////////////////////////////////////////////////////

#define COMMAND_SECTION_ID                          SYSTEM_LIST_SECTION+2
#define GET_COMMAND_SECTION(i)                      ( command_data_t * )system_List_Get_Item(COMMAND_SECTION_ID,i)
#define DECLARE_COMMAND_SECTION(a)                  SYSTEM_SECTION_LIST(a, COMMAND_SECTION_ID)

///////////////////////////////////////////////////////////////

void console_init( void );
void console_process( void );

int console_printk( const char *fmt, ... );
int console_flaged_printk( const char *fmt, ... );
int console_command_printk( device_t * dev_tty, const char *fmt, ... );

char * console_concatenate_path( char * input, bool_t * is_found, console_instance_t * p_instance );
int console_tokenizer( char * input, char ** output, char * delim, int max );

void console_show_mounted_devices( device_t * dev );

console_instance_t * console_create_instance( void );
console_instance_t * console_accept_telnet( console_instance_t * p_instance );
console_instance_t * console_listen_telnet( uint16_t telnet_port );
console_instance_t * console_connect_uart( const char * dev_name );

void console_add_instance( console_instance_t * p_instance );

///////////////////////////////////////////////////////////////

#endif // __commands_h__
