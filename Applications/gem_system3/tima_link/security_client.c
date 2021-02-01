#include "system.h"
#include "security_client.h"
#include "tima_link.h"
#include "tima_link_data.h"
#include "datalink.h"
#include "ignition_ctrl.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

static datalink_node_t tima_node;
static message_data_t security_message;

static bool_t key_inserted;

////////////////////////////////////////////////////////////////////

static void security_client_process( void )
{
    uint8_t * buffer;
    uint16_t size;
    uint8_t resp = KEY_CONTROLLER_REMOVAL_OK;
    uint16_t message;

    buffer = datalink_waitfor_node( &tima_node, &size );
    if( buffer != NULL )
    {
        DEBUG_PRINTK( "security_client_process -> 0x%02x, %d\r\n", buffer[0], buffer[1] );

        if( buffer[1] == KEY_CONTROLLER_REQUEST_INSERT )
        {
            message_Post( &security_message, SecurityMessage_InsertOk );
            ignition_key_state( TRUE );
        }
        else if( buffer[1] == KEY_CONTROLLER_REQUEST_REMOVE )
        {
            if( ignition_remove_key_allowed() )
            {
                message_Post( &security_message, SecurityMessage_RemovalOk );
                ignition_key_state( FALSE );
            }
            else
            {
                message_Post( &security_message, SecurityMessage_InvalidRemoval );
            }
        }

        MFREE( buffer );
    }

    if( ( message = message_Get( &security_message ) ) != Message_NoMessage )
    {
        switch( message )
        {
            case SecurityMessage_RemovalOk:
                resp = KEY_CONTROLLER_REMOVAL_OK;
                datalink_send_data( &tima_node, buffer[0], &resp, sizeof( resp ) );
                break;

            case SecurityMessage_InvalidRemoval:
                resp = KEY_CONTROLLER_INVALID_REMOVAL;
                datalink_send_data( &tima_node, buffer[0], &resp, sizeof( resp ) );
                break;

            case SecurityMessage_InsertOk:
                resp = KEY_CONTROLLER_INSERT_OK;
                datalink_send_data( &tima_node, buffer[0], &resp, sizeof( resp ) );
                break;
        }
    }
}

void security_client_init( void )
{
    message_Init( &security_message );
    ignition_control_init();
    datalink_setup_node( &tima_node, ( device_t * )tima_link_get_device(), TIMA_GEM_SYSTEM_SECURITY_CLIENT );
    key_inserted = FALSE;
}

DECLARE_PROCESS_SECTION(security_client_process);
