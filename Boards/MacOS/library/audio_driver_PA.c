#ifdef WIN32
#include <SDL.h>
#include <Windows.h>
#include <conio.h>
#elif defined __MACH__
#include <SDL2/SDL.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "audio_layer.h"
#include "portaudio.h"
#include "system.h"

///////////////////////////////////////////////////////////////

#define AUDIO_INDEX_OUTPUT              0
#define AUDIO_INDEX_INPUT               1

#define AUDIO_TEMP_BUFFER_COUNT         4

#define AUDIO_CHANNELS                  2
#define AUDIO_SAMPLE_COUNTER            160

///////////////////////////////////////////////////////////////

typedef struct WaveDeviceDataT
{
    PaStreamParameters streamParameters;
    PaStream *stream;

    int device;
    uint32_t sample_rate;
    pipe_data_t pipe;
    int16_t buffer[AUDIO_SAMPLE_COUNTER * AUDIO_CHANNELS * AUDIO_TEMP_BUFFER_COUNT];
    uint32_t pointer;

} audio_dev_t;

////////////////////////////////////////////////////////////////////////////////

static audio_dev_t audio_devs[2];

////////////////////////////////////////////////////////////////////////////////

static int audio_recording_callback( const void *inputBuffer, void *outputBuffer,
                                     unsigned long framesPerBuffer,
                                     const PaStreamCallbackTimeInfo* timeInfo,
                                     PaStreamCallbackFlags statusFlags,
                                     void *userData )
{
    audio_dev_t * audio_dev = ( audio_dev_t * )userData; //  is the used defined data
    const int16_t * p_audio = ( const int16_t * )inputBuffer;
    uint16_t size = AUDIO_SAMPLE_COUNTER * AUDIO_CHANNELS * sizeof( int16_t );
    
    if( p_audio != NULL ) memcpy( &audio_dev->buffer[audio_dev->pointer], p_audio, size );
    else memset( &audio_dev->buffer[audio_dev->pointer], 0x00, size );
    
    pipe_write( &audio_dev->pipe, ( uint8_t * )&audio_dev->buffer[ audio_dev->pointer ], size );
    
    audio_dev->pointer += AUDIO_SAMPLE_COUNTER * AUDIO_CHANNELS;
    if( audio_dev->pointer >= ( AUDIO_SAMPLE_COUNTER * AUDIO_CHANNELS * AUDIO_TEMP_BUFFER_COUNT ) )
    {
        audio_dev->pointer = 0;
    }
    
    return 0;
}

static int audio_playback_callback( const void *inputBuffer, void *outputBuffer,
                                    unsigned long framesPerBuffer,
                                    const PaStreamCallbackTimeInfo* timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void *userData )
{
    audio_dev_t * audio_dev = ( audio_dev_t * )userData; //  is the used defined data
    int16_t * p_audio = ( int16_t * )outputBuffer;
    uint16_t size = AUDIO_SAMPLE_COUNTER * AUDIO_CHANNELS * sizeof( int16_t );
    
    memcpy( p_audio, &audio_dev->buffer[audio_dev->pointer], size );

    audio_dev->pointer += AUDIO_SAMPLE_COUNTER * AUDIO_CHANNELS;
    if( audio_dev->pointer >= ( AUDIO_SAMPLE_COUNTER * AUDIO_CHANNELS * AUDIO_TEMP_BUFFER_COUNT ) )
    {
        audio_dev->pointer = 0;
    }
    
    pipe_write( &audio_dev->pipe, ( uint8_t * )&audio_dev->buffer[ audio_dev->pointer ], size );

    return 0;
}

