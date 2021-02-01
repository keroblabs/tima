#include "system.h"

#ifdef WANT_SPEEX

#include "audio_layer.h"
#include "file.h"
#include "commands.h"
#include "vocoder.h"
#include "modes.h"
#include "pipe.h"

#ifdef WANT_GRAPHICS
#include "graphics.h"
#include "draw.h"
#endif

///////////////////////////////////////////////////////////////

#define FILE_LINE_SIZE		100
#define PIPE_MAX_SIZE		8

///////////////////////////////////////////////////////////////

enum
{
	SPEEX_COMMAND_NONE,
	SPEEX_COMMAND_RECORD,
	SPEEX_COMMAND_PLAYBACK,
	SPEEX_COMMAND_BYPASS,
	SPEEX_COMMAND_FINISH,
};

///////////////////////////////////////////////////////////////

static void * p_audio_out;
static void * p_audio_in;
static uint16_t bypass_size;

static void * p_vocoder;
static File * fp;
static uint8_t stream[50];
static uint8_t encoded_payload[256];
static uint32_t rec_time;
static uint32_t encoded_index;
static int16_t audio_bypass[512];

static uint32_t encoded_size;	
static uint32_t decoded_size;	

static uint8_t command_mode;
static bool_t is_recording_first;
	
static uint16_t scope_posx;
static int16_t scope_data[800];

///////////////////////////////////////////////////////////////

#ifdef WANT_GRAPHICS
void audio_plot_scope( int16_t * buf, uint32_t size )
{
	uint32_t i;

	for( i = 0; i < size; i += 8 )
	{
		graphics_set_pixel( scope_posx, ( uint16_t )scope_data[ scope_posx ], APP_RGB(0,0,0) );
		scope_data[ scope_posx ] = (graphics_height()/2) + buf[i] / 100;

		if( scope_data[ scope_posx ] < 0 ) scope_data[ scope_posx ] = 0;
		if( scope_data[ scope_posx ] >= graphics_height() ) scope_data[ scope_posx ] = graphics_height()-1;

		draw_line( scope_posx, 0, scope_posx, graphics_height(), APP_RGB(0,0,0) );

		if( scope_posx == 0 )
		{
			graphics_set_pixel( scope_posx, ( uint16_t )scope_data[ scope_posx ], APP_RGB(255,255,255) );
		}
		else
		{
			draw_line( scope_posx - 1, scope_data[ scope_posx - 1 ], scope_posx, scope_data[ scope_posx ], APP_RGB(255,255,255) );
		}

		scope_posx++;

		if( scope_posx >= graphics_width() ) scope_posx = 0;
	}
}
#endif

void cmd_speex_process( void )
{
	//uint16_t i;

	if( command_mode == SPEEX_COMMAND_NONE ) return;

	if( command_mode == SPEEX_COMMAND_PLAYBACK )
	{
		if( fp == NULL ) return;
		if( efs_feof( fp ) )
		{
			command_mode = SPEEX_COMMAND_FINISH;
		}

		if( vocoder_audio_output_process( p_vocoder, stream, encoded_size ) == TRUE )
		{
			efs_fread( stream, encoded_size, 1, fp );
		}
	}
	else if( command_mode == SPEEX_COMMAND_RECORD )
	{
		if( fp == NULL ) return;
		if( ( encoded_size = vocoder_audio_input_process( p_vocoder, stream, MAX_ENCODED_SIZE ) ) )
		{
			if( is_recording_first == TRUE )
			{
				is_recording_first = FALSE;
				efs_fwrite( ( uint8_t * )&decoded_size, sizeof( decoded_size ), 1, fp );
				efs_fwrite( ( uint8_t * )&encoded_size, sizeof( encoded_size ), 1, fp );
				rec_time = 0;
			}

			// efs_fwrite( stream, encoded_size, 1, fp );
			memcpy( &encoded_payload[encoded_index], stream, encoded_size );
			encoded_index += encoded_size;
			if( ( encoded_index + encoded_size ) >= 128 )
			{
				efs_fwrite( encoded_payload, encoded_index, 1, fp );
				encoded_index = 0;
			}
			rec_time += 20;
		}
	}
	else if( command_mode == SPEEX_COMMAND_BYPASS )
	{
		if( bypass_size == 0 )
		{
			if( ( bypass_size = vocoder_audio_input_process( p_audio_in, ( uint8_t * )audio_bypass, 256 ) ) > 0 )
			{
				//for( i = 0; i < bypass_size; i++ )
				//{
				//	audio_bypass[i] >>= 8;
				//}
				#ifdef WANT_GRAPHICS
				audio_plot_scope( audio_bypass, bypass_size );
				#endif
			}
		}

		if( ( bypass_size ) && ( vocoder_audio_output_process( p_audio_out, ( uint8_t * )audio_bypass, 256 ) ) )
		{
			bypass_size = 0;
		}
	}
	
	if( command_mode == SPEEX_COMMAND_FINISH )
	{
		if( fp == NULL ) return;
		efs_fclose( fp );
		fp = NULL;
		command_mode = SPEEX_COMMAND_NONE;
		vocoder_audio_stop( p_vocoder );
		vocoder_destroy( p_vocoder );
		p_vocoder = NULL;
		return;
	}
}

