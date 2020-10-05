
#include "main.h"

#include <natus/audio/buffer/pcm_buffer.hpp>
#include <natus/log/global.h>

#include <cstdlib>
#include <chrono>
#include <limits>

int main( int argc, char ** argv )
{
    typedef natus::audio::pcm_buffer< uint8_t, 2 > buffer_t ;
    buffer_t buffer ;

    {
        buffer_t::sample_t smp ;
        smp.left( 0 ).right( 1 ) ;

        buffer.push_back( smp ) ;
    }

    natus::audio::ibuffer_res_t bres = natus::memory::res<buffer_t>(buffer) ;

    {
        if( natus::memory::res<buffer_t>::castable( bres ) )
        {
            natus::log::global_t::status("ok, is castable") ;
        }
    }
    
    return 0 ;
}
