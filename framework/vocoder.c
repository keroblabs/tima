#include "types.h"
#include "vocoder.h"
#include "audio_layer.h"

#include "speex_config.h"
#include "speex.h"
#include "nb_celp.h"

/////////////////////////////////////////////////////////////////////

#if defined WANT_SPEEX && defined WANT_AUDIO_INTERFACE

/////////////////////////////////////////////////////////////////////

#define SAMPLE_RATE         8000

#if 1
#define TOTAL_SAMPLES		AUDIO_BUFFER_MONO_SIZE
#define AUDIO_MODE			AUDIO_MONO
#else
#define TOTAL_SAMPLES		AUDIO_BUFFER_STEREO_SIZE
#define AUDIO_MODE			AUDIO_STEREO
#endif

enum
{
	VOCODER_MODE_NONE,
	VOCODER_MODE_SPEEX,
	VOCODER_MODE_BYPASS,
} vocoder_mode_t;

/////////////////////////////////////////////////////////////////////

typedef struct _vocoder_t_
{
	uint32_t vocoder_mode;

	/* Holds the states of the encoder & the decoder */
	void *enc_state;
	void *dec_state;

	/* Holds bits so they can be read and written by the Speex routines */
	SpeexBits enc_bits;
	SpeexBits dec_bits;

	const SpeexMode *mode;

	int16_t samples[ TOTAL_SAMPLES ];
	uint8_t enc_stream[ MAX_ENCODED_SIZE ];
	uint16_t enc_size;

	audio_data_t * audio;

} vocoder_t;

/////////////////////////////////////////////////////////////////////

void * vocoder_init( uint8_t mode )
{
	vocoder_t * p_data;

	p_data = ( vocoder_t * )MMALLOC( sizeof( vocoder_t ) );
	if( p_data == NULL ) return NULL;

	p_data->audio = audio_start( SAMPLE_RATE, p_data->samples, mode );
	p_data->vocoder_mode = VOCODER_MODE_BYPASS;

	return p_data;
}

void * vocoder_speex_init( uint8_t mode )
{
	vocoder_t * p_data = ( vocoder_t * ) vocoder_init( mode );
    spx_int32_t speex_bitrate = 8000;

    // init speex
    p_data->mode = speex_lib_get_mode();

    /* Speex encoding initializations */
    p_data->enc_state = nb_encoder_init(p_data->mode);
    nb_encoder_ctl( p_data->enc_state, SPEEX_SET_BITRATE, &speex_bitrate );
    speex_bits_init(&p_data->enc_bits);

    /* speex decoding initialization */
    p_data->dec_state = nb_decoder_init(p_data->mode);
    speex_bits_init(&p_data->dec_bits);

	p_data->enc_size = 0;
	p_data->vocoder_mode = VOCODER_MODE_SPEEX;

	return p_data;
}

void vocoder_destroy( void * pp_data )
{
	MFREE( pp_data );
}

uint16_t vocoder_encode_voice( void * pp_data, int16_t * input, uint8_t * output )
{
    uint16_t nbytes;
	vocoder_t * p_data = ( vocoder_t * )pp_data;

	if( p_data->vocoder_mode != VOCODER_MODE_SPEEX ) return 0;

    // encode voice
    nb_encode( p_data->enc_state, input, &p_data->enc_bits );
    nbytes = speex_bits_write( &p_data->enc_bits, ( char * )output, MAX_ENCODED_SIZE );
    speex_bits_reset( &p_data->enc_bits );

    return( nbytes );
}

void vocoder_decode_voice( void * pp_data, uint8_t * input, uint16_t size, int16_t * output )
{
	vocoder_t * p_data = ( vocoder_t * )pp_data;

	if( p_data->vocoder_mode != VOCODER_MODE_SPEEX ) return;

    // decode speex
    speex_bits_read_from(&p_data->dec_bits, ( char * )input, size);
    nb_decode(&p_data->dec_bits, ( spx_int16_t * )output);
}

void vocoder_audio_stop( void * pp_data )
{
	vocoder_t * p_data = ( vocoder_t * )pp_data;
	audio_stop( p_data->audio );
}

bool_t vocoder_audio_output_process( void * pp_data, uint8_t * buffer, uint16_t size )
{
	vocoder_t * p_data = ( vocoder_t * )pp_data;
	bool_t ret = FALSE;

	if( ( p_data->enc_size == 0 ) && ( p_data->vocoder_mode == VOCODER_MODE_SPEEX ) )
	{
		memcpy( p_data->enc_stream, buffer, size );
		p_data->enc_size = size;
	}

    // audio handler
    if( audio_is_ready( p_data->audio ) == TRUE )
    {
		if( p_data->vocoder_mode == VOCODER_MODE_SPEEX )
		{
			speex_bits_read_from(&p_data->dec_bits, ( char * )p_data->enc_stream, p_data->enc_size);
			nb_decode(&p_data->dec_bits, ( spx_int16_t * )p_data->samples);
			p_data->enc_size = 0;
		}
		else
		{
			memcpy( p_data->samples, buffer, AUDIO_BUFFER_SIZE*2 );
		}

		ret = TRUE;
    }

	return ret;
}

uint16_t vocoder_audio_input_process( void * pp_data, uint8_t * buffer, uint16_t max_size )
{
	vocoder_t * p_data = ( vocoder_t * )pp_data;
	uint16_t nbytes = 0;

    // audio handler
    if( audio_is_ready( p_data->audio ) == TRUE )
    {
		if( p_data->vocoder_mode == VOCODER_MODE_SPEEX )
		{
			nb_encode( p_data->enc_state, p_data->samples, &p_data->enc_bits );
			nbytes = speex_bits_write( &p_data->enc_bits, ( char * )buffer, max_size );
			speex_bits_reset( &p_data->enc_bits );
		}
		else
		{
			memcpy( buffer, p_data->samples, AUDIO_BUFFER_SIZE*2 );
			nbytes = AUDIO_BUFFER_SIZE;
		}
    }

	return nbytes;
}

/////////////////////////////////////////////////////////////////////

#endif


