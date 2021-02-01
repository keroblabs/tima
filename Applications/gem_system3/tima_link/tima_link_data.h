#ifndef APPLICATIONS_INSTRUMENTS_SOURCE3_TIMA_LINK_DATA_H_
#define APPLICATIONS_INSTRUMENTS_SOURCE3_TIMA_LINK_DATA_H_

////////////////////////////////////////////////////////////////////

#include "types.h"
#include "datalink.h"

////////////////////////////////////////////////////////////////////

#define TIMA_LINK_HEADER        0x5A
#define TIMA_LINK_FOOTER        0x1E
#define TIMA_LINK_CRC           0x4439A47d

////////////////////////////////////////////////////////////////////

enum
{
    TIMA_GEM_SYSTEM  = DATALINK_BROADCAST + 1,
    TIMA_INSTRUMENTS = 0x02,

    TIMA_USER_NODES = 0x10,

    TIMA_INSTRUMENTS_KEYCONTROLLER,
    TIMA_INSTRUMENTS_FILESYSTEM_SERVER,
    TIMA_INSTRUMENTS_FILESYSTEM_CLIENT,
    TIMA_INSTRUMENTS_GPIO,

    TIMA_GEM_SYSTEM_IGNITION_NODE,
    TIMA_GEM_SYSTEM_SECURITY_CLIENT,
    TIMA_GEM_SYSTEM_GPIO,
};

enum
{
    TIMA_LINK_FILESYSTEM_OPEN = 0x01,
    TIMA_LINK_FILESYSTEM_CLOSE,
    TIMA_LINK_FILESYSTEM_READ_LINE,
    TIMA_LINK_FILESYSTEM_WRITE_LINE,
    TIMA_LINK_FILESYSTEM_SET_POS,

    GPIO_LINK_QUERY_PIN,
    GPIO_LINK_QUERY_VALUE,
    GPIO_LINK_SET_PIN,
    GPIO_LINK_SET_VALUE,
    GPIO_LINK_SET_IDENTITY,

    KEY_CONTROLLER_REQUEST_REMOVE,
    KEY_CONTROLLER_REQUEST_INSERT,
    KEY_CONTROLLER_INVALID_REMOVAL,
    KEY_CONTROLLER_INSERT_OK,
    KEY_CONTROLLER_REMOVAL_OK,
    KEY_CONTROLLER_FORCE_REMOVAL,
};

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_INSTRUMENTS_SOURCE3_TIMA_LINK_DATA_H_ */
