
#include "main.h"

#include <natus/soil/res.hpp>
#include <natus/soil/rptr.hpp>

namespace this_file
{
    using namespace natus::core::types ;

    class x
    {
    public:

        virtual void_t dummy( void ) const = 0 ;

    public:
        virtual ~x() {}
    };
    natus_typedefs( natus::soil::res<x>, resx ) ;

    class a : public x
    {
    public: 

        void_t print( std::string const & msg )
        {
            natus::log::global_t::status( msg ) ;
        }

        virtual void_t dummy( void ) const
        {
            natus::log::global_t::status( "[a::dummy]" ) ;
        }

    public:
        a( void_t ) 
        {
            natus::log::global_t::status( "[a::a()]" ) ;
        }
        virtual ~a(){}
    };
    natus_typedefs( natus::soil::res<a>, resa ) ;

    class b : public a
    {

    public: 

        int_t some_int = 0 ;

    public:

        b( void_t ) {}
        b( int_t const i ) : some_int( i ) {}
        b( b const & rhv ) { some_int = rhv.some_int ; } 
        b( b&& rhv ) { some_int = rhv.some_int ; }

        virtual void_t dummy( void ) const
        {
            natus::log::global_t::status( "[b::dummy]" ) ;
        }
       
        void_t print( void_t )
        {
            natus::log::global_t::status( "Hello from b " + std::to_string(some_int) ) ;
        }

    };
    natus_typedefs( natus::soil::res<b>, resb ) ;

    class c
    {
    };
    natus_typedefs( natus::soil::res<c>, resc ) ;

    natus::soil::res_t some_funk( natus::soil::res_rref_t id_ )
    {
        return std::move( id_ ) ;
    }
}


int main( int argc, char ** argv )
{
    {
        std::shared_ptr< this_file::x > ptr = std::shared_ptr< this_file::a >( new this_file::a() ) ;
        ptr->dummy() ;
    }
    {
        this_file::resa_t a = this_file::a() ;
        this_file::resb_t b = this_file::b(10) ;

        natus::soil::rptr< this_file::x* > ptr_to_a( a ) ;
        natus::soil::rptr< this_file::x const * > const ptr_to_b( b ) ;

        ptr_to_a->dummy() ;
        ptr_to_b->dummy() ;
    }

    {
        this_file::resb_t b = this_file::b(10) ;
        natus::soil::rptr< this_file::x const * > const ptr_to_b( b ) ;

        ptr_to_b->dummy() ;

        this_file::resb_t b2 = ptr_to_b.res() ;
        b2->print() ;
    }

    {
    }

    return 0 ;
}


