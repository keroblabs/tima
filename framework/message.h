#ifndef _MESSAGE_H_
#define _MESSAGE_H_

///////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////////////////////////////

enum
{
	Message_NoMessage = 0x0000,

	/* add here kernel messages */

	Message_User_Space = 0x2000,
	Message_Redraw_Screen,
	Message_Update_Screen,
	Message_KeyPressed,
	Message_KeyLongPres,
	Message_KeyReleased,    
    Message_Encoder,

	Message_GPIO_Update = 0x2100,
    
    Message_User_Defined = 0x3000,
    
	Message_Exit_Priority = 0xB000,
	Message_Exit,

	Message_User_Priority = 0xB100,

	Message_MAX = 0xFFFF
};

///////////////////////////////////////////////////////////////////////////////////////

#define MESSAGE_MAX 64

typedef struct
{
    uint8_t pointer_in;
    uint8_t pointer_out;
    uint16_t message_buffer[ MESSAGE_MAX ];
    
} message_data_t;

///////////////////////////////////////////////////////////////////////////////////////

void message_Init( message_data_t * msg );
uint16_t message_Get( message_data_t * msg );
uint16_t message_Read( message_data_t * msg );
void message_Remove( message_data_t * msg );
void message_Post( message_data_t * msg, uint16_t in_data );

///////////////////////////////////////////////////////////////////////////////////////

#endif // _MESSAGE_H_

