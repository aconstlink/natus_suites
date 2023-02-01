
#include <natus/audio/backend/oal/oal.h>

#include <natus/log/global.h>
#include <natus/math/vector/vector2.hpp>

#include <cstdlib>
#include <chrono>
#include <limits>
#include <thread>

typedef std::chrono::high_resolution_clock clk_t ;

int main( int argc, char ** argv )
{
    natus::audio::oal_backend_t backend ;
    backend.init() ;

    natus::audio::capture_object_res_t config = natus::audio::capture_object_t() ;
    
    backend.configure( natus::audio::capture_type::what_u_hear, config ) ;
    
    
    clk_t::duration d = std::chrono::milliseconds(0) ;
    clk_t::time_point tp = clk_t::now() ;

    while( d < std::chrono::seconds(10) )
    {
        // enable and do capture
        backend.capture( config ) ;
        backend.begin() ;
        backend.end() ;

        float_t avg = 0 ;
        natus::math::vec2f_t mm( 
            std::numeric_limits<float_t>::max(), 
            std::numeric_limits<float_t>::min() ) ;

        // grab data
        config->for_each_sample( [&] ( size_t const i, float_t const v ) 
        { 
            avg += v ;
            mm[ 0 ] = std::min( mm[ 0 ], v ) ;
            mm[ 1 ] = std::max( mm[ 1 ], v ) ;
        } ) ;

        avg /= float_t( config->num_samples() ) ;
        size_t const count = config->num_samples() ;

        natus::log::global_t::status( "[cnt, avg, minmax] : [" + std::to_string( count ) + "] " +
            "[" + std::to_string( avg ) + "(" + std::to_string( mm.x() ) + "," + std::to_string( mm.y() ) + "]" ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;
        d = std::chrono::duration_cast< std::chrono::milliseconds >( clk_t::now() - tp ) ;
    }

    // disable capture
    backend.capture( config, false ) ;
    
    return 0 ;
}
