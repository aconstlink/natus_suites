
#include "main.h"

#include <natus/concurrent/task/loose_thread_scheduler.hpp>
#include <natus/log/global.h>


using namespace natus::core::types ;

int main( int argc, char ** argv )
{
    bool_t run_loop = true ;

    natus::concurrent::task_res_t t0 = natus::concurrent::task_t( [=]( natus::concurrent::task_res_t )
    {
        natus::log::global_t::status( "t0" ) ;
    } ) ;

    natus::concurrent::task_res_t t1 = natus::concurrent::task_t( [=]( natus::concurrent::task_res_t )
    {
        natus::log::global_t::status( "t1" ) ;
    } ) ;

    natus::concurrent::task_res_t t2 = natus::concurrent::task_t( [=]( natus::concurrent::task_res_t )
    {
        natus::log::global_t::status( "t2" ) ;
    } ) ;

    natus::concurrent::task_res_t t3 = natus::concurrent::task_t( [&]( natus::concurrent::task_res_t )
    {
        run_loop = false ;
        natus::log::global_t::status( "good bye" ) ;
    } ) ;

    natus::concurrent::task_res_t t4 = natus::concurrent::task_t( [=]( natus::concurrent::task_res_t )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;
        natus::log::global_t::status( "t4" ) ;
    } ) ;

    natus::concurrent::task_res_t t5 = natus::concurrent::task_t( [=]( natus::concurrent::task_res_t )
    {
        natus::log::global_t::status( "t5" ) ;
    } ) ;

    // build graph
    {
        t0->then( t1 )->then( t2 )->then( t3 ) ;
        t0->in_between( t4 ) ;
        t0->in_between( t5 ) ;
    }

    natus::concurrent::loose_thread_scheduler lts ;
    lts.init() ;

    lts.schedule( t0 ) ;

    while( run_loop )
    {
        lts.update() ;
        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;
    }

    lts.deinit() ;

    return 0 ;
}
