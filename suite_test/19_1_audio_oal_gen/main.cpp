
#include "main.h"

#include <natus/ntd/string.hpp>
#include <natus/log/global.h>
#include <natus/ntd/vector.hpp>
#include <natus/math/vector/vector2.hpp>

#include <AL/alc.h>
#include <AL/al.h>
#include <AL/alext.h>

#include <cstdlib>
#include <chrono>
#include <limits>
#include <cstring>
#include <thread>
//
// - generate some tones
//

#ifndef M_PI
#define M_PI    (3.14159265358979323846)
#endif

/* InitAL opens a device and sets up a context using default attributes, making
 * the program ready to call OpenAL functions. */
int InitAL(char ***argv, int *argc)
{
    const ALCchar *name;
    ALCdevice *device;
    ALCcontext *ctx;

    /* Open and initialize a device */
    device = NULL;
    if(argc && argv && *argc > 1 && strcmp((*argv)[0], "-device") == 0)
    {
        device = alcOpenDevice((*argv)[1]);
        if(!device)
            fprintf(stderr, "Failed to open \"%s\", trying default\n", (*argv)[1]);
        (*argv) += 2;
        (*argc) -= 2;
    }
    if(!device)
        device = alcOpenDevice(NULL);
    if(!device)
    {
        fprintf(stderr, "Could not open a device!\n");
        return 1;
    }

    ctx = alcCreateContext(device, NULL);
    if(ctx == NULL || alcMakeContextCurrent(ctx) == ALC_FALSE)
    {
        if(ctx != NULL)
            alcDestroyContext(ctx);
        alcCloseDevice(device);
        fprintf(stderr, "Could not set a context!\n");
        return 1;
    }

    name = NULL;
    if(alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT"))
        name = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
    if(!name || alcGetError(device) != AL_NO_ERROR)
        name = alcGetString(device, ALC_DEVICE_SPECIFIER);
    printf("Opened \"%s\"\n", name);

    return 0;
}

/* CloseAL closes the device belonging to the current context, and destroys the
 * context. */
void CloseAL(void)
{
    ALCdevice *device;
    ALCcontext *ctx;

    ctx = alcGetCurrentContext();
    if(ctx == NULL)
        return;

    device = alcGetContextsDevice(ctx);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
}

enum WaveType {
    WT_Sine,
    WT_Square,
    WT_Sawtooth,
    WT_Triangle,
    WT_Impulse,
    WT_WhiteNoise,
};

static inline ALuint dither_rng( ALuint* seed )
{
    *seed = ( *seed * 96314165 ) + 907633515;
    return *seed;
}

static void ApplySin( ALfloat* data, ALdouble g, ALuint srate, ALuint freq )
{
    ALdouble smps_per_cycle = ( ALdouble ) srate / freq;
    ALuint i;
    for( i = 0; i < srate; i++ )
    {
        ALdouble ival;
        data[ i ] += ( ALfloat ) ( sin( modf( i / smps_per_cycle, &ival ) * 2.0 * M_PI ) * g );
    }
}

/* Generates waveforms using additive synthesis. Each waveform is constructed
 * by summing one or more sine waves, up to (and excluding) nyquist.
 */
static ALuint CreateWave(enum WaveType type, ALuint freq, ALuint srate)
{
    ALuint seed = 22222;
    ALuint data_size;
    
    ALuint buffer;
    ALenum err;
    ALuint i;

    data_size = (ALuint)(srate * sizeof(ALfloat));
    natus::ntd::vector< ALfloat > data( data_size ) ;

    switch(type)
    {
        case WT_Sine:
            ApplySin(data.data(), 1.0, srate, freq);
            break;
        case WT_Square:
            for(i = 1;freq*i < srate/2;i+=2)
                ApplySin(data.data(), 4.0/M_PI * 1.0/i, srate, freq*i);
            break;
        case WT_Sawtooth:
            for(i = 1;freq*i < srate/2;i++)
                ApplySin(data.data(), 2.0/M_PI * ((i&1)*2 - 1.0) / i, srate, freq*i);
            break;
        case WT_Triangle:
            for(i = 1;freq*i < srate/2;i+=2)
                ApplySin(data.data(), 8.0/(M_PI*M_PI) * (1.0 - (i&2)) / (i*i), srate, freq*i);
            break;
        case WT_Impulse:
            /* NOTE: Impulse isn't handled using additive synthesis, and is
             * instead just a non-0 sample at a given rate. This can still be
             * useful to test (other than resampling, the ALSOFT_DEFAULT_REVERB
             * environment variable can prove useful here to test the reverb
             * response).
             */
            for(i = 0;i < srate;i++)
                data[i] = (i%(srate/freq)) ? 0.0f : 1.0f;
            break;
        case WT_WhiteNoise:
            /* NOTE: WhiteNoise is just uniform set of uncorrelated values, and
             * is not influenced by the waveform frequency.
             */
            for(i = 0;i < srate;i++)
            {
                ALuint rng0 = dither_rng(&seed);
                ALuint rng1 = dither_rng(&seed);
                data[i] = (ALfloat)(rng0*(1.0/std::numeric_limits<natus::core::uint_t>::max()) - rng1*(1.0/std::numeric_limits<natus::core::uint_t>::max()));
            }
            break;
    }

    /* Buffer the audio data into a new buffer object. */
    buffer = 0;
    alGenBuffers( 1, &buffer );
    alBufferData( buffer, AL_FORMAT_MONO_FLOAT32, data.data(), ( ALsizei ) data_size, ( ALsizei ) srate );
    
    /* Check if an error occured, and clean up if so. */
    err = alGetError();
    if( err != AL_NO_ERROR )
    {
        fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
        if(alIsBuffer(buffer))
            alDeleteBuffers(1, &buffer);
        return 0;
    }

    return buffer;
}

int main( int argc, char ** argv )
{
    enum WaveType wavetype = WT_Sine;
    const char* appname = argv[ 0 ];
    ALuint source, buffer;
    ALint last_pos, num_loops;
    ALint max_loops = 4;
    ALint srate = -1;
    ALint tone_freq = 100;
    ALCint dev_rate;
    ALenum state;
    //int i;

    argv++; argc--;
    if( InitAL( &argv, &argc ) != 0 )
        return 1;

    if( !alIsExtensionPresent( "AL_EXT_FLOAT32" ) )
    {
        fprintf( stderr, "Required AL_EXT_FLOAT32 extension not supported on this device!\n" );
        CloseAL();
        return 1;
    }

    {
        ALCdevice* device = alcGetContextsDevice( alcGetCurrentContext() );
        alcGetIntegerv( device, ALC_FREQUENCY, 1, &dev_rate );
        if( alGetError() != AL_NO_ERROR )
        {
            natus::log::global_t::error( "Failed to get device sample rate" ) ;
            exit( 1 ) ;
        }
    }
    if( srate < 0 )
        srate = dev_rate;

    /* Load the sound into a buffer. */
    buffer = CreateWave( wavetype, ( ALuint ) tone_freq, ( ALuint ) srate );
    if( !buffer )
    {
        CloseAL();
        return 1;
    }

    /*printf( "Playing %dhz %s-wave tone with %dhz sample rate and %dhz output, for %d second%s...\n",
        tone_freq, GetWaveTypeName( wavetype ), srate, dev_rate, max_loops + 1, max_loops ? "s" : "" );
    fflush( stdout );*/

    /* Create the source to play the sound with. */
    source = 0;
    alGenSources( 1, &source );
    alSourcei( source, AL_BUFFER, ( ALint ) buffer );
    if( alGetError() != AL_NO_ERROR )
    {
        natus::log::global_t::error( "Failed to setup sound source" ) ;
        exit( 1 ) ;
    }

    /* Play the sound for a while. */
    num_loops = 0;
    last_pos = 0;
    alSourcei( source, AL_LOOPING, ( max_loops > 0 ) ? AL_TRUE : AL_FALSE );
    alSourcePlay( source );
    do {
        ALint pos;
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        alGetSourcei( source, AL_SAMPLE_OFFSET, &pos );
        alGetSourcei( source, AL_SOURCE_STATE, &state );
        if( pos < last_pos && state == AL_PLAYING )
        {
            ++num_loops;
            if( num_loops >= max_loops )
                alSourcei( source, AL_LOOPING, AL_FALSE );
            printf( "%d...\n", max_loops - num_loops + 1 );
            fflush( stdout );
        }
        last_pos = pos;
    } while( alGetError() == AL_NO_ERROR && state == AL_PLAYING );

    /* All done. Delete resources, and close OpenAL. */
    alDeleteSources( 1, &source );
    alDeleteBuffers( 1, &buffer );

    /* Close up OpenAL. */
    CloseAL();

    return 0;

    return 0 ;
}
