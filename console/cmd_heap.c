#include "system.h"
#include "file.h"
#include "commands.h"
#include "tima_libc.h"

///////////////////////////////////////////////////////////////

static device_t * curr_dev;

///////////////////////////////////////////////////////////////

int cmd_heap_print_handler( const char *fmt, ... )
{
	va_list args;	
	int printed_len;
    
	/* Emit the output into the temporary buffer */ 
	va_start(args, fmt);	
	printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args); 
	va_end(args);	

    device_write_buffer( curr_dev, ( uint8_t * )vsf_printk_buf, printed_len );

    return 0;
}

int cmd_heap_handler( console_instance_t * p_instance, char * input )
{
	console_command_printk( p_instance->dev_console_tty, "Used heap:  %d bytes\r\n", sys_heap_used() );

	if( ( input != NULL ) && ( !strcmp( input, "list" ) ) )
	{
		curr_dev = p_instance->dev_console_tty;
		sys_heap_list(cmd_heap_print_handler);
	}

	console_command_printk( p_instance->dev_console_tty, "Total heap: %d bytes\r\n", sys_heap_total() );
	return 0;
}

void cmd_heap_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_heap = 
{
	"heap",
	cmd_heap_init,
	cmd_heap_handler
};

DECLARE_COMMAND_SECTION( command_heap );
