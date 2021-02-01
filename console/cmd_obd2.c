#include "system.h"
#include "commands.h"
#include "tima_libc.h"

///////////////////////////////////////////////////////////////

#define TOTAL_OBD2_PARAMETERS		7

///////////////////////////////////////////////////////////////

void cmd_obd2_tokenizer( char * input, char * delim, int * values, int max )
{
	char * p_input;
	char * out;
	char * last;
	char str_val[10];
	int size;
	int i;

	p_input = input;
	i = 0;
	values[0] = values[1] = values[2] = 0;

	while( tima_tokenizer( p_input, ( const char ** )&out, &size, delim, &last ) )
	{
		strncpy( str_val, out, size );
		str_val[size] = 0;
		values[i] = atoi( str_val );
		i++;
		p_input = NULL;

		if( i >= max ) break;
	}
}

int cmd_obd2_handler( console_instance_t * p_instance, char * input )
{
	int values[TOTAL_OBD2_PARAMETERS];

	if( input != NULL )
	{
		if( !strcmp( input, "?" ) )
		{
			CONSOLE_PRINTK( "obd2_cmd <speed>,<rpm>,<odometer>,<temp>,<fuel>,<mask1>,<mask2>" );
		}
		else
		{
			cmd_obd2_tokenizer( input, ",", values, TOTAL_OBD2_PARAMETERS );
			CONSOLE_PRINTK( "\r\n" );
			CONSOLE_PRINTK( "%d km/h\r\n", values[0] );
			CONSOLE_PRINTK( "%d RPM\r\n",  values[1] );
			CONSOLE_PRINTK( "%d km\r\n",   values[2] );
			CONSOLE_PRINTK( "%d deg\r\n",  values[3] );
			CONSOLE_PRINTK( "%d l\r\n",    values[4] );
			CONSOLE_PRINTK( "0x%08x\r\n",  values[5] );
			CONSOLE_PRINTK( "0x%08x\r\n",  values[6] );
		}
	}

	return 0;
}

const command_data_t command_obd2 = 
{
	"obd2_cmd",
	NULL,
	cmd_obd2_handler
};

DECLARE_COMMAND_SECTION( command_obd2 );
