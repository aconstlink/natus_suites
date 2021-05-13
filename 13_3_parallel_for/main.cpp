
#include "main.h"


#include <natus/concurrent/parallel_for.hpp>
#include <natus/log/global.h>
#include <algorithm>

using namespace natus::core::types ;

//
// testing the parallel_for
//
int main( int argc, char ** argv )
{
    natus::concurrent::mutex_t mtx ;
    typedef std::pair< std::thread::id, size_t > count_t ;
    natus::ntd::vector< count_t > counts ;
    natus::concurrent::semaphore_t task_counter ;
    
    
    auto inc_thread_counter = [&]( void_t )
    {
        auto const id = std::this_thread::get_id() ;

        natus::concurrent::lock_guard_t lk( mtx ) ;
        auto iter = std::find_if( counts.begin(), counts.end(), [&]( count_t const & c )
        {
            return c.first == id ;
        } ) ;
        if( iter != counts.end() ) iter->second++ ;
        else counts.emplace_back( count_t( id, 1 ) ) ;
    } ;

    // single parallel_for
    {
        size_t const n = 10000000000 ;
        natus::concurrent::semaphore_t loop_counter ;

        natus::concurrent::parallel_for<size_t>( 
        natus::concurrent::range_1d<size_t>(0,n), 
        [&]( natus::concurrent::range_1d<size_t> const & r )
        {
            inc_thread_counter() ;
            ++task_counter ;
            for( size_t i=r.begin(); i<r.end(); ++i )
            {
            }
            loop_counter.increment_by( r.difference() ) ;
        } ) ;

        natus_assert( loop_counter.value() == n) ;
    }

    // nested parallel_for
    {
        size_t const n1 = 137103 ;
        size_t const n2 = 100000 ;

        natus::concurrent::semaphore_t loop_counter ;

        natus::concurrent::parallel_for<size_t>( 
        natus::concurrent::range_1d<size_t>(0,n1), 
        [&]( natus::concurrent::range_1d<size_t> const & r0 )
        {
            inc_thread_counter() ;
            ++task_counter ;

            for( size_t i0=r0.begin(); i0<r0.end(); ++i0 )
            {
                natus::concurrent::parallel_for<size_t>( 
                natus::concurrent::range_1d<size_t>(0,n2), 
                [&]( natus::concurrent::range_1d<size_t> const & r )
                {
                    inc_thread_counter() ;
                    ++task_counter ;
                    for( size_t i=r.begin(); i<r.end(); ++i )
                    {
                    }
                    loop_counter.increment_by( r.difference() ) ;
                } ) ;
            }
        } ) ;

        natus_assert( loop_counter.value() == n1*n2) ;
    }
    
    return 0 ;
}
