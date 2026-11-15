#include "system.h"
#include "device.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
//#include <sys/filio.h>

#include "types.h"

///////////////////////////////////////////////////////////////////////////

#define CONSOLE_THREAD_STACK_SIZE       1024
#define CONSOLE_BUFFER_SIZE             256

#define UNKNOWN_PORT_INDEX              0xFF

#define TTY_PORT_COUNT                  (sizeof(tty_config) / sizeof(tty_config[0]))

///////////////////////////////////////////////////////////////////////////

typedef struct tty_config_t_
{
    char * dev_name;
    uint32_t index;

} tty_config_t;

///////////////////////////////////////////////////////////////////////////

static const tty_config_t tty_config[] = 
{
    { "/dev/ttyUSB0", 31 },
    { "/dev/ttyUSB1", 32 },
    {  NULL, 0 }
};

static int serial_fd[TTY_PORT_COUNT];
static struct termios tty_linux_data[TTY_PORT_COUNT];

//static const char * mac_serial_port[TTY_PORT_COUNT+1] = { "", MAC_SERIAL_PORT0, MAC_SERIAL_PORT1 };

///////////////////////////////////////////////////////////////////////////

static uint8_t console_get_index( uint32_t index )
{
    uint8_t count;

    for( count = 0; count < TTY_PORT_COUNT; count++ )
    {
        if( tty_config[ count ].index == index )
        {
            return count;
        }
    }

    return UNKNOWN_PORT_INDEX;
}

static uint32_t console_output( uint32_t index, uint8_t * text, uint32_t size )
{
	//printf( text );
    
    index = console_get_index( index );
    if( index == UNKNOWN_PORT_INDEX ) return 0;

    write( serial_fd[index], text, size );

    return size;
}

static uint32_t console_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
    int num_bytes_present = 0;
    
    index = console_get_index( index );
    if( index == UNKNOWN_PORT_INDEX ) return 0;

    ioctl(serial_fd[index], FIONREAD, &num_bytes_present);
    
    if( ( num_bytes_present > 0 ) && ( buffer != NULL ) )
    {
        if( num_bytes_present > size ) num_bytes_present = size;
        if( read( serial_fd[index], buffer, num_bytes_present ) < 0 )
        {
            printf( "error\n" );
        }
    }

    return ( uint32_t )num_bytes_present;
}

static void console_drv_close( uint32_t index )
{
    index = console_get_index( index );
    if( index == UNKNOWN_PORT_INDEX ) return;

    close(serial_fd[index]);
    serial_fd[index] = 0;
}

static void console_drv_init( uint32_t index )
{
    index = console_get_index( index );
    if( index == UNKNOWN_PORT_INDEX ) return;
 
    if( serial_fd[index] != 0 ) close( serial_fd[index] );
    serial_fd[index] = open(tty_config[index].dev_name, O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (serial_fd[index] == -1)
    {
        serial_fd[index] = 0;
        return;
    }

    memset( &tty_linux_data[index], 0x00, sizeof(tty_linux_data[index]));

    tty_linux_data[ index ].c_cflag |= B115200;
    tty_linux_data[ index ].c_cflag |= IGNPAR;
    tty_linux_data[ index ].c_cflag |= CS8;
    tty_linux_data[ index ].c_cflag |= CREAD;
    tty_linux_data[ index ].c_cc[VTIME] = 0;
    tty_linux_data[ index ].c_cc[VMIN] = 1;

    cfsetospeed(&tty_linux_data[ index ], B115200);
    cfsetispeed(&tty_linux_data[ index ], cfgetospeed(&tty_linux_data[ index ]));

    tcflush( serial_fd[index], TCIFLUSH );
    if( tcsetattr( serial_fd[index], TCSANOW, &tty_linux_data[ index ] ) )
    {
        printf( "error\n" );
    }
}

static void console_drv_set_baud( uint32_t index, uint32_t baud )
{
    struct termios options;

    index = console_get_index( index );
    if( index == UNKNOWN_PORT_INDEX ) return;
    
    if( serial_fd[index] == 0 ) return;
    
    fcntl(serial_fd[index], F_SETFL, 0);    // clear all flags on descriptor, enable direct I/O
    
    tcgetattr(serial_fd[index], &options);   // read serial port options
    
    // enable receiver, set 8 bit data, ignore control lines
    options.c_cflag |= (CLOCAL | CREAD | CS8);
    
    // disable parity generation and 2 stop bits
    options.c_cflag &= ~(PARENB | CSTOPB);
    
    options.c_ispeed = baud;
    options.c_ospeed = baud;
    
    // set the new port options
    tcsetattr(serial_fd[index], TCSANOW, &options);
}

static bool_t validate( uint32_t index )
{
    if( console_get_index( index ) != UNKNOWN_PORT_INDEX ) return TRUE;
    return FALSE;
}

static uint32_t console_drv_ioctrl( uint32_t index, uint32_t param, void * data )
{
    uint32_t *p_baud;
    
	if( param == DEVICE_SET_CONFIG_DATA )
	{
		p_baud = ( uint32_t * )data;
        if( validate( index ) )
		{
			console_drv_set_baud( index, *p_baud );
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t console_device =
{
    "tty*",
    &_in_use,
    
    console_drv_init,
    console_drv_close,
    NULL,
    validate,
    NULL,
    console_drv_ioctrl,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    console_input,
    console_output,
    
};

DECLARE_DEVICE_SECTION( console_device );

