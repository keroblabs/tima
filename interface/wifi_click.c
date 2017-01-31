#include "system.h"
#include "timer.h"
#include "pipe.h"
#include "message.h"

#include "debug.h"

#include "wifi_click.h"
#include "wifi_click_data.h"

///////////////////////////////////////////////////////////////////////////

#define WIFI_SEND_COMMAND_DELAY_MS          200

#define WIFI_HEADER0						0x55
#define WIFI_HEADER1						0xAA
#define WIFI_TAIL							0x45

#define PIGGYBACKACK_BIT                    0x8000

#define WIFI_ENABLE_DHCP                    0x0000
#define WIFI_ENABLE_STATIC_IP               0x0100
#define WIFI_ENABLE_FULL_SCAN               0x00FF

#define DEV_WIFI_CLICK                      "/dev/tty15"

///////////////////////////////////////////////////////////////////////////

enum
{
    Wifi_No_Command,
    
    Wifi_Command_Reset,
    Wifi_Command_Set_DHCP,
    Wifi_Command_Start_Scan,
    Wifi_Command_Get_Version,
    Wifi_Command_Get_Network_Status,
    Wifi_Command_CP_Set_SSID,
    Wifi_Command_CP_Security,
    Wifi_Command_CP_Connect,
    
    Wifi_Command_Socket_Create,
    Wifi_Command_Socket_Bind,
    Wifi_Command_Socket_Listen,
    Wifi_Command_Socket_Accept,
    Wifi_Command_Socket_Connect,
    
    Wifi_Command_Get_Scan_Result,
    Wifi_Command_Get_Scan_Result_Max = Wifi_Command_Get_Scan_Result+100,
    
};

enum
{
    Wifi_Message_Idle = Message_User_Defined,
    Wifi_Message_Received,
};

enum
{
    APP_Wifi_Init,
    APP_Wifi_Ready,
    APP_Wifi_Scan,
    APP_Wifi_Set_CP,
    APP_Wifi_Connect,
    APP_Wifi_Connected,
};

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

typedef struct _wifi_data_t_
{
	uint8_t header0;
	uint8_t header1;
	uint16_t msg_type;
	uint16_t msg_len;
	uint8_t data;

} wifi_data_t;

typedef struct _wifi_resp_t_
{
    uint16_t msg_type;
    uint8_t data;
    
} wifi_resp_t;

///////////////////////////////////////////////////////////////////////////

static device_t * wifi_dev;
static uint16_t last_command;

static uint8_t wifi_recv_mode;
static uint8_t wifi_header[6];
static uint8_t header_offset;
static uint8_t header_size;
static uint8_t init_cmd_counter;
static uint8_t wifi_scan_total;
static uint8_t wifi_scan_cnt;
static uint8_t wifi_curr_state;

static bool_t wifi_init_flag;

static uint8_t * p_wifi_buffer;
static uint8_t * p_wifi_rx;

static char * connect_key;
static uint8_t security_mode;

static uint32_t wifi_rx_cnt;

static timer_data_t send_cmd_ms;
static uint16_t wifi_command;

static pipe_data_t pipe_rx_wifi;
static pipe_data_t pipe_tx_wifi;

static wifi_ssid_list_t * p_scan_first;
//static message_data_t wifi_message;

static uint32_t last_created_socket;
static uint8_t last_connected_ip[4];

static uint8_t app_wifi_state;
static wifi_ssid_list_t * app_ssid_item;

static uint32_t socket_send_pointer;
static uint32_t socket_send_size;
static uint8_t * socket_send_buffer;

///////////////////////////////////////////////////////////////////////////

static void wifi_click_send_command( uint16_t msg, uint8_t * data, uint16_t size )
{
	uint8_t * tx_buffer;
	uint32_t pack_len;
	wifi_data_t * wifi_data;

	// set state machine to waiting mode
	wifi_recv_mode = WIFI_RECEIVE_WAITING;

	// prepare package
	pack_len = 7 + size;
	tx_buffer = ( uint8_t * )MMALLOC( pack_len );
	if( tx_buffer == NULL ) return;

	wifi_data = ( wifi_data_t * )tx_buffer;
	wifi_data->header0 = WIFI_HEADER0;
	wifi_data->header1 = WIFI_HEADER1;
	wifi_data->msg_type = msg;
	wifi_data->msg_len = ( uint16_t )size;

    if( data ) memcpy( &wifi_data->data, data, size );
	tx_buffer[pack_len-1] = WIFI_TAIL;

	// send to the module
	device_write_buffer( wifi_dev, tx_buffer, pack_len );
	MFREE( tx_buffer );
    
    header_size = ( uint8_t )sizeof( wifi_header );
    header_offset = 0;
    
    wifi_curr_state = Wifi_State_Busy;
}

