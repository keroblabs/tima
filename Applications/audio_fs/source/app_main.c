#include "system.h"
#include "tima_libc.h"
#include "timer.h"
#include "pipe.h"
#include "commands.h"
#include "efs.h"
#include "debug.h"
#include "audio_layer.h"
#include "graphics.h"
#include "draw.h"
#include "vocoder.h"

/////////////////////////////////////////////////////////////////////////////////////////

#define DECODER_PIPE_TRIGGER        3
#define DECODER_PIPE_SIZE           6

/////////////////////////////////////////////////////////////////////////////////////////

static vocoder_t vocoder_speex;
static pipe_data_t * decoder_pipe;
static pipe_data_t * encoded_pipe;
static uint32_t total_data;
static uint32_t playback_index;
static uint8_t encoded_buffer[8192];
static uint16_t step_size;
static uint8_t mode;

void app_audio_init( void )
{
    audio_layer_init();
    
    vocoder_speex = vocoder_start( VOCODER_MODE_SPEEX, DECODER_PIPE_SIZE, DECODER_PIPE_TRIGGER );
    decoder_pipe = vocoder_decoder_get_pipe( vocoder_speex );
    encoded_pipe = vocoder_encoder_get_pipe( vocoder_speex );
    
    total_data = 0;
    step_size = 0;
    mode = 0;
    
    DEBUG_PRINTK( "Recording\n" );
}

void app_audio_process( void )
{
    if( pipe_has_data( encoded_pipe ) && ( mode == 0 ) )
    {
        uint16_t size;
        uint8_t * data = pipe_read( encoded_pipe, &size );
        
        memcpy( &encoded_buffer[ total_data ], data, size );
        pipe_release_buffer( data );
        
        step_size = size;
        total_data += size;
        
        if( total_data >= 5000 )
        {
            DEBUG_PRINTK( "Playback\n" );
            playback_index = 0;
            mode = 1;
        }
    }
    else if( ( mode == 1 ) && ( pipe_is_full( decoder_pipe ) == FALSE ) )
    {
        pipe_send_buffer( decoder_pipe, &encoded_buffer[playback_index], step_size );
        playback_index += step_size;
        if( playback_index > 5000 )
        {
            DEBUG_PRINTK( "Flushing\n" );
            mode = 3;
        }
    }
    else if( ( mode == 3 ) && ( pipe_is_empty( decoder_pipe ) == TRUE ) )
    {
        DEBUG_PRINTK( "Recording\n" );
        total_data = 0;
        mode = 0;
    }
}

void app_main( void )
{
    timer_data_t timer_sec;
    uint32_t new_value;

    timer_Start(&timer_sec, 1000);
    new_value = 0;
    
    graphics_init();
    app_audio_init();
    
    //console_init();
    // console_connect_uart( DEV_TTY_COMMAND );
    //console_listen_telnet( 12222 );

    //efs_init();
    //MAL_Init_All();
    
    // draw_line( 0, 10, 100, 10, APP_RGB(255, 255, 255));

    DEBUG_PRINTK( "Tima Test\r\n" );

    while( 1 )
    {
    	if( timer_Check(&timer_sec) == TRUE )
        {
        	timer_Reload( &timer_sec );
        	DEBUG_PRINTK( "Counter = %d [%d]\r\n", new_value, total_data );
        	new_value++;
        }

        //console_process();
        audio_layer_process();
        app_audio_process();
        
        SYSTEM_EVENTS();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

DECLARE_APPLICATION_SECTION( app_main )
