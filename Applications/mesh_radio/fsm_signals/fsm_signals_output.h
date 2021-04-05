#ifndef APPLICATIONS_MESH_RADIO_RADIO_FSM_SIGNALS_OUTPUT_H_
#define APPLICATIONS_MESH_RADIO_RADIO_FSM_SIGNALS_OUTPUT_H_

////////////////////////////////////////////////////////////////////

#include "types.h"
#include "fsm.h"

////////////////////////////////////////////////////////////////////

typedef enum TX_FLAGS_enum_t_
{
    TX_FLAGS_confirm_required = 0x0001,
    TX_FLAGS_no_ack = 0x0002,
} TX_FLAGS_enum_t;

typedef enum RESULT_enum_t_
{
    RESULT_success,
    RESULT_tx_failure,
    RESULT_invalid_state,
    RESULT_invalid_signal,
    RESULT_invalid_param,
} RESULT_enum_t;

////////////////////////////////////////////////////////////////////

typedef struct Received_Frame_indication_t_
{
    fsm_signal_t signal;
    uint16_t channel;
    uint16_t rssi;
    uint16_t snr;
} Received_Frame_indication_t;

#define Received_Frame_indication_id 1
#define SEND_Received_Frame_indication(_ds,_pd,_channel,_rssi,_snr) do { Received_Frame_indication_t * _pp_data = ( Received_Frame_indication_t * )MMALLOC( sizeof( Received_Frame_indication_t ) ); \
                                                                    _pp_data->signal.data   = _pd; \
                                                                    _pp_data->signal.dest   = _ds; \
                                                                    _pp_data->signal.source = fsm_self(); \
                                                                    _pp_data->channel = (uint16_t)_channel; \
                                                                    _pp_data->rssi = (uint16_t)_rssi; \
                                                                    _pp_data->snr = (uint16_t)_snr; \
                                                                    fsm_send_signal( _ds, Received_Frame_indication_id, _pp_data ); } while( 0 )

typedef struct Transmit_Frame_request_t_
{
    fsm_signal_t signal;
    uint8_t channel;
    uint16_t power;
    uint16_t flags;
    uint32_t stay_on_channel;
} Transmit_Frame_request_t;

#define Transmit_Frame_request_id 2
#define SEND_Transmit_Frame_request(_ds,_pd,_channel,_power,_flags,_stay_on_channel) do { Transmit_Frame_request_t * _pp_data = ( Transmit_Frame_request_t * )MMALLOC( sizeof( Transmit_Frame_request_t ) ); \
                                                                                     _pp_data->signal.data   = _pd; \
                                                                                     _pp_data->signal.dest   = _ds; \
                                                                                     _pp_data->signal.source = fsm_self(); \
                                                                                     _pp_data->channel = (uint8_t)_channel; \
                                                                                     _pp_data->power = (uint16_t)_power; \
                                                                                     _pp_data->flags = (uint16_t)_flags; \
                                                                                     _pp_data->stay_on_channel = (uint32_t)_stay_on_channel; \
                                                                                     fsm_send_signal( _ds, Transmit_Frame_request_id, _pp_data ); } while( 0 )

typedef struct Transmit_Done_indication_t_
{
    fsm_signal_t signal;
    RESULT_enum_t result_code;
    uint8_t channel;
} Transmit_Done_indication_t;

#define Transmit_Done_indication_id 3
#define SEND_Transmit_Done_indication(_ds,_result_code,_channel) do { Transmit_Done_indication_t * _pp_data = ( Transmit_Done_indication_t * )MMALLOC( sizeof( Transmit_Done_indication_t ) ); \
                                                                 _pp_data->signal.dest   = _ds; \
                                                                 _pp_data->signal.source = fsm_self(); \
                                                                 _pp_data->result_code = (RESULT_enum_t)_result_code; \
                                                                 _pp_data->channel = (uint8_t)_channel; \
                                                                 fsm_send_signal( _ds, Transmit_Done_indication_id, _pp_data ); } while( 0 )

typedef struct Set_Channel_request_t_
{
    fsm_signal_t signal;
    uint8_t channel;
    uint32_t listening_time;
} Set_Channel_request_t;

