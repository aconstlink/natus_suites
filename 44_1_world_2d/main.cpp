

#include "world.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/simple_app_essentials.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/gfx/sprite/sprite_render_2d.h>

#include <natus/profile/macros.h>

#include <random>
#include <thread>

// this tests uses(copy) what has been done in the last test
// and adds textured quads to each region.
namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::graphics::async_views_t _graphics ;
        natus::application::util::simple_app_essentials_t _ae ;

        world::grid_t _grid = world::grid_t( 
            world::dimensions_t( 
                natus::math::vec2ui_t(1000), // regions_per_grid
                natus::math::vec2ui_t(16), // cells_per_region
                natus::math::vec2ui_t(8)  // pixels_per_cell
            ) 
        ) ;

    private: //

        bool_t _draw_debug = false ;
        bool_t _draw_grid = false ;

        natus::gfx::sprite_render_2d_res_t _sr ;
        world::dimensions::regions_and_cells_t rac_ ;

    private:

        natus::math::vec2f_t _target = natus::math::vec2f_t( 800, 600.0f ) ;
        natus::math::vec2f_t _aspect_scale = natus::math::vec2f_t( 1.0f ) ;
        natus::math::vec2f_t _extend = natus::math::vec2f_t( 100, 100 ) ;
        natus::math::vec2f_t _preload_extend = natus::math::vec2f_t( 100, 100 ) ;

        bool_t _use_window_for_camera = true ;

        void_t update_preload_extenxd( void_t ) noexcept
        {
            _preload_extend = _extend + natus::math::vec2f_t( _grid.get_dims().get_pixels_per_region() * 2 ) ;
        }

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
            _ae = std::move( rhv._ae ) ;
            _graphics = std::move( rhv._graphics ) ;
            _sr = std::move( rhv._sr ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;
            natus::math::vec2f_t const target = _target ; 
            natus::math::vec2f_t const window = _ae.get_window_dims() ;

            natus::math::vec2f_t const ratio = window / target ;
            
            _aspect_scale = ratio ;

            this_t::update_extend() ;
            this_t::update_preload_extenxd() ;

            rac_ = world::dimensions::regions_and_cells_t() ;

            return natus::application::result::ok ;
        }

        // some side-effects: updates internal variables.
        void_t update_extend( void_t ) noexcept
        {
            auto const ratio = _aspect_scale ;
            _extend = _target * (ratio.x() < ratio.y() ? ratio.xx() : ratio.yy()) ;
        }
        

    private:

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::application::util::simple_app_essentials_t::init_struct is = 
            {
                { "myapp", _graphics }, 
                { natus::io::path_t( DATAPATH ), "./working", "data" }
            } ;

            _ae.init( is ) ;

            {
                world::ij_id_t id = _grid.get_dims().calc_cell_ij_id( natus::math::vec2ui_t( 10, 10 ) ) ;
                int const bp = 0 ;
            }
            
            {
                // taking all slices
                natus::graphics::image_t imgs ;

                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto item = mod_reg->import_from( natus::io::location_t( "images.tileset_64x64.png" ), _ae.db() ) ;

                natus::format::image_item_res_t ii = item.get() ;
                if( ii.is_valid() )
                {
                    imgs.append( *ii->img ) ;
                }

                natus::graphics::image_object_res_t ires = natus::graphics::image_object_t( 
                    "tile_sets", std::move( imgs ) )
                    .set_type( natus::graphics::texture_type::texture_2d_array )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                    .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                    .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( ires ) ;
                } ) ;
            }

            // prepare sprite render
            {
                _sr = natus::gfx::sprite_render_2d_res_t( natus::gfx::sprite_render_2d_t() ) ;
                _sr->init( "sprite_render", "tile_sets", _graphics ) ;
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;
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

        void_t draw_content( world::dimensions::regions_and_cells_cref_t rac ) noexcept
        {
            auto pr = _ae.get_prim_render() ;

            auto const cells_start = rac.cell_min() ;

            for( auto y = rac.ocell_min().y(); y < rac.ocell_max().y(); ++y )
            {
                for( auto x = rac.ocell_min().x(); x < rac.ocell_max().x(); ++x )
                {
                    auto const cur_pos = natus::math::vec2i_t( x, y ) ;
                    natus::math::vec2f_t const p0 = _grid.get_dims().cells_to_pixels( cur_pos ) ;
                    natus::math::vec2f_t const d = natus::math::vec2f_t( _grid.get_dims().get_pixels_per_cell() ) ;
                    natus::math::vec2f_t const dh = natus::math::vec2f_t( _grid.get_dims().get_pixels_per_cell() >> uint_t(1) ) ;

                    {
                        auto const p1 = p0 + natus::math::vec2f_t( 0.0f, d.y() ) ;
                        auto const p2 = p0 + natus::math::vec2f_t( d.x(), d.y() ) ;
                        auto const p3 = p0 + natus::math::vec2f_t( d.x(), 0.0f ) ;

                        int_t f = int_t( 10.0f * std::sin( x * 100.0f ) - y ) ;

                        if( f > 0 )
                        {
                            pr->draw_rect( 0, p0, p1, p2, p3, natus::math::vec4f_t(0.0f,0.0f,1.0f,1.0f), natus::math::vec4f_t(0.6f) ) ;
                        }                        
                        else
                        {
                            pr->draw_rect( 0, p0, p1, p2, p3, natus::math::vec4f_t(0.6f,0.1f,0.3f,1.0f), natus::math::vec4f_t(0.6f) ) ;
                        }

                    }

                    #if 0
                    if( std::abs( 10.0f * std::sin( cur_pos.x()*100.0f ) - cur_pos.y() ) < 2.0f  ) 
                    {
                        pr->draw_circle( 0, 10, p0 + off, 2.0f, natus::math::vec4f_t(1.0f), natus::math::vec4f_t(0.6f) ) ;
                    }

                    
                    if( std::abs( 10.0f * std::cos( cur_pos.x()*100.0f ) - cur_pos.y() ) < 2.0f  ) 
                    {
                        pr->draw_circle( 1, 10, p0 + off, 2.0f, natus::math::vec4f_t(0.0f,0.0f,1.0f,1.0f), natus::math::vec4f_t(0.6f) ) ;
                    }
                    #endif
                }
            }
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t  rd ) noexcept 
        {
            auto pr = _ae.get_prim_render() ;
            auto tr = _ae.get_text_render() ;

            _ae.on_graphics_begin( rd ) ;

            if( !_use_window_for_camera )
            {
                this_t::update_extend() ;
                _ae.get_camera_0()->set_dims( _extend.x(), _extend.y(), 1.0f, 1000.0f ) ;
            }
            else
                _ae.get_camera_0()->set_dims( float_t(_ae.get_window_dims().x()), float_t(_ae.get_window_dims().y()), 1.0f, 1000.0f ) ;

            _ae.get_camera_0()->orthographic() ;

            pr->set_view_proj( _ae.get_camera_0()->mat_view(), _ae.get_camera_0()->mat_proj() ) ;
            tr->set_view_proj( _ae.get_camera_0()->mat_view(), _ae.get_camera_0()->mat_proj() ) ;

            // grid rendering
            if( _draw_grid )
            {
                auto const cpos = _ae.get_camera_0()->get_position().xy() ;

                // draw grid for extend
                {
                    world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( natus::math::vec2i_t( cpos ), 
                        natus::math::vec2ui_t( _extend ) >> natus::math::vec2ui_t( 1 ) ) ;

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
                    natus::math::vec2f_t p1 = start + natus::math::vec2f_t(0.0f, float_t( cdims.y() )) ;
                    natus::math::vec2f_t p2 = start + cdims ;
                    natus::math::vec2f_t p3 = start + natus::math::vec2f_t( float_t(cdims.x()), 0.0f ) ;

                    pr->draw_rect( 0, p0, p1,p2,p3,
                        natus::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ),
                        natus::math::vec4f_t( 1.0f ) ) ;

                    tr->draw_text( 1, 0, 13, natus::math::vec2f_t(p0), natus::math::vec4f_t(1.0f), 
                        "(i,j) : (" + std::to_string( ij.x() ) + ", " + std::to_string( ij.y() ) + ")" ) ;
                }
            }

            // content section
            {
                auto const cpos = _ae.get_camera_0()->get_position().xy() ;

                world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( natus::math::vec2i_t( cpos ), 
                        natus::math::vec2ui_t( _extend ) >> natus::math::vec2ui_t( 1 ) ) ;
                
                this_t::draw_content( rac ) ;

                rac_ = rac ;
            }
            
            // draw extend of aspect
            if( _draw_debug )
            {
                auto const cpos = _ae.get_camera_0()->get_position().xy() ;

                natus::math::vec2f_t p0 = cpos + _extend * natus::math::vec2f_t(-0.5f,-0.5f) ;
                natus::math::vec2f_t p1 = cpos + _extend * natus::math::vec2f_t(-0.5f,+0.5f) ;
                natus::math::vec2f_t p2 = cpos + _extend * natus::math::vec2f_t(+0.5f,+0.5f) ;
                natus::math::vec2f_t p3 = cpos + _extend * natus::math::vec2f_t(+0.5f,-0.5f) ;

                natus::math::vec4f_t color0( 1.0f, 1.0f, 1.0f, 0.0f ) ;
                natus::math::vec4f_t color1( 1.0f, 0.0f, 0.0f, 1.0f ) ;

                pr->draw_rect( 50, p0, p1, p2, p3, color0, color1 ) ;
            }

            // draw preload extend 
            if( _draw_debug )
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

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t ) noexcept
        {
            if( !_ae.do_tool() ) return natus::application::result::no_tool ;

            ImGui::Begin( "Control and Info" ) ;
            {
                int_t data[2] = { int_t( _extend.x() ), int_t( _extend.y() ) } ;
                ImGui::SliderInt2( "Extend", data, 0, 1000, "%i", 0 ) ;
                _extend.x( float_t( data[0] ) ) ; _extend.y( float_t( data[1] ) ) ;
                this_t::update_preload_extenxd() ;
            }

            {
                ImGui::Checkbox( "Windows Dims for Camera", &_use_window_for_camera ) ;
            }

            {
                ImGui::Checkbox( "Draw Debug", &_draw_debug ) ;
            }

            {
                ImGui::Checkbox( "Draw Grid", &_draw_grid ) ;
            }

            {
                float_t data[2] = {_ae.get_camera_0()->get_position().x(), _ae.get_camera_0()->get_position().y() } ;
                ImGui::SliderFloat2( "Cam Pos", data, -1000.0f, 1000.0f, "%f" ) ;
                _ae.get_camera_0()->translate_to( natus::math::vec3f_t( data[0], data[1], _ae.get_camera_0()->get_position().z() ) ) ;
                
            }

            {
                ImGui::Text( "mx: %f, my: %f", _ae.get_cur_mouse_pos().x(), _ae.get_cur_mouse_pos().y() ) ;
                //_cur_mouse
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
