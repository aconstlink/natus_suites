
#include <natus/core/types.hpp>
using namespace natus::core::types ;

#include <vector>
#include <random>
#include <ctime>
#include <functional>

typedef std::function< void_t ( size_t const a, size_t const b ) > swap_funk_t ;

template< typename T >
void_t insertion_sort( size_t const a, size_t const b, std::vector< T > & points, swap_funk_t funk = [](size_t const a, size_t const b){} )
{
    for( size_t i=a+1; i<b; ++i )
    {
        size_t tmp = points[i] ;

        for( size_t j=i-1; j!=(a-1); --j )
        {
            size_t const i2 = j + 1 ;
            
            if( points[j] < tmp ) break ;
            points[i2] = points[j] ;
            points[j] = tmp ;
            funk( j - a, i2 - a ) ;
        }
    }
}

template< typename T >
void_t insertion_sort( std::vector< T > & points )
{
    insertion_sort( 0, points.size(), points ) ;
}

int main( int argc, char ** argv )
{
    // test 1
    {
        std::vector< size_t > points( {10,9,8,7,6,5,4,3,2,1} ) ;

        insertion_sort( points ) ;

        int bpt = 0 ;
        
    }
    
    // test 2
    {
        std::srand(std::time(nullptr));
        std::vector< size_t > points(10) ;
        for( size_t i=0; i<points.size(); ++i ) points[i] = rand() % 10 ;

        insertion_sort( points ) ;

        int bp = 0; 
    }

    // test 3
    {
        std::vector< size_t > points( {10,9,8,7,6,5,4,3,2,1} ) ;

        insertion_sort( 3, 8, points ) ;

        int bp = 0; 
    }

    // test 4
    {
        std::vector< size_t > points( {10,9,8,7,6,5,4,3,2,1} ) ;
        auto points2 = points ;

        insertion_sort( 3, 8, points, [&]( size_t const a, size_t const b )
        {
            auto const tmp = points2[ 3+a ] ;
            points2[ 3+a ] = points2[3+b] ;
            points2[ 3+b] = tmp ;
        } ) ;

        int bp = 0; 
    }

    return 0 ;
}
