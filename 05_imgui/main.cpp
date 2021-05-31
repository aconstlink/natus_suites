
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/tool/imgui/imgui.h>
#include <natus/graphics/variable/variable_set.hpp>
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

        app::window_async_t _wid_async ;

        float_t _demo_width = 10.0f ;
        float_t _demo_height = 10.0f ;

        natus::graphics::image_object_res_t _checkerboard = natus::graphics::image_object_t() ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _fullscreen = false ;
        bool_t _vsync = true ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            wi.w = 1000 ;
            wi.h = 1000 ;
            _wid_async = this_t::create_window( "An Imgui Rendering Test", wi ) ;
            _wid_async.window().fullscreen( _fullscreen ) ;
            _wid_async.window().vsync( _vsync ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = std::move( rhv._wid_async ) ;

            _dev_mouse = std::move( rhv._dev_mouse ) ;
            _dev_ascii = std::move( rhv._dev_ascii ) ;
        }
        virtual ~test_app( void_t ) 
        {
        }

    private:

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::device::global_t::system()->search( [&] ( natus::device::idevice_res_t dev_in )
            {
                if( natus::device::three_device_res_t::castable( dev_in ) )
                {
                    _dev_mouse = dev_in ;
                }
                else if( natus::device::ascii_device_res_t::castable( dev_in ) )
                {
                    _dev_ascii = dev_in ;
                }
            } ) ;

            if( !_dev_mouse.is_valid() )
            {
                natus::log::global_t::status( "no three mosue found" ) ;
            }

            if( !_dev_ascii.is_valid() )
            {
                natus::log::global_t::status( "no ascii keyboard found" ) ;
            }

            // A checker board image
            {
                natus::graphics::image_t img = natus::graphics::image_t( natus::graphics::image_t::dims_t( 100, 100 ) )
                    .update( [&] ( natus::graphics::image_ptr_t, natus::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                {
                    typedef natus::math::vector4< uint8_t > rgba_t ;
                    auto* data = reinterpret_cast< rgba_t* >( data_in ) ;

                    size_t const w = 5 ;

                    size_t i = 0 ;
                    for( size_t y = 0; y < dims.y(); ++y )
                    {
                        bool_t const odd = ( y / w ) & 1 ;

                        for( size_t x = 0; x < dims.x(); ++x )
                        {
                            bool_t const even = ( x / w ) & 1 ;

                            data[ i++ ] = even || odd ? rgba_t( 255 ) : rgba_t( 0, 0, 0, 255 );
                            //data[ i++ ] = rgba_t(255) ;
                        }
                    }
                } ) ;

                _checkerboard = natus::graphics::image_object_t( "user.checkerboard", ::std::move( img ) )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                    .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                    .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );

                _wid_async.async().configure( _checkerboard ) ;
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            _demo_width = float_t( wei.w ) ;
            _demo_height = float_t( wei.h ) ;
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
        { 
            {
                natus::device::layouts::ascii_keyboard_t ascii( _dev_ascii ) ;
                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f8 ) == 
                    natus::device::components::key_state::released )
                {
                    _fullscreen = !_fullscreen ;
                    _wid_async.window().fullscreen( _fullscreen ) ;
                }
                else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f9 ) ==
                    natus::device::components::key_state::released )
                {
                    _vsync = !_vsync ;
                    _wid_async.window().vsync( _vsync ) ;
                }
            }
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui )
        {
            bool_t open = true ;
            //ImGui::SetWindowSize( ImVec2( { _demo_width*0.5f, _demo_height*0.5f } ) ) ;
            ImGui::ShowDemoWindow( &open ) ;

            ImGui::SetNextWindowPos( ImVec2( { _demo_width * 0.5f, _demo_height * 0.5f } ) ) ;
            ImGui::SetNextWindowSize( ImVec2( { _demo_width * 0.5f, _demo_height * 0.5f } ) ) ;
            ImGui::Begin( "Hello Image" ) ;

            {
                ImVec2 const dims = ImGui::GetWindowSize() ;
                ImGui::Image( imgui.texture( "user.checkerboard" ), dims ) ;
            }

            ImGui::End() ;

            ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) ) ;
            ImGui::SetNextWindowSize( ImVec2( { _demo_width * 0.5f, _demo_height * 0.5f } ) ) ;

            ImGui::Begin( "Hello UI" ) ;
            ImGui::Text( "Hello, world %d", 123 );

            float_t f = 1.0f ;
            ImGui::SliderFloat( "float", &f, 0.0f, 1.0f );
            ImGui::End() ;

            return natus::application::result::ok ;
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) noexcept 
        {
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_shutdown( void_t ) noexcept 
        { return natus::application::result::ok ; }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}
