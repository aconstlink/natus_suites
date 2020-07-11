
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gpu/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>

#include <natus/device/layouts/three_mouse.hpp>
#include <natus/device/global.h>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::wid_async_t _wid_async ;
        
        natus::device::three_device_res_t dev ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = ::std::move( rhv._wid_async ) ;
            dev = ::std::move( rhv.dev ) ;
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual natus::application::result on_init( void_t )
        { 
            natus::device::global_t::system()->search( [&] ( natus::device::idevice_res_t dev_in )
            {
                if( natus::device::three_device_res_t::castable( dev_in ) )
                {
                    dev = dev_in ;
                }
            } ) ;

            if( !dev.is_valid() )
            {
                natus::log::global_t::status( "no three device found" ) ;
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( void_t ) 
        { 
            // test buttons
            if( dev.is_valid() )
            {
                auto button_funk = [&] ( natus::device::layouts::three_mouse_t::button const button )
                {
                    static bool_t pressing = false ;

                    if( natus::device::layouts::three_mouse_t::is_pressed( dev, button ) )
                    {
                        natus::log::global_t::status( "button pressed: " + natus::device::layouts::three_mouse_t::to_string( button ) ) ;
                    }
                    else if( natus::device::layouts::three_mouse_t::is_pressing( dev, button ) && !pressing )
                    {
                        pressing = true ;
                        natus::log::global_t::status( "button pressing: " + natus::device::layouts::three_mouse_t::to_string( button ) ) ;
                    }
                    else if( natus::device::layouts::three_mouse_t::is_released( dev, button ) )
                    {
                        pressing = false ;
                        natus::log::global_t::status( "button released: " + natus::device::layouts::three_mouse_t::to_string( button ) ) ;
                    }
                } ;

                button_funk( natus::device::layouts::three_mouse_t::button::left ) ;
                button_funk( natus::device::layouts::three_mouse_t::button::right ) ;
                button_funk( natus::device::layouts::three_mouse_t::button::middle ) ;
            }

            // test coords
            {
                static bool_t show_coords = false ;
                if( natus::device::layouts::three_mouse_t::is_released( dev, natus::device::layouts::three_mouse_t::button::right ) )
                {
                    show_coords = !show_coords ;
                }
                if( show_coords )
                {
                    natus::log::global_t::status(
                        "local : [" + ::std::to_string( natus::device::layouts::three_mouse_t::get_local(dev).x() ) + ", " + ::std::to_string( natus::device::layouts::three_mouse_t::get_local(dev).y() ) + "]" 
                    ) ;

                    natus::log::global_t::status(
                        "global : [" + ::std::to_string( natus::device::layouts::three_mouse_t::get_global(dev).x() ) + ", " + ::std::to_string( natus::device::layouts::three_mouse_t::get_global(dev).y() ) + "]"
                    ) ;
                }
            }

            // test scroll
            {
                float_t s ;
                if( natus::device::layouts::three_mouse_t::get_scroll( dev, s ) )
                {
                    natus::log::global_t::status( "scroll : " + ::std::to_string( s ) ) ;
                }
            }

            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 1 ) ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_render( void_t ) 
        { 
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_shutdown( void_t ) 
        { return natus::application::result::ok ; }
    };
    natus_soil_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}