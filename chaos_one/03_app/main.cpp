
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    public:

        test_app( void_t ) {}
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) {}
        virtual ~test_app( void_t ) {}

    private:

        virtual natus::application::result init( void_t ) 
        { return natus::application::result::ok ; }

        virtual natus::application::result update( void_t ) 
        { return natus::application::result::ok ; }

        virtual natus::application::result render( void_t ) 
        { return natus::application::result::ok ; }

        virtual natus::application::result shutdown( void_t ) 
        { return natus::application::result::ok ; }
    };
    natus_typedef( test_app ) ;
    typedef natus::soil::res< test_app_t > test_app_res_t ;
}

int main( int argc, char ** argv )
{
    natus::application::application_rptr_t prog =
        natus::application::global_t::create_application() ;
    
    prog->set( this_file::test_app_res_t( this_file::test_app_t() ) ) ;

    prog->exec() ;

    return 0 ;
}
