#ifndef __mac_layer_h__
#define __mac_layer_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

typedef struct _mac_driver_t
{
    void ( *set_channel )( uint8_t channel );
    void ( *set_power )( uint16_t power );
    
    bool_t ( *is_tx_success )( void );
    bool_t ( *is_tx_completed )( void );
    bool_t ( *is_listening )( void );
    
    uint8_t ( *curr_channel )( void );
    
    void * ( *failed_frame )( void );

} mac_driver_t;

///////////////////////////////////////////////////////////////

#endif // __mac_layer_h__
