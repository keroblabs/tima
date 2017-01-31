#include "system.h"
#include "timer.h"
#include "tima_libc.h"

///////////////////////////////////////////////////////////////////////////

#define CONSOLE_BUFFER_SIZE             256
#define SOCKET_PARAM_LEN				128

///////////////////////////////////////////////////////////////////////////

static device_socket_t * p_socket;

///////////////////////////////////////////////////////////////////////////

/*
device range:

0...1023    : client sockets
1024...2048 : server sockets
2048...     : accepted sockets (from listening)

Client sockets behave like a tty device, and are opened like: /dev/socket?127.0.0.1:8000

Server sockets are opened like: /dev/socket?listen:8000.

Server sockets will return accepted sockets via ioctrl. The value returned will be 
used by Net library to create new device_t device and it will behave like a tty device.
*/

///////////////////////////////////////////////////////////////////////////

static void socket_net_global_init( void )
{
	p_socket = NULL;
}

static void socket_driver_local_init( void )
{
	device_t * socket_device_hdl;

	if( p_socket == NULL )
	{
		socket_device_hdl = device_open( "/dev/socket" );
		if( socket_device_hdl == NULL ) return;
		device_ioctrl( socket_device_hdl, DEVICE_GET_API, &p_socket );
		device_close( socket_device_hdl );

		p_socket->init();
	}
}

///////////////////////////////////////////////////////////////////////////

static uint32_t socket_driver_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
	return ( uint32_t )p_socket->write( index, buffer, size );
}

static uint32_t socket_driver_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
    return ( uint32_t )p_socket->read( index, buffer, size );
}

static void socket_driver_init( uint32_t index )
{
}

static void socket_driver_close( uint32_t index )
{
	p_socket->close( index );
}

static uint32_t socket_driver_ioctrl( uint32_t index, uint32_t param, void * value )
{
	uint32_t * p_socket_num = ( uint32_t * )value;
    bool_t * p_ready = ( bool_t * )value;

	if( param == (DEVICE_GET_RUNTIME_VALUE+1) )
	{
		*p_socket_num = p_socket->accept( index );
	}
    if( param == (DEVICE_GET_RUNTIME_VALUE+0) )
	{
		*p_ready = TRUE;
	}
	else if( param == DEVICE_SET_RUNTIME_VALUE )
	{
		p_socket->close( *p_socket_num );
	}
    return 0;
}

static void socket_driver_trig_tx( uint32_t index )
{
}

static int socket_driver_get_index( char * param )
{
	static char socket_param[SOCKET_PARAM_LEN+1];
	char * arg0, * arg1;
	bool_t is_listening;
	uint32_t ret;
	uint8_t ip_addr[4];
	uint32_t port_num;
	bool_t open_url = FALSE;

	if( param[0] != '?' ) return -1;

	socket_driver_local_init();
	if( p_socket == NULL ) return -1;

	is_listening = FALSE;

	strncpy( socket_param, &param[1], SOCKET_PARAM_LEN );
	arg0 = &socket_param[0];
	arg1 = strchr( arg0, ':' );
	if( arg1 == NULL ) return -1;
	*arg1 = 0;
	arg1++;

	if( !strcmp( arg0, "listen" ) )
	{
		// listening socket
		is_listening = TRUE;
	}
	else if( tima_str_to_ip( arg0, ip_addr ) == TRUE )
	{
		// valid ip address
	}
	else
	{
		// passed URL
		open_url = TRUE;
	}

	if( tima_is_numeric( arg1 ) == FALSE )
	{
		// invalid port number
		return -1;
	}

	// check port number
	port_num = (uint32_t)tima_atoi( arg1 );
	if( ( port_num == 0 ) || ( port_num >= 65536 ) ) return -1;

	if( is_listening == TRUE )
	{
		ret = p_socket->listen( port_num );
		if( ret == SOCKET_FAIL_LISTEN ) return -1;
	}
	else if( open_url == TRUE )
	{
		ret = p_socket->connect_url( arg0, port_num );
		if( ret == SOCKET_FAIL_OPEN ) return -1;
	}
	else
	{
		ret = p_socket->connect( ip_addr, port_num );
		if( ret == SOCKET_FAIL_OPEN ) return -1;
	}

	return (int)ret;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const device_data_t socket_net_device =
{
    "socket?",
    &_in_use,
    
    socket_driver_init,
    socket_driver_close,
    socket_driver_trig_tx,
    NULL,
    socket_driver_get_index,
    socket_driver_ioctrl,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    socket_driver_input,
    socket_driver_output,
    
};

DECLARE_INIT_POWERUP_SECTION( socket_net_global_init );
DECLARE_DEVICE_SECTION( socket_net_device );

