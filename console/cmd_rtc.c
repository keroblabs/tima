#include "system.h"
#include "timer.h"
#include "commands.h"
#include "tima_libc.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

void cmd_rtc_tokenizer( char * input, char * delim, int * values, int max )
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

int cmd_date_handler( console_instance_t * p_instance, char * input )
{
	date_time_t * local_time;
	int values[3];
	device_t * dev = p_instance->dev_console_tty;

	local_time = tima_localtime( timer_Now() );

	if( input != NULL )
	{
		if( !strcmp( input, "?" ) )
		{
			console_command_printk( dev, "date <day>/<month>/<year>" );
		}
		else
		{
			cmd_rtc_tokenizer( input, "/", values, 3 );
			local_time->tm_mday = values[0];
			local_time->tm_mon = values[1]-1;
			local_time->tm_year = values[2]-1900;
			timer_SetRTC(tima_mktime(local_time));
		}
	}

	console_command_printk( dev, "%02d/%02d/%02d", local_time->tm_mday, local_time->tm_mon+1, local_time->tm_year+1900 );

	return 0;
}

int cmd_time_handler( console_instance_t * p_instance, char * input )
{
	int values[3];
	date_time_t * local_time;
	device_t * dev = p_instance->dev_console_tty;

	local_time = tima_localtime( timer_Now() );

	if( input != NULL )
	{
		if( !strcmp( input, "?" ) )
		{
			console_command_printk( dev, "time <hour>:<minute>[:<seconds>]" );
		}
		else
		{
			cmd_rtc_tokenizer( input, ":", values, 3 );
			local_time->tm_hour = values[0];
			local_time->tm_min = values[1];
			local_time->tm_sec = values[2];
			timer_SetRTC(tima_mktime(local_time));
		}
	}

	console_command_printk( dev, "%02d:%02d:%02d", local_time->tm_hour, local_time->tm_min, local_time->tm_sec );

	return 0;
}

void cmd_rtc_init( void )
{
}

///////////////////////////////////////////////////////////////

const command_data_t command_date = 
{
    CONSOLE_COMMAND_TYPE,
    "date",
	cmd_rtc_init,
	cmd_date_handler
};

const command_data_t command_time = 
{
    CONSOLE_COMMAND_TYPE,
    "time",
	NULL,
	cmd_time_handler
};

DECLARE_COMMAND_SECTION( command_date );
DECLARE_COMMAND_SECTION( command_time );
