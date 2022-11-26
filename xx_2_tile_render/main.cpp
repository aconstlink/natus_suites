

#include "world.h"
#include "tile_render.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/primitive/primitive_render_2d.h> 
#include <natus/gfx/font/text_render_2d.h>
#include <natus/gfx/sprite/sprite_render_2d.h>

#include <natus/format/global.h>
#include <natus/format/nsl/nsl_module.h>
#include <natus/format/future_items.hpp>

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

        natus::graphics::state_object_res_t _root_render_states ;
                
        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        natus::math::vec2f_t _cur_mouse ;

        bool_t _do_tool = true ;
        
        world::grid_t _grid = world::grid_t( 
            world::dimensions_t( 
                natus::math::vec2ui_t(1000), // regions_per_grid
                natus::math::vec2ui_t(16), // cells_per_region
                natus::math::vec2ui_t(8)  // pixels_per_cell
            ) 
        ) ;

        natus::io::database_res_t _db ;

    private: //

        natus::gfx::text_render_2d_res_t _tr ;
        natus::gfx::primitive_render_2d_res_t _pr ;

        bool_t _draw_debug = false ;

        proto::tile_render_2d_res_t _tir ;
        natus::gfx::sprite_render_2d_res_t _sr ;

        world::dimensions::regions_and_cells_t rac_ ;

    private:

        natus::math::vec2f_t _target = natus::math::vec2f_t( 800, 600.0f ) ;
        natus::math::vec2f_t _window_dims = natus::math::vec2f_t( 1.0f ) ;
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
                { natus::graphics::backend_type::gl4, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _graphics = natus::graphics::async_views_t( { view1.async(), view2.async() } ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl4, natus::graphics::backend_type::d3d11 } ) ;
            _graphics = natus::graphics::async_views_t( { view1.async() } ) ;
            #endif

            _db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) noexcept : app( ::std::move( rhv ) ) 
        {
            _camera_0 = std::move( rhv._camera_0 ) ;
            _graphics = std::move( rhv._graphics ) ;
            _db = std::move( rhv._db ) ;
            _tir = std::move( rhv._tir ) ;
            _sr = std::move( rhv._sr ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei ) noexcept
        {
            natus::math::vec2f_t const target = _target ; 
            natus::math::vec2f_t const window = natus::math::vec2f_t( float_t(wei.w), float_t(wei.h) ) ;

            natus::math::vec2f_t const ratio = window / target ;

            _window_dims = window ;
            _aspect_scale = ratio ;

            this_t::update_extend() ;
            this_t::update_preload_extenxd() ;

            _tir->release_all() ;
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
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back;
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

            // import fonts and create text render
            {
                natus::property::property_sheet_res_t ps = natus::property::property_sheet_t() ;

                {
                    natus::font::code_points_t pts ;
                    for( uint32_t i = 33; i <= 126; ++i ) pts.emplace_back( i ) ;
                    for( uint32_t i : {uint32_t( 0x00003041 )} ) pts.emplace_back( i ) ;
                    ps->set_value< natus::font::code_points_t >( "code_points", pts ) ;
                }

                {
                    natus::ntd::vector< natus::io::location_t > locations = 
                    {
                        natus::io::location_t("fonts.mitimasu.ttf"),
                        //natus::io::location_t("")
                    } ;
                    ps->set_value( "additional_locations", locations ) ;
                }

                {
                    ps->set_value<size_t>( "atlas_width", 128 ) ;
                    ps->set_value<size_t>( "atlas_height", 512 ) ;
                    ps->set_value<size_t>( "point_size", 90 ) ;
                }

                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem = mod_reg->import_from( natus::io::location_t( "fonts.LCD_Solid.ttf" ), _db, ps ) ;
                natus::format::glyph_atlas_item_res_t ii = fitem.get() ;
                if( ii.is_valid() )
                {
                    _tr = natus::gfx::text_render_2d_res_t( natus::gfx::text_render_2d_t(  ) ) ;
                    
                    _tr->init( "my_text_render", _graphics, std::move( *ii->obj ) ) ;
                }
            }

            {
                world::ij_id_t id = _grid.get_dims().calc_cell_ij_id( natus::math::vec2ui_t( 10, 10 ) ) ;
                int const bp = 0 ;
            }

            {
                _tir = proto::tile_render_2d_res_t( proto::tile_render_2d_t( _graphics ) ) ;
                _tir->init( "tile_render", _grid.get_dims().get_pixels_per_region().x(), _grid.get_dims().get_pixels_per_region().x() ) ;
            }
            
            {
                // taking all slices
                natus::graphics::image_t imgs ;

                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto item = mod_reg->import_from( natus::io::location_t( "images.tileset_64x64.png" ), _db ) ;

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

            {
                natus::device::layouts::three_mouse_t mouse( _dev_mouse ) ;
                _cur_mouse = mouse.get_local() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;
                _cur_mouse = _cur_mouse * (_window_dims * natus::math::vec2f_t(0.5f) );
            }

            // move camera with mouse
            {
                natus::device::layouts::three_mouse_t mouse( _dev_mouse ) ;
                static auto m_rel_old = natus::math::vec2f_t() ;
                auto const m_rel = mouse.get_local() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;

                auto const cpos = _camera_0.get_position().xy() ;
                auto const m = cpos + (m_rel-m_rel_old).negated() * 100.0f ;

                if( mouse.is_pressing( natus::device::layouts::three_mouse::button::left ) )
                {
                    _camera_0.translate_to( natus::math::vec3f_t( m.x(), m.y(), _camera_0.get_position().z() ) ) ;
                }

                m_rel_old = m_rel ;
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
                    _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
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
                    _pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.6f) ) ;
                    p0 = p0 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                    p1 = p1 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                }
            }
        }

        void_t draw_regions( world::dimensions::regions_and_cells_cref_t rac, 
            size_t l, natus::math::vec4f_cref_t border_color = natus::math::vec4f_t(1.0f) ) noexcept
        {
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
                    _pr->draw_line( l, p0, p1, border_color ) ;
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
                    _pr->draw_line( l, p0, p1, border_color ) ;
                    p0 = p0 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                    p1 = p1 + natus::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                }
            }
        }

        void_t release_tiles( world::dimensions::regions_and_cells_cref_t rac, world::dimensions::regions_and_cells_cref_t rac_old ) noexcept
        {
            auto const num_region = rac_old.region_dif() ;

            for( uint_t x = 0 ; x < num_region.x() ; ++x )
            {
                for( uint_t y = 0 ; y < num_region.y(); ++y )
                {
                    if( !rac.is_region_inside( rac_old.region_min() + natus::math::vec2ui_t( x, y ) ) )
                    {
                        world::ij_id_t const id = _grid.get_dims().calc_region_ij_id( rac_old.region_min() + natus::math::vec2ui_t( x, y ) ) ;
                        _tir->release_tile( 0, id.get_id() ) ;

                        //natus::log::global_t::status("Release : " + std::to_string(id.get_id()) ) ;
                    }
                }
            }
        }

        void_t new_tiles( world::dimensions::regions_and_cells_cref_t rac, world::dimensions::regions_and_cells_cref_t rac_old ) noexcept
        {
            auto const num_region = rac.region_dif() ;

            for( uint_t i = 0 ; i < num_region.x() ; ++i )
            {
                for( uint_t j = 0 ; j < num_region.y(); ++j )
                {
                    if( !rac_old.is_region_inside( rac.region_min() + natus::math::vec2ui_t( i, j ) ) )
                    {
                        world::ij_id_t const id = _grid.get_dims().calc_region_ij_id( rac.region_min() + natus::math::vec2ui_t( i, j ) ) ;
                        auto t = _tir->acquire_tile( 0, id.get_id() ) ;

                        auto const cpr = _grid.get_dims().get_cells_per_region() ;
                        t->resize( cpr.x() * cpr.y() ) ;
                        t->draw( [&]( proto::tile_render_2d::tile::items_ref_t items )
                        {
                            proto::tile_render_2d::tile::item_t item ;
                            item.layer = 0 ;
                            item.color = natus::math::vec4f_t() ;
                            item.mid = 0 ;
                            item.pos = natus::math::vec2f_t() ;
                            item.scale = natus::math::vec2f_t() ;

                            //items[i++] = item ;
                            

                            for( size_t x = 0 ; x < cpr.x(); ++x )
                            {
                                for( size_t y = 0 ; y < cpr.y(); ++y )
                                {

                                }
                            }
                        } ) ;

                        natus::log::global_t::status("New : " + std::to_string(id.get_id()) ) ;
                    }
                }
            }
        }

        void_t update_tiles( world::dimensions::regions_and_cells_cref_t rac ) noexcept
        {
            auto const num_region = rac.region_dif() ;

            for( uint_t x = 0 ; x < num_region.x() ; ++x )
            {
                for( uint_t y = 0 ; y < num_region.y(); ++y )
                {
                    world::ij_id_t const id = _grid.get_dims().calc_region_ij_id( rac.region_min() + natus::math::vec2ui_t( x, y ) ) ;
                    auto t = _tir->acquire_tile( 0, id.get_id() ) ;
                    
                    
                }
            }
        }

        void_t move_tiles( world::dimensions::regions_and_cells_cref_t rac ) noexcept
        {
            auto const num_region = rac.region_dif() ;
            auto const pixels_min = _grid.get_dims().regions_to_pixels( rac.region_min() ) ;

            auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;

            for( uint_t x = 0 ; x < num_region.x() ; ++x )
            {
                for( uint_t y = 0 ; y < num_region.y(); ++y )
                {
                    world::ij_id_t const id = _grid.get_dims().calc_region_ij_id( rac.region_min() + natus::math::vec2ui_t( x, y ) ) ;
                    auto t = _tir->acquire_tile( 0, id.get_id() ) ;
                    
                    natus::math::vec2f_t const pixels_per_regions( _grid.get_dims().get_pixels_per_region() ) ;

                    natus::math::vec2f_t p0 = start + pixels_per_regions * natus::math::vec2f_t( float_t( x ), float_t( y ) ) +
                        pixels_per_regions * natus::math::vec2f_t(0.5f) ;

                    t->transform( p0, pixels_per_regions * natus::math::vec2f_t(0.5f) ) ;
                }
            }
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t  ) noexcept 
        {
            if( !_use_window_for_camera )
            {
                this_t::update_extend() ;
                _camera_0.set_dims( _extend.x(), _extend.y(), 1.0f, 1000.0f ) ;
            }
            else
                _camera_0.set_dims( float_t(_window_dims.x()), float_t(_window_dims.y()), 1.0f, 1000.0f ) ;

            _camera_0.orthographic() ;

            _pr->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;
            _tr->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;
            _tir->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;

            // grid rendering
            {
                auto const cpos = _camera_0.get_position().xy() ;

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
                    auto const cpos = _camera_0.get_position().xy() ;

                    auto const m = _cur_mouse + cpos ;
                    auto const mouse_global = _grid.get_dims().calc_cell_ij_global( natus::math::vec2i_t( m ) ) ;
                    auto const ij = mouse_global ;
                    
                    auto const start = _grid.get_dims().transform_to_center( _grid.get_dims().cells_to_pixels( mouse_global ) )  ;
                    auto const cdims = _grid.get_dims().get_pixels_per_cell() ;

                    natus::math::vec2f_t p0 = start ;
                    natus::math::vec2f_t p1 = start + natus::math::vec2f_t(0.0f,cdims.y()) ;
                    natus::math::vec2f_t p2 = start + cdims ;
                    natus::math::vec2f_t p3 = start + natus::math::vec2f_t(cdims.x(),0.0f) ;

                    _pr->draw_rect( 0, p0, p1,p2,p3,
                        natus::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ),
                        natus::math::vec4f_t( 1.0f ) ) ;

                    _tr->draw_text( 1, 0, 13, natus::math::vec2f_t(p0), natus::math::vec4f_t(1.0f), 
                        "(i,j) : (" + std::to_string( ij.x() ) + ", " + std::to_string( ij.y() ) + ")" ) ;
                }
            }

            {
                auto const cpos = _camera_0.get_position().xy() ;

                world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( natus::math::vec2i_t( cpos ), 
                        natus::math::vec2ui_t( _extend ) >> natus::math::vec2ui_t( 1 ) ) ;

                this_t::release_tiles( rac, rac_ ) ;
                this_t::new_tiles( rac, rac_ ) ;
                //this_t::update_tiles( rac ) ;
                this_t::move_tiles( rac ) ;

                rac_ = rac ;
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

            // draw preload extend 
            if( _draw_debug )
            {
                auto const cpos = _camera_0.get_position().xy() ;

                natus::math::vec2f_t p0 = cpos + _preload_extend * natus::math::vec2f_t(-0.5f,-0.5f) ;
                natus::math::vec2f_t p1 = cpos + _preload_extend * natus::math::vec2f_t(-0.5f,+0.5f) ;
                natus::math::vec2f_t p2 = cpos + _preload_extend * natus::math::vec2f_t(+0.5f,+0.5f) ;
                natus::math::vec2f_t p3 = cpos + _preload_extend * natus::math::vec2f_t(+0.5f,-0.5f) ;

                natus::math::vec4f_t color0( 1.0f, 1.0f, 1.0f, 0.0f ) ;
                natus::math::vec4f_t color1( 0.0f, 0.0f, 1.0f, 1.0f ) ;

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
                _tr->prepare_for_rendering() ;
                _tir->prepare_for_rendering() ;

                for( size_t i=0; i<100+1; ++i )
                {
                    _pr->render( i ) ;
                    _tr->render( i ) ;
                    _tir->render( i ) ;
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

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t ) noexcept
        {
            if( !_do_tool ) return natus::application::result::no_tool ;

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
                float_t data[2] = {_camera_0.get_position().x(), _camera_0.get_position().y() } ;
                ImGui::SliderFloat2( "Cam Pos", data, -1000.0f, 1000.0f, "%f", 1.0f ) ;
                _camera_0.translate_to( natus::math::vec3f_t( data[0], data[1], _camera_0.get_position().z() ) ) ;
                
            }

            {
                ImGui::Text( "mx: %f, my: %f", _cur_mouse.x(), _cur_mouse.y() ) ;
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