int cmd_speex_record_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	device_t * dev = p_instance->dev_console_tty;

	if( input != NULL )
	{
		path = console_concatenate_path( input, NULL, p_instance );

		p_vocoder = vocoder_speex_init(AUDIO_MODE_INPUT | AUDIO_MODE_MONO);
		if( p_vocoder == NULL ) return -1;

		if( fp != NULL )
		{
			efs_fclose( fp );
		}

		fp = efs_fopen( path, FS_MODE_WRITE );
		if( fp == NULL ) return -1;

		// write WAVE header (empty)
		efs_fwrite( stream, 44, 1, fp );

		decoded_size = DECODED_SIZE_8KBPS;
		command_mode = SPEEX_COMMAND_RECORD;
		encoded_index = 0;
		is_recording_first = TRUE;

		console_command_printk( dev, "Recording" );
	}
	else
	{
		if( command_mode == SPEEX_COMMAND_RECORD )
		{
			command_mode = SPEEX_COMMAND_FINISH;
			console_command_printk( dev, "Stopped, total %d secs", rec_time/1000 );
		}
		else
		{
			console_command_printk( dev, "speex_rec <file.spx>" );
		}
	}

	return 0;
}

int cmd_speex_play_handler( console_instance_t * p_instance, char * input )
{
	char * path;
	bool_t is_found;
	device_t * dev = p_instance->dev_console_tty;

	if( input != NULL )
	{
		path = console_concatenate_path( input, &is_found, p_instance );

		if( is_found )
		{
			p_vocoder = vocoder_speex_init(AUDIO_MODE_OUTPUT | AUDIO_MODE_MONO);
			if( p_vocoder == NULL ) return -1;

			if( fp != NULL )
			{
				efs_fclose( fp );
			}

			fp = efs_fopen( path, FS_MODE_READ );
			if( fp == NULL ) return -1;

			// skip WAVE header
			efs_fread( stream, 44, 1, fp );

			// read the frame size and total of bytes
			efs_fread( ( uint8_t * )&decoded_size, sizeof( decoded_size ), 1, fp );
			efs_fread( ( uint8_t * )&encoded_size, sizeof( encoded_size ), 1, fp );

			// read the first audio frame
			efs_fread( stream, encoded_size, 1, fp );

			command_mode = SPEEX_COMMAND_PLAYBACK;
		}
		else
		{
			console_command_printk( dev, "Path not found" );
		}
	}
	else
	{
		if( command_mode == SPEEX_COMMAND_PLAYBACK )
		{
			command_mode = SPEEX_COMMAND_FINISH;
			console_command_printk( dev, "Stopped" );
		}
		else
		{
			console_command_printk( dev, "speex_play <file.spx>" );
		}
	}

	return 0;
}

int cmd_audio_bypass_handler( console_instance_t * p_instance, char * input )
{
	device_t * dev = p_instance->dev_console_tty;

	if( command_mode == SPEEX_COMMAND_NONE )
	{
		p_audio_in = vocoder_init( AUDIO_MODE_INPUT | AUDIO_MODE_MONO );
		if( p_audio_in == NULL ) return -1;

		p_audio_out = vocoder_init( AUDIO_MODE_OUTPUT | AUDIO_MODE_MONO );
		if( p_audio_out == NULL )
		{
			vocoder_destroy( p_audio_in );
			return -1;
		}

		command_mode = SPEEX_COMMAND_BYPASS;
		bypass_size = 0;
		console_command_printk( dev, "Bypass started" );
	}
	else if( command_mode == SPEEX_COMMAND_BYPASS )
	{
		vocoder_audio_stop( p_audio_in );
		vocoder_audio_stop( p_audio_out );
		
		vocoder_destroy( p_audio_in );
		vocoder_destroy( p_audio_out );

		command_mode = SPEEX_COMMAND_NONE;
		console_command_printk( dev, "Bypass stopped" );
	}
	else
	{
		console_command_printk( dev, "Current mode not allow for bypass, stop playback or recoding" );
	}

	return 0;
}

void cmd_audio_bypass_init( void )
{
	p_audio_in = NULL;
	p_audio_out = NULL;
	scope_posx = 0;
	memset( scope_data, 0x00, sizeof( scope_data ) );
}

void cmd_speex_init( void )
{
	p_vocoder = NULL;
}

///////////////////////////////////////////////////////////////

const command_data_t command_speex = 
{
	"speex_play",
	cmd_speex_init,
	cmd_speex_play_handler
};


const command_data_t command_speex_rec = 
{
	"speex_rec",
	cmd_speex_init,
	cmd_speex_record_handler
};

const command_data_t command_audio_bypass = 
{
	"audio_proc",
	cmd_audio_bypass_init,
	cmd_audio_bypass_handler
};


DECLARE_COMMAND_SECTION( command_speex );
DECLARE_COMMAND_SECTION( command_speex_rec );
DECLARE_COMMAND_SECTION( command_audio_bypass );

DECLARE_PROCESS_SECTION( cmd_speex_process );

#endif
