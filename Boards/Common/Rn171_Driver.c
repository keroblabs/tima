#include "system.h"
#include "debug.h"
#include "hardware.h"

/////////////////////////////////////////////////////////////////////

// this driver only listen to one port and only accept one port
#define WIFI_SOCKET_LISTEN      0x334455
#define WIFI_SOCKET_ACCEPT      0x220099

#define NET_RX_BUFFER_SIZE      2048
#define WIFI_STACK_SIZE         2048

#define COM_RN171				"/dev/tty3"
#define WIFI_BUF_SIZE			1024

#define WIFI_TIME_CMD			400
#define WIFI_TIME_READY			3000
#define WIFI_TIME_LED			500
#define WIFI_CON_RATE			9600

#define DATALINK_TOKEN_OPEN     '*'
#define DATALINK_TOKEN_CLOSE    '@'
#define DATALINK_HEADER_CODE    0x5A

#define WEBSERVER_TIMEOUT		10000

#define WIFI_TOTAL_SECTIONS		7

#define DATALINK_AUTO_RESP		"OK\r\n"
#define WIFI_CLOSE_CONN			"close\r\n"
#define WIFI_EXIT_CONN			"exit\r\n"

#define WIFI_STATUS_PRINTF      DEBUG_PRINTK
//#define WIFI_STATUS_PRINTF(...)

//#define WIFI_PRINTF             DEBUG_PRINTK
#define WIFI_PRINTF(...)

// #define WIFI_DUMP_PRINTF        DEBUG_PRINTK
#define WIFI_DUMP_PRINTF(...)

/////////////////////////////////////////////////////////////////////

enum
{
    WIFI_LED_OFF,
    WIFI_LED_ON,
    WIFI_LED_INIT_PHASE,
    WIFI_LED_CONNECTED
            
};

enum
{
    WIFI_STATE_NONE,
    WIFI_STATE_IDLE,
    WIFI_STATE_INIT,
    WIFI_STATE_READY,

    WIFI_NORMAL_RX,
    WIFI_INTERNAL_OPEN,
    WIFI_INTERNAL_CLOSE,
    WIFI_INTERNAL_RESP,
    
    WIFI_STATE_ERROR,
};

/////////////////////////////////////////////////////////////////////

static const char * wifi_init_commands[] =
{
    "set comm open *\r\n\0",
    "set comm close @\r\n\0",
	"set comm remote 0\r\n\0",
	"set wlan auth 4\r\n\0",
	"set wlan phrase kwucyckd\r\n\0",		
	"set wlan join 0\r\n\0",
	"set ip dhcp 1\r\n\0",
	"join virginmedia2464632\r\n\0",
	""
};

static const char * wifi_stop_commands[] =
{
	"leave\r\n\0",
	""
};

static const char * wifi_close_commands[] =
{
	"close\r\n\0",
	""
};

/////////////////////////////////////////////////////////////////////

static char             wifi_listen_port_str[40];
static char				wifi_rx[WIFI_BUF_SIZE+1];
static thread_data_t    wifi_context;
static uint8_t          wifi_stack[ WIFI_STACK_SIZE ];
static File             hwlink_hdl;
static uint8_t          wifi_module_state;

static handler_t        gpio_handler;
static timer_data_t     led_timer;
static uint8_t          led_state;
static uint8_t          led_mode;
static uint32_t         led_on_time;
static uint32_t         led_off_time;
static uint32_t         wifi_port_num;

static bool_t           is_exit;

static uint8_t          net_rx_buffer[ NET_RX_BUFFER_SIZE ];
static buffer_data_t   	net_rx_buffer_data;

/////////////////////////////////////////////////////////////////////

