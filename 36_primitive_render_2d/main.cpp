

#include <natus/application/global.h>
#include <natus/application/app.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/primitive/line_render_2d.h>
#include <natus/gfx/primitive/tri_render_2d.h>

#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profile/macros.h>

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

        natus::graphics::state_object_res_t _root_render_states ;
                
        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _do_tool = false ;

        natus::gfx::line_render_2d_res_t _lr ;
        natus::gfx::tri_render_2d_res_t _tr ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            auto view1 = this_t::create_window( "A Render Window", wi ) ;
            auto view2 = this_t::create_window( "A Render Window", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _graphics = natus::graphics::async_views_t( { view1.async(), view2.async() } ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
            _graphics = natus::graphics::async_views_t( { view1.async() } ) ;
            #endif
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _camera_0 = std::move( rhv._camera_0 ) ;
            _graphics = std::move( rhv._graphics ) ;
            _lr = std::move( rhv._lr ) ;
            _tr = std::move( rhv._tr ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei ) noexcept
        {
            _camera_0.set_dims( float_t(wei.w) , float_t(wei.h), 1.0f, 1000.0f ) ;
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

            if( !_dev_mouse.is_valid() )
            {
                natus::log::global_t::status( "no three mosue found" ) ;
            }

            if( !_dev_ascii.is_valid() )
            {
                natus::log::global_t::status( "no ascii keyboard found" ) ;
            }

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
            
            // prepare line render
            {
                _lr = natus::gfx::line_render_2d_res_t( natus::gfx::line_render_2d_t() ) ;
                _lr->init( "debug_line_render", _graphics ) ;

                _tr = natus::gfx::tri_render_2d_res_t( natus::gfx::tri_render_2d_t() ) ;
                _tr->init( "debug_tri_render", _graphics ) ;
            }
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
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

            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) noexcept 
        { 
            static float_t inc = 0.0f ;

            #if 0
            natus::math::vec2f_t pos( -1.0f, 0.5f ) ;
            for( size_t i=0; i<1000; ++i )
            {
                _lr->draw( 0, 
                    pos + natus::math::vec2f_t( 0.0f, -0.4f ), 
                    pos + natus::math::vec2f_t( 0.0f, +0.4f ),
                    natus::math::vec4f_t( 
                        (inc + float_t(i+600)/1000.0f)/2.0f, 
                        (inc + float_t((i+300)%1000)/1000.0f)/2.0f, 
                        (inc + float_t((i+900)%1000)/1000.0f)/2.0f, 
                        1.0f)) ;

                pos += natus::math::vec2f_t( float_t(i) / 1000.0f, 0.0f ) ;
            }

            pos = natus::math::vec2f_t( 1.0f, -0.5f ) ;

            for( size_t i=0; i<1000; ++i )
            {
                size_t const idx = 999 - i ;
                _lr->draw( 1, 
                    pos + natus::math::vec2f_t( 0.0f, -0.4f ), 
                    pos + natus::math::vec2f_t( 0.0f, +0.4f ),
                    natus::math::vec4f_t( 
                        (float_t(idx)/1000.0f), 
                        (float_t((2*idx+300)%1000)/1000.0f), 
                        (float_t((idx+900)%1000)/1000.0f)/2.0f, 
                        1.0f)) ;

                pos -= natus::math::vec2f_t( (float_t(i) / 1000.0f), 0.0f ) ;
            }

            // tris
            {
                pos = natus::math::vec2f_t( 1.0f, -0.0f ) ;

                size_t const max_tris = 50 ;
                for( size_t i=0; i<max_tris; ++i )
                {
                    size_t const idx = max_tris - 1 - i ;
                    _tr->draw( 0, 
                        pos + natus::math::vec2f_t( -.02f, -0.04f ), 
                        pos + natus::math::vec2f_t( +0.02f, -0.04f ),
                        pos + natus::math::vec2f_t( 0.0f, 0.04f ),
                        natus::math::vec4f_t( 
                            (inc + float_t((idx+600)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+300)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+900)%max_tris)/float_t(max_tris))/2.0f, 
                            0.5f + (float_t(idx)/float_t(max_tris))* 0.5f )  +
                        natus::math::vec4f_t( size_t( inc * float_t(max_tris) ) == i, 0.0f,0.0f,0.0f )
                    )  ;

                    pos -= natus::math::vec2f_t( (1.0f/float_t(max_tris))*2.0f, 0.0f ) ;
                }
            }

            // tris
            {
                pos = natus::math::vec2f_t( -1.0f, -0.0f ) ;

                size_t const max_tris = 50 ;
                for( size_t i=0; i<max_tris; ++i )
                {
                    size_t const idx = i ;
                    _tr->draw( 0, 
                        pos + natus::math::vec2f_t( +.02f, 0.04f ), 
                        pos + natus::math::vec2f_t( -0.02f, 0.04f ),
                        pos + natus::math::vec2f_t( 0.0f, -0.04f ),
                        natus::math::vec4f_t( 
                            (inc + float_t((idx+600)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+300)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+900)%max_tris)/float_t(max_tris))/2.0f, 
                            0.5f + float_t(idx)/float_t(max_tris) * 0.5f )  +
                        natus::math::vec4f_t( size_t( inc * float_t(max_tris) ) == idx, 0.0f,0.0f,0.0f )
                    ) ;

                    pos += natus::math::vec2f_t( (1.0f/float_t(max_tris))*2.0f, 0.0f ) ;
                }
            }

            // rects
            {
                pos = natus::math::vec2f_t( -1.0f, 0.1f ) ;

                size_t const max_tris = 40 ;
                for( size_t i=0; i<max_tris; ++i )
                {
                    size_t const idx = i ;
                    _tr->draw_rect( i%5, 
                        pos + natus::math::vec2f_t( -.02f, -0.04f ), 
                        pos + natus::math::vec2f_t( -.02f, +0.04f ),
                        pos + natus::math::vec2f_t( +.02f, +0.04f ),
                        pos + natus::math::vec2f_t( +.02f, -0.04f ),
                        natus::math::vec4f_t( 
                            0.5f,0.5f,0.5f,
                            0.5f + float_t(idx)/float_t(max_tris) * 0.5f )  +
                        natus::math::vec4f_t( size_t( inc * float_t(max_tris) ) == idx, 0.0f,0.0f,0.0f )
                    ) ;

                    pos += natus::math::vec2f_t( (1.0f/float_t(max_tris))*2.0f, 0.0f ) ;
                }
            }

            // tris
            {
                pos = natus::math::vec2f_t( 1.0f, -0.1f ) ;

                size_t const max_tris = 50 ;
                for( size_t i=0; i<max_tris; ++i )
                {
                    size_t const idx = max_tris - 1 - i ;
                    _tr->draw_circle( i%5, 10, pos, float_t(0.04f),
                        natus::math::vec4f_t( 
                            0.5f,0.5f,0.5f,
                            0.5f + (float_t(idx)/float_t(max_tris))* 0.5f )  +
                        natus::math::vec4f_t( size_t( inc * float_t(max_tris) ) == i, 0.0f,0.0f,0.0f )
                    )  ;

                    pos -= natus::math::vec2f_t( (1.0f/float_t(max_tris))*2.0f, 0.0f ) ;
                }
            }

            #else // test simple here

            // rects
            {
                natus::math::vec2f_t pos = natus::math::vec2f_t( -0.7f, 0.1f ) ;

                size_t const max_tris = 2 ;
                for( size_t i=0; i<max_tris; ++i )
                {
                    size_t const idx = i ;
                    _tr->draw_rect( i%2, 
                        pos + natus::math::vec2f_t( -.02f, -0.04f ), 
                        pos + natus::math::vec2f_t( -.02f, +0.04f ),
                        pos + natus::math::vec2f_t( +.02f, +0.04f ),
                        pos + natus::math::vec2f_t( +.02f, -0.04f ),
                        natus::math::vec4f_t( 
                            0.5f,0.5f,0.5f,
                            0.5f + float_t(idx)/float_t(max_tris) * 0.5f )  +
                        natus::math::vec4f_t( size_t( inc * float_t(max_tris) ) == idx, 0.0f,0.0f,0.0f )
                    ) ;

                    pos += natus::math::vec2f_t( (1.0f/float_t(max_tris))*2.0f, 0.0f ) ;
                }
            }
            #endif

            inc += 0.01f  ;
            inc = natus::math::fn<float_t>::mod( inc, 1.0f ) ;

            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

            
            

            
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
            }
            {
                _lr->prepare_for_rendering() ;
                _tr->prepare_for_rendering() ;
                for( size_t i=0; i<100; ++i )
                {
                    _lr->render( i ) ;
                    _tr->render( i ) ;
                }
            }
            

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui ) noexcept
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "do something" ) ;

            ImGui::End() ;
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
