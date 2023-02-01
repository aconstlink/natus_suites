
#include "main.h"

#include <natus/ntd/string.hpp>
#include <natus/log/global.h>
#include <natus/ntd/vector.hpp>
#include <natus/math/vector/vector2.hpp>

#include <cstdlib>
#include <chrono>
#include <limits>
#include <thread>
#include <functional>

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <windows.h>

// Testing audio capture of system output/mix using wasapi
// https://docs.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording
// https://docs.microsoft.com/en-us/windows/win32/coreaudio/capturing-a-stream

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

using namespace natus::core::types ;

#if 0
struct loopback_device
{
    natus_this_typedefs( loopback_device ) ;

public:

    loopback_device( void_t ) {}
    loopback_device( this_cref_t ) = delete ;
    loopback_device( this_rref_t rhv )
    {
        natus_move_member_ptr( def_device_ptr, rhv ) ;
        natus_move_member_ptr( audio_client_ptr, rhv ) ;
        natus_move_member_ptr( wav_format_ptr, rhv ) ;
        natus_move_member_ptr( capture_client_ptr, rhv ) ;

    }
    ~loopback_device( void_t ) {}

public:

    IMMDevice * def_device_ptr = NULL ;
    IAudioClient *audio_client_ptr = NULL;
    IAudioCaptureClient * capture_client_ptr = NULL ;