void wifi_set_led_mode( uint32_t mode )
{
    led_mode = mode;
    
    switch( led_mode )
    {
        case WIFI_LED_OFF:
            // turn off LED
            gpio_reset( gpio_handler );
            return;
            
        case WIFI_LED_ON:
            // turn on LED
            gpio_set( gpio_handler );
            return;
            
        case WIFI_LED_INIT_PHASE:
            timer_Stop( &led_timer );
            led_on_time = 500;
            led_off_time = 500;
            led_state = 0;
            timer_Start( &led_timer, 50 );
            break;

        case WIFI_LED_CONNECTED:
            timer_Stop( &led_timer );
            led_on_time = 200;
            led_off_time = 1000;
            led_state = 0;
            timer_Start( &led_timer, 50 );
            break;
    }
}

void wifi_led_process( void )
{
    switch( led_mode )
    {
        case WIFI_LED_OFF:
        case WIFI_LED_ON:
            return;
    }
    
    if( timer_Check( &led_timer ) == TRUE )
    {
        if( led_state & 0x01 )
        {
            // turn on LED
            gpio_set( gpio_handler );
            timer_Start( &led_timer, led_on_time );
        }
        else
        {
            // turn off LED
            gpio_reset( gpio_handler );
            timer_Start( &led_timer, led_off_time );
        }

        led_state++;
    }    
}

void wifi_receive_process( void )
{
	uint16_t read_size;
	static char rx_buffer[20];
    
    if( file_feof( &hwlink_hdl ) == FALSE )
    {
		// clear buffer
		memset( rx_buffer, 0x00, sizeof( rx_buffer ) );

		// shift buffer
		read_size = device_total_rx_data( &hwlink_hdl );
		if( read_size > 18 ) read_size = 18;
		
		memcpy( wifi_rx, &wifi_rx[read_size], WIFI_BUF_SIZE - read_size );

		// receive data
		device_read_buffer( &hwlink_hdl, rx_buffer, read_size );
		strcat( wifi_rx, rx_buffer );

		// print data
		WIFI_PRINTF( "%s", rx_buffer );
    }
}

void wifi_wait_time( uint32_t time_ms )
{
	timer_data_t wifi_timer;
    
    led_state = 0;

    timer_Start( &wifi_timer, time_ms );
    
    while( timer_Check( &wifi_timer ) == FALSE )
    {
        wifi_receive_process();
        wifi_led_process();
        thread_yield();
    }
}

bool_t wifi_wait_for_data( int32_t timeout )
{
    timer_data_t wifi_timer;
    
    if( timeout == -1 )
    {
        timer_Start( &wifi_timer, 0 );
        timer_Stop( &wifi_timer );
    }
    else if( timeout > 0 )
    {
        timer_Start( &wifi_timer, ( uint32_t )timeout );
    }
    else
    {
        if( file_feof( &hwlink_hdl ) == TRUE ) return FALSE;
        else return TRUE;
    }
    
    while( ( file_feof( &hwlink_hdl ) == TRUE ) &&
           ( timer_Check( &wifi_timer ) == FALSE ) )
    {
        if( is_exit == TRUE ) return TRUE;
        wifi_led_process();
        thread_yield();
    }
    
    if( timer_Check( &wifi_timer ) == TRUE ) return FALSE;
    return TRUE;
}

void wifi_wait_for_string( char * str )
{
    while( strstr( wifi_rx, str ) == NULL )
    {
        wifi_receive_process();
        wifi_led_process();
        thread_yield();
    }
}

void wifi_execute_command_list( const char * commands[] )
{
    uint8_t cnt;
    
    cnt = 0;
    while( strlen( commands[cnt] ) > 1 )
    {
        file_fputs( &hwlink_hdl, commands[cnt] ); 
        cnt++;
        wifi_wait_time( WIFI_TIME_CMD );
    }   
}

