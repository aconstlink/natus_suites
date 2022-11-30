

#include "world.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/gfx/sprite/sprite_render_2d.h>

#include <natus/profile/macros.h>

#include <random>
#include <thread>

// this is about preparing the uniform grid methods for games
// it draws the grid with the camera extend and an extened extend 
// which represents the preload region for texture generation/fetching 
namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;
                
        world::grid_t _grid = world::grid_t( 
            world::dimensions_t( 
                natus::math::vec2ui_t(1000), // regions_per_grid
                natus::math::vec2ui_t(16), // cells_per_region
                natus::math::vec2ui_t(8)  // pixels_per_cell
            ) 
        ) ;

    private:

        enum class draw_type
        {
            none, line, rect, circle
        };

        draw_type _draw_type = draw_type::none ;
        natus::math::vec2ui_t _start_ij ;
        natus::math::vec2ui_t _end_ij ;

    private:

        natus::math::vec2f_t _target = natus::math::vec2f_t( 800, 600.0f ) ;
        natus::math::vec2f_t _aspect_scale = natus::math::vec2f_t( 1.0f ) ;
        natus::math::vec2f_t _preload_extend = natus::math::vec2f_t( 100, 100 ) ;

        bool_t _use_window_for_camera = true ;

        void_t update_preload_extenxd( void_t ) noexcept
        {
            _preload_extend = _ae.get_extent() + natus::math::vec2f_t( _grid.get_dims().get_pixels_per_region() * 2 ) ;
        }

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            auto view1 = this_t::create_window( "A Render Window Default", wi ) ;
            auto view2 = this_t::create_window( "A Render Window Additional", wi,
                { natus::graphics::backend_type::gl4, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _ae = natus::application::util::app_essentials_t( 
                natus::graphics::async_views_t( { view1.async(), view2.async() } ) ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl4, natus::graphics::backend_type::d3d11 } ) ;
            _ae = natus::application::util::app_essentials_t( 
                natus::graphics::async_views_t( { view1.async() } ) ) ;
            #endif
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) noexcept : app( ::std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;

            this_t::update_preload_extenxd() ;

            return natus::application::result::ok ;
        }
        

    private:

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::application::util::app_essentials_t::init_struct is = 
            {
                { "myapp" }, 
                { natus::io::path_t( DATAPATH ), "./working", "data" }
            } ;

            _ae.init( is ) ;

            {
                world::ij_id_t id = _grid.get_dims().calc_cell_ij_id( natus::math::vec2ui_t( 10, 10 ) ) ;
                int const bp = 0 ;
            }
            
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;

            // used for drawing algorithms in cells
            {
                natus::device::layouts::three_mouse_t mouse( _ae.get_mouse_dev() ) ;

                auto const cpos = _ae.get_camera_0()->get_position().xy() ;
                auto const m = _ae.get_cur_mouse_pos() + cpos ;
                _end_ij = _grid.get_dims().calc_cell_ij_global( natus::math::vec2i_t( m ) ) ;

                if( mouse.is_pressed( natus::device::layouts::three_mouse::button::left ) )
                {
                    _draw_type = draw_type::line ;
                    _start_ij = _end_ij ;
                }
                else if( mouse.is_released( natus::device::layouts::three_mouse::button::left ) )
                {
                    _draw_type = draw_type::none ;
                }
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;
            return natus::application::result::ok ; 
        }

        void_t draw_cells( world::dimensions::regions_and_cells_cref_t rac ) noexcept
        {
            auto pr = _ae.get_prim_render() ;

            auto const num_cells = rac.cell_dif() ;
            auto const pixels_min = _grid.get_dims().cells_to_pixels( rac.cell_min() ) ;
            auto const view_pixels = _grid.get_dims().cells_to_pixels( num_cells ) ;

            // x lines / vertical lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                natus::math::vec2f_t p0( start ) ;
                natus::math::vec2f_t p1( start + natus::math::vec2i_t( 0, view_pixels.y() ) ) ;

                for( uint_t i = 0 ; i < num_cells.x() +1 ; ++i )
                {
                    pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
                    p0 = p0 + natus::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                    p1 = p1 + natus::math::vec2f_t( float_t(_grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                }
            }

            // y lines / horizontal lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                natus::math::vec2f_t p0( start ) ;
                natus::math::vec2f_t p1( start + natus::math::vec2i_t( view_pixels.x(), 0  ) ) ;
                        
                for( uint_t i = 0 ; i < num_cells.y() + 1 ; ++i )
                {
                    pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
                    p0 = p0 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                    p1 = p1 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                }
            }
        }

        void_t draw_regions( world::dimensions::regions_and_cells_cref_t rac, 
            size_t l, natus::math::vec4f_cref_t border_color = natus::math::vec4f_t(1.0f) ) noexcept
        {
            auto pr = _ae.get_prim_render() ;

            auto const num_region = rac.region_dif() ;
            auto const pixels_min = _grid.get_dims().regions_to_pixels( rac.region_min() ) ;
            auto const view_pixels = _grid.get_dims().regions_to_pixels( num_region ) ;

            // x lines / vertical lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                natus::math::vec2f_t p0( start ) ;
                natus::math::vec2f_t p1( start + natus::math::vec2i_t( 0, view_pixels.y() ) ) ;

                for( uint_t i = 0 ; i < num_region.x() + 1; ++i )
                {
                    pr->draw_line( l, p0, p1, border_color ) ;
                    p0 = p0 + natus::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;
                    p1 = p1 + natus::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;
                }
            }

            // y lines / horizontal lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                natus::math::vec2f_t p0( start ) ;
                natus::math::vec2f_t p1( start + natus::math::vec2i_t( view_pixels.x(), 0  ) ) ;
                        
                for( uint_t i = 0 ; i < num_region.y() + 1; ++i )
                {
                    pr->draw_line( l, p0, p1, border_color ) ;
                    p0 = p0 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                    p1 = p1 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                }
            }
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _ae.on_graphics_begin( rd ) ;

            auto pr = _ae.get_prim_render() ;
            auto tr = _ae.get_text_render() ;

            _ae.get_camera_0()->set_dims( float_t(_ae.get_window_dims().x()), float_t(_ae.get_window_dims().y()), 1.0f, 1000.0f ) ;

            _ae.get_camera_0()->orthographic() ;

            pr->set_view_proj( _ae.get_camera_0()->mat_view(), _ae.get_camera_0()->mat_proj() ) ;
            tr->set_view_proj( _ae.get_camera_0()->mat_view(), _ae.get_camera_0()->mat_proj() ) ;

            // grid rendering
            {
                auto const cpos = _ae.get_camera_0()->get_position().xy() ;

                // draw grid for extend
                {
                    world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( natus::math::vec2i_t( cpos ), 
                        natus::math::vec2ui_t( _ae.get_extent() ) >> natus::math::vec2ui_t( 1 ) ) ;

                    // draw cells
                    this_t::draw_cells( rac ) ;

                    // draw regions
                    this_t::draw_regions( rac, 3 ) ;
                }

                // draw regions for preload extend
                {
                    world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( natus::math::vec2i_t( cpos ), 
                        natus::math::vec2ui_t( _preload_extend ) >> natus::math::vec2ui_t( 1 ) ) ;

                    // draw regions
                    this_t::draw_regions( rac, 2, natus::math::vec4f_t( 0.0f, 0.0f, 0.5f, 1.0f) ) ;
                }

                // draw current cells for mouse pos in grid
                {
                    auto const cpos = _ae.get_camera_0()->get_position().xy() ;

                    auto const m = _ae.get_cur_mouse_pos() + cpos ;
                    auto const mouse_global = _grid.get_dims().calc_cell_ij_global( natus::math::vec2i_t( m ) ) ;
                    auto const ij = mouse_global ;
                    
                    auto const start = _grid.get_dims().transform_to_center( _grid.get_dims().cells_to_pixels( mouse_global ) )  ;
                    auto const cdims = _grid.get_dims().get_pixels_per_cell() ;

                    natus::math::vec2f_t p0 = start ;
                    natus::math::vec2f_t p1 = start + natus::math::vec2f_t(0.0f, float_t(cdims.y())) ;
                    natus::math::vec2f_t p2 = start + cdims ;
                    natus::math::vec2f_t p3 = start + natus::math::vec2f_t(float_t(cdims.x()),0.0f) ;

                    pr->draw_rect( 0, p0, p1,p2,p3,
                        natus::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ),
                        natus::math::vec4f_t( 1.0f ) ) ;

                    tr->draw_text( 1, 0, 13, natus::math::vec2f_t(p0), natus::math::vec4f_t(1.0f), 
                        "(i,j) : (" + std::to_string( ij.x() ) + ", " + std::to_string( ij.y() ) + ")" ) ;
                }
            }
            
            #if 1
            // draw test primitives
            {
                // Bresenham line algo from Wikipedia at the bottom of the page
                if( _draw_type == draw_type::line )
                {
                    auto const a = natus::math::vec2i_t( _start_ij ) ;
                    auto const b = natus::math::vec2i_t( _end_ij ) ;

                    auto const dd = (a - b).abs() * natus::math::vec2i_t( 1, -1 ) ;
                    auto const sd = a.less_than( b ).select( 
                        natus::math::vec2i_t(1,1), natus::math::vec2i_t(-1,-1) ) ;

                    int_t err = dd.dot( natus::math::vec2i_t( 1 ) ) ;
                    auto x = a ;

                    while( true )
                    {
                        // draw
                        {
                            auto const mouse_global = x ;
                            auto const cdims = _grid.get_dims().get_pixels_per_cell() ;

                            auto const start = _grid.get_dims().transform_to_center( _grid.get_dims().cells_to_pixels( mouse_global ) )  ;

                            natus::math::vec2f_t p0 = start ;
                            natus::math::vec2f_t p1 = start + natus::math::vec2f_t(0.0f,float_t(cdims.y())) ;
                            natus::math::vec2f_t p2 = start + cdims ;
                            natus::math::vec2f_t p3 = start + natus::math::vec2f_t(float_t(cdims.x()),0.0f) ;

                            pr->draw_rect( 1, p0, p1,p2,p3,
                                natus::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ),
                                natus::math::vec4f_t( 1.0f ) ) ;
                        }
                        if( x.equal( _end_ij ).all() ) break ;

                        int_t const e2 = err * 2 ;

                        auto const selector = 
                            natus::math::vec2i_t( e2, dd.x() ).greater_equal_than( 
                            natus::math::vec2i_t( dd.y(), e2 ) ) ;

                        x += selector.select( sd, natus::math::vec2i_t( 0 ) ) ;
                        err += selector.select( dd.yx(), natus::math::vec2i_t(0) ).dot( natus::math::vec2i_t(1) ) ;
                    }
                }
            }
            #endif

            // draw preload extend 
            if( _ae.debug_draw() )
            {
                auto const cpos = _ae.get_camera_0()->get_position().xy() ;

                natus::math::vec2f_t p0 = cpos + _preload_extend * natus::math::vec2f_t(-0.5f,-0.5f) ;
                natus::math::vec2f_t p1 = cpos + _preload_extend * natus::math::vec2f_t(-0.5f,+0.5f) ;
                natus::math::vec2f_t p2 = cpos + _preload_extend * natus::math::vec2f_t(+0.5f,+0.5f) ;
                natus::math::vec2f_t p3 = cpos + _preload_extend * natus::math::vec2f_t(+0.5f,-0.5f) ;

                natus::math::vec4f_t color0( 1.0f, 1.0f, 1.0f, 0.0f ) ;
                natus::math::vec4f_t color1( 0.0f, 0.0f, 1.0f, 1.0f ) ;

                pr->draw_rect( 50, p0, p1, p2, p3, color0, color1 ) ;
            }

            _ae.on_graphics_end( 100 ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Control and Info" ) ;

            {
                ImGui::Checkbox( "Windows Dims for Camera", &_use_window_for_camera ) ;
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
