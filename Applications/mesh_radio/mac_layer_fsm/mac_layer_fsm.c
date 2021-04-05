#include "mac_layer_fsm.h"
#include "device.h"
#include "system.h"

#include "pipe.h"
#include "buffer_data.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

static void mac_layer_fsm_init( fsm_mac_layer_t * fsm_data )
{
    if( device_open_ex( "/dev/mac", &fsm_data->mac_device ) == FALSE )
    {
        fsm_exit();
    }
    else
    {
        fsm_data->client.client = INVALID_PID;
        fsm_data->mac_driver = ( mac_driver_t * )device_get_open_api( &fsm_data->mac_device );
    }
}

static void mac_layer_fsm_exit( fsm_mac_layer_t * fsm_data )
{
}

static bool_t set_channel( fsm_mac_layer_t * fsm_data, uint32_t client, uint8_t channel, uint32_t listen_ms )
{
    bool_t ret = FALSE;
    
    if( channel != fsm_data->client.channel )
    {
        fsm_data->client.client = client;
        fsm_data->client.channel = channel;
        fsm_data->client.listen_time = listen_ms;
        
        fsm_data->mac_driver->set_channel( channel );
        ret = TRUE;
    }
    
    if( listen_ms > 0 )
    {
        fsm_set_timer( Listen_Timeout_indication_id, listen_ms );
    }
    
    return ret;
}

static void Idle_transition_Transmit_Frame_request( fsm_mac_layer_t * fsm_data, Transmit_Frame_request_t * signal )
{
    // save the signal for later
    fsm_save_signal( fsm_data, ( fsm_signal_t * )signal );
    
    set_channel( fsm_data, signal->signal.source, signal->channel, signal->stay_on_channel );
    fsm_set_state( scheduling_state_mac_layer_fsm );
}

static void Idle_transition_Set_Channel_request( fsm_mac_layer_t * fsm_data, Set_Channel_request_t * signal )
{
    if( ( fsm_data->client.client == INVALID_PID ) || ( fsm_data->client.client == signal->signal.source ) )
    {
        if( set_channel( fsm_data, signal->signal.source, signal->channel, signal->listening_time ) )
        {
            fsm_set_state( scheduling_state_mac_layer_fsm );
        }
    }
    else
    {
        fsm_save_signal( fsm_data, ( fsm_signal_t * )signal );
    }
}

static void on_channel_transition_background( fsm_mac_layer_t * fsm_data )
{
    if( device_pipe_has_data_input( &fsm_data->mac_device ) == TRUE )
    {
        uint16_t size;
        uint8_t * rx_data = device_pipe_read_input( &fsm_data->mac_device, &size );
        
        if( fsm_data->client.client != INVALID_PID )
        {
            uint16_t rssi = rx_data[0];
            uint16_t snr = rx_data[1];
            uint8_t * data = &rx_data[2];
            
            SEND_Received_Frame_indication( fsm_data->client.client, buffer_data_create( data, size - 2 ), fsm_data->client.channel, rssi, snr );
        }
        else
        {
            pipe_release_buffer( rx_data );
        }
    }
}

static void on_channel_transition_Set_Channel_request( fsm_mac_layer_t * fsm_data, Set_Channel_request_t * signal )
{
    Idle_transition_Set_Channel_request( fsm_data, signal );
}

static void on_channel_transition_Transmit_Frame_request( fsm_mac_layer_t * fsm_data, Transmit_Frame_request_t * signal )
{
    if( ( fsm_data->client.client != INVALID_PID ) && ( fsm_data->client.client == signal->signal.source ) )
    {
        buffer_t * buffer = ( buffer_t * )signal->signal.data;
        uint8_t * data = ( uint8_t * )buffer_data_get_rw( buffer );
        
        fsm_data->client.tx_flags = signal->flags;
        fsm_data->mac_driver->set_power( signal->power );

        if( signal->stay_on_channel > 0 )
        {
            fsm_set_timer( Listen_Timeout_indication_id, signal->stay_on_channel );
        }

        device_pipe_send_buffer_output( &fsm_data->mac_device, data, buffer_data_size( buffer ) );
        fsm_set_state( tx_frame_state_mac_layer_fsm );
    }
    else
    {
        fsm_invalid_signal( fsm_data, ( fsm_signal_t * )signal );
    }
}

static void on_channel_transition_Unset_Channel_request( fsm_mac_layer_t * fsm_data, Unset_Channel_request_t * signal )
{
    if( ( fsm_data->client.client != INVALID_PID ) && ( fsm_data->client.client == signal->signal.source ) )
    {
        on_channel_transition_Listen_Timeout_indication( fsm_data, NULL );
    }
    else
    {
        fsm_invalid_signal( fsm_data, signal );
    }
}

static void on_channel_transition_Listen_Timeout_indication( fsm_mac_layer_t * fsm_data, Listen_Timeout_indication_t * signal )
{
    fsm_data->client.client = INVALID_PID;
    fsm_data->client.channel = 0;
    
    fsm_set_state( Idle_state_mac_layer_fsm );
    SEND_Listen_Done_indication( fsm_data->client.client, RESULT_success, fsm_data->client.channel );
}

static void tx_frame_transition_background( fsm_mac_layer_t * fsm_data )
{
    on_channel_transition_background( fsm_data );
    
    if( fsm_data->mac_driver->is_tx_completed() == TRUE )
    {
        RESULT_enum_t result_code = RESULT_success;
        if( fsm_data->mac_driver->is_tx_success() == FALSE )
        {
            uint8_t * cancel_frame = ( uint8_t * )fsm_data->mac_driver->failed_frame();
            if( cancel_frame != NULL ) pipe_release_buffer( cancel_frame );
            result_code = RESULT_tx_failure;
        }
        
        if( fsm_data->client.tx_flags & TX_FLAGS_confirm_required )
        {
            SEND_Transmit_Done_indication( fsm_data->client.client, result_code, fsm_data->client.channel );
        }
        
        fsm_set_timer( Listen_Timeout_indication_id, fsm_data->client.listen_time );
        fsm_set_state( on_channel_state_mac_layer_fsm );
    }
}

static void scheduling_transition_background( fsm_mac_layer_t * fsm_data )
{
    if( fsm_data->mac_driver->is_listening() == TRUE )
    {
        SEND_Listening_indication( fsm_data->client.client, RESULT_success, fsm_data->mac_driver->curr_channel() );
        fsm_set_state( on_channel_state_mac_layer_fsm );
    }
}

////////////////////////////////////////////////////////////////////
