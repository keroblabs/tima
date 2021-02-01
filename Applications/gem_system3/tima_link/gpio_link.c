#include "system.h"
#include "gpio_link.h"
#include "tima_link.h"
#include "tima_link_data.h"
#include "datalink.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////

typedef struct _gpio_link_t
{
    int gpio_state;
    message_data_t *message_data;

} gpio_link_t;

////////////////////////////////////////////////////////////////////

#ifdef _USE_RPI
#define GPIO_LINK_ID    TIMA_INSTRUMENTS_GPIO
#else
#define GPIO_LINK_ID    TIMA_GEM_SYSTEM_GPIO
#endif

////////////////////////////////////////////////////////////////////

/*
 * GPIO_PH00 = washer level
 * GPIO_PH01 =
 *
 * GPIO_PI00 = indicator  (off, left, right)
 * GPIO_PI01 = full beam  (0ff, flash, hold)
 * GPIO_PI02 = wiper mode (off, intermittent, slow, fast)
 * GPIO_PI03 = timer pos  (0, 1, 2, 3, 4, 5)
 * GPIO_PI10 = washer sw  (off, 1, 2)
 * GPIO_PI11 = washer pad (off, front, rear)
 * GPIO_PI14 = horn button
 * GPIO_PI15 = handbrake (released, moving, pulled)
 */

////////////////////////////////////////////////////////////////////

static datalink_node_t tima_node;
static gpio_link_t gpio_state[GPIO_MAX];
static uint8_t responder = 0;

////////////////////////////////////////////////////////////////////

static void gpio_link_send_identity( void )
{
    uint8_t buffer[2];

    buffer[0] = GPIO_LINK_SET_IDENTITY;
    buffer[1] = GPIO_LINK_ID;

    datalink_send_data( &tima_node, responder, buffer, sizeof( buffer ) );
}

static void gpio_link_process( void )
{
    uint8_t * buffer;
    uint8_t * command;
    uint16_t size;
    uint16_t message;
    uint8_t source;

    buffer = datalink_waitfor_node( &tima_node, &size );
    if( buffer != NULL )
    {
        source = buffer[0];
        command = &buffer[1];

        DEBUG_PRINTK( "-> 0x%02x, %d\r\n", source, command[0] );
        responder = source;

        if( command[0] == GPIO_LINK_QUERY_PIN )
        {
            gpio_link_set( ( gpio_t )command[1], (int)gpio_state[(int) command[1]].gpio_state );
        }
        else if( command[0] == GPIO_LINK_SET_PIN )
        {
            gpio_state[command[1]].gpio_state = command[2];
            if( gpio_state[command[1]].message_data != NULL )
            {
                message_Post( gpio_state[command[1]].message_data, MESSAGE_GPIO_CHANGED + command[1] );
            }
        }

        MFREE( buffer );
    }
}

#if defined _USE_MULTITHREAD
static void gpio_link_process_thread( void )
{
    while( 1 )
    {
        gpio_link_process();
        tthread_sleep_ms( 10 );
    }
}
#else
DECLARE_PROCESS_SECTION(gpio_link_process);
#endif

void gpio_link_attach( gpio_t gpio, message_data_t * message_data )
{
    gpio_state[(int) gpio].message_data = message_data;
}

int gpio_link_get( gpio_t gpio )
{
    return (int)gpio_state[(int) gpio].gpio_state;
}

void gpio_link_set( gpio_t gpio, int state )
{
    uint8_t buffer[3];

    gpio_state[(int) gpio].gpio_state = ( uint8_t )state;

    buffer[0] = GPIO_LINK_SET_PIN;
    buffer[1] = ( uint8_t )gpio;
    buffer[2] = ( uint8_t )state;

    datalink_send_data( &tima_node, responder, buffer, sizeof( buffer ) );
}

void gpio_link_init( void )
{
    datalink_setup_node( &tima_node, ( device_t * )tima_link_get_device(), GPIO_LINK_ID );
    responder = DATALINK_BROADCAST;

    memset( gpio_state, 0x00, sizeof( gpio_state ) );

#if defined _USE_MULTITHREAD
    tthread_create( gpio_link_process_thread );
#endif
}

