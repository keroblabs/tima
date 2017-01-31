#include "system.h"
#include "hardware.h"

////////////////////////////////////////////////////////////

#define KEYPAD_STACK_SIZE       2048

#define KEYPAD_TIME_LONG        1500
#define KEYPAD_TIME_REPEAT      100
#define KEYPAD_DELAY_REPEAT     500

////////////////////////////////////////////////////////////

static uint32_t         prev_state;
static uint32_t         curr_state;

static int16_t          encoder_pos;

static handler_t        gpio_enc_a;
static handler_t        gpio_enc_b;
static handler_t        gpio_bot_a;
static handler_t        gpio_bot_b;
static handler_t        gpio_bot_enter;
static handler_t        gpio_bot_ptt;

static thread_data_t    keypad_context;
static uint8_t          keypad_stack[ KEYPAD_STACK_SIZE ];

static bool_t           _keypad_in_use = FALSE;
static bool_t           _is_long;

static int16_t          prev_keycode;
static int16_t          curr_keycode;
static timer_data_t     long_timer;
static timer_data_t     repeat_timer;

static message_data_t   key_msg;

////////////////////////////////////////////////////////////

void _keypad_encoder_read( void )
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

        message_Post( &key_msg, Message_Encoder );
    }

    prev_state = curr_state;    
}

void _keypad_key_read( void )
{
    uint16_t new_key;
    
    new_key = Key_NoKey;
    if( GPIO_read( gpio_bot_enter ) ) new_key = Key_Enter;
    if( GPIO_read( gpio_bot_a ) ) new_key = Key_F1;
    if( GPIO_read( gpio_bot_b ) ) new_key = Key_F2;
    
    if( ( new_key != Key_NoKey ) && ( prev_keycode == Key_NoKey ) )
    {
        prev_keycode = curr_keycode = new_key;
        
        if( ( curr_keycode >= Key_Navigation_Start ) && ( curr_keycode <= Key_Navigation_End ) )
        {
            // repeat delay start for navigation key
            message_Post( &key_msg, Message_KeyPressed );
            timer_Start( &repeat_timer, KEYPAD_DELAY_REPEAT );
        }
        else if( ( curr_keycode >= Key_PressHold_Start ) && ( curr_keycode <= Key_PressHold_End ) )
        {
            // press-hold keys
            message_Post( &key_msg, Message_KeyPressed );
        }
        else
        {
            // all other keys, use short/long delays
            timer_Start( &long_timer, KEYPAD_TIME_LONG );
        }
    }
    
    if( ( timer_Check( &repeat_timer ) ) && ( new_key == prev_keycode ) && ( prev_keycode != Key_NoKey ) )
    {
        // repeat key every 100 ms
        timer_Start( &repeat_timer, 100 );
        message_Post( &key_msg, Message_KeyPressed );
    }    
    
    if( ( timer_Check( &long_timer ) ) && 
        ( new_key == prev_keycode ) && 
        ( prev_keycode != Key_NoKey ) && 
        ( _is_long == FALSE ) )
    {
        // long_press
        timer_Stop( &long_timer );
        message_Post( &key_msg, Message_KeyLongPres );
        
        // release
        _is_long = TRUE;        
    }    
    
    if( ( new_key == Key_NoKey ) && ( prev_keycode != Key_NoKey ) )
    {
        if( _is_long == TRUE )
        {
            // long key released
        }
        else if( ( curr_keycode >= Key_Navigation_Start ) && ( curr_keycode <= Key_Navigation_End ) )
        {
            // repeating keys released
        }
        else if( ( curr_keycode >= Key_PressHold_Start ) && ( curr_keycode <= Key_PressHold_End ) )
        {
            // press&hold keys released
            message_Post( &key_msg, Message_KeyReleased );
        }
        else
        {
            // all other keys released
            message_Post( &key_msg, Message_KeyPressed );
        }
        
        // release
        prev_keycode = Key_NoKey;
        _is_long = FALSE;

        // reset alll timers        
        timer_Stop( &long_timer );
        timer_Stop( &repeat_timer );
    }    
}

void _keypad_thread( void * args )
{
    while( _keypad_in_use )
    {
        _keypad_encoder_read();
        _keypad_key_read();
        thread_yield();
    }
}

void _keypad_init( uint32_t index )
{
    gpio_enc_a = GPIO_get_port( GPIO_ENCODER_A );
    gpio_enc_b = GPIO_get_port( GPIO_ENCODER_B );
    gpio_bot_enter = GPIO_get_port( BUTTON_GPIO_ENTER );
        
    _is_long = FALSE;
    prev_state = curr_state = 0;
    encoder_pos = 0;
    _keypad_encoder_read();
    encoder_pos = 0;
    
    prev_keycode = Key_NoKey;
    
    message_Init( &key_msg );
    
    thread_create( &keypad_context, _keypad_thread, NULL, keypad_stack, KEYPAD_STACK_SIZE );
}

bool_t _keypad_read( keypad_data_t * key_data )
{
    uint16_t message;
    
    message = message_Get( &key_msg );
    
    if( message == Message_NoMessage ) return FALSE;

    key_data->mode = message;
    
    if( message == Message_Encoder )
    {
        key_data->code = encoder_pos;
        encoder_pos = 0;
    }
    else
    {
        key_data->code = curr_keycode;
    }    
    
    return TRUE;
}

uint32_t _keypad_get( uint32_t index, uint32_t * value, uint32_t size )
{
    if( ( value == NULL ) && ( message_Read( &key_msg ) != Message_NoMessage ) )
    {
        return sizeof( keypad_data_t );
    }
    
    if( _keypad_read( ( keypad_data_t * )value ) == TRUE )
    {
        return sizeof( keypad_data_t );
    }
    
    return 0;
}

static bool_t validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

const static device_data_t keypad_device = 
{
    "keypad*",
    &_keypad_in_use,
    
    _keypad_init,
    NULL,
    NULL,
    validate,
    NULL,
    NULL,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    _keypad_get,
    NULL,
};

DECLARE_DEVICE_SECTION( keypad_device )
