#ifndef APPLICATIONS_BCM_001_SOURCE_CAN_COMMS_H_
#define APPLICATIONS_BCM_001_SOURCE_CAN_COMMS_H_

////////////////////////////////////////////////////////////////////

#include "types.h"

////////////////////////////////////////////////////////////////////

#define FUSE_BOX_BCM_01                         0
#define REAR_BCM_01                             1

#define CAN_VALUE_TO_24BIT(msg, i)             (((int)(msg[(i)]) << 16) + ((int)(msg[(i)+1]) << 8) + (int)msg[(i)+2])
#define CAN_VALUE_TO_16BIT(msg, i)             (((int)(msg[(i)]) * 256) + (int)msg[(i)+1])
#define CAN_VALUE_TO_8BIT(msg,i)                ((int)msg[(i)])

#define CAN_MESSAGE_BCM_ID                      0

#define CAN_DATA_MAX_SIZE       				8

////////////////////////////////////////////////////////////////////

typedef void ( *hook_handler_t )( uint16_t id, uint8_t * buffer );

typedef struct hook_data_t_
{
    struct hook_data_t_ * p_next;
    hook_handler_t handler;
    uint16_t id;
} hook_data_t;

////////////////////////////////////////////////////////////////////

void can_comms_init( void );
void can_comms_hook_id( hook_data_t * hook_data, uint16_t id, hook_handler_t hook );
void can_comms_send_data( uint16_t id, uint8_t * buffer );

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_BCM_001_SOURCE_CAN_COMMS_H_ */
