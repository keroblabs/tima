#ifndef __commands_h__
#define __commands_h__

///////////////////////////////////////////////////////////////

#include "types.h"
#include "system.h"

///////////////////////////////////////////////////////////////

#ifndef MAX_LONG_FILENAME_LENGHT
#define MAX_LONG_FILENAME_LENGHT 128
#endif

#define CONSOLE_COMMAND_TYPE        0x01
#define FTM_COMMAND_TYPE            0x02

///////////////////////////////////////////////////////////////

#define DECLARE_COMMAND_SECTION(a)					SYSTEM_SECTION_LIST(a, COMMAND_SECTION_ID)
#define GET_COMMAND_SECTION(i)                      ( command_data_t * )system_List_Get_Item(COMMAND_SECTION_ID,i)
#define GET_FTM_DATA_SECTION(i)                     ( ftm_command_t * )system_List_Get_Item(COMMAND_SECTION_ID,i)

///////////////////////////////////////////////////////////////

enum
{
    FTM_STATE_NONE,
    FTM_STATE_COMMAND,
    FTM_STATE_UPPER_SIZE,
    FTM_STATE_LOWER_SIZE,
    FTM_STATE_DATA_START,
    FTM_STATE_RX_DATA,
    FTM_STATE_DATA_END
};

enum
{
    FTM_MODE_NONE,
    
    FTM_PACK_PROTO_START = 0xC0,
    FTM_PACK_DATA_START,
    FTM_PACK_RESP,
    FTM_PACK_EOF,
    FTM_PACK_ACK,
    FTM_PACK_ERROR,
};

///////////////////////////////////////////////////////////////

typedef struct _console_instance_t_
{
    //char * command_buffer;
    
	//uint16_t command_pointer;

    uint8_t * buffer;
    
    uint8_t ftm_state;
    uint8_t curr_cmd;
    uint16_t rx_size;
    uint16_t rx_count;
    
    bool_t echo_mode;
	bool_t command_mode;
	bool_t carriage_returned;
    bool_t is_ready;
	uint16_t telnet_port;

	device_t * dev_console_tty;

	char cmd_curr_dir[MAX_LONG_FILENAME_LENGHT]; 

	struct _console_instance_t_ * p_next;

} console_instance_t;

typedef struct _command_data_t_
{
    uint8_t type;
	char * name;
	void (* init )( void );
	int (* handler )( console_instance_t * p_instance, char * input );

} command_data_t;


typedef struct _ftm_command_t_
{
    uint8_t type;
    uint8_t ftm_cmd;
    void ( * init )( void );
    void ( * handler )( console_instance_t * p_instance );
    
} ftm_command_t;

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

///////////////////////////////////////////////////////////////

void console_init( void );
void console_process( void );

int console_printk( const char *fmt, ... );
int console_flaged_printk( const char *fmt, ... );
int console_command_printk( device_t * dev_tty, const char *fmt, ... );

char * console_concatenate_path( char * input, bool_t * is_found, console_instance_t * p_instance );
int console_tokenizer( char * input, char ** output, char * delim, int max );

void console_send_ftm_packet( console_instance_t * p_instance, uint8_t command, uint8_t * buffer, uint16_t size );

console_instance_t * console_listen_telnet( uint16_t telnet_port );
console_instance_t * console_connect_uart( const char * dev_name );

///////////////////////////////////////////////////////////////

#endif // __commands_h__
