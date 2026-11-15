#include "system.h"
#include "tima_libc.h"
#include "can_comms.h"
#include "debug.h"
#include "timer.h"

////////////////////////////////////////////////////////////////////

#define MAX_CAN_DEVICES                         10

#define CAN_IGNITION_MASK                       0x20
#define CAN_STARTER_MASK                        0x10

////////////////////////////////////////////////////////////////////

typedef void ( *filter_handler_t )( const void * data, uint8_t * buffer );

typedef struct can_comms_data_t_
{
    uint16_t filter;
    filter_handler_t handler;

} can_comms_data_t;

////////////////////////////////////////////////////////////////////

static void filter_can( const void * data, uint8_t * buffer );

////////////////////////////////////////////////////////////////////

static timer_data_t timer;
static int index_debug;
static hook_data_t * hook_list;

static const can_comms_data_t can_comms_list[MAX_CAN_DEVICES] =
{
    { 0x07f0, filter_can },
    { 0x07f1, filter_can },
    { 0x07f2, filter_can },
    { 0x07f3, filter_can },
    { 0xFFFF, NULL },
};

static device_t * device_list[MAX_CAN_DEVICES];

const static uint8_t tx_test_data[4][8] =
{
    { 0x05, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x00 },
    { 0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 },
    { 0x05, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 },
    { 0x05, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00 },
};

////////////////////////////////////////////////////////////////////

static void open_can_devices( void )
{
    int counter;
    char dev_name[20];

    for( counter = 0; counter < MAX_CAN_DEVICES; counter++ )
    {
        if( can_comms_list[counter].handler == NULL ) break;

        sprintk( dev_name, "/dev/can?h%03x", can_comms_list[counter].filter );
        device_list[counter] = device_open( dev_name );
    }
}

static void check_can_devices( void )
{
    int counter = 0;
    uint8_t buffer[8];

    for( counter = 0; counter < MAX_CAN_DEVICES; counter++ )
    {
        if( can_comms_list[counter].handler == NULL ) break;

        if( ( device_list[counter] != NULL ) && device_is_rx_pending( device_list[counter] ) )
        {
            device_read_buffer( device_list[counter], ( uint8_t * )buffer, CAN_DATA_MAX_SIZE );
            can_comms_list[counter].handler( &can_comms_list[counter], buffer );
        }
    }
}

void can_comms_send_data( uint16_t id, uint8_t * buffer )
{
    int counter;

    for( counter = 0; counter < MAX_CAN_DEVICES; counter++ )
    {
        if( can_comms_list[counter].handler == NULL ) break;

        if( ( device_list[counter] != NULL ) && ( can_comms_list[counter].filter == id ) )
        {
            device_write_buffer( device_list[counter], ( uint8_t * )buffer, CAN_DATA_MAX_SIZE );
            break;
        }
    }
}

static void debug_can_data( uint16_t id, uint8_t * buffer )
{
    int counter;

    DEBUG_PRINTK( "0x%03x:", id );
    for( counter = 0; counter < CAN_DATA_MAX_SIZE; counter++ )
    {
        DEBUG_PRINTK( " 0x%02x", buffer[counter] );
    }
    DEBUG_PRINTK( "\r\n" );
}

static void filter_can( const void * p_data, uint8_t * buffer )
{
    const can_comms_data_t * data = ( const can_comms_data_t * )p_data;

    hook_data_t * search = hook_list;

    while( search != NULL )
    {
        if( search->id == data->filter )
        {
            search->handler( search->id, buffer );
        }

        search = search->p_next;
    }

    #ifdef DEBUG_ENABLED
    debug_can_data( data->filter, buffer );
    #endif
}

static void can_comms_process( void )
{
    check_can_devices();

    if( timer_Check( &timer ) )
    {
        timer_Reload( &timer );
        can_comms_send_data( 0x07F3, ( uint8_t * )tx_test_data[(index_debug >> 2) & 0x03] );
        index_debug++;
    }
}

void can_comms_hook_id( hook_data_t * hook_data, uint16_t id, hook_handler_t hook )
{
    hook_data->id = id;
    hook_data->handler = hook;

    hook_data->p_next = hook_list;
    hook_list = hook_data;
}

void can_comms_init( void )
{
    hook_list = NULL;
    index_debug = 0;
    open_can_devices();
    timer_Start( &timer, 500 );
}

DECLARE_PROCESS_SECTION( can_comms_process );