void wifi_init_thread( void * args )
{
    int cnt;
    int len;
    uint32_t last_rx_ms;
    timer_data_t wifi_timer;
    uint8_t internal_cmd;
    
    WIFI_STATUS_PRINTF( "\r\nInitialization in progress\r\n" );    
    wifi_module_state = WIFI_STATE_INIT;
    wifi_set_led_mode( WIFI_LED_INIT_PHASE );

    // initialize WIFI module
    WIFI_PRINTF( "wait a bit before initialize\r\n" );
    wifi_wait_time( WIFI_TIME_CMD );
    
    // send token
    WIFI_PRINTF( "Set to command mode\r\n" );
    file_fputs( &hwlink_hdl, "$$$" );		

    // wait for a bit
    WIFI_PRINTF( "wait a bit\r\n" );
    wifi_wait_time( WIFI_TIME_CMD );   
    
    // received "CMD" string
    WIFI_PRINTF( "wait for CMD\r\n" );
    wifi_wait_for_string( "CMD" );
    WIFI_PRINTF( "\r\nModule is ready\r\n" );
    
    // set listening port
    sprintk( wifi_listen_port_str, "set ip localport %d\r\n\0", wifi_port_num );
    file_fputs( &hwlink_hdl, wifi_listen_port_str );
    wifi_wait_time( WIFI_TIME_CMD );
    
    // send wifi commands (should be from a file)
    wifi_execute_command_list( wifi_init_commands );
    
    // wait for module to listen
    timer_Start( &wifi_timer, 10000 );
    
    // wifi_wait_for_string( "Listen" );
    while( TRUE )
    {
        if( strstr( wifi_rx, "Listen" ) != NULL )
        {
            WIFI_STATUS_PRINTF( "\r\nModule is listening\r\n" );
            break;
        }
        else if( strstr( wifi_rx, "AUTH-ERR" ) != NULL )
        {
            WIFI_STATUS_PRINTF( "\r\nAuthentication error\r\n" );            
            wifi_set_led_mode( WIFI_LED_OFF );
            wifi_module_state = WIFI_STATE_ERROR;
            return;
        }
        else if( timer_Check( &wifi_timer ) == TRUE )
        {
            WIFI_STATUS_PRINTF( "\r\nConnection timeout\r\n" );            
            wifi_set_led_mode( WIFI_LED_OFF );
            wifi_module_state = WIFI_STATE_ERROR;
            return;
        }
        
        wifi_receive_process();
        wifi_led_process();
        thread_yield();
    }

    wifi_set_led_mode( WIFI_LED_ON );
    wifi_module_state = WIFI_STATE_READY;
    last_rx_ms = 0;
    
    // wifi process
    while( TRUE )
    {
        // wait until something comes through
        while( wifi_wait_for_data( NEG_U32 ) == FALSE ) {}
        
        if( is_exit == TRUE ) break;
        
        wifi_rx[0] = ( char )file_fgetc( &hwlink_hdl );
        internal_cmd = WIFI_NORMAL_RX;

        // check content
        if( wifi_rx[0] == DATALINK_TOKEN_OPEN )
        {
            last_rx_ms = timer_get_MS() - last_rx_ms;

            if( last_rx_ms > 1000 )
            {
                internal_cmd = wifi_module_state = WIFI_INTERNAL_OPEN; 
                
                WIFI_PRINTF( "Connection is Opened\r\n" );
                wifi_set_led_mode( WIFI_LED_CONNECTED );                    
                // wifi_wait_time( 250 );  
                timer_Start( &wifi_timer, 250 );
                thread_wait( timer_Check( &wifi_timer ) == TRUE );
            }
        }
        else if( wifi_rx[0] == DATALINK_TOKEN_CLOSE )
        {
            len = 1;
        	wifi_rx[1] = '@';

            if( wifi_wait_for_data(WIFI_TIME_CMD) == TRUE )
            {
                wifi_rx[1] = ( char )file_fgetc( &hwlink_hdl );
                len = 2;
            }
            
            // if received an internal command, there shouldnt be any other byte within a safe time
            if( ( wifi_wait_for_data(WIFI_TIME_CMD) == FALSE ) &&
            	( wifi_rx[0] == DATALINK_TOKEN_CLOSE ) &&
                ( wifi_rx[1] == DATALINK_TOKEN_CLOSE ) )
            {
                internal_cmd = WIFI_INTERNAL_CLOSE;
                wifi_module_state = WIFI_STATE_READY;
                
                WIFI_STATUS_PRINTF( "Connection is Closed\r\n" );
                wifi_set_led_mode( WIFI_LED_ON );
            }
            else
            {
                if( wifi_module_state == WIFI_INTERNAL_OPEN )
                {
                    buffer_write_buffer( &net_rx_buffer_data, ( uint8_t * )wifi_rx, len );       
                    
                    // make sure there is nothing else at the end of the loop
                    continue;
                }                
            }
        }

        if( internal_cmd == WIFI_NORMAL_RX )
        {
            len = file_fread( &wifi_rx[1], WIFI_BUF_SIZE-5, &hwlink_hdl );
            
            if( wifi_module_state == WIFI_INTERNAL_OPEN )
            {
                buffer_write_buffer( &net_rx_buffer_data, ( uint8_t * )wifi_rx, len+1 );       
            }
            
            // register last time we ve been here
            last_rx_ms = timer_get_MS();        

            for( cnt = 0; cnt <= len; cnt++ )
            {
                WIFI_DUMP_PRINTF( "%02X ", wifi_rx[cnt] );
            }
        }
    }
    
    // wait for a bit
    WIFI_PRINTF( "wait a bit\r\n" );
    wifi_wait_time( WIFI_TIME_CMD ); 
    
    // send token
    if( wifi_module_state != WIFI_STATE_READY )
    {
        WIFI_PRINTF( "Set to command mode\r\n" );
        file_fputs( &hwlink_hdl, "$$$" );		
        wifi_wait_time( WIFI_TIME_CMD ); 
    }
        
    // send wifi commands (should be from a file)
    wifi_execute_command_list( wifi_stop_commands );
    
    wifi_module_state = WIFI_STATE_IDLE;
    wifi_set_led_mode( WIFI_LED_OFF );
}