static int hw_init_audio_ex( int rec, audio_dev_t * audio_dev )
{
    PaError err;

    err = Pa_Initialize();
    if( err != paNoError ) return 0;

    pipe_init( &audio_dev->pipe, rec ? "PIPE_AUDIO_REC" : "PIPE_AUDIO_PLAY", AUDIO_TEMP_BUFFER_COUNT );
    memset( &audio_dev->buffer, 0x00, sizeof( audio_dev->buffer ) );
    audio_dev->pointer = 0;

    audio_dev->streamParameters.device           = rec ? Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice();
    audio_dev->streamParameters.suggestedLatency = rec ? Pa_GetDeviceInfo( audio_dev->streamParameters.device )->defaultLowInputLatency :
                                                         Pa_GetDeviceInfo( audio_dev->streamParameters.device )->defaultLowOutputLatency;
    audio_dev->streamParameters.channelCount = 2;
    audio_dev->streamParameters.sampleFormat = paInt16;
    audio_dev->streamParameters.hostApiSpecificStreamInfo = NULL;
    
    err = Pa_OpenStream( &audio_dev->stream,
                         rec ? &audio_dev->streamParameters : NULL,
                         rec ? NULL : &audio_dev->streamParameters,
                         audio_dev->sample_rate,
                         AUDIO_BUFFER_SIZE,
                         paClipOff,
                         rec ? audio_recording_callback : audio_playback_callback,
                         audio_dev );
    if( err != paNoError ) return 0;
    
    err = Pa_StartStream( audio_dev->stream );
    if( err != paNoError ) return 0;

    return rec + 1;
}

static void hw_close_audio( audio_dev_t * audio_dev )
{
    Pa_StopStream( audio_dev->stream );
    Pa_CloseStream( audio_dev->stream );
}

///////////////////////////////////////////////////////////////

static void audio_driver_global_init( void )
{
    audio_devs[0].device = FALSE;
    audio_devs[1].device = FALSE;
    
    Pa_Initialize();
#if 0
    int total = ( int )Pa_GetDeviceCount();
    
    for( int i = 0; i < total; i++ )
    {
        const PaDeviceInfo * info = Pa_GetDeviceInfo( (PaDeviceIndex)i );
        printf( "PA info %s\n", info->name );
    }
#endif
}

static void audio_driver_open( uint32_t index )
{
    if( audio_devs[index].device != 0 ) return;
    audio_devs[index].device = hw_init_audio_ex( index, &audio_devs[index] );
}

static void audio_driver_close( uint32_t index )
{
    if( audio_devs[index].device == 0 ) return;
    hw_close_audio( &audio_devs[index] );
}

static int audio_driver_get_index( char * param )
{
    char temp_param[20];
    char * arg0, * arg1;
    uint8_t audio_mask;
    int index = -1;
    
    if( param[0] != '?' ) return -1;
    
    strncpy( temp_param, param, 19 );
    
    arg0 = &temp_param[1];
    arg1 = strchr( arg0, ',' ); if( arg1 == NULL ) return -1; *arg1 = 0; arg1++;
    
    audio_mask = 0;
    
    if( !strcmp( arg1, "out" ) ) index = AUDIO_INDEX_OUTPUT;
    else if( !strcmp( arg1, "in" ) ) index = AUDIO_INDEX_INPUT;
    else return -1;
    
    audio_devs[index].sample_rate = atoi(arg0);
    //if( !strcmp( arg0, "8000" ) ) sample_rate = 8000;
    //else return -1;
    
    return index;
}

static bool_t audio_driver_validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

static uint32_t audio_driver_io_ctrl( uint32_t index, uint32_t param, void * value )
{
    if( param == DEVICE_SET_RUNTIME_VALUE )
    {
        // set volume to the audio DAC, if there is any
    }
    
    return 0;
}

static pipe_data_t * audio_input_pipe( void )
{
    return &audio_devs[AUDIO_INDEX_INPUT].pipe;
}

static pipe_data_t * audio_output_pipe( void )
{
    return &audio_devs[AUDIO_INDEX_OUTPUT].pipe;
}

///////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t audio_device_driver =
{
    "audio?",
    &_in_use,
    
    audio_driver_open,
    audio_driver_close,
    NULL,
    audio_driver_validate,
    audio_driver_get_index,
    audio_driver_io_ctrl,
    NULL,
    
    DEV_MODE_PIPE,
    
    audio_input_pipe,
    audio_output_pipe,
};

DECLARE_DEVICE_SECTION(audio_device_driver);
DECLARE_INIT_CRITICAL_SECTION(audio_driver_global_init);
