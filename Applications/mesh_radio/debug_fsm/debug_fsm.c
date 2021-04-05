#include "debug_fsm.h"
#include "device.h"
#include "system.h"
#include "tima_libc.h"

#include "pipe.h"
#include "buffer_data.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

static void debug_fsm_init( debug_fsm_t * fsm_data )
{
    uint32_t baud = 115200;
    
    if( device_open_ex( DEV_PRINTK, &fsm_data->debug_device ) == TRUE )
    {
        device_ioctrl( &fsm_data->debug_device, DEVICE_SET_CONFIG_DATA, &baud );
    }
}

static void debug_fsm_exit( debug_fsm_t * fsm_data )
{
}

static void Idle_transition_Debug_Print_indication( debug_fsm_t * fsm_data, Debug_Print_indication_t * signal )
{
    //buffer_data_t * print_data = ( buffer_data_t * )signal->signal.data;
    //device_write_buffer( &fsm_data->debug_device, ( uint8_t * )print_data->data, print_data->size );
}

////////////////////////////////////////////////////////////////////

int debug_fsm_printk( const char *fmt, ... )
{
    va_list args;
    int printed_len;
    debug_fsm_t * fsm_data = DEBUG_FSM_GET_DATA();
    if( fsm_data == NULL ) return 0;
    
    /* Emit the output into the temporary buffer */
    va_start(args, fmt);
    printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args);
    va_end(args);
    
    SEND_Debug_Print_indication( DEBUG_FSM_PID, buffer_data_create( vsf_printk_buf, printed_len ), 0 );
    return printed_len;
}
