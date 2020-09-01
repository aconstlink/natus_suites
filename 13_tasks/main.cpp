
#include "main.h"

#include <natus/log/global.h>
#include <natus/concurrent/task.hpp>
#include <natus/concurrent/isleep.hpp>

using namespace natus::core::types ;

//
// 
// 
//
int main( int argc, char ** argv )
{
    natus::concurrent::isleep_t isleep ;

    // [t1, t4, t2, t3] -> t5 -> t6
    auto t0 = natus::concurrent::task_t( [&]( natus::concurrent::task_ref_t self )
    {
        natus::log::global_t::status( "tasks 1" ) ;

        self.insert( [=] ( natus::concurrent::task_ref_t ) 
        { 
            natus::log::global_t::status( "tasks 2" ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) ) ;
        } ) ;

        self.insert( [=] ( natus::concurrent::task_ref_t )
        {
            natus::log::global_t::status( "tasks 3" ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;
        } ) ;

    } ).insert( [=] ( natus::concurrent::task_ref_t )
    {
        natus::log::global_t::status( "tasks 4" ) ;
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
    } ).then( [=] ( natus::concurrent::task_ref_t )
    {
        natus::log::global_t::status( "tasks 5 : merge task" ) ;
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
    } ).then( [&] ( natus::concurrent::task_ref_t )
    {
        natus::log::global_t::status( "tasks 6 : serial task" ) ;
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;

        isleep.interrupt() ;

    } ) ;

    // executer here


    while( !isleep.sleep_for( std::chrono::milliseconds(100) ) )
    {
        natus::log::global_t::status( "waiting for results" ) ;
    }

    natus::log::global_t::status( "Interrupted" ) ;

    return 0 ;
}