static void wifi_click_receive_process( void )
{
	wifi_data_t * wifi_data = NULL;
	uint32_t rx_len = 0;
	uint8_t wifi_tail;
	timer_data_t flush_ms;
    
    if( wifi_recv_mode == WIFI_RECEIVE_IDLE )
    {
        // if we detect anything received in idle mode, start receiving
        if( device_is_rx_pending( wifi_dev ) )
        {
            // there is more pending message
            wifi_recv_mode = WIFI_RECEIVE_WAITING;
            
            // check for funny zero data
            device_read_buffer( wifi_dev, &wifi_header[header_offset], 1 );
            if( wifi_header[header_offset] != 0 )
            {
                header_offset++;
                header_size--;
            }
        }
    }

	if( wifi_recv_mode == WIFI_RECEIVE_WAITING )
	{
		// receive the header first
		if( device_read_buffer( wifi_dev, &wifi_header[header_offset], header_size ) >= header_size )
		{
			// buffer header has been received
			wifi_data = ( wifi_data_t * )wifi_header;

			// check header
			if( ( wifi_data->header0 != WIFI_HEADER0 ) || ( wifi_data->header1 != WIFI_HEADER1 ) )
			{
				timer_Start( &flush_ms, 300 );

				// flush data
				while( ( device_read_buffer( wifi_dev, ( uint8_t * )&rx_len, sizeof( rx_len ) ) ) ||
					   ( timer_Check( &flush_ms ) == 0 ) )
				{
					SYSTEM_EVENTS();
				}

                wifi_recv_mode = WIFI_RECEIVE_IDLE;
				return;
			}
            
            // clear the piggybackack bit
            wifi_data->msg_type &= ~PIGGYBACKACK_BIT;
            
			// allocate space for payload plus message type
			p_wifi_buffer = ( uint8_t * )MMALLOC( wifi_data->msg_len + sizeof( uint16_t ) );
			if( p_wifi_buffer == NULL ) return;

			// copy message type to the beginning of the buffer
			memcpy( p_wifi_buffer, &wifi_data->msg_type, sizeof( uint16_t ) );

            if( wifi_data->msg_len )
            {
                // and point the payload space
                p_wifi_rx = &p_wifi_buffer[2];
                wifi_rx_cnt = wifi_data->msg_len;
                
                // start receiving data
                wifi_recv_mode = WIFI_RECEIVE_RECEIVING;
            }
            else
            {
                // start receiving data
                wifi_recv_mode = WIFI_RECEIVE_TAIL;
            }
		}
	}

	if( wifi_recv_mode == WIFI_RECEIVE_RECEIVING )
	{
		wifi_data = ( wifi_data_t * )wifi_header;

		if( wifi_data->msg_len == 0 )
		{
			// empty message, send to pipe and finish
			pipe_send_buffer( &pipe_rx_wifi, ( uint8_t * )&wifi_data->msg_type, sizeof( uint16_t ) );
            wifi_recv_mode = WIFI_RECEIVE_IDLE;
		}
		else
		{
			// start receiving the payload
			rx_len = device_read_buffer( wifi_dev, p_wifi_rx, wifi_rx_cnt );
			p_wifi_rx += rx_len;
			wifi_rx_cnt -= rx_len;
			if( wifi_rx_cnt == 0 )
			{
				wifi_recv_mode = WIFI_RECEIVE_TAIL;
			}
		}
	}

	if( ( wifi_recv_mode == WIFI_RECEIVE_TAIL ) && ( wifi_data != NULL ) )
	{
		// after payload, receive tail
		if( device_read_buffer( wifi_dev, &wifi_tail, 1 ) )
		{
			// send payload with message type over the pipe
			pipe_write( &pipe_rx_wifi, p_wifi_buffer, wifi_data->msg_len + sizeof( uint16_t ) );
            wifi_recv_mode = WIFI_RECEIVE_IDLE;
		}
	}
}

