

#include "world.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/primitive/primitive_render_2d.h> 

#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profile/macros.h>

#include <natus/physics/particle_system.h>
#include <natus/physics/force_fields.hpp>

#include <natus/geometry/mesh/polygon_mesh.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>
#include <natus/geometry/3d/tetra.h>
#include <natus/math/utility/fn.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/3d/transformation.hpp>

#include <random>
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

        bool_t _do_tool = true ;
        
        uniform_grid::grid_t _grid = uniform_grid::grid_t( 
            uniform_grid::dimensions_t( 
                natus::math::vec2ui_t(10), // regions_per_grid
                natus::math::vec2ui_t(16), // cells_per_region
                natus::math::vec2ui_t(16)  // pixels_per_cell
            ) 
        ) ;

    private: //

        
        natus::gfx::primitive_render_2d_res_t _pr ;

        bool_t _draw_debug = false ;

    private:

        natus::math::vec2f_t _extend = natus::math::vec2f_t( 100, 100 ) ;

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
        test_app( this_rref_t rhv ) noexcept : app( ::std::move( rhv ) ) 
        {
            _camera_0 = std::move( rhv._camera_0 ) ;
            _graphics = std::move( rhv._graphics ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei ) noexcept
        {
            natus::math::vec2f_t const target = natus::math::vec2f_t(800, 600) ; 
            natus::math::vec2f_t const window = natus::math::vec2f_t( float_t(wei.w), float_t(wei.h) ) ;

            natus::math::vec2f_t const ratio = window / target ;

            _camera_0.orthographic( float_t(wei.w), float_t(wei.h), 1.0f, 1000.0f ) ;

            _extend = target * (ratio.x() < ratio.y() ? ratio.xx() : ratio.yy()) ;

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
                _camera_0.look_at( natus::math::vec3f_t( 0.0f, 0.0f, -10.0f ),
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

                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.clear_color = natus::math::vec4f_t( 0.5f, 0.5f, 0.5f, 1.0f ) ;
                   
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }
            
            // prepare primitive
            {
                _pr = natus::gfx::primitive_render_2d_res_t( natus::gfx::primitive_render_2d_t() ) ;
                _pr->init( "particle_prim_render", _graphics ) ;
            }

            // init particles
            {
               
            }

            {
                uniform_grid::ij_id_t id = _grid.get_dims().calc_cell_ij_id( natus::math::vec2ui_t( 10, 10 ) ) ;
                int const bp = 0 ;
            }
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_device( device_data_in_t ) noexcept 
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

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            //std::this_thread::sleep_for( std::chrono::milliseconds(5) ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;

            static float_t f = 0.0f ;
            f += 0.0001f ;
            if( f > 2.0f * natus::math::constants<float_t>::pi() )
                f = 0.0f ;

            //_camera_0.translate_by( natus::math::vec3f_t( 10.0f * std::sin( f ), 0.0f, 0.0f ) ) ;
            

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t  ) noexcept 
        {
            _pr->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;

            {
                auto const cpos = _camera_0.get_position().xy() ;
                natus::math::vec2f_t p0 = cpos + _extend * natus::math::vec2f_t(-0.5f,-0.5f) ;
                natus::math::vec2f_t p1 = cpos + _extend * natus::math::vec2f_t(-0.5f,+0.5f) ;
                natus::math::vec2f_t p2 = cpos + _extend * natus::math::vec2f_t(+0.5f,+0.5f) ;
                natus::math::vec2f_t p3 = cpos + _extend * natus::math::vec2f_t(+0.5f,-0.5f) ;

                uniform_grid::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( natus::math::vec2i_t( cpos.ceiled() ), 
                        natus::math::vec2ui_t( _extend.ceiled() ) >> natus::math::vec2ui_t( 1 ) ) ;

                // draw cells
                {
                    auto const num_cells = rac.cell_dif() ;
                    auto const pixels_min = _grid.get_dims().cells_to_pixels( rac.cell_min() ) ;
                    auto const view_pixels = _grid.get_dims().cells_to_pixels( num_cells ) ;

                    // x lines / vertical lines
                    {
                        auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                        natus::math::vec2f_t p0( start ) ;
                        natus::math::vec2f_t p1( start + natus::math::vec2i_t( 0, view_pixels.y() ) ) ;

                        for( uint_t i = 0 ; i < num_cells.x() ; ++i )
                        {
                            _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
                            p0 = p0 + natus::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                            p1 = p1 + natus::math::vec2f_t( float_t(_grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                        }
                        _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
                    }

                    // y lines / horizontal lines
                    {
                        auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                        natus::math::vec2f_t p0( start ) ;
                        natus::math::vec2f_t p1( start + natus::math::vec2i_t( view_pixels.x(), 0  ) ) ;
                        
                        for( uint_t i = 0 ; i < num_cells.y() ; ++i )
                        {
                            _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
                            p0 = p0 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                            p1 = p1 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                        }
                        _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
                    }
                }

                // draw regions
                {
                    auto const num_cells = rac.cell_dif() ;
                    auto const num_region = rac.region_dif() ;
                    auto const pixels_min = _grid.get_dims().regions_to_pixels( rac.region_min() ) ;
                    auto const view_pixels = _grid.get_dims().regions_to_pixels( num_region ) ;

                    // x lines / vertical lines
                    {
                        auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                        natus::math::vec2f_t p0( start ) ;
                        natus::math::vec2f_t p1( start + natus::math::vec2i_t( 0, view_pixels.y() ) ) ;

                        for( uint_t i = 0 ; i < num_region.x() ; ++i )
                        {
                            _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(1.0f) ) ;
                            p0 = p0 + natus::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;
                            p1 = p1 + natus::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;
                        }
                        _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(1.0f) ) ;
                    }

                    // y lines / horizontal lines
                    {
                        auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                        natus::math::vec2f_t p0( start ) ;
                        natus::math::vec2f_t p1( start + natus::math::vec2i_t( view_pixels.x(), 0  ) ) ;
                        
                        for( uint_t i = 0 ; i < num_region.y() ; ++i )
                        {
                            _pr->draw_line( 0, p0, p1, natus::math::vec4f_t( 1.0f) ) ;
                            p0 = p0 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                            p1 = p1 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                        }
                        _pr->draw_line( 0, p0, p1, natus::math::vec4f_t( 1.0f) ) ;
                    }
                }
            }
            
            // draw extend of aspect
            if( _draw_debug )
            {
                auto const cpos = _camera_0.get_position().xy() ;

                natus::math::vec2f_t p0 = cpos + _extend * natus::math::vec2f_t(-0.5f,-0.5f) ;
                natus::math::vec2f_t p1 = cpos + _extend * natus::math::vec2f_t(-0.5f,+0.5f) ;
                natus::math::vec2f_t p2 = cpos + _extend * natus::math::vec2f_t(+0.5f,+0.5f) ;
                natus::math::vec2f_t p3 = cpos + _extend * natus::math::vec2f_t(+0.5f,-0.5f) ;

                natus::math::vec4f_t color0( 1.0f, 1.0f, 1.0f, 0.0f ) ;
                natus::math::vec4f_t color1( 1.0f, 0.0f, 0.0f, 1.0f ) ;

                _pr->draw_rect( 50, p0, p1, p2, p3, color0, color1 ) ;
            }

            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

            // render all
            {
                _pr->prepare_for_rendering() ;
                for( size_t i=0; i<100+1; ++i )
                {
                    _pr->render( i ) ;
                }
            }
            
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
            }

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui ) noexcept
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "View Control" ) ;
            {
                float_t data[2] = {_extend.x(), _extend.y() } ;
                ImGui::SliderFloat2( "Extend", data, 0.0f, 1000.0f, "%f", 1.0f ) ;
                _extend.x( data[0] ) ; _extend.y( data[1] ) ;
            }

            {
                ImGui::Checkbox( "Draw Debug", &_draw_debug ) ;
            }

            {
                float_t data[2] = {_camera_0.get_position().x(), _camera_0.get_position().y() } ;
                ImGui::SliderFloat2( "Cam Pos", data, -10000.0f, 10000.0f, "%f", 1.0f ) ;
                _camera_0.translate_to( natus::math::vec3f_t( data[0], data[1], _camera_0.get_position().z() ) ) ;
                
            }

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
