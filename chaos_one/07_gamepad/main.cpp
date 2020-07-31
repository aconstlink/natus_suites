
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gpu/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>

#include <natus/device/layouts/xbox_controller.hpp>
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
        
        natus::device::xbc_device_res_t xbox_dev ;

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
            xbox_dev = ::std::move( rhv.xbox_dev ) ;
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual natus::application::result on_init( void_t )
        { 
            natus::device::global_t::system()->search( [&] ( natus::device::idevice_res_t dev_in )
            {
                if( natus::device::xbc_device_res_t::castable( dev_in ) )
                {
                    if( xbox_dev.is_valid() ) return ;

                    xbox_dev = dev_in ;
                }
            } ) ;

            if( !xbox_dev.is_valid() )
            {
                natus::log::global_t::status( "no xbox controller found" ) ;
            }

            return natus::application::result::ok ; 
        }

        void_t test_device( void_t ) 
        {
            natus::device::layouts::xbox_controller_t ctrl( xbox_dev ) ;

            if( xbox_dev.is_valid() )
            {
                // buttons
                {
                    auto button_funk = [&] ( natus::device::layouts::xbox_controller_t::button const button )
                    {
                        static bool_t pressing = false ;

                        if( ctrl.is( button, natus::device::components::button_state::pressed ) )
                        {
                            natus::log::global_t::status( "button pressed: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::pressing ) )
                        {
                            pressing = true ;
                            natus::log::global_t::status( "button pressing: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::released ) )
                        {
                            pressing = false ;
                            natus::log::global_t::status( "button released: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                    } ;

                    using part_t = natus::device::layouts::xbox_controller_t::button ;

                    button_funk( part_t::start ) ;
                    button_funk( part_t::back ) ;
                    button_funk( part_t::a ) ;
                    button_funk( part_t::b ) ;
                    button_funk( part_t::x ) ;
                    button_funk( part_t::y ) ;
                }

                // dpad
                {
                    auto button_funk = [&] ( natus::device::layouts::xbox_controller_t::dpad const button )
                    {
                        static bool_t pressing = false ;

                        if( ctrl.is( button, natus::device::components::button_state::pressed ) )
                        {
                            natus::log::global_t::status( "dpad pressed: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::pressing ) )
                        {
                            pressing = true ;
                            natus::log::global_t::status( "dpad pressing: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::released ) )
                        {
                            pressing = false ;
                            natus::log::global_t::status( "dpad released: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                    } ;

                    using part_t = natus::device::layouts::xbox_controller_t::dpad ;

                    button_funk( part_t::left ) ;
                    button_funk( part_t::right ) ;
                    button_funk( part_t::up ) ;
                    button_funk( part_t::down ) ;
                }

                // shoulder
                {
                    auto button_funk = [&] ( natus::device::layouts::xbox_controller_t::shoulder const button )
                    {
                        static bool_t pressing = false ;

                        if( ctrl.is( button, natus::device::components::button_state::pressed ) )
                        {
                            natus::log::global_t::status( "shoulder pressed: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::pressing ) )
                        {
                            pressing = true ;
                            natus::log::global_t::status( "shoulder pressing: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::released ) )
                        {
                            pressing = false ;
                            natus::log::global_t::status( "shoulder released: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                    } ;

                    using part_t = natus::device::layouts::xbox_controller_t::shoulder ;

                    button_funk( part_t::left ) ;
                    button_funk( part_t::right ) ;
                }

                // thumb
                {
                    auto button_funk = [&] ( natus::device::layouts::xbox_controller_t::thumb const button )
                    {
                        static bool_t pressing = false ;

                        if( ctrl.is( button, natus::device::components::button_state::pressed ) )
                        {
                            natus::log::global_t::status( "thumb pressed: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::pressing ) )
                        {
                            pressing = true ;
                            natus::log::global_t::status( "thumb pressing: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::released ) )
                        {
                            pressing = false ;
                            natus::log::global_t::status( "thumb released: " + natus::device::layouts::xbox_controller_t::to_string( button ) ) ;
                        }
                    } ;

                    using part_t = natus::device::layouts::xbox_controller_t::thumb ;

                    button_funk( part_t::left ) ;
                    button_funk( part_t::right ) ;
                }

                // trigger
                {
                    auto button_funk = [&] ( natus::device::layouts::xbox_controller_t::trigger const button )
                    {
                        static bool_t pressing = false ;

                        float_t value = 0.0f ;

                        if( ctrl.is( button, natus::device::components::button_state::pressed, value ) )
                        {
                            natus::log::global_t::status( "thumb pressed: " + natus::device::layouts::xbox_controller_t::to_string( button ) + " [" + ::std::to_string(value) + "]") ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::pressing, value ) )
                        {
                            pressing = true ;
                            natus::log::global_t::status( "thumb pressing: " + natus::device::layouts::xbox_controller_t::to_string( button ) + " [" + ::std::to_string(value) + "]") ;
                        }
                        else if( ctrl.is( button, natus::device::components::button_state::released, value ) )
                        {
                            pressing = false ;
                            natus::log::global_t::status( "thumb released: " + natus::device::layouts::xbox_controller_t::to_string( button ) + " [" + ::std::to_string(value) + "]") ;
                        }
                    } ;

                    using part_t = natus::device::layouts::xbox_controller_t::trigger ;

                    button_funk( part_t::left ) ;
                    button_funk( part_t::right ) ;
                }

                // sticks
                {
                    auto funk = [&] ( natus::device::layouts::xbox_controller_t::stick const s )
                    {
                        natus::math::vec2f_t value ;

                        if( ctrl.is( s, natus::device::components::stick_state::tilted, value ) )
                        {
                            natus::log::global_t::status( "stick pressed: " + natus::device::layouts::xbox_controller_t::to_string( s ) + " [" + ::std::to_string( value.x() ) + "," + ::std::to_string( value.y() ) + "]" ) ;
                        }
                        else if( ctrl.is( s, natus::device::components::stick_state::tilting, value ) )
                        {
                            natus::log::global_t::status( "stick pressing: " + natus::device::layouts::xbox_controller_t::to_string( s ) + " [" + ::std::to_string( value.x() ) + "," + ::std::to_string( value.y() ) + "]" ) ;
                        }
                        else if( ctrl.is( s, natus::device::components::stick_state::untilted, value ) )
                        {
                            natus::log::global_t::status( "stick released: " + natus::device::layouts::xbox_controller_t::to_string( s ) + " [" + ::std::to_string( value.x() ) + "," + ::std::to_string( value.y() ) + "]" ) ;
                        }
                    } ;

                    using part_t = natus::device::layouts::xbox_controller_t::stick ;

                    funk( part_t::left ) ;
                    funk( part_t::right ) ;
                }

                // motor
                {
                    float_t value = 0.0f ;
                    {
                        if( ctrl.is( natus::device::layouts::xbox_controller_t::trigger::left, natus::device::components::button_state::pressing, value ) )
                        {
                            ctrl.set_motor( natus::device::layouts::xbox_controller_t::motor::left, value ) ;
                        }
                        else if( ctrl.is( natus::device::layouts::xbox_controller_t::trigger::left, natus::device::components::button_state::released, value ) )
                        {
                            ctrl.set_motor( natus::device::layouts::xbox_controller_t::motor::left, value ) ;
                        }
                    }

                    {
                        if( ctrl.is( natus::device::layouts::xbox_controller_t::trigger::right, natus::device::components::button_state::pressing, value ) )
                        {
                            ctrl.set_motor( natus::device::layouts::xbox_controller_t::motor::right, value ) ;
                        }
                        else if( ctrl.is( natus::device::layouts::xbox_controller_t::trigger::right, natus::device::components::button_state::released, value ) )
                        {
                            ctrl.set_motor( natus::device::layouts::xbox_controller_t::motor::right, value ) ;
                        }
                    }
                }
            }
        }

        virtual natus::application::result on_update( void_t ) 
        { 
            this_t::test_device() ;

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
