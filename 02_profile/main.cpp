
#include "main.h"

#include <natus/profile/global.h>
#include <natus/log/global.h>
#include <natus/ntd/vector.hpp>

using namespace natus::core::types ;

//
int main( int argc, char ** argv )
{

    natus::profile::system_t psys ;

    {
        natus::profile::entry_t e( "simple" ) ;

        for( size_t i=0; i<1000000; ++i ) {}

        psys.make_entry( std::move( e ) ) ;
    }

    {
        natus::profile::entry_t e( "simple" ) ;

        for( size_t i=0; i<1000000; ++i ) {}

        psys.make_entry( std::move( e ) ) ;
    }

    {
        natus::profile::entry_t e( "in_thread", "in_group", "my_unique" ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;

        psys.unique_entry( std::move( e ) ) ;
    }

    {
        natus::profile::entry_t e( "in_thread", "in_group", "my_unique" ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds(200) ) ;

        psys.unique_entry( std::move( e ) ) ;
    }

    {
        auto const entries = psys.get_entries() ;
        for( auto const & e : entries )
        {
            natus::log::global_t::status( e.get_key() + " : " + std::to_string( e.get_duration<std::chrono::microseconds>().count() ) + " [micro]" ) ;
        }
    }
    return 0 ;
}