void wifi_click_debug_process( void )
{
    wifi_resp_t * wifi_resp;
    uint8_t * p_data;
    uint16_t buffer_len = 0;
    uint8_t event_id;
    
#ifdef TIMA_DEBUG
    uint16_t cnt;
#endif
    
    if( pipe_is_empty( &pipe_rx_wifi ) ) return;
    
    wifi_resp = ( wifi_resp_t * )pipe_read( &pipe_rx_wifi, &buffer_len );
    
    // discount message type
    event_id = 0;
    buffer_len -= 2;
    p_data = &wifi_resp->data;
    if( ( wifi_resp->msg_type == WICOMM_EVENT_MSG ) && buffer_len )
    {
        buffer_len--;
        event_id = *p_data;
        p_data++;
    }
    
#ifdef TIMA_DEBUG
    DEBUG_PRINTK( "WIFI resp: %04x (%d)\r\n", wifi_resp->msg_type, buffer_len );
    
    if( buffer_len )
    {
        if( wifi_resp->msg_type == WICOMM_EVENT_MSG )
        {
            DEBUG_PRINTK( "[%02d] ", event_id );
        }
        
        for( cnt = 0; cnt < buffer_len; cnt++ )
        {
            DEBUG_PRINTK( "%02X ", p_data[cnt] );
        }
        
        DEBUG_PRINTK( "\r\n" );
    }
#endif
    
    if( wifi_resp->msg_type == WICOMM_ACK_MSG )
    {
        switch( last_command )
        {
            case Wifi_Command_Set_DHCP:
                // inform application that WifiClick is ready
                wifi_curr_state = Wifi_State_Startup_Ready;
                break;
                
            case Wifi_Command_CP_Set_SSID:
                // as part of CP setting, after SSID is set, set security
                wifi_command = Wifi_Command_CP_Security;
                timer_Reload( &send_cmd_ms );
                break;
                
            case Wifi_Command_CP_Security:
                // inform application that WifiClick is ready
                wifi_curr_state = Wifi_State_Setup_Done;
                break;
                
            case Wifi_Command_CP_Connect:
                // inform application that WifiClick is ready
                wifi_curr_state = Wifi_State_Connecting;
                break;
        }
    }
    else if( wifi_resp->msg_type == WICOMM_SOCKET_SEND_RESP_MSG )
    {
        // send any pending packet
        wifi_click_socket_send_process();
    }
    else if( wifi_resp->msg_type == WICOMM_NETWORK_STATUS_RESP_MSG )
    {
        // ignore this message because it does not contain valid IP
    }
    else if( wifi_resp->msg_type == WICOMM_SOCKET_CREATE_RESP_MSG )
    {
        // get created socket handler
        last_created_socket = p_data[0];
        
        // inform application that WifiClick is ready
        wifi_curr_state = Wifi_State_Socket_Created;
    }
    else if( wifi_resp->msg_type == WICOMM_SOCKET_LISTEN_RESP_MSG )
    {
        // inform application that WifiClick is ready
        wifi_curr_state = Wifi_State_Socket_Listening;
    }
    else if( wifi_resp->msg_type == WICOMM_SOCKET_BIND_RES_MSG )
    {
        // check bind result
        if( p_data[2] == 0 )
        {
            // inform application that WifiClick is ready
            wifi_curr_state = Wifi_State_Socket_Binded;
        }
        else
        {
            // inform application that WifiClick failed
            wifi_curr_state = Wifi_State_Error;
        }
    }
    else if( wifi_resp->msg_type == WICOMM_SOCKET_ACCEPT_RESP_MSG )
    {
        // inform application that WifiClick is ready
        wifi_curr_state = Wifi_State_Socket_Accepted;
    }
    else if( wifi_resp->msg_type == WICOMM_SOCKET_CONNECT_RESP_MSG )
    {
        // inform application that WifiClick is ready
        wifi_curr_state = Wifi_State_Socket_Connected;
    }
    else if( wifi_resp->msg_type == WICOMM_SCAN_RESULT_MSG )
    {
        // process the received scan item
        wifi_click_process_scan_item( p_data );
    }
    else if( wifi_resp->msg_type == WICOMM_EVENT_MSG )
    {
        switch( event_id )
        {
            case WICOMM_EVENT_ID_STARTUP:
                // EVENT_MSG – STARTUP EVENT received
                if( last_command == Wifi_Command_Reset ) wifi_command = Wifi_Command_Get_Version;
                else if( last_command == Wifi_Command_Get_Version ) wifi_command = Wifi_Command_Set_DHCP;
                
                timer_Reload( &send_cmd_ms );
                break;
                
            case WICOMM_EVENT_WIFI_SCAN_READY:
                // save the total of scanned items
                wifi_scan_total = p_data[0];
                wifi_scan_cnt = 0;
                
                // request first item
                wifi_command = Wifi_Command_Get_Scan_Result+wifi_scan_cnt;
                wifi_scan_cnt++;
                timer_Reload( &send_cmd_ms );
                break;
                
            case WICOMM_EVENT_WIFI_CONNECTION:
                // as part of connecting procedures, after the event is received,
                // request network details
                wifi_command = Wifi_Command_Get_Network_Status;
                timer_Reload( &send_cmd_ms );
                break;
                
            case WICOMM_EVENT_ID_IP_ASSIGNED:
                // get IP address
                memcpy( last_connected_ip, &p_data[1], 4 );
                
                // inform application that WifiClick is ready
                wifi_curr_state = Wifi_State_Connected;
                break;
        }
    }
}

