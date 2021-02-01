#ifndef APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_SECURITY_CLIENT_H_
#define APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_SECURITY_CLIENT_H_

////////////////////////////////////////////////////////////////////

#include "types.h"
#include "message.h"

////////////////////////////////////////////////////////////////////

enum
{
    SecurityMessage_Idle = Message_User_Defined,

    SecurityMessage_InvalidRemoval,
    SecurityMessage_InsertOk,
    SecurityMessage_RemovalOk,
};

////////////////////////////////////////////////////////////////////

void security_client_init( void );

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_SECURITY_CLIENT_H_ */
