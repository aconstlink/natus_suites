
#include "main.h"

#include <natus/nsl/ast/nodes.hpp>
#include <natus/nsl/ast/components.hpp>
#include <natus/nsl/ast/visitors/visitor.hpp>

#include <natus/format/global.h>
#include <natus/io/database.h>
#include <natus/log/global.h>

using namespace natus::core::types ;


template< typename T >
T make_some_components( T && nin )
{
    {
        natus::nsl::version_component_t comp ;
        comp.add( {"natus"} ).add( { "gl3", "es3" } ) ;
        nin.add_component( std::move( comp ) ) ;
    }

    return std::move( nin ) ;
}

natus::nsl::group_t make_some_tree( void_t ) 
{
    natus::nsl::group_t g ;

    {
        natus::nsl::group_t g1 ;
        {
            natus::nsl::leaf_t l ;
            g1.add( std::move( l ) ) ;
        }
        g.add( std::move( g1 ) ) ;
    }

    {
        natus::nsl::group_t g1 ;
        {
            natus::nsl::leaf_t l ;
            g1.add( std::move( l ) ) ;
        }
        {
            natus::nsl::group_t g2 = make_some_components<natus::nsl::group_t>( natus::nsl::group_t() ) ;
            {
                natus::nsl::leaf_t l ;
                g2.add( std::move( l ) ) ;
            }
            {
                natus::nsl::leaf_t l ;
                g2.add( std::move( l ) ) ;
            }
            g1.add( std::move( g2 ) ) ;
        }

        g.add( std::move( g1 ) ) ;
    }

    return std::move( g ) ;
}

//
// 
//
int main( int argc, char ** argv )
{
    natus::nsl::group_t g = make_some_tree() ;
    {
        natus::nsl::print_visitor_t v ;
        g.apply( &v ) ;
    }

    return 0 ;
}
