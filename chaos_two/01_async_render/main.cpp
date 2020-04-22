
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

    public:

        virtual natus::application::result on_init( void_t )
        {
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_update( void_t )
        {
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_render( void_t )
        {
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_shutdown( void_t )
        {
            return natus::application::result::ok ;
        }
    };
    natus_typedef( test_app ) ;
    typedef natus::soil::res< test_app_t > test_app_res_t ;
}

int main( int argc, char** argv )
{
    natus::application::platform_application_res_t prog =
        natus::application::global_t::create_application(
            this_file::test_app_res_t()) ;

    prog->exec() ;

    return 0 ;
}
