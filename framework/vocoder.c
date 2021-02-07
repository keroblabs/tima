#include "types.h"
#include "vocoder.h"
#include "audio_layer.h"

#include "speex_config.h"
#include "speex.h"
#include "nb_celp.h"

/////////////////////////////////////////////////////////////////////

#if defined WANT_SPEEX && defined WANT_AUDIO_INTERFACE

/////////////////////////////////////////////////////////////////////

#define SAMPLE_RATE                 8000

#define TOTAL_SAMPLES		        AUDIO_BUFFER_MONO_SIZE
#define AUDIO_MODE			        AUDIO_MONO

/////////////////////////////////////////////////////////////////////

typedef struct _vocoder_t_
{
	uint32_t vocoder_mode;

    union
    {
        speex_data_t speed_data;
    } u;

    int16_t samples_rec[ TOTAL_SAMPLES ];
    int16_t samples_play[ TOTAL_SAMPLES ];
    
    pipe_data_t decoder_pipe;
    pipe_data_t encoded_pipe;
    
    bool_t decoder_trigger;
    uint16_t pipe_size;
    uint16_t decoder_threshold;

	audio_data_t * audio_rec;
	audio_data_t * audio_play;

} vocoder_data_t;

/////////////////////////////////////////////////////////////////////

static bool_t is_mode_valid( uint8_t mode )
{
    switch( mode )
    {
        case VOCODER_MODE_SPEEX:
            return TRUE;
            
        default:
            break;
    }
    
    return FALSE;
}

static void vocoder_encoder_audio_proc( void * pp_data )
{
    vocoder_data_t * p_data = ( vocoder_data_t * )pp_data;
    uint16_t nbytes = 0;
    uint8_t output[MAX_ENCODED_SIZE];
    
    int16_t * input = audio_get_buffer( p_data->audio_rec );
    
    switch( p_data->vocoder_mode )
    {
        case VOCODER_MODE_SPEEX:
            nbytes = nb_speex_encode( &p_data->u.speed_data, input, output, MAX_ENCODED_SIZE );
            pipe_send_buffer( &p_data->encoded_pipe, output, nbytes );
            break;
            
        case VOCODER_MODE_BYPASS:
            break;
    }
}

static void vocoder_decoder_audio_proc( void * pp_data )
{
    vocoder_data_t * p_data = ( vocoder_data_t * )pp_data;
    
    if( ( p_data->decoder_trigger == FALSE ) && ( pipe_get_used( &p_data->decoder_pipe ) >= p_data->decoder_threshold ) )
    {
        p_data->decoder_trigger = TRUE;
    }
    
    if( p_data->decoder_trigger == TRUE )
    {
        if( pipe_has_data( &p_data->decoder_pipe ) )
        {
            uint16_t size;
            uint8_t * input = pipe_read( &p_data->decoder_pipe, &size );
            int16_t * output = audio_get_buffer( p_data->audio_play );
            
            switch( p_data->vocoder_mode )
            {
                case VOCODER_MODE_SPEEX:
                    nb_speex_decode( &p_data->u.speed_data, input, size, output );
                    break;
                    
                case VOCODER_MODE_BYPASS:
                    break;
            }

            pipe_release_buffer( input );
        }
        else
        {
            p_data->decoder_trigger = FALSE;
        }
    }
}

/////////////////////////////////////////////////////////////////////

vocoder_t vocoder_start( uint8_t vocoder_mode, uint16_t pipe_size, uint16_t decode_threshold )
{
    vocoder_data_t * p_data;
    
    if( is_mode_valid( vocoder_mode ) == FALSE ) return NULL;
    
    p_data = ( vocoder_data_t * )MMALLOC( sizeof( vocoder_data_t ) );
    if( p_data == NULL ) return NULL;

    p_data->vocoder_mode = vocoder_mode;
    p_data->pipe_size = pipe_size;
    p_data->decoder_threshold = decode_threshold;

    p_data->audio_rec = audio_start( SAMPLE_RATE, p_data->samples_rec, AUDIO_MODE_MONO | AUDIO_MODE_INPUT, vocoder_encoder_audio_proc, p_data );
    if( p_data->audio_rec == NULL )
    {
        MFREE( p_data );
        return NULL;
    }

    p_data->audio_play = audio_start( SAMPLE_RATE, p_data->samples_play, AUDIO_MODE_MONO | AUDIO_MODE_OUTPUT, vocoder_decoder_audio_proc, p_data );
    if( p_data->audio_play == NULL )
    {
        audio_stop( p_data->audio_rec );
        MFREE( p_data );
        return NULL;
    }
    
    pipe_init( &p_data->decoder_pipe, "DECODER_PIPE", p_data->pipe_size );
    pipe_init( &p_data->encoded_pipe, "ENCODED_PIPE", p_data->pipe_size );

    switch( p_data->vocoder_mode )
    {
        case VOCODER_MODE_SPEEX:
            nb_speex_init( &p_data->u.speed_data, SAMPLE_RATE );
            break;
            
        case VOCODER_MODE_BYPASS:
            break;
            
        default:
            break;
    }
    
    return ( vocoder_t )p_data;
}

pipe_data_t * vocoder_decoder_get_pipe( vocoder_t pp_data )
{
    vocoder_data_t * p_data = ( vocoder_data_t * )pp_data;
    return &p_data->decoder_pipe;
}

pipe_data_t * vocoder_encoder_get_pipe( vocoder_t pp_data )
{
    vocoder_data_t * p_data = ( vocoder_data_t * )pp_data;
    return &p_data->encoded_pipe;
}

void vocoder_audio_stop( vocoder_t pp_data )
{
    vocoder_data_t * p_data = ( vocoder_data_t * )pp_data;
    audio_stop( p_data->audio_rec );
    audio_stop( p_data->audio_play );
    MFREE( pp_data );
}

/////////////////////////////////////////////////////////////////////

#endif