void wifi_click_send_command_process( void )
{
    if( !timer_Check( &send_cmd_ms ) ) return;
    timer_Stop( &send_cmd_ms );
    
    switch( wifi_command )
    {
        case Wifi_Command_Get_Network_Status:
            wifi_click_get_network_status();
            break;
            
        case Wifi_Command_Get_Version:
            wifi_click_get_version();
            break;
            
        case Wifi_Command_Set_DHCP:
            wifi_click_set_dhcp();
            break;
            
        case Wifi_Command_CP_Security:
            wifi_click_setup_cp_security();
            break;
            
        case Wifi_Command_Reset:
            wifi_click_reset();
            break;
    }
    
    if( ( wifi_command >= Wifi_Command_Get_Scan_Result ) &&
       ( wifi_command < Wifi_Command_Get_Scan_Result_Max ) )
    {
        wifi_click_get_scan_result( wifi_command - Wifi_Command_Get_Scan_Result );
    }
    
	last_command = wifi_command;
    wifi_command = Wifi_No_Command;
}

void wifi_click_process_scan_item( uint8_t * p_data )
{
    wifi_ssid_list_t * p_scan_entry;
    
    // process received item
    p_scan_entry = ( wifi_ssid_list_t * )MMALLOC( sizeof( wifi_ssid_list_t ) );
    if( p_scan_entry == NULL ) return;
    
    p_scan_entry->ssid = ( char * )MMALLOC( p_data[6]+1 );
    if( p_scan_entry->ssid == NULL )
    {
        MFREE( p_scan_entry );
        return;
    }
    
    memcpy( p_scan_entry->ssid, &p_data[7], p_data[6] );
    p_scan_entry->ssid[p_data[6]] = 0;
    
    p_scan_entry->rssi = p_data[52];
    p_scan_entry->security_mask = p_data[39];
    
    if( p_scan_first != NULL ) p_scan_entry->p_next = p_scan_first;
    p_scan_first = p_scan_entry;
    
    // debug
    DEBUG_PRINTK( "SSID = %s\r\n", p_scan_entry->ssid );
    
    // is there more results?
    if( wifi_scan_cnt < wifi_scan_total )
    {
        timer_Reload( &send_cmd_ms );
        
        // get more results
        wifi_command = Wifi_Command_Get_Scan_Result+wifi_scan_cnt;
        wifi_scan_cnt++;
    }
    else
    {
        // inform application that scan has finished
        wifi_curr_state = Wifi_State_Scan_Ready;
    }
}

void wifi_click_connect_cp( void )
{
    uint8_t msg[2];
    
    msg[0] = WICOMM_CP_PROFILE_1;
    msg[1] = 0;
    
    // setup command for recv process
    last_command = Wifi_Command_CP_Connect;
    wifi_command = Wifi_No_Command;
    
    wifi_click_send_command( WICOMM_WIFI_CONNECT_MSG, msg, sizeof( msg ) );
}

