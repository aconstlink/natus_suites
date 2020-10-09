
#include "main.h"

#include <natus/audio/backend/oal/oal.h>
#include <natus/audio/buffer/pcm_buffer.hpp>

#include <natus/log/global.h>

#include <cstdlib>
#include <chrono>
#include <limits>

typedef std::chrono::high_resolution_clock clk_t ;

int main( int argc, char ** argv )
{
    natus::audio::oal_backend_t backend ;

    natus::audio::capture_object_res_t config = natus::audio::capture_object_t(
        natus::audio::capture_type::what_u_hear, natus::audio::channels::mono ) ;
        
    {
        backend.configure( config ) ;
    }
    
    clk_t::duration d = std::chrono::milliseconds(0) ;
    clk_t::time_point tp = clk_t::now() ;

    while( d < std::chrono::seconds(10) )
    {
        // enable and do capture
        backend.capture( config, true ) ;

        // grab data

        std::this_thread::sleep_for( std::chrono::milliseconds( 19 ) ) ;
        d = std::chrono::duration_cast< std::chrono::milliseconds >( clk_t::now() - tp ) ;
    }

    // disable capture
    backend.capture( config, false ) ;
    
    return 0 ;
}
