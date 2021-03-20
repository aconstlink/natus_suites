
#include "main.h"

#include <natus/log/global.h>
#include <future>

using namespace natus::core::types ;

int main( int argc, char ** argv )
{
    auto f0 = std::async( std::launch::async, [] () 
    { 
        natus::log::global_t::status( "async 0.0" ) ;
        

        auto f1 = std::async( std::launch::async, [] ()
        {
            natus::log::global_t::status( "async 1 : begin" ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) ) ;
            natus::log::global_t::status( "async 1 : end" ) ;
        } ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
        natus::log::global_t::status( "async 0 : waiting for f1" ) ;
        f1.wait() ;

    } ) ;

    auto f1 = std::async( std::launch::async, [] ()
    {
        natus::log::global_t::status( "async 0.1" ) ;
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
    } ) ;

    natus::log::global_t::status( "start waiting in main" ) ;
    f0.wait() ;
    f1.wait() ;

    return 0 ;
}
