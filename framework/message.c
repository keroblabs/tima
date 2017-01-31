#include "message.h"

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

void message_Init( message_data_t * msg )
{
	msg->pointer_in = 0;
	msg->pointer_out = 0;
	
	memset( msg->message_buffer, 0x00, sizeof( msg->message_buffer ) );
}

uint16_t message_Get( message_data_t * msg )
{
	uint16_t ret_value;

	if( msg->pointer_in == msg->pointer_out ) return( Message_NoMessage );

	ret_value = msg->message_buffer[ msg->pointer_out ];
	
	if( ret_value < Message_Exit_Priority )
	{
		msg->pointer_out++;
		if( msg->pointer_out == MESSAGE_MAX ) msg->pointer_out = 0;
	}

	return( ret_value );
}

uint16_t message_Read( message_data_t * msg )
{
	uint16_t ret_value;

	if( msg->pointer_in == msg->pointer_out ) return( Message_NoMessage );

	ret_value = msg->message_buffer[ msg->pointer_out ];
	
	return( ret_value );
}

void message_Remove( message_data_t * msg )
{
	if( msg->pointer_in == msg->pointer_out ) return;
	msg->pointer_out++;
	if( msg->pointer_out == MESSAGE_MAX ) msg->pointer_out = 0;
}

void message_Post( message_data_t * msg, uint16_t in_data )
{
	if( in_data == Message_NoMessage ) return;
	
	msg->message_buffer[ msg->pointer_in ] = in_data;
	msg->pointer_in++;
	if( msg->pointer_in == MESSAGE_MAX ) msg->pointer_in = 0;

	if( msg->pointer_in == msg->pointer_out ) msg->pointer_out++;
	if( msg->pointer_out == MESSAGE_MAX ) msg->pointer_out = 0;
}





