
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

#include <natus/audio/backend/wasapi/wasapi_capture_helper.h>

using namespace natus::core::types ;

//
// - test audio capture with wasapi loopback device
// using the internal helper
//
int main( int argc, char ** argv )
{
    natus::ntd::vector< float_t > samples ;

    natus::audio::wasapi_capture_helper_t hlp ;

    hlp.init( natus::audio::channels::mono, natus::audio::frequency::freq_48k ) ;

    {
        hlp.start() ;

        typedef std::chrono::high_resolution_clock clock_t ;
        clock_t::time_point tp = clock_t::now() ;

        while( std::chrono::duration_cast<std::chrono::seconds>( clock_t::now() - tp ) < std::chrono::seconds( 3 ) )
        {
            if( hlp.capture( samples ) )
            {
                // print data
                {
                    float_t accum = 0.0f ;
                    for( auto const s : samples ) accum += s ;

                    natus::log::global_t::status( std::to_string(accum / float_t(samples.size()))  ) ;
                }
                samples.clear() ;
            }
        }

        hlp.stop() ;
    }
    
    {
        hlp.start() ;

        typedef std::chrono::high_resolution_clock clock_t ;
        clock_t::time_point tp = clock_t::now() ;

        while( std::chrono::duration_cast<std::chrono::seconds>( clock_t::now() - tp ) < std::chrono::seconds( 10 ) )
        {
            if( hlp.capture( samples ) )
            {
                // print data
                {
                    float_t accum = 0.0f ;
                    for( auto const s : samples ) accum += s ;

                    natus::log::global_t::status( std::to_string(accum / float_t(samples.size()))  ) ;
                }
                samples.clear() ;
            }
        }
        hlp.stop() ;
    }

    hlp.release() ;

    return 0 ;
}