void wifi_click_setup_cp_security( void )
{
    uint8_t * security_msg;
    uint32_t len;
    uint32_t msg_len;

    if( security_mode & 0xC0 )
    {
        // WPA or WPA2
        len = (uint32_t)strlen( connect_key );
        security_msg = ( uint8_t * )MMALLOC( len + 4 );
        if( security_msg == NULL ) return;
        
        security_msg[0] = WICOMM_CP_PROFILE_1;
        security_msg[1] = WICOMM_CP_WPA_ASCII_HIGHEST;
        security_msg[2] = 0;
        security_msg[3] = len;
        memcpy( &security_msg[4], connect_key, len );
        wifi_click_send_command(WICOMM_SET_CP_SECURITY_WPA_MSG, security_msg, len+4 );
    }
    else if( security_mode & 0x10 )
    {
        // WEP
        len = (uint32_t)strlen( connect_key );
        
        if( len == 13 )
        {
            // WEP104
            msg_len = ( WICOMM_CP_WEP104_KEY_LEN * 4 ) + 4;
        }
        else if( len == 5 )
        {
            // WEP40
            msg_len = ( WICOMM_CP_WEP40_KEY_LEN * 4 ) + 4;
        }
        
        security_msg = ( uint8_t * )MMALLOC( msg_len );
        if( security_msg == NULL ) return;
        memset( security_msg, 0x00, msg_len );
        
        if( security_mode & 0x01 ) security_msg[1] = WICOMM_CP_WEP_SHARED_KEY;
        else security_msg[1] = WICOMM_CP_WEP_OPEN_KEY;
        
        security_msg[0] = WICOMM_CP_PROFILE_1;
        security_msg[2] = 0;
        security_msg[3] = 0;
        memcpy( &security_msg[4], connect_key, len );
        
        if( len == 13 )
        {
            // WEP104
            wifi_click_send_command(WICOMM_SET_CP_SECURITY_WEP104_MSG, security_msg, msg_len );
        }
        else if( len == 5 )
        {
            // WEP40
            wifi_click_send_command(WICOMM_SET_CP_SECURITY_WEP40_MSG, security_msg, msg_len );
        }
    }
}

void wifi_click_socket_send_process( void )
{
    uint8_t * tx_data;
    uint16_t tx_size = 0;
    
    uint8_t * msg;
    uint8_t * buffer;
    uint32_t send_size;
    uint32_t socket;
    
    if( pipe_is_empty( &pipe_tx_wifi ) ) return;
    
    tx_data = pipe_read( &pipe_tx_wifi, &tx_size );
    send_size = tx_size-1;
    socket = ( uint32_t )tx_data[0];
    buffer = tx_data+1;
    
    msg = ( uint8_t * )MMALLOC( send_size+4 );
    if( msg == NULL ) return;
    
    msg[0] = ( uint8_t )socket;
    msg[1] = 0;
    msg[2] = ( uint8_t )send_size;
    msg[3] = 0;
    
    memcpy( &msg[4], tx_data, send_size );
    
    wifi_click_send_command( WICOMM_SOCKET_SEND_MSG, msg, send_size+4 );
    MFREE( msg );
    MFREE( tx_data );
}

void wifi_click_init( void )
{
    uint32_t baud = 115200;
    
    wifi_dev = device_open( DEV_WIFI_CLICK );
    device_ioctrl( wifi_dev, DEVICE_SET_CONFIG_DATA, &baud );
    
    timer_Start( &send_cmd_ms, WIFI_SEND_COMMAND_DELAY_MS );
    timer_Stop( &send_cmd_ms );
    
    pipe_init( &pipe_rx_wifi, "PIPE_WIFI_RX", 8 );
    
    header_size = ( uint8_t )sizeof( wifi_header );
    header_offset = 0;
    
    wifi_recv_mode = WIFI_RECEIVE_IDLE;
    init_cmd_counter = 0;
    wifi_scan_cnt = 0;
    wifi_command = Wifi_No_Command;
    last_command = Wifi_No_Command;
    wifi_curr_state = Wifi_State_Idle;
    
    p_scan_first = NULL;
    connect_key = NULL;
    socket_send_buffer = NULL;
    
    wifi_init_flag = TRUE;
    
    wifi_command = Wifi_Command_Reset;
    timer_Reload( &send_cmd_ms );
    
    app_wifi_state = APP_Wifi_Init;
    app_ssid_item = NULL;
}

void wifi_click_process( void )
{
    wifi_click_send_command_process();
    wifi_click_receive_process();
    wifi_click_debug_process();
    wifi_click_socket_send_process();
}

uint8_t wifi_click_get_curr_state( void )
{
    return wifi_curr_state;
}

