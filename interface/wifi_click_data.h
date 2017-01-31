#ifndef __WIFI_CLICK_DATA_h__
#define __WIFI_CLICK_DATA_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

enum
{
    WIFI_RECEIVE_IDLE,
    WIFI_RECEIVE_WAITING,
    WIFI_RECEIVE_RECEIVING,
    WIFI_RECEIVE_TAIL
};

///////////////////////////////////////////////////////////////////////////

// host -> module
#define WICOMM_SET_PWR_SAVE_MODE_MSG		102
#define WICOMM_PING_SEND_MSG				121
#define WICOMM_RESET_MSG					170
#define WICOMM_SET_ARP_TIME_MSG				173

#define WICOMM_SOCKET_CREATE_MSG			110
#define WICOMM_SOCKET_CLOSE_MSG				111
#define WICOMM_SOCKET_BIND_MSG				112
#define WICOMM_SOCKET_CONNECT_MSG			113
#define WICOMM_SOCKET_LISTEN_MSG			114
#define WICOMM_SOCKET_ACCEPT_MSG			115
#define WICOMM_SOCKET_SEND_MSG				116
#define WICOMM_SOCKET_RECV_MSG				117
#define WICOMM_SOCKET_SEND_TO_MSG			118
#define WICOMM_SOCKET_RECV_FROM_MSG			119

#define WICOMM_SOCKET_ALLOCATE_MSG			122

#define WICOMM_GET_VERSION_MSG				23

#define WICOMM_SET_IP_ADDRESS_MSG			41
#define WICOMM_SET_NETWORK_MASK_MSG			42
#define WICOMM_SET_GATEWAY_IP_ADDR_MSG		44

#define WICOMM_GET_NETWORK_STATUS			48
#define WICOMM_SET_MAC_ADDR_MSG				49
#define WICOMM_SET_CP_NETWORK_MODE_MSG		55
#define WICOMM_SET_REGIONAL_DOMAIN_MSG		56
#define WICOMM_SET_CP_SSID_MSG				57

#define WICOMM_SET_CHANNEL_LIST_MSG			58
#define WICOMM_SET_LIST_RETRY_CNT_MSG		59
#define WICOMM_SET_CP_SECURITY_OPEN_MSG		65
#define WICOMM_SET_CP_SECURITY_WEP40_MSG	66
#define WICOMM_SET_CP_SECURITY_WEP104_MSG	67
#define WICOMM_SET_CP_SECURITY_WPA_MSG		68
#define WICOMM_GET_CP_WPAKEY_MSG			71

#define WICOMM_SCAN_START_MSG				80
#define WICOMM_SCAN_GET_RESULTS_MSG			81

// module -> host
#define WICOMM_ACK_MSG						0
#define WICOMM_EVENT_MSG					1

#define WICOMM_SCAN_RESULT_MSG				22

#define WICOMM_NETWORK_STATUS_RESP_MSG		48
#define WICOMM_WAPKEY_RESPONSE_MSG			49

#define WICOMM_WIFI_CONNECT_MSG				90
#define WICOMM_WIFI_DISCONNECT_MSG			91

#define WICOMM_SOCKET_CREATE_RESP_MSG		23
#define WICOMM_SOCKET_BIND_RES_MSG			24
#define WICOMM_SOCKET_CONNECT_RESP_MSG		25
#define WICOMM_SOCKET_LISTEN_RESP_MSG		26
#define WICOMM_SOCKET_ACCEPT_RESP_MSG		27
#define WICOMM_SOCKET_SEND_RESP_MSG			28
#define WICOMM_SOCKET_RECV_RESP_MSG			29
#define WICOMM_SOCKET_SENT_TO_RESP_MSG		30
#define WICOMM_SOCKET_RECV_FROM_RESP_MSG	31

#define WICOMM_SOCKET_ALLOCATE_RESP_MSG		32

#define WICOMM_EVENT_MSG_STARTUP_LEN        6

#define WICOMM_EVENT_ID_IP_CONNECTED        17
#define WICOMM_EVENT_ID_STARTUP             27
#define WICOMM_EVENT_ID_IP_ASSIGNED         16
#define WICOMM_EVENT_ID_ERROR               255
#define WICOMM_EVENT_WIFI_CONNECTION        8
#define WICOMM_EVENT_WIFI_SCAN_READY        9
#define WICOMM_EVENT_PING_RESPONSE          26

#define WICOMM_CP_PROFILE_1                 1
#define WICOMM_CP_PROFILE_2                 2

#define WICOMM_CP_WPA_ASCII_HIGHEST         8

#define WICOMM_CP_WEP_OPEN_KEY              0
#define WICOMM_CP_WEP_SHARED_KEY            1

#define WICOMM_CP_WEP40_KEY_LEN             5
#define WICOMM_CP_WEP104_KEY_LEN            13

#define WICOMM_SOCKET_TCP_MODE              1

#define WICOMM_SOCKET_DEFAULT_BACKLOG       20
#define WICOMM_SOCKET_DEFAULT_SIZE          256

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

void wifi_click_get_version( void );
void wifi_click_set_dhcp( void );
void wifi_click_get_network_status( void );
void wifi_click_setup_cp_security( void );

void wifi_click_debug_process( void );

void wifi_click_get_scan_result( uint8_t index );
void wifi_click_process_scan_item( uint8_t * p_data );

void wifi_click_socket_send_process( void );

///////////////////////////////////////////////////////////////

#endif // __WIFI_CLICK_DATA_h__