uint32_t wifi_socket_accept( uint32_t socket )
{
    if( wifi_module_state == WIFI_STATE_ERROR )
    {
        // something happened, what shall we do??
        return ( uint32_t )SOCKET_FAIL_LISTEN;
    }
    
    else if( wifi_module_state == WIFI_INTERNAL_OPEN )
    {
        return ( uint32_t )WIFI_SOCKET_ACCEPT;
    }
    
    // not connected yet
    return SOCKET_NOT_READY;    
}

uint32_t wifi_socket_listen( uint16_t port_num )
{
    if( wifi_module_state == WIFI_STATE_NONE )
    {
        return ( uint32_t ) SOCKET_FAIL_LISTEN;
    }
    
    wifi_port_num = port_num;
    is_exit = FALSE;
    thread_create( &wifi_context, wifi_init_thread, NULL, wifi_stack, WIFI_STACK_SIZE );
#if 0
    while( TRUE )
    {
        if( wifi_module_state == WIFI_STATE_READY )
        {
            // worked, now it's listening
            return ( uint32_t )WIFI_SOCKET_LISTEN;
        }
        else if( wifi_module_state == WIFI_STATE_ERROR )
        {
            // something happened, what shall we do??
            break;
        }

        thread_yield();
    }
    
    return ( uint32_t )SOCKET_FAIL_LISTEN;
#endif
    return ( uint32_t )WIFI_SOCKET_LISTEN;
}

void wifi_socket_close( uint32_t socket )
{
    if( socket == WIFI_SOCKET_ACCEPT )
    {
        if( wifi_module_state == WIFI_STATE_READY ) return;
        
        if( wifi_module_state == WIFI_INTERNAL_OPEN )
        {
            // wait for a bit
            WIFI_PRINTF( "wait a bit\r\n" );
            wifi_wait_time( WIFI_TIME_CMD );   
        
            // send token
            WIFI_PRINTF( "Set to command mode\r\n" );
            file_fputs( &hwlink_hdl, "$$$" );		
        }
        
        // wait for a bit
        WIFI_PRINTF( "wait a bit\r\n" );
        wifi_wait_time( WIFI_TIME_CMD );   
        
        // close this connection
        wifi_execute_command_list( wifi_close_commands );
        
        WIFI_STATUS_PRINTF( "Socket Closed\r\n" );
        wifi_set_led_mode( WIFI_LED_ON );
        wifi_module_state = WIFI_STATE_READY;
    }
    else if( socket == WIFI_SOCKET_LISTEN )
    {
        // force wifi disconnection
        is_exit = TRUE;
        
        while( TRUE )
        {
            if( wifi_module_state == WIFI_STATE_IDLE )
            {
                WIFI_STATUS_PRINTF( "Disconnected\r\n" );
                break;
            }
            
            thread_yield();
        }
    }
}

