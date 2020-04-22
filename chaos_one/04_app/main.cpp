
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

    private:

        natus::application::app::window_id_t _wid ;
        natus::gpu::async_res_t _async ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            _wid = this_t::create_window( "A Render Window", wi ) ;
            _async = this_t::gpu_async( _wid ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid = rhv._wid ;
            rhv._wid = size_t( -1 ) ;
            _async = ::std::move( rhv._async ) ;
        }
        virtual ~test_app( void_t ) 
        {

        }

    private:

        virtual natus::application::result on_init( void_t )
        { 
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( void_t ) 
        { return natus::application::result::ok ; }

        virtual natus::application::result on_render( void_t ) 
        { return natus::application::result::ok ; }

        virtual natus::application::result on_shutdown( void_t ) 
        { return natus::application::result::ok ; }
    };
    natus_soil_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;

    return 0 ;
}
