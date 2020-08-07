
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gpu/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>

#include <natus/device/layouts/three_mouse.hpp>
#include <natus/device/layouts/ascii_keyboard.hpp>
#include <natus/device/global.h>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        natus::device::three_device_res_t mouse_dev ;
        natus::device::ascii_device_res_t ascii_dev ;

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
            mouse_dev = ::std::move( rhv.mouse_dev ) ;
            ascii_dev = ::std::move( rhv.ascii_dev ) ;
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
                    mouse_dev = dev_in ;
                }
                else if( natus::device::ascii_device_res_t::castable( dev_in ) )
                {
                    ascii_dev = dev_in ;
                }
            } ) ;

            if( !mouse_dev.is_valid() )
            {
                natus::log::global_t::status( "no three mosue found" ) ;
            }

            if( !ascii_dev.is_valid() )
            {
                natus::log::global_t::status( "no ascii keyboard found" ) ;
            }

            return natus::application::result::ok ; 
        }

        void_t test_mouse( void_t ) 
        {
            natus::device::layouts::three_mouse_t mouse( mouse_dev ) ;

            // test buttons
            if( mouse_dev.is_valid() )
            {
                auto button_funk = [&] ( natus::device::layouts::three_mouse_t::button const button )
                {
                    static bool_t pressing = false ;

                    if( mouse.is_pressed( button ) )
                    {
                        natus::log::global_t::status( "button pressed: " + natus::device::layouts::three_mouse_t::to_string( button ) ) ;
                    }
                    else if( mouse.is_pressing( button ) && !pressing )
                    {
                        pressing = true ;
                        natus::log::global_t::status( "button pressing: " + natus::device::layouts::three_mouse_t::to_string( button ) ) ;
                    }
                    else if( mouse.is_released( button ) )
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
                if( mouse.is_released( natus::device::layouts::three_mouse_t::button::right ) )
                {
                    show_coords = !show_coords ;
                }
                if( show_coords )
                {
                    auto const locals = mouse.get_local() ;
                    auto const globals = mouse.get_global() ;

                    natus::log::global_t::status(
                        "local : [" + ::std::to_string( locals.x() ) + ", " +
                        ::std::to_string( locals.y() ) + "]"
                    ) ;

                    natus::log::global_t::status(
                        "global : [" + ::std::to_string( globals.x() ) + ", " +
                        ::std::to_string( globals.y() ) + "]"
                    ) ;
                }
            }

            // test scroll
            {
                float_t s ;
                if( mouse.get_scroll( s ) )
                {
                    natus::log::global_t::status( "scroll : " + ::std::to_string( s ) ) ;
                }
            }
        }

        void_t test_ascii( void_t ) 
        {
            if( !ascii_dev.is_valid() ) return ;

            natus::device::layouts::ascii_keyboard_t keyboard( ascii_dev ) ;
            
            using layout_t = natus::device::layouts::ascii_keyboard_t ;
            using key_t = layout_t::ascii_key ;
            
            for( size_t i=0; i<size_t(key_t::num_keys); ++i )
            {
                auto const ks = keyboard.get_state( key_t( i ) ) ;
                if( ks != natus::device::components::key_state::none ) 
                {
                    natus::log::global_t::status( layout_t::to_string( key_t(i) ) + " : " + 
                        natus::device::components::to_string(ks) ) ;
                }
            }
        }

        virtual natus::application::result on_update( void_t ) 
        { 
            this_t::test_mouse() ;
            this_t::test_ascii() ;

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

#if 0 // can be used for gamepads for example

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <istream>
#include <unistd.h>
#include <linux/input.h>

int main( int argc, char ** argv )
{
    int fd ;
    struct input_event ie ;

    if( (fd=open( "/dev/input/event6", O_RDONLY )) == -1 )
    {
        perror( "opening device" ) ;
        exit( EXIT_FAILURE ) ;
    }
    
printf( "starting" ) ;
    while( read(fd, &ie, sizeof(struct input_event) ) )
    {
        unsigned char * ptr = (unsigned char * ) &ie ;
        for( int i=0; i<sizeof(ie); ++i )
        {
            printf( "%02X ", *ptr++ ) ;
        }
        printf("\n") ;
    }

}

#endif