#define Set_Channel_request_id 4
#define SEND_Set_Channel_request(_ds,_channel,_listening_time) do { Set_Channel_request_t * _pp_data = ( Set_Channel_request_t * )MMALLOC( sizeof( Set_Channel_request_t ) ); \
                                                               _pp_data->signal.dest   = _ds; \
                                                               _pp_data->signal.source = fsm_self(); \
                                                               _pp_data->channel = (uint8_t)_channel; \
                                                               _pp_data->listening_time = (uint32_t)_listening_time; \
                                                               fsm_send_signal( _ds, Set_Channel_request_id, _pp_data ); } while( 0 )

typedef struct Set_Channel_confirm_t_
{
    fsm_signal_t signal;
    RESULT_enum_t result_code;
    uint8_t channel;
} Set_Channel_confirm_t;

#define Set_Channel_confirm_id 5
#define SEND_Set_Channel_confirm(_ds,_result_code,_channel) do { Set_Channel_confirm_t * _pp_data = ( Set_Channel_confirm_t * )MMALLOC( sizeof( Set_Channel_confirm_t ) ); \
                                                            _pp_data->signal.dest   = _ds; \
                                                            _pp_data->signal.source = fsm_self(); \
                                                            _pp_data->result_code = (RESULT_enum_t)_result_code; \
                                                            _pp_data->channel = (uint8_t)_channel; \
                                                            fsm_send_signal( _ds, Set_Channel_confirm_id, _pp_data ); } while( 0 )

typedef struct Unset_Channel_request_t_
{
    fsm_signal_t signal;
    uint8_t channel;
} Unset_Channel_request_t;

#define Unset_Channel_request_id 6
#define SEND_Unset_Channel_request(_ds,_channel) do { Unset_Channel_request_t * _pp_data = ( Unset_Channel_request_t * )MMALLOC( sizeof( Unset_Channel_request_t ) ); \
                                                 _pp_data->signal.dest   = _ds; \
                                                 _pp_data->signal.source = fsm_self(); \
                                                 _pp_data->channel = (uint8_t)_channel; \
                                                 fsm_send_signal( _ds, Unset_Channel_request_id, _pp_data ); } while( 0 )

typedef struct Listening_indication_t_
{
    fsm_signal_t signal;
    RESULT_enum_t result_code;
    uint8_t channel;
} Listening_indication_t;

#define Listening_indication_id 7
#define SEND_Listening_indication(_ds,_result_code,_channel) do { Listening_indication_t * _pp_data = ( Listening_indication_t * )MMALLOC( sizeof( Listening_indication_t ) ); \
                                                             _pp_data->signal.dest   = _ds; \
                                                             _pp_data->signal.source = fsm_self(); \
                                                             _pp_data->result_code = (RESULT_enum_t)_result_code; \
                                                             _pp_data->channel = (uint8_t)_channel; \
                                                             fsm_send_signal( _ds, Listening_indication_id, _pp_data ); } while( 0 )

typedef struct Listen_Done_indication_t_
{
    fsm_signal_t signal;
    RESULT_enum_t result_code;
    uint8_t channel;
} Listen_Done_indication_t;

#define Listen_Done_indication_id 8
#define SEND_Listen_Done_indication(_ds,_result_code,_channel) do { Listen_Done_indication_t * _pp_data = ( Listen_Done_indication_t * )MMALLOC( sizeof( Listen_Done_indication_t ) ); \
                                                               _pp_data->signal.dest   = _ds; \
                                                               _pp_data->signal.source = fsm_self(); \
                                                               _pp_data->result_code = (RESULT_enum_t)_result_code; \
                                                               _pp_data->channel = (uint8_t)_channel; \
                                                               fsm_send_signal( _ds, Listen_Done_indication_id, _pp_data ); } while( 0 )

typedef struct Listen_Timeout_indication_t_
{
    fsm_signal_t signal;
} Listen_Timeout_indication_t;

#define Listen_Timeout_indication_id 9

typedef struct Debug_Print_indication_t_
{
    fsm_signal_t signal;
    uint32_t timestamp;
} Debug_Print_indication_t;

#define Debug_Print_indication_id 10
#define SEND_Debug_Print_indication(_ds,_pd,_timestamp) do { Debug_Print_indication_t * _pp_data = ( Debug_Print_indication_t * )MMALLOC( sizeof( Debug_Print_indication_t ) ); \
                                                        _pp_data->signal.data   = _pd; \
                                                        _pp_data->signal.dest   = _ds; \
                                                        _pp_data->signal.source = fsm_self(); \
                                                        _pp_data->timestamp = (uint32_t)_timestamp; \
                                                        fsm_send_signal( _ds, Debug_Print_indication_id, _pp_data ); } while( 0 )

typedef struct Scan_Start_request_t_
{
    fsm_signal_t signal;
    uint8_t scan_type;
    uint32_t channel_mask;
} Scan_Start_request_t;

#define Scan_Start_request_id 11
#define SEND_Scan_Start_request(_ds,_scan_type,_channel_mask) do { Scan_Start_request_t * _pp_data = ( Scan_Start_request_t * )MMALLOC( sizeof( Scan_Start_request_t ) ); \
                                                              _pp_data->signal.dest   = _ds; \
                                                              _pp_data->signal.source = fsm_self(); \
                                                              _pp_data->scan_type = (uint8_t)_scan_type; \
                                                              _pp_data->channel_mask = (uint32_t)_channel_mask; \
                                                              fsm_send_signal( _ds, Scan_Start_request_id, _pp_data ); } while( 0 )

typedef struct Scan_Start_confirm_t_
{
    fsm_signal_t signal;
    RESULT_enum_t result_code;
    uint16_t scan_id;
} Scan_Start_confirm_t;

#define Scan_Start_confirm_id 12
#define SEND_Scan_Start_confirm(_ds,_result_code,_scan_id) do { Scan_Start_confirm_t * _pp_data = ( Scan_Start_confirm_t * )MMALLOC( sizeof( Scan_Start_confirm_t ) ); \
                                                           _pp_data->signal.dest   = _ds; \
                                                           _pp_data->signal.source = fsm_self(); \
                                                           _pp_data->result_code = (RESULT_enum_t)_result_code; \
                                                           _pp_data->scan_id = (uint16_t)_scan_id; \
                                                           fsm_send_signal( _ds, Scan_Start_confirm_id, _pp_data ); } while( 0 )

typedef struct Scan_Result_indication_t_
{
    fsm_signal_t signal;
    uint16_t scan_id;
    uint16_t rssi;
    uint16_t channel;
} Scan_Result_indication_t;

#define Scan_Result_indication_id 13
#define SEND_Scan_Result_indication(_ds,_pd,_scan_id,_rssi,_channel) do { Scan_Result_indication_t * _pp_data = ( Scan_Result_indication_t * )MMALLOC( sizeof( Scan_Result_indication_t ) ); \
                                                                     _pp_data->signal.data   = _pd; \
                                                                     _pp_data->signal.dest   = _ds; \
                                                                     _pp_data->signal.source = fsm_self(); \
                                                                     _pp_data->scan_id = (uint16_t)_scan_id; \
                                                                     _pp_data->rssi = (uint16_t)_rssi; \
                                                                     _pp_data->channel = (uint16_t)_channel; \
                                                                     fsm_send_signal( _ds, Scan_Result_indication_id, _pp_data ); } while( 0 )

typedef struct Scan_Finish_indication_t_
{
    fsm_signal_t signal;
    uint16_t scan_id;
} Scan_Finish_indication_t;

#define Scan_Finish_indication_id 13
#define SEND_Scan_Finish_indication(_ds,_pd,_scan_id) do { Scan_Finish_indication_t * _pp_data = ( Scan_Finish_indication_t * )MMALLOC( sizeof( Scan_Finish_indication_t ) ); \
                                                      _pp_data->signal.data   = _pd; \
                                                      _pp_data->signal.dest   = _ds; \
                                                      _pp_data->signal.source = fsm_self(); \
                                                      _pp_data->scan_id = (uint16_t)_scan_id; \
                                                      fsm_send_signal( _ds, Scan_Finish_indication_id, _pp_data ); } while( 0 )
extern const char * signal_debug[];

////////////////////////////////////////////////////////////////////

#endif // APPLICATIONS_MESH_RADIO_RADIO_FSM_SIGNALS_OUTPUT_H_

