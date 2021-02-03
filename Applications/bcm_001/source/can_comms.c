#include "system.h"
#include "tima_libc.h"
#include "can_comms.h"
#include "debug.h"
#include "timer.h"
#include "pipe.h"

////////////////////////////////////////////////////////////////////

#define MAX_CAN_DEVICES     10

////////////////////////////////////////////////////////////////////

typedef void ( *filter_handler_t )( const void * data, uint8_t * buffer );

typedef struct can_comms_data_t_
{
    uint8_t  index;
    uint16_t filter;
    filter_handler_t handler;

} can_comms_data_t;

typedef struct can_dev_data_t_
{
    device_t * device;
    pipe_data_t * tx_pipe;
    pipe_data_t * rx_pipe;

} can_dev_data_t;

////////////////////////////////////////////////////////////////////

static void filter_7f0( const void * data, uint8_t * buffer );
static void filter_7f1( const void * data, uint8_t * buffer );
static void filter_7f2( const void * data, uint8_t * buffer );
static void filter_7f3( const void * data, uint8_t * buffer );

////////////////////////////////////////////////////////////////////

static timer_data_t timer;
static const can_comms_data_t can_comms_list[MAX_CAN_DEVICES] =
{
    { 0, 0x07f0, filter_7f0 },
    { 1, 0x07f1, filter_7f1 },
    { 2, 0x07f2, filter_7f2 },
    { 3, 0x07f3, filter_7f3 },
    { 0, 0xFFFF, NULL },
};

static can_dev_data_t * device_list[MAX_CAN_DEVICES];

const static uint8_t tx_test_data[8] = { 0x0b, 0xb8, 0x00, 0xa1, 0x00, 0x01, 0x00, 0x01 };

////////////////////////////////////////////////////////////////////

static void open_can_devices( void )
{
    int counter;
    char dev_name[20];

    for( counter = 0; counter < MAX_CAN_DEVICES; counter++ )
    {
        if( can_comms_list[counter].handler == NULL ) break;

        sprintk( dev_name, "/dev/can?h%03x", can_comms_list[counter].filter );
        device_list[counter]->device = device_open( dev_name );
        device_list[counter]->rx_pipe = device_get_rx_pipe( device_list[counter]->device );
        device_list[counter]->tx_pipe = device_get_tx_pipe( device_list[counter]->device );
    }
}

static void check_can_devices( void )
{
    int counter = 0;
    uint8_t * buffer;

    for( counter = 0; counter < MAX_CAN_DEVICES; counter++ )
    {
        if( can_comms_list[counter].handler == NULL ) break;

        if( ( device_list[counter]->device != NULL ) && ( pipe_is_empty( device_list[counter]->rx_pipe ) == FALSE ) )
        {
            buffer = pipe_read( device_list[counter]->rx_pipe, NULL );
            can_comms_list[counter].handler( &can_comms_list[counter], buffer );
            pipe_release_buffer( buffer );
        }
    }
}

static void send_can_devices( uint16_t id, uint8_t * buffer )
{
    int counter;

    for( counter = 0; counter < MAX_CAN_DEVICES; counter++ )
    {
        if( can_comms_list[counter].handler == NULL ) break;

        if( ( device_list[counter]->device != NULL ) && ( can_comms_list[counter].filter == id ) )
        {
            pipe_send_buffer( device_list[counter]->tx_pipe, buffer, 8 );
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
        send_can_devices( 0x07F3, ( uint8_t * )tx_test_data );
    }
}

void can_comms_init( void )
{
    open_can_devices();
    timer_Start( &timer, 500 );
}

DECLARE_PROCESS_SECTION( can_comms_process );
