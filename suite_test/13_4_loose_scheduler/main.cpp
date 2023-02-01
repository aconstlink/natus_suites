
#include "main.h"


#include <natus/concurrent/global.h>
#include <natus/log/global.h>


using namespace natus::core::types ;

// 
// testing the loose tasks. This is a task that 
// run on a rouge(outside thread pool) thread of
// execution.
//
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

    natus::concurrent::global_t::schedule( t0, natus::concurrent::schedule_type::loose ) ;

    while( run_loop )
    {
        natus::concurrent::global_t::update() ;
        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;
    }

    return 0 ;
}
