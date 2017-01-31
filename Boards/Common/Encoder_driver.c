#include "system.h"
#include "hardware.h"

////////////////////////////////////////////////////////////

#define ENCODER_STACK_SIZE      2048

////////////////////////////////////////////////////////////

static uint32_t         prev_state;
static uint32_t         curr_state;

static int32_t          encoder_pos;
static bool_t           encoder_moved;

static handler_t        gpio_enc_a;
static handler_t        gpio_enc_b;

static thread_data_t    encoder_context;
static uint8_t          encoder_stack[ ENCODER_STACK_SIZE ];

static bool_t           _encoder_in_use = FALSE;

////////////////////////////////////////////////////////////

void _encoder_read( void )
{
    uint8_t gpio_a;
    uint8_t gpio_b;
    
    gpio_a = GPIO_read( gpio_enc_a );
    gpio_b = GPIO_read( gpio_enc_b );

    if( gpio_b )
    {
        gpio_a = gpio_a ? 0 : 1;            
    }

    curr_state = 0;
    if( gpio_a ) curr_state |= 1;
    if( gpio_b ) curr_state |= 2;

    if( prev_state != curr_state )
    {
        if( ( ( prev_state + 1 ) & 0x3 ) == curr_state ) encoder_pos++;
        else encoder_pos--;

        encoder_moved = TRUE;
    }

    prev_state = curr_state;    
}

void _encoder_thread( void * args )
{
    while( _encoder_in_use )
    {
        _encoder_read();
        
        thread_yield();
    }
}

void _encoder_init( uint32_t index )
{
    gpio_enc_a = GPIO_get_port( GPIO_ENCODER_A );
    gpio_enc_b = GPIO_get_port( GPIO_ENCODER_B );
        
    prev_state = curr_state = 0;
    encoder_pos = 0;
    _encoder_read();
    encoder_pos = 0;
    encoder_moved = FALSE;
    
    thread_create( &encoder_context, _encoder_thread, NULL, encoder_stack, ENCODER_STACK_SIZE );
}

uint32_t _encoder_ioctrl( uint32_t index, uint32_t param, void * data )
{
    uint32_t * p_value = ( uint32_t * )data;
    
    if( param == DEVICE_GET_RUNTIME_VALUE )
    {
        if( p_value != NULL ) *p_value = encoder_pos;
    }
    else if( param == DEVICE_SET_RUNTIME_VALUE )
    {
        if( p_value != NULL ) encoder_pos = *p_value;
    }

    return( 0 );
}

uint32_t _encoder_get( uint32_t index, uint32_t * value, uint32_t size )
{
    // indicate no value available if encoder didnt move
    if( encoder_moved == FALSE ) return FALSE;
    
    // encoder moved
    if( value != NULL )
    {
        encoder_moved = FALSE;
        *value = encoder_pos;
    }

    return sizeof( uint32_t );
}

uint32_t _encoder_set( uint32_t index, uint32_t * value, uint32_t size )
{
    encoder_pos = 0;    
    return sizeof( uint32_t );
}

static bool_t validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

const static device_data_t encoder_device = 
{
    "encoder*", // ms_timer_name,
    &_encoder_in_use,
    
    _encoder_init,
    NULL,
    NULL,
    validate,
    NULL,
    _encoder_ioctrl,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    _encoder_get,
    _encoder_set,
};

DECLARE_DEVICE_SECTION( encoder_device )