wifi_ssid_list_t * wifi_click_get_scan_item( uint8_t index )
{
    wifi_ssid_list_t * search = p_scan_first;
    uint8_t cnt = 0;
    
    while( search != NULL )
    {
        if( cnt == index ) return search;
        search = search->p_next;
        cnt++;
    }
    
    return NULL;
}

void wifi_click_setup_connection( wifi_ssid_list_t * p_item, char * key )
{
    uint8_t * set_ssid_msg;
    uint32_t len;
    
    if( connect_key != NULL )
    {
        MFREE( connect_key );
        connect_key = NULL;
    }
    
    security_mode = p_item->security_mask & 0xF0;
    
    // store the connection key
    connect_key = ( char * )MMALLOC( (int)strlen( key ) + 1 );
    if( connect_key == NULL ) return;
    strcpy( connect_key, key );
    
    // create SSID message
    len = (uint32_t)strlen( p_item->ssid );
    set_ssid_msg = ( uint8_t * )MMALLOC( len + 2 );
    if( set_ssid_msg == NULL )
    {
        MFREE( connect_key );
        return;
    }
    
    // setup command for recv process
    last_command = Wifi_Command_CP_Set_SSID;
    wifi_command = Wifi_No_Command;

    // create SSID message and send
    set_ssid_msg[0] = WICOMM_CP_PROFILE_1;
    set_ssid_msg[1] = (uint8_t)len;
    memcpy( &set_ssid_msg[2], p_item->ssid, strlen( p_item->ssid ) );
    wifi_click_send_command(WICOMM_SET_CP_SSID_MSG, set_ssid_msg, len+2 );
    MFREE( set_ssid_msg );
}

uint32_t wifi_click_get_created_socket( void )
{
    return last_created_socket;
}

void wifi_click_get_connected_ip( uint8_t * ip )
{
    memcpy( ip, last_connected_ip, 4 );
}

void wifi_click_reset( void )
{
    wifi_click_send_command(WICOMM_RESET_MSG, NULL, 0);
}

void wifi_click_get_version( void )
{
    wifi_click_send_command(WICOMM_GET_VERSION_MSG, NULL, 0);
}

void wifi_click_get_network_status( void )
{
    wifi_click_send_command(WICOMM_GET_NETWORK_STATUS, NULL, 0);
}

void wifi_click_set_dhcp( void )
{
    uint8_t msg[17];
    
    memset( msg, 0x00, sizeof( msg ) );
    
    msg[1] = WIFI_ENABLE_DHCP;
    
    wifi_click_send_command(WICOMM_SET_IP_ADDRESS_MSG, msg, sizeof( msg ) );
}

void wifi_click_get_scan_result( uint8_t index )
{
    uint8_t msg[2];
    
    msg[0] = index;
    msg[1] = 0;
    
    wifi_click_send_command( WICOMM_SCAN_GET_RESULTS_MSG, msg, sizeof( msg ) );
}

void wifi_click_socket_create( void )
{
    uint8_t msg[2];
    
    msg[0] = WICOMM_SOCKET_TCP_MODE;
    msg[1] = 0;
    
    wifi_click_send_command( WICOMM_SOCKET_CREATE_MSG, msg, sizeof( msg ) );
}

void wifi_click_socket_bind( uint32_t socket, uint16_t port_num )
{
    uint8_t msg[4];
    
    msg[0] = ( port_num & 0x0FF );
    msg[1] = ( port_num >> 8 );
    msg[2] = (uint8_t)socket;
    msg[3] = 0;
    
    wifi_click_send_command( WICOMM_SOCKET_BIND_MSG, msg, sizeof( msg ) );
}

void wifi_click_socket_listen( uint32_t socket )
{
    uint8_t msg[2];
    
    msg[0] = ( uint8_t )socket;
    msg[1] = WICOMM_SOCKET_DEFAULT_BACKLOG;
    
    wifi_click_send_command( WICOMM_SOCKET_LISTEN_MSG, msg, sizeof( msg ) );
}

void wifi_click_socket_is_accept( uint32_t socket )
{
    uint8_t msg[2];
    
    msg[0] = ( uint8_t )socket;
    msg[1] = 0;
    
    wifi_click_send_command( WICOMM_SOCKET_ACCEPT_MSG, msg, sizeof( msg ) );
}

