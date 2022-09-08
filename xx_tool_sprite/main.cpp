
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>

#include <natus/tool/imgui/sprite_editor.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>

#include <natus/graphics/shader/nsl_bridge.hpp>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profile/macros.h>

#include <natus/format/global.h>
#include <natus/format/nsl/nsl_module.h>

#include <natus/geometry/mesh/polygon_mesh.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>
#include <natus/geometry/3d/tetra.h>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/3d/transformation.hpp>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::graphics::async_views_t _graphics ;
        app::window_async_t _wid_async ;
        app::window_async_t _wid_async2 ;

        natus::graphics::state_object_res_t _root_render_states ;
                
        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::io::database_res_t _db ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _do_tool = true ;
        natus::tool::sprite_editor_res_t _se ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 0
            _wid_async = this_t::create_window( "A Render Window", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11} ) ;
            _wid_async2 = this_t::create_window( "A Render Window", wi) ;

            _wid_async.window().position( 50, 50 ) ;
            _wid_async.window().resize( 800, 800 ) ;
            _wid_async2.window().position( 50 + 800, 50 ) ;
            _wid_async2.window().resize( 800, 800 ) ;

            _graphics = natus::graphics::async_views_t( { _wid_async.async(), _wid_async2.async() } ) ;
            #else
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
            _wid_async.window().resize( 1000, 1000 ) ;
            #endif

            _db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;

            _se = natus::tool::sprite_editor_res_t( natus::tool::sprite_editor_t( _db )  ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = std::move( rhv._wid_async ) ;
            _wid_async2 = std::move( rhv._wid_async2 ) ;
            _camera_0 = std::move( rhv._camera_0 ) ;
            _db = std::move( rhv._db ) ;
            _graphics = std::move( rhv._graphics ) ;
            _se = std::move( rhv._se ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei ) noexcept
        {
            _camera_0.set_dims( float_t(wei.w), float_t(wei.h), 1.0f, 1000.0f ) ;
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ) ) ;

            return natus::application::result::ok ;
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

            if( !_dev_mouse.is_valid() ) natus::log::global_t::status( "no three mouse found" ) ;
            if( !_dev_ascii.is_valid() ) natus::log::global_t::status( "no ascii keyboard found" ) ;
            

            {
                _camera_0.look_at( natus::math::vec3f_t( 0.0f, 60.0f, -50.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }

            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "root_render_states" ) ;

                {
                    natus::graphics::render_state_sets_t rss ;

                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = false ;

                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = natus::graphics::fill_mode::fill ;
                   
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }
            
            {
                //_se->add_sprite_sheet( "industrial", natus::io::location_t( "images.industrial.industrial.v2.png" ) ) ;
                //_se->add_sprite_sheet( "enemies", natus::io::location_t( "images.Paper-Pixels-8x8.Enemies.png" ) ) ;
                //_se->add_sprite_sheet( "player", natus::io::location_t( "images.Player.png" ) ) ;
                //_se->add_sprite_sheet( "tiles", natus::io::location_t( "images.Tiles.png" ) ) ;
                _se->add_sprite_sheet( "sprite_sheets", natus::io::location_t( "sprite_sheets.natus" ) ) ;
                
            }
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_device( natus::application::app_t::device_data_in_t ) noexcept 
        { 
            {
                natus::device::layouts::ascii_keyboard_t ascii( _dev_ascii ) ;
                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f8 ) ==
                    natus::device::components::key_state::released )
                {
                }
                else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f9 ) ==
                    natus::device::components::key_state::released )
                {
                }
                else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f2 ) ==
                    natus::device::components::key_state::released )
                {
                    _do_tool = !_do_tool ;
                }
            }
            NATUS_PROFILING_COUNTER_HERE( "Device Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) noexcept 
        { 
            // render the root render state sets render object
            // this will set the root render states
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

            // render the root render state sets render object
            // this will set the root render states
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state );
                } ) ;
            }

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_do_tool ) return natus::application::result::no_tool ;

            {
                bool_t show_demo = true ;
                ImGui::ShowDemoWindow( &show_demo ) ;
            }

            _se->do_tool( td.imgui ) ;

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
