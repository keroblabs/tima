#include "system.h"
#include "timer.h"
#include "random.h"
#include "tima_libc.h"

#include "gem_simulator.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static int obd_rpm;
static int obd_speed;
static int obd_temp_coolant;

static uint32_t obd_mask;

static char obd_string_send[200];

static device_t * obd_dev;
static timer_data_t update_timer;
static timer_data_t send_timer;

static int64_t obd_seed;

///////////////////////////////////////////////////////////////

void gem_simulator_init( void )
{
    uint32_t baud;
    rand_get_seed( &obd_seed );
    
    obd_rpm = 0;
    obd_speed = 0;
    obd_temp_coolant = 0;
    obd_mask = 0;
    
    obd_dev = device_open( "/dev/tty15" );
    if( obd_dev == NULL ) return;
    
    baud = 115200;
    device_ioctrl( obd_dev, DEVICE_SET_CONFIG_DATA, &baud );
    
    timer_Start( &update_timer, 2000 );
    timer_Start( &send_timer, 300 );
}

void gem_simulator_process( void )
{
    uint32_t val;
    
    if( obd_dev == NULL ) return;
    
    if( timer_Check( &send_timer ) )
    {
        timer_Reload( &send_timer );
        
        sprintk( obd_string_send, "obd_send %d,%d,%d,%d\r\n", obd_rpm, obd_speed, obd_temp_coolant, obd_mask );
        device_write_buffer( obd_dev, ( uint8_t * )obd_string_send, ( uint32_t )strlen(obd_string_send ) );
    }
    
    if( timer_Check( &update_timer ) )
    {
        val = rand_get_next( &obd_seed ) & 0xFFF;
        obd_rpm = 800 + ( int )val;
        
        val = rand_get_next( &obd_seed ) & 0x07F;
        obd_speed = 5 + ( int )val;
        
        val = rand_get_next( &obd_seed ) & 0x03F;
        obd_temp_coolant = 30 + ( int )val;
        
        timer_Reload( &update_timer );
        //if( obd_temp_coolant < 100 ) obd_temp_coolant += 5;
    }
    
}