void wifi_click_socket_send_data( uint32_t socket, uint8_t * buffer, uint32_t size )
{
    uint8_t * msg;
    uint32_t curr_pos;
    uint32_t curr_size;
    uint32_t pipe_size;
    
    curr_pos = 0;
    curr_size = size;
    
    while( curr_size )
    {
        pipe_size = curr_size;
        if( pipe_size > WICOMM_SOCKET_DEFAULT_SIZE ) pipe_size = WICOMM_SOCKET_DEFAULT_SIZE;
        
        msg = ( uint8_t * )MMALLOC( pipe_size+1 );
        if( msg == NULL ) break;

        msg[0] = ( uint8_t )socket;
        memcpy( &msg[1], &buffer[curr_pos], pipe_size );
        
        pipe_write( &pipe_tx_wifi, msg, curr_size );
        
        curr_pos += pipe_size;
        curr_size -= pipe_size;
    }
    
    wifi_click_socket_send_process();
}

void wifi_click_socket_connect( uint32_t socket, uint8_t * ip, uint16_t port_num )
{
    uint8_t msg[20];
    
    memset( msg, 0x00, sizeof( msg ) );
    
    msg[0] = ( uint8_t )socket;
    msg[1] = 0;
    msg[2] = ( port_num & 0x0FF );
    msg[3] = ( port_num >> 8 );
    
    memcpy( &msg[4], ip, 4 );
    
    wifi_click_send_command( WICOMM_SOCKET_CONNECT_MSG, msg, sizeof( msg ) );
}

void wifi_click_start_scan( void )
{
    uint8_t msg[2];
    wifi_ssid_list_t * p_search;
    wifi_ssid_list_t * p_next;
    
    if( p_scan_first != NULL )
    {
        // erase previous scan list
        p_search = p_scan_first;
        
        while( p_search != NULL )
        {
            p_next = p_search->p_next;
            
            MFREE( p_search->ssid );
            MFREE( p_search );
            
            p_search = p_next;
        }
        p_scan_first = NULL;
    }
    
    wifi_scan_cnt = 0;
    
    msg[0] = WIFI_ENABLE_FULL_SCAN;
    msg[1] = 0x0000;

    wifi_click_send_command(WICOMM_SCAN_START_MSG, msg, sizeof( msg ) );
}

void wifi_click_application_process( void )
{
    if( app_wifi_state == APP_Wifi_Init )
    {
        if( wifi_click_get_curr_state() == Wifi_State_Startup_Ready )
        {
            DEBUG_PRINTK( "WIFI Click ready\r\n" );
            app_wifi_state = APP_Wifi_Ready;
            
            DEBUG_PRINTK( "Scanning ...\r\n" );
            wifi_click_start_scan();
        }
    }
    
    if( app_wifi_state == APP_Wifi_Ready )
    {
        if( wifi_click_get_curr_state() == Wifi_State_Scan_Ready )
        {
            wifi_ssid_list_t * item;
            uint8_t cnt = 0;
            
            app_wifi_state = APP_Wifi_Scan;
            
            while( ( item = wifi_click_get_scan_item( cnt ) ) != NULL )
            {
                if( !strcmp( item->ssid, "MCHER" ) )
                {
                    wifi_click_setup_connection( item, "zoadorpitinga" );
                    app_ssid_item = item;
                    app_wifi_state = APP_Wifi_Set_CP;
                }
                DEBUG_PRINTK( "%d: SSID = %s (%d)\r\n", cnt+1, item->ssid, item->rssi );
                cnt++;
            }
            
            if( app_wifi_state == APP_Wifi_Set_CP )
            {
                DEBUG_PRINTK( "Found %s\r\n", app_ssid_item->ssid );
                
            }
        }
    }
    
    if( app_wifi_state == APP_Wifi_Set_CP )
    {
        if( wifi_click_get_curr_state() == Wifi_State_Setup_Done )
        {
            DEBUG_PRINTK( "Connecting ...\r\n" );
            wifi_click_connect_cp();
            app_wifi_state = APP_Wifi_Connect;
        }
    }
    
    if( app_wifi_state == APP_Wifi_Connect )
    {
        if( wifi_click_get_curr_state() == Wifi_State_Connected )
        {
            uint8_t ip[4];
            wifi_click_get_connected_ip(ip);
            DEBUG_PRINTK( "Connected %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3] );
            app_wifi_state = APP_Wifi_Connected;
        }
    }
    
    wifi_click_process();
}

