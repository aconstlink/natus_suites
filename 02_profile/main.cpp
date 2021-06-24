
#include "main.h"

#include <natus/profile/global.h>
#include <natus/log/global.h>
#include <natus/ntd/vector.hpp>

using namespace natus::core::types ;

void_t test_local_system()
{
    natus::profile::system_t psys ;

    {
        natus::profile::entry_t e( "simple" ) ;

        for( size_t i=0; i<1000000; ++i ) {}

        psys.add_entry( std::move( e ) ) ;
    }

    {
        natus::profile::entry_t e( "simple" ) ;

        for( size_t i=0; i<1000000; ++i ) {}

        psys.add_entry( std::move( e ) ) ;
    }

    {
        natus::profile::entry_t e( "thread.group.my_unique" ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;

        psys.unique_entry( std::move( e ) ) ;
    }

    {
        natus::profile::entry_t e( "thread.group.my_unique" ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds(200) ) ;

        psys.unique_entry( std::move( e ) ) ;
    }

    natus::log::global_t::status( "******************************** Local System ********************************** " ) ;

    {
        auto const entries = psys.get_and_reset_entries() ;
        for( auto const & e : entries )
        {
            natus::log::global_t::status( e.get_name() + " : " + std::to_string( e.get_duration<std::chrono::microseconds>().count() ) + " [micro]" ) ;
        }
    }
}

void_t test_global_system()
{
    natus::log::global_t::status( "******************************** Global System ********************************** " ) ;
    {
        natus::profile::entry_t e( "thread.group.my_unique" ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds(200) ) ;

        natus::profile::global_t::sys().unique_entry( std::move( e ) ) ;
    }


    {
        auto const entries = natus::profile::global_t::sys().get_and_reset_entries() ;
        for( auto const & e : entries )
        {
            natus::log::global_t::status( e.get_name() + " : " + std::to_string( e.get_duration<std::chrono::microseconds>().count() ) + " [micro]" ) ;
        }
    }
}

//
int main( int argc, char ** argv )
{
    test_local_system() ;
    test_global_system() ;

    return 0 ;
}
