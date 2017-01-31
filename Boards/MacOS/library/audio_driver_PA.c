#include "audio_layer.h"
#include "portaudio.h"
#include "system.h"

////////////////////////////////////////////////////////////////////////////////

#define AUDIO_TOTAL_BUFFERS				2

////////////////////////////////////////////////////////////////////////////////

#if 0

typedef struct WaveDeviceDataT
{
    PaStreamParameters streamParameters;
    PaStream *stream;
    
    void * wave_hdl;

} wave_dev_t;

////////////////////////////////////////////////////////////////////////////////

wave_dev_t wave_dev_out;
wave_dev_t wave_dev_in;

////////////////////////////////////////////////////////////////////////////////

void audio_driver_init( audio_data_t * audio );
void audio_dma_out_start( audio_data_t * audio );
void audio_dma_in_start( audio_data_t * audio );
void audio_dma_out_stop( audio_data_t * audio );
void audio_dma_in_stop( audio_data_t * audio );

uint32_t audio_driver_codec_cmd( audio_data_t * audio, uint32_t command, uint32_t param );

////////////////////////////////////////////////////////////////////////////////

static int audio_playback_callback( const void *inputBuffer, void *outputBuffer,
                                    unsigned long framesPerBuffer,
                                    const PaStreamCallbackTimeInfo* timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void *userData )
{
    audio_data_t *audio = ( audio_data_t * )userData; //  is the used defined data
    int16_t * p_audio;
    
    p_audio = ( int16_t * )outputBuffer;
    
    if( audio->on_audio_event != NULL )
    {
        audio->on_audio_event( audio, p_audio );
    }
    
    return 0;
}

static int audio_recording_callback( const void *inputBuffer, void *outputBuffer,
                                     unsigned long framesPerBuffer,
                                     const PaStreamCallbackTimeInfo* timeInfo,
                                     PaStreamCallbackFlags statusFlags,
                                     void *userData )
{
    audio_data_t *audio = ( audio_data_t * )userData; //  is the used defined data
    int16_t * p_audio;
    
    p_audio = ( int16_t * )inputBuffer;
    
    if( audio->on_audio_event != NULL )
    {
        audio->on_audio_event( audio, p_audio );
    }
    
    return 0;
}

uint32_t audio_driver_codec_cmd( audio_data_t * audio, uint32_t command, uint32_t param )
{
	return 0;
}

void audio_driver_init( audio_data_t * audio )
{
    PaError err;

    audio->handler = NULL;
    
    err = Pa_Initialize();
    if( err != paNoError ) return;

    if( audio->is_output == TRUE )
	{
		audio->handler = &wave_dev_out;

		wave_dev_out.streamParameters.device = Pa_GetDefaultOutputDevice();
		wave_dev_out.streamParameters.suggestedLatency = Pa_GetDeviceInfo( wave_dev_out.streamParameters.device )->defaultLowOutputLatency;
		wave_dev_out.streamParameters.channelCount = 2;
		wave_dev_out.streamParameters.sampleFormat = paInt16;
		wave_dev_out.streamParameters.hostApiSpecificStreamInfo = NULL;

		err = Pa_OpenStream( &wave_dev_out.stream,
							 NULL,
							 &wave_dev_out.streamParameters,
							 audio->sample_rate,
							 AUDIO_BUFFER_SIZE,
							 paClipOff,
							 audio_playback_callback,
							 audio );
	}
	else 
	{
		audio->handler = &wave_dev_in;

		wave_dev_in.streamParameters.device = Pa_GetDefaultInputDevice();
		wave_dev_in.streamParameters.suggestedLatency = Pa_GetDeviceInfo( wave_dev_in.streamParameters.device )->defaultLowInputLatency;
		wave_dev_in.streamParameters.channelCount = 2;
		wave_dev_in.streamParameters.sampleFormat = paInt16;
		wave_dev_in.streamParameters.hostApiSpecificStreamInfo = NULL;

		err = Pa_OpenStream( &wave_dev_in.stream,
							 &wave_dev_in.streamParameters,
							 NULL,
							 audio->sample_rate,
							 AUDIO_BUFFER_SIZE,
							 paClipOff,
							 audio_recording_callback,
							 audio );
	}

    if( err != paNoError ) return;
}

void audio_dma_out_start( audio_data_t * audio )
{
	wave_dev_t * dev_wave = ( wave_dev_t * )audio->handler;
    Pa_StartStream( dev_wave->stream );
}

void audio_dma_in_start( audio_data_t * audio )
{
	wave_dev_t * dev_wave = ( wave_dev_t * )audio->handler;
    Pa_StartStream( dev_wave->stream );
}

void audio_dma_out_stop( audio_data_t * audio )
{
	wave_dev_t * dev_wave = ( wave_dev_t * )audio->handler;
    Pa_StopStream( dev_wave->stream );
    Pa_CloseStream( dev_wave->stream );
}

void audio_dma_in_stop( audio_data_t * audio )
{
	wave_dev_t * dev_wave = ( wave_dev_t * )audio->handler;
    Pa_StopStream( dev_wave->stream );
    Pa_CloseStream( dev_wave->stream );
}

////////////////////////////////////////////////////////////////////////////////

static const audio_driver_t _win_audio_driver = 
{
	audio_driver_init,

	audio_driver_codec_cmd,

	audio_dma_out_start,
	audio_dma_in_start,
	audio_dma_out_stop,
	audio_dma_in_stop,
};

////////////////////////////////////////////////////////////////////////////////

audio_driver_t * audio_driver = ( audio_driver_t * )&_win_audio_driver;

#endif