bool_t wifi_socket_is_open( uint32_t index )
{
    if( index == WIFI_SOCKET_ACCEPT )
    {
        if( wifi_module_state > WIFI_STATE_READY ) return TRUE;
    }
    else if( index == WIFI_SOCKET_LISTEN )
    {
        if( wifi_module_state > WIFI_STATE_IDLE ) return TRUE;
    }
    
    return FALSE;
}

int wifi_socket_receive( uint32_t index, uint8_t * buffer, uint32_t size )
{
    if( size == 0 )
    {
        return buffer_get_size( &net_rx_buffer_data );
    }
    
    return ( int )buffer_read_buffer( &net_rx_buffer_data, buffer, ( uint16_t )size );
}

int wifi_socket_transmit( uint32_t index, uint8_t * buffer, uint32_t size )
{
    uint32_t cnt;
    
    while( TRUE )
    {
        cnt = device_available_tx_data( &hwlink_hdl );
        if( cnt > size ) break;
        thread_yield();
    }
    
    for( cnt = 0; cnt <= size; cnt++ )
    {
        WIFI_DUMP_PRINTF( "[%02X] ", buffer[cnt] );
    }
    
    return ( int )file_fwrite( buffer, size, &hwlink_hdl );
}

uint32_t wifi_socket_connect( uint8_t * ip_addr, uint16_t port )
{
    return 0;
}

char * wifi_curr_ip( void )
{
    static const char curr_ip[] = "127.0.0.1";
    return ( char * )curr_ip;
}

void wifi_init( void )
{
    uint32_t baud = 9600;
    
    wifi_module_state = WIFI_STATE_NONE;
    if( file_fopen( &hwlink_hdl, COM_RN171, FS_MODE_DEVICE ) != FILE_OK ) return;
    file_fioctrl( &hwlink_hdl, DEVICE_SET_CONFIG_DATA, &baud );
    
    wifi_module_state = WIFI_STATE_IDLE;
    gpio_handler = gpio_get_port( GPIO_WIFI_LED );
    gpio_reset( gpio_handler );
    
    // init incoming buffer
    buffer_init( &net_rx_buffer_data, net_rx_buffer, NET_RX_BUFFER_SIZE );

    memset( wifi_rx, '-', sizeof( wifi_rx ) );
	wifi_rx[WIFI_BUF_SIZE-5] = 0;    
}

static const socket_api_t socket_api = 
{
    wifi_socket_listen,
    wifi_socket_connect,
    wifi_socket_accept,
    
    wifi_socket_receive,
    wifi_socket_transmit,
    
    wifi_socket_close,
    wifi_socket_is_open,
    wifi_curr_ip
};

uint32_t wifi_socket_get_api( uint32_t index, socket_api_t ** api, uint32_t size )
{
    if( api != NULL )
        *api = ( socket_api_t * )&socket_api;

	return sizeof( socket_api_t );
}

///////////////////////////////////////////////////////////////////////////

static bool_t _socket_in_use;

const static device_data_t socket_driver_device = 
{
    "socket",
    &_socket_in_use,
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    wifi_socket_get_api,
    NULL,
    
};

///////////////////////////////////////////////////////////////////////////

DECLARE_INIT_DEVICE_SECTION( wifi_init );
DECLARE_DEVICE_SECTION( socket_driver_device );

