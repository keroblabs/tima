#include "tone_player.h"
#include "runtime.h"
#include "timer.h"
#include "tone.h"
#include "audio_layer.h"

///////////////////////////////////////////////////////////////

enum
{
	TONE_IDLE,
	TONE_START,
	TONE_PLAY
};

///////////////////////////////////////////////////////////////

static void tone_proc( void );

///////////////////////////////////////////////////////////////

static uint32_t tone_pid;
static tone_t tone;
static const tone_data_t * curr_tone;
static timer_data_t tone_duration;
static uint8_t tone_state;
static audio_data_t * audio_data;
static int32_t tone_d;
static int32_t tone_n;
static int16_t audio_buffer[ 160 ];

///////////////////////////////////////////////////////////////

void tone_init( void )
{
	curr_tone = NULL;
	tone_state = TONE_IDLE;
	tone_n = 1;
	tone_d = 1;
}

bool_t tone_is_busy( void )
{
	if( curr_tone != NULL ) return TRUE;
	return FALSE;
}

void tone_play( const tone_data_t * tone )
{
	if( tone_is_busy() ) return;

	curr_tone = tone;
	tone_state = TONE_START;
	tone_pid = runtime_add( NULL, tone_proc );
	audio_data = audio_start( 8000, audio_buffer, AUDIO_MODE_OUTPUT | AUDIO_MODE_MONO );
}

void tone_set_gain( int32_t val_n, int32_t val_d )
{
	tone_n = val_n;
	tone_d = val_d;
}

void tone_proc( void )
{
	int16_t * p_buffer;

	switch( tone_state )
	{
		case TONE_IDLE:
			break;

		case TONE_START:
			toneInit( &tone, curr_tone->freq, 8000 );
			toneSetGain( &tone, tone_n, tone_d );
			timer_Start( &tone_duration, curr_tone->duration );
			tone_state = TONE_PLAY;
			break;

		case TONE_PLAY:
			if( audio_is_ready( audio_data ) )
			{
				p_buffer = audio_get_buffer( audio_data );
				toneGenerate( &tone, p_buffer, AUDIO_BUFFER_SIZE );
			}

			if( timer_Check( &tone_duration ) )
			{
				timer_Stop( &tone_duration );
				curr_tone++;
				if( curr_tone->duration == 0 )
				{
					tone_state = TONE_IDLE;
					curr_tone = NULL;
					runtime_terminate( tone_pid );
					audio_stop( audio_data );
				}
				else
				{
					tone_state = TONE_START;
				}
			}
			break;
	}
}

