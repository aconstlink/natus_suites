
#include "main.h"

#include <natus/soil/res.hpp>

namespace this_file
{
    using namespace natus::core::types ;

    class x
    {
    public:

        virtual void_t dummy( void ) = 0 ;

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

        virtual void_t dummy( void ) 
        {
            natus::log::global_t::status( "[a::dummy]" ) ;
        }

    public:
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

        virtual void_t dummy( void )
        {
            natus::log::global_t::status( "[b::dummy]" ) ;
        }
       
    };
    natus_typedefs( natus::soil::res<b>, resb ) ;

    class c
    {
    };
    natus_typedefs( natus::soil::res<c>, resc ) ;

    
}


int main( int argc, char ** argv )
{
    natus::std::vector< natus::soil::res_t > ids ;

    // test cast-ability of tag types.
    {
        natus::soil::detail::tag<this_file::a> ta ;
        natus::soil::detail::tag<this_file::b> tb ;
        natus::soil::detail::tag<this_file::c> tc ;

        // upcast
        // must not fail. b is derived from a.
        {
            // using base tag class here
            natus::soil::detail::tag_ptr_t tptr = &tb ;
            auto* dyn = dynamic_cast< this_file::a* >( tptr ) ;
            if( natus::core::is_not_nullptr(dyn ) )
            {
                natus::log::global_t::status( "[] : b tag cast possible to a tag - OK" ) ;
            }
        }

        // fail cast
        // must fail. c is not derived from a.
        {
            // using base tag class here
            natus::soil::detail::tag_ptr_t tptr = &tc ;
            auto* dyn = dynamic_cast< this_file::a* >( tptr ) ;
            if( natus::core::is_nullptr( dyn ) )
            {
                natus::log::global_t::status( "[] : c tag cast NOT possible to a tag - OK" ) ;
            }
        }

        int const bp = 0 ;
    }

    // 1. concrete to concrete
    {
        natus::log::global_t::status("[*********** [ 01 ] *************]") ;
        this_file::resb_t b1 = this_file::b( 1 ) ;
        this_file::resb_t b2 = b1 ;

        ids.emplace_back( b2 ) ;
    }

    // 2. concrete to abstract
    {
        natus::log::global_t::status("[*********** [ 02 ] *************]") ;
        this_file::resb_t b = this_file::b( 2 ) ;
        natus::soil::res_t some = b ;

        ids.emplace_back( some ) ;
        ids.emplace_back( some ) ;
    }

    // 3. abstract to abstract
    {
        natus::log::global_t::status("[*********** [ 03 ] *************]") ;
        this_file::resa_t a = this_file::a() ;
        natus::soil::res_t some = a ;
        natus::soil::res_t some2 = some ;

        ids.emplace_back( some2 ) ;
    }

    // 4. abstract to concrete
    {
        natus::log::global_t::status("[*********** [ 04 ] *************]") ;
        this_file::resb_t b = this_file::b(4) ;
        natus::soil::res_t some = b ;

        // is cast possible? It should!
        this_file::resb_t b2 = some ;
        this_file::resb_t b3 = natus::soil::res_t( b2 ) ;

        natus::log::global_t::status( "[some_int] : " + ::std::to_string(b3->some_int) ) ;

        ids.emplace_back( b3 ) ;
    }

    // 5. as cast
    {
        natus::log::global_t::status("[*********** [ 05 ] *************]") ;
        this_file::resb_t b = this_file::b(5) ;
        this_file::resc_t c = this_file::c() ;
        
        // test unsafe cast.
        {
            b.cast< this_file::a* >()->print( "hello from a with unsafe cast" ) ;
        }

        // test safe cast
        {
            b.scast< this_file::a* >()->print( "hello from a with safe cast" ) ;
        }

        // test castability
        {
            std::string const test = b.is_castable< this_file::a* >() ? "OK" : "Failed" ;
            natus::log::global_t::status( "b res is castable to a* : " + test ) ;
        }

        // test castability
        {
            std::string const test = c.is_castable< this_file::a* >() ? "OK" : "Failed" ;
            natus::log::global_t::status( "c res is castable to a* : " + test ) ;
        }
        
    }
    
    // 6. cast to abstract interface
    {
        natus::log::global_t::status( "[*********** [ 06 ] *************]" ) ;
        this_file::resa_t a = this_file::a() ;
        this_file::resb_t b = this_file::b( 5 ) ;
        
        natus::soil::res_t r = a ;

        // test castability
        {
            // a to x
            a.scast< this_file::x* >()->dummy() ;
            // b to x 
            b.scast< this_file::x* >()->dummy() ;
            // a to x via res<void_t>
            r.scast< this_file::x* >()->dummy() ;
        }
    }

    /*{
        natus::soil::id<this_file::b> b = this_file::b() ;
        some = b ;
        this_file::some_funk( ::std::move( b ) ) ;
    }

    {
        natus::soil::id<this_file::b> b = some ;
    }*/

    

    int const bp = 0 ;

    return 0 ;
}


