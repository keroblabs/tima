#ifndef __WIFI_CLICK_h__
#define __WIFI_CLICK_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

enum
{
    Wifi_Error_No_Error,
    
    Wifi_Error_Already_Connected,
    Wifi_Error_Already_Disconnected,
    Wifi_Error_Close_Socket_Failed,
    Wifi_Error_Socket_Send_Timeout,
    Wifi_Error_Ping_Flood,
    Wifi_Error_Ping_In_Use,
    Wifi_Error_Socket_Receive_Failed,
    Wifi_Error_Socket_TX_Buffer,
    Wifi_Error_Scan_Mode_Busy,
    Wifi_Error_Security_Failed,

};

enum
{
    Wifi_State_Idle,
    Wifi_State_Busy,
    Wifi_State_Scan_Ready,
    Wifi_State_Startup_Ready,
    Wifi_State_Setup_Done,
    Wifi_State_Connecting,
    Wifi_State_Connected,
    
    Wifi_State_Socket_Created,
    Wifi_State_Socket_Binded,
    Wifi_State_Socket_Listening,
    Wifi_State_Socket_Connected,
    Wifi_State_Socket_Accepted,
    Wifi_State_Socket_NotReady,
    
    Wifi_State_Error,
};

///////////////////////////////////////////////////////////////

#define WIFI_CLICK_PIPE     "WIFI_CLICK"

///////////////////////////////////////////////////////////////

typedef struct _wifi_status_t
{
    uint8_t error_code;
    
} wifi_status_t;

typedef struct _wifi_ssid_list_t
{
    uint8_t index;
    char * ssid;
    uint8_t security_mask;
    uint8_t rssi;
    
    struct _wifi_ssid_list_t * p_next;
    
} wifi_ssid_list_t;

typedef struct _wifi_cp_data_t_
{
    wifi_ssid_list_t * ssid_entry;
    
    uint8_t * wep40_key;
    uint32_t wep40_key_len;
    
    uint8_t * wep104_key;
    uint32_t wep104_key_len;
    
    uint8_t * wap_key;
    uint32_t wap_key_len;
    
} wifi_cp_data_t;

///////////////////////////////////////////////////////////////

void wifi_click_init( void );
void wifi_click_process( void );
void wifi_click_application_process( void );

void wifi_click_reset( void );
void wifi_click_start_scan( void );

uint8_t wifi_click_get_last_error( void );
uint8_t wifi_click_get_curr_state( void );

wifi_ssid_list_t * wifi_click_get_scan_item( uint8_t index );

void wifi_click_setup_connection( wifi_ssid_list_t * p_item, char * key );

void wifi_click_connect_cp( void );
void wifi_click_get_connected_ip( uint8_t * ip );

///////////////////////////////////////////////////////////////

#endif // __WIFI_CLICK_h__
