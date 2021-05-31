
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>

#include <natus/graphics/shader/nsl_bridge.hpp>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profiling/macros.h>

#include <natus/format/global.h>
#include <natus/format/nsl/nsl_module.h>
#include <natus/format/future_items.hpp>

#include <natus/nsl/parser.h>
#include <natus/nsl/database.hpp>
#include <natus/nsl/dependency_resolver.hpp>
#include <natus/nsl/generator_structs.hpp>

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

#include <natus/font/stb/stb_glyph_atlas_creator.h>

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
        
        natus::graphics::image_object_res_t _imgconfig = natus::graphics::image_object_t() ;

        natus::graphics::state_object_res_t _root_render_states ;
        natus::ntd::vector< natus::graphics::render_object_res_t > _render_objects ;
        natus::ntd::vector< natus::graphics::geometry_object_res_t > _geometries ;

        natus::graphics::framebuffer_object_res_t _fb = natus::graphics::framebuffer_object_t() ;
        natus::graphics::render_object_res_t _rc_map = natus::graphics::render_object_t() ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec3f_t nrm ; natus::math::vec2f_t tx ; } ;
        struct vertex2 { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;
        
        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::io::database_res_t _db ;
        natus::nsl::database_res_t _ndb ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _do_tool = false ;

        typedef std::function< void_t ( void_t ) > work_task_t ;

        natus::ntd::vector< std::future<void_t> > _tasks ;

        natus::io::monitor_res_t _shader_mon = natus::io::monitor_t() ;

        natus::math::vec4ui_t _fb_dims = natus::math::vec4ui_t( 0, 0, 1280, 768 ) ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
            _wid_async2 = this_t::create_window( "A Render Window", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11}) ;

            _wid_async.window().position( 50, 50 ) ;
            _wid_async.window().resize( 800, 800 ) ;
            _wid_async2.window().position( 50 + 800, 50 ) ;
            _wid_async2.window().resize( 800, 800 ) ;

            _graphics = natus::graphics::async_views_t( { _wid_async.async(), _wid_async2.async() } ) ;
            #else
            _wid_async = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::es3, natus::graphics::backend_type::d3d11 } ) ;
            #endif

            _db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
            _ndb = natus::nsl::database_t() ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = std::move( rhv._wid_async ) ;
            _wid_async2 = std::move( rhv._wid_async2 ) ;
            _camera_0 = std::move( rhv._camera_0 ) ;
            _geometries = std::move( rhv._geometries ) ;
            _render_objects = std::move( rhv._render_objects ) ;
            _fb = std::move( rhv._fb ) ;
            _rc_map = std::move( rhv._rc_map ) ;
            _ndb = std::move( rhv._ndb ) ;
            _db = std::move( rhv._db ) ;
            _shader_mon = std::move( rhv._shader_mon ) ;
            _graphics = std::move( rhv._graphics ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ),
                float_t(wei.w) / float_t(wei.h), 1.0f, 1000.0f ) ;

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

                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                   
                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = _fb_dims ;
                   
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }

            // geometry configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                    .resize( 4 ).update<vertex2>( [=] ( vertex2* array, size_t const ne )
                {
                    array[ 0 ].pos = natus::math::vec3f_t( -0.5f, -0.5f, 0.0f ) ;
                    array[ 1 ].pos = natus::math::vec3f_t( -0.5f, +0.5f, 0.0f ) ;
                    array[ 2 ].pos = natus::math::vec3f_t( +0.5f, +0.5f, 0.0f ) ;
                    array[ 3 ].pos = natus::math::vec3f_t( +0.5f, -0.5f, 0.0f ) ;

                    array[ 0 ].tx = natus::math::vec2f_t( -0.0f, -0.0f ) ;
                    array[ 1 ].tx = natus::math::vec2f_t( -0.0f, +1.0f ) ;
                    array[ 2 ].tx = natus::math::vec2f_t( +1.0f, +1.0f ) ;
                    array[ 3 ].tx = natus::math::vec2f_t( +1.0f, -0.0f ) ;
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).resize( 6 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 2 ;

                    array[ 3 ] = 0 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 3 ;
                } ) ;

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "quad",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;
            }

            // image configuration
            #if 0
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem = mod_reg->import_from( natus::io::location_t( "images.checker.png" ), _db ) ;
                natus::format::image_item_res_t ii = fitem.get() ;
                if( ii.is_valid() )
                {
                    natus::graphics::image_t img = *ii->img ;

                    _imgconfig = natus::graphics::image_object_t( "loaded_image", std::move( img ) )
                        .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                        .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                        .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                        .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );
                }

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _imgconfig ) ;
                } ) ;
            }
            #endif

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
                    ps->set_value<size_t>( "atlas_width", 512 ) ;
                    ps->set_value<size_t>( "atlas_height", 512 ) ;
                    ps->set_value<size_t>( "point_size", 100 ) ;
                }

                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem = mod_reg->import_from( natus::io::location_t( "fonts.LCD_Solid.ttf" ), _db, ps ) ;
                natus::format::glyph_atlas_item_res_t ii = fitem.get() ;
                if( ii.is_valid() )
                {
                    natus::font::glyph_atlas_t ga = std::move( *ii->obj ) ;

                    natus::graphics::image_t img = natus::graphics::image_t(
                        natus::graphics::image_t::dims_t( ga.get_width(), ga.get_height(), 1 ) )
                        .update( [&] ( natus::graphics::image_ptr_t, natus::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                    {
                        typedef natus::math::vector4< uint8_t > rgba_t ;
                        auto* data_ = reinterpret_cast< rgba_t* >( data_in ) ;

                        auto* raw = ga.get_image( 0 ) ;
                        for( size_t y = 0; y < dims.y(); ++y )
                        {
                            for( size_t x = 0; x < dims.x() ; ++x )
                            {
                                *data_ = rgba_t( 0 ) ;
                                size_t idx = y * dims.y() + x ;
                                data_[ idx ][ 0 ] = raw->get_plane()[ idx ] ;
                            }
                        }
                    } ) ;

                    _imgconfig = natus::graphics::image_object_t( "loaded_image", std::move( img ) )
                        .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                        .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                        .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                        .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );

                    _graphics.for_each( [&] ( natus::graphics::async_view_t a )
                    {
                        a.configure( _imgconfig ) ;
                    } ) ;
                }
            }
            // load
            {
                natus::ntd::vector< natus::io::location_t > shader_locations = {
                    natus::io::location_t( "shaders.map_glyph_atlas.nsl" ),
                    natus::io::location_t( "shaders.post_blit.nsl" )
                };

                natus::ntd::vector< natus::nsl::symbol_t > config_symbols ;

                for( auto const & l : shader_locations )
                {
                    natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                    auto fitem2 = mod_reg->import_from( l, _db ) ;

                    natus::format::nsl_item_res_t ii = fitem2.get() ;
                    if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ), config_symbols ) ;

                    _db->attach( l.as_string(), _shader_mon ) ;
                }

                // generate configs
                for( auto const & s : config_symbols )
                {
                    natus::nsl::generatable_t res = natus::nsl::dependency_resolver_t().resolve(
                        _ndb, s ) ;

                    if( res.missing.size() != 0 )
                    {
                        natus::log::global_t::warning( "We have missing symbols." ) ;
                        for( auto const& s : res.missing )
                        {
                            natus::log::global_t::status( s.expand() ) ;
                        }
                    }

                    auto const sc = natus::graphics::nsl_bridge_t().create(
                        natus::nsl::generator_t( std::move( res ) ).generate() ).set_name( s.expand() ) ;

                    _graphics.for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.configure( sc ) ;
                    } ) ;
                }
            }

            // the rendering objects
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "quad"  ) ;

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "myshaders.map_glyph_atlas" ) ;
                }

                // add variable set 
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    {
                        auto* var = vars->texture_variable( "tex" ) ;
                        var->set( "loaded_image" ) ;
                    }
                    rc.add_variable_set( std::move( vars ) ) ;
                }
                
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( rc ) ;
                } ) ;
                
                _render_objects.emplace_back( std::move( rc ) ) ;
            }

            // framebuffer
            {
                _fb = natus::graphics::framebuffer_object_t( "the_scene" ) ;
                _fb->set_target( natus::graphics::color_target_type::rgba_uint_8, 1 )
                    .set_target( natus::graphics::depth_stencil_target_type::depth32 )
                    .resize( _fb_dims.z(), _fb_dims.w() ) ;

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _fb ) ;
                } ) ;
            }
            
            // blit framebuffer render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "blit" ) ;

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "post_blit" ) ;
                }

                // add variable set 
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    {
                        {
                            auto* var = vars->texture_variable( "u_tex_0" ) ;
                            var->set( "the_scene.0" ) ;
                            //var->set( "checker_board" ) ;
                        }
                    }

                    rc.add_variable_set( std::move( vars ) ) ;
                }

                _rc_map = std::move( rc ) ;

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _rc_map ) ;
                } ) ;
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
            

            // use the framebuffer
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( _fb ) ;
                } ) ;
            }

            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

            for( size_t i=0; i<_render_objects.size(); ++i )
            {
                natus::graphics::backend_t::render_detail_t detail ;
                detail.start = 0 ;
                //detail.num_elems = 3 ;
                detail.varset = 0 ;
                //detail.render_states = _render_states ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.render( _render_objects[i], detail ) ;
                } ) ;
            }

            // un-use the framebuffer
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( natus::graphics::framebuffer_object_t() ) ;
                } ) ;
            }

            // render the root render state sets render object
            // this will set the root render states
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
            }

            // perform mapping
            _graphics.for_each( [&]( natus::graphics::async_view_t a )
            {
                a.render( _rc_map ) ;
            } ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui )
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
