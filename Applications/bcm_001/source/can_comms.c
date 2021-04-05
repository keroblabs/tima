#include "system.h"
#include "tima_libc.h"
#include "can_comms.h"
#include "debug.h"
#include "timer.h"

////////////////////////////////////////////////////////////////////

#define MAX_CAN_DEVICES     10

////////////////////////////////////////////////////////////////////

typedef void ( *filter_handler_t )( const void * data, uint8_t * buffer );

typedef struct can_comms_data_t_
{
    uint16_t filter;
    filter_handler_t handler;

} can_comms_data_t;

////////////////////////////////////////////////////////////////////

static void filter_7f0( const void * data, uint8_t * buffer );
static void filter_7f1( const void * data, uint8_t * buffer );
static void filter_7f2( const void * data, uint8_t * buffer );
static void filter_7f3( const void * data, uint8_t * buffer );

////////////////////////////////////////////////////////////////////

static timer_data_t timer;
static int index_debug;

static const can_comms_data_t can_comms_list[MAX_CAN_DEVICES] =
{
    { 0x07f0, filter_7f0 },
    { 0x07f1, filter_7f1 },
    { 0x07f2, filter_7f2 },
    { 0x07f3, filter_7f3 },
    { 0xFFFF, NULL },
};

static device_t * device_list[MAX_CAN_DEVICES];

const static uint8_t tx_test_data[4][8] =
{
    { 0x0b, 0xb8, 0x00, 0xa1, 0x00, 0x01, 0x00, 0x01 },
    { 0x0b, 0x01, 0x00, 0xa1, 0x00, 0x01, 0x00, 0x01 },
    { 0x0b, 0x02, 0x00, 0xa1, 0x00, 0x01, 0x00, 0x01 },
    { 0x0b, 0x04, 0x00, 0xa1, 0x00, 0x01, 0x00, 0x01 },
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
            device_read_buffer( device_list[counter], ( uint8_t * )buffer, 8 );
            can_comms_list[counter].handler( &can_comms_list[counter], buffer );
        }
    }
}

static void send_can_devices( uint16_t id, uint8_t * buffer )
{
    int counter;

    for( counter = 0; counter < MAX_CAN_DEVICES; counter++ )
    {
        if( can_comms_list[counter].handler == NULL ) break;

        if( ( device_list[counter] != NULL ) && ( can_comms_list[counter].filter == id ) )
        {
            device_write_buffer( device_list[counter], ( uint8_t * )buffer, 8 );
            break;
        }
    }
}

static void debug_can_data( const void * p_data, uint8_t * buffer )
{
    const can_comms_data_t * data = ( const can_comms_data_t * )p_data;
    int counter;

    DEBUG_PRINTK( "0x%03x:", data->filter );
    for( counter = 0; counter < 8; counter++ )
    {
        DEBUG_PRINTK( " 0x%02x", buffer[counter] );
    }
    DEBUG_PRINTK( "\r\n" );
}

static void filter_7f0( const void * p_data, uint8_t * buffer )
{
    debug_can_data( p_data, buffer );
}

static void filter_7f1( const void * p_data, uint8_t * buffer )
{
    debug_can_data( p_data, buffer );
}

static void filter_7f2( const void * p_data, uint8_t * buffer )
{
    debug_can_data( p_data, buffer );
}

static void filter_7f3( const void * p_data, uint8_t * buffer )
{
    debug_can_data( p_data, buffer );
}

static void can_comms_process( void )
{
    check_can_devices();

    if( timer_Check( &timer ) )
    {
        timer_Reload( &timer );
        send_can_devices( 0x07F3, ( uint8_t * )tx_test_data[(index_debug >> 2) & 0x03] );
        index_debug++;
    }
}

void can_comms_init( void )
{
    index_debug = 0;
    open_can_devices();
    timer_Start( &timer, 500 );
}

DECLARE_PROCESS_SECTION( can_comms_process );
