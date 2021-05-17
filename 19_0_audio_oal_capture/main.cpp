
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
#include <thread>

//
// - just tests some OpenAL functions
// - test audio capture but system audio is not a standard capture
//
int main( int argc, char ** argv )
{
    auto const res = alcIsExtensionPresent( NULL, "ALC_ENUMERATION_EXT" ) ;
    if( res != ALC_TRUE ) std::exit( 1 ) ;

    natus::ntd::string_t whatuhear ;

    // print capture device
    {
        auto const * list = alcGetString( NULL, ALC_CAPTURE_DEVICE_SPECIFIER ) ;
        size_t sib = 0 ;
        while( true )
        {
            natus::ntd::string_t item = list + sib  ;

            if( item.size() == 0 ) break ;

            natus::log::global_t::status( item ) ;
            sib += item.size() + 1 ;

            if( item.find( "What U Hear" ) != std::string::npos )
            {
                whatuhear = item ;
                break ;
            }
        }
    }

    if( whatuhear.empty() )
    {
        natus::log::global_t::status("Can not find 'what you hear' capture device" ) ;
        return 1 ;
    }

    // print what you hear device
    {
        natus::log::global_t::status( "What you hear device:" ) ;
        ALCdevice* dev = alcCaptureOpenDevice( whatuhear.c_str(), 96000, AL_FORMAT_STEREO16, 32768 ) ;
        auto const* s = alcGetString( dev, ALC_CAPTURE_DEVICE_SPECIFIER ) ;
        natus::log::global_t::status( s ) ;

        ALCint size[20] ;
        alcGetIntegerv( dev, ALC_ALL_ATTRIBUTES, 9, size ) ;

        alcCaptureCloseDevice( dev ) ;

        natus::log::global_t::status( "***************************" ) ;
    }

    // print default device
    {
        natus::log::global_t::status( "Default Capture Device: " ) ;
        ALCdevice* dev = alcCaptureOpenDevice( NULL, 96000, AL_FORMAT_STEREO16, 32768 ) ;
        auto const* s = alcGetString( dev, ALC_CAPTURE_DEVICE_SPECIFIER ) ;
        natus::log::global_t::status( s ) ;
        alcCaptureCloseDevice( dev ) ;
        natus::log::global_t::status( "***************************" ) ;
    }


    // Make some capture on "What U Hear"
    {
        natus::log::global_t::status( "Make some capture" ) ;
        ALCdevice* dev = alcCaptureOpenDevice( whatuhear.c_str(), 96000, AL_FORMAT_STEREO16, 32768 ) ;

        alcCaptureStart( dev ) ;

        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point tp = __clock_t::now() ;

        natus::ntd::vector< ALbyte > buffer ;

        size_t const channels = 2 ;

        // format stereo and 16
        size_t const frame_size = channels * 16 / 8 ;

        while( true ) 
        {
            ALCint count = 0 ;
            alcGetIntegerv( dev, ALC_CAPTURE_SAMPLES, 1, &count ) ;

            buffer.resize( size_t(count) * frame_size) ;

            alcCaptureSamples( dev, buffer.data(), count  ) ;
            if( alcGetError(dev) != AL_NO_ERROR )
            {
                continue;
            }

            //natus::log::global_t::status( "Samples : " + std::to_string( buffer.size() ) ) ;

            int avg[ 2 ] = { 0, 0 } ;
            natus::math::vec2i_t mm[ 2 ] = {
                natus::math::vec2i_t( std::numeric_limits<int>::max(), std::numeric_limits<int>::min() ),
                natus::math::vec2i_t( std::numeric_limits<int>::max(), std::numeric_limits<int>::min() ) } ;

            if( count > 0 )
            {
                for( size_t i = 0; i < count; ++i )
                {
                    for( size_t j=0; j<channels; ++j )
                    {
                        size_t const idx = i * frame_size + j ;

                        int value = int( buffer[ idx + 1 ] << 8 ) & int( buffer[ idx + 0 ] << 0 ) ;

                        avg[j] += int( value ) ;
                        mm[j] = natus::math::vec2i_t( 
                            std::min( mm[j].x(), int(value ) ),
                            std::max( mm[j].y(), int(value  ) ) ) ;
                    }
                }
                for( size_t j=0; j<channels; ++j )
                    avg[j] /= int(frame_size) * count ;
            }
            natus::log::global_t::status( "[cnt, avg, minmax] : [" + std::to_string(count) + "] " + 
                "[" + std::to_string( avg[0] ) + "," + std::to_string( avg[1] ) + "]" +
                "(" + std::to_string( mm[0].x() ) + "," + std::to_string( mm[0].y() ) + "]" ) ;
            
            
            
            auto const s = std::chrono::duration_cast< std::chrono::seconds >( __clock_t::now() - tp ).count() ;
            if( s > 10 ) break ;
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;
        }

        alcCaptureStop( dev ) ;

        alcCaptureCloseDevice( dev ) ;
    }

    return 0 ;
}