    // format see: mmreg.h
    WAVEFORMATEX * wav_format_ptr = NULL ;
};
#endif
//
// - test audio capture with wasapi loopback device
//
int main( int argc, char ** argv )
{
    IMMDeviceEnumerator *pEnumerator = NULL ;
    IMMDevice *pDevice = NULL ;

    // format see: mmreg.h
    WAVEFORMATEX *pwfx = NULL ;
    IAudioClient *pAudioClient = NULL;
    UINT32 bufferFrameCount;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    IAudioCaptureClient *pCaptureClient = NULL;

    WAVEFORMATEXTENSIBLE * fmt_ext = NULL ;

    bool_t is_float = false ;

    //so_audio::ipcm_buffer_ptr_t the_buffer = nullptr ;

    {
        auto const res = CoInitializeEx( NULL, COINIT_MULTITHREADED ) ;
        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : CoInitializeEx" ) ;
            return 1 ;
        }
    }

    {
        HRESULT const hr = CoCreateInstance(
            CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_ALL, IID_IMMDeviceEnumerator,
            (void**)&pEnumerator ) ;

        if( hr != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : CoCreateInstance" ) ;
            return 1 ;
        }
    }

    {
        auto const res = pEnumerator->GetDefaultAudioEndpoint(
            eRender, eConsole, &pDevice ) ;

        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : GetDefaultAudioEndpoint" ) ;
            return 1 ;
        }
    }

    {
        auto const res = pDevice->Activate( IID_IAudioClient, CLSCTX_ALL,
            NULL, (void**)&pAudioClient );

        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : Device Activate" ) ;
            return 1 ;
        }
    }

    std::function< void_t ( BYTE const * buffer, UINT32 const num_frames, natus::ntd::vector< float_t > & ) > copy_funk =
        [=]( BYTE const * buffer, UINT32 const num_frames, natus::ntd::vector< float_t > & ){} ;
    
    {
        auto const res = pAudioClient->GetMixFormat( &pwfx ) ;
        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : GetMixFormat" ) ;
            return 1 ;
        }

        if( pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
        {
            fmt_ext = (WAVEFORMATEXTENSIBLE*)pwfx ;

            if( fmt_ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM )
            {
                int const bp = 0 ;

                // create integral type buffer
                if( pwfx->wBitsPerSample == 8 )
                {
                    copy_funk = [=]( BYTE const * buffer, UINT32 const num_frames, natus::ntd::vector< float_t > & samples )
                    {
                        samples.resize( num_frames ) ;
                        for( size_t i=0; i<num_frames; ++i )
                        {
                            samples[i] = float_t(double_t(int8_cptr_t(buffer)[i])/125.0) ;
                        }
                    } ;
                }
                else if( pwfx->wBitsPerSample == 16 )
                {
                    copy_funk = [=]( BYTE const * buffer, UINT32 const num_frames, natus::ntd::vector< float_t > & samples )
                    {
                        samples.resize( num_frames ) ;
                        for( size_t i=0; i<num_frames; ++i )
                        {
                            samples[i] = float_t(double_t(int16_cptr_t(buffer)[i])/32768.0) ;
                        }
                    } ;
                }
                else if( pwfx->wBitsPerSample == 32 )
                {
                    copy_funk = [=]( BYTE const * buffer, UINT32 const num_frames, natus::ntd::vector< float_t > & samples )
                    {
                        samples.resize( num_frames ) ;
                        for( size_t i=0; i<num_frames; ++i )
                        {
                            samples[i] = float_t(double_t(int32_cptr_t(buffer)[i])/2147483648.0) ;
                        }
                    } ;
                }

            }
            else if( fmt_ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT )
            {
                copy_funk = [=]( BYTE const * buffer, UINT32 const num_frames, natus::ntd::vector< float_t > & samples )
                {
                    samples.resize( num_frames ) ;
                    for( size_t i=0; i<num_frames; ++i )
                    {
                        samples[i] = float_cptr_t(buffer)[i] ;
                    }
                } ;
            }
        }
    }

    {
        auto const res = pAudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration,
            0, pwfx, NULL ) ;

        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : AudioClient Initialize" ) ;
            return 1 ;
        }
    }

    {
        auto const res = pAudioClient->GetBufferSize( &bufferFrameCount ) ;
        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : GetBufferSize" ) ;
            return 1 ;
        }
    }

    {
        auto const res = pAudioClient->GetService( IID_IAudioCaptureClient, (void**)&pCaptureClient ) ;
        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : AudioClient GetService" ) ;
            return 1 ;
        }
    }

    {
        auto const res = pAudioClient->Start() ;
        if( res != S_OK )
        {
            natus::log::global::error( "[wasapi_engine::initialize] : AudioClient GetService" ) ;
            return 1 ;
        }
    }

    size_t const num_bits_per_sample = pwfx->wBitsPerSample ;

    {
        natus::ntd::vector< float_t > samples ;

        typedef std::chrono::high_resolution_clock clock_t ;
        clock_t::time_point tp = clock_t::now() ;

        while( std::chrono::duration_cast<std::chrono::seconds>( clock_t::now() - tp ) < std::chrono::seconds( 10 ) )
        {
            UINT32 packetLength = 0 ;

            // read available data
            {
                auto const res = pCaptureClient->GetNextPacketSize( &packetLength ) ;
                if( res != S_OK )
                {
                    natus::log::global::error( "[wasapi_engine] : GetNextPacketSize" ) ;
                    break ;
                }
            }

            while( packetLength != 0 )
            {
                // get loopback data
                BYTE *data_ptr ;
                UINT32 num_frames_available ;
                DWORD flags ;

                {
                    auto const res = pCaptureClient->GetBuffer( &data_ptr,
                        &num_frames_available, &flags, NULL, NULL ) ;

                    if( res != S_OK )
                    {
                        natus::log::global::error( "[wasapi_engine] : unable to get audio buffer."
                            "Will cancel thread." ) ;
                        break ;
                    }
                }

                copy_funk( data_ptr, num_frames_available, samples ) ;

                // release wasapi buffer
                {
                    auto const res = pCaptureClient->ReleaseBuffer( num_frames_available ) ;
                    if( res != S_OK )
                    {
                        natus::log::global::error( "[wasapi_engine] : ReleaseBuffer" ) ;
                        break ;
                    }
                }

                // print data
                {
                    float_t accum = 0.0f ;
                    for( auto const s : samples ) accum += s ;

                    natus::log::global_t::status( std::to_string(accum / float_t(samples.size()))  ) ;
                    
                }

                // read available data
                {
                    auto const res = pCaptureClient->GetNextPacketSize( &packetLength ) ;
                    if( res != S_OK )
                    {
                        natus::log::global::error( "[wasapi_engine] : GetNextPacketSize" ) ;
                        break ;
                    }
                }
            }
        }
    }

    return 0 ;
}
