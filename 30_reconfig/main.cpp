
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

#include <natus/nsl/parser.hpp>
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

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

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

        virtual natus::application::result on_init( void_t )
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
                    rss.depth_s.ss.do_activate = true ;
                    rss.depth_s.ss.do_depth_write = true ;

                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::counter_clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = natus::graphics::fill_mode::fill ;

                   
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _wid_async.async().configure( _root_render_states ) ;
                _wid_async2.async().configure( _root_render_states ) ;
            }

            // cube
            {
                natus::geometry::cube_t::input_params ip ;
                ip.scale = natus::math::vec3f_t( 1.0f ) ;
                ip.tess = 100 ;

                natus::geometry::tri_mesh_t tm ;
                natus::geometry::cube_t::make( &tm, ip ) ;
                
                natus::geometry::flat_tri_mesh_t ftm ;
                tm.flatten( ftm ) ;

                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::normal, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                    .resize( ftm.get_num_vertices() ).update<vertex>( [&] ( vertex* array, size_t const ne )
                {
                    for( size_t i=0; i<ne; ++i )
                    {
                        array[ i ].pos = ftm.get_vertex_position_3d( i ) ;
                        array[ i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                        array[ i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                    }
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).resize( ftm.indices.size() ).
                    update<uint_t>( [&] ( uint_t* array, size_t const ne )
                {
                    for( size_t i = 0; i < ne; ++i ) array[ i ] = ftm.indices[ i ] ;
                } ) ;

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "geometry",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _wid_async.async().configure( geo ) ;
                _wid_async2.async().configure( geo ) ;
                _geometries.emplace_back( std::move( geo ) ) ;
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

                _wid_async.async().configure( geo ) ;
                _wid_async2.async().configure( geo ) ;
                _geometries.emplace_back( std::move( geo ) ) ;
            }

            // image configuration
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

                _wid_async.async().configure( _imgconfig ) ;
                _wid_async2.async().configure( _imgconfig ) ;
            }

            // import nsl shader
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem2 = mod_reg->import_from( natus::io::location_t( "shaders.just_render.nsl" ), _db ) ;

                // do the config
                {
                    natus::format::nsl_item_res_t ii = fitem2.get() ;
                    if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ) ) ;
                }
            
                natus::nsl::generatable_t res = natus::nsl::dependency_resolver_t().resolve(
                    _ndb, natus::nsl::symbol( "just_render" ) ) ;

                if( res.missing.size() != 0 )
                {
                    natus::log::global_t::warning( "We have missing symbols." ) ;
                    for( auto const& s : res.missing )
                    {
                        natus::log::global_t::status( s.expand() ) ;
                    }
                }

                auto const sc = natus::graphics::nsl_bridge_t().create(
                    natus::nsl::generator_t( std::move( res ) ).generate() ).set_name( "just_render" ) ;

                _wid_async.async().configure( sc ) ;
                _wid_async2.async().configure( sc ) ;
            }

            // import nsl shader
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem2 = mod_reg->import_from( natus::io::location_t( "shaders.post_blit.nsl" ), _db ) ;

                // do the config
                {
                    natus::format::nsl_item_res_t ii = fitem2.get() ;
                    if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ) ) ;
                }

                natus::nsl::generatable_t res = natus::nsl::dependency_resolver_t().resolve(
                    _ndb, natus::nsl::symbol( "post_blit" ) ) ;

                if( res.missing.size() != 0 )
                {
                    natus::log::global_t::warning( "We have missing symbols." ) ;
                    for( auto const& s : res.missing )
                    {
                        natus::log::global_t::status( s.expand() ) ;
                    }
                }

                auto const sc = natus::graphics::nsl_bridge_t().create(
                    natus::nsl::generator_t( std::move( res ) ).generate() ).set_name( "post_blit" ) ;

                _wid_async.async().configure( sc ) ;
                _wid_async2.async().configure( sc ) ;
            }

            // the rendering objects
            size_t const num_object = 30 ;
            for( size_t i=0; i<num_object;++i )
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( 
                    "object." + std::to_string(i)  ) ;

                {
                    rc.link_geometry( "geometry" ) ;
                    rc.link_shader( "just_render" ) ;
                }

                // add variable set 
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    {
                        auto* var = vars->data_variable< natus::math::vec4f_t >( "color" ) ;
                        var->set( natus::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                    }

                    {
                        auto* var = vars->data_variable< float_t >( "u_time" ) ;
                        var->set( 0.0f ) ;
                    }

                    {
                        auto* var = vars->texture_variable( "tex" ) ;
                        var->set( "loaded_image" ) ;
                    }

                    {
                        float_t const angle = ( float( i ) / float_t( num_object - 1 ) ) * 2.0f * natus::math::constants<float_t>::pi() ;

                        
                        natus::math::m3d::trafof_t trans ;

                        natus::math::m3d::trafof_t rotation ;
                        rotation.rotate_by_axis_fr( natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), angle ) ;
                        
                        natus::math::m3d::trafof_t translation ;
                        translation.translate_fr( natus::math::vec3f_t( 
                            0.0f,
                            10.0f * std::sin( (angle/4.0f) * 2.0f * natus::math::constants<float_t>::pi() ), 
                            -50.0f ) ) ;
                        

                        trans.transform_fl( rotation ) ;
                        trans.transform_fl( translation ) ;
                        trans.transform_fl( rotation ) ;

                        auto* var = vars->data_variable< natus::math::mat4f_t >( "world" ) ;
                        var->set( trans.get_transformation() ) ;
                    }

                    rc.add_variable_set( std::move( vars ) ) ;
                }
                
                _wid_async.async().configure( rc ) ;
                _wid_async2.async().configure( rc ) ;
                _render_objects.emplace_back( std::move( rc ) ) ;
            }

            // framebuffer
            {
                _fb = natus::graphics::framebuffer_object_t( "the_scene" ) ;
                _fb->set_target( natus::graphics::color_target_type::rgba_uint_8, 3 )
                    .set_target( natus::graphics::depth_stencil_target_type::depth32 )
                    .resize( 1024, 1024 ) ;

                _wid_async.async().configure( _fb ) ;
                _wid_async2.async().configure( _fb ) ;
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
                        {
                            auto* var = vars->texture_variable( "u_tex_1" ) ;
                            var->set( "the_scene.1" ) ;
                            //var->set( "checker_board" ) ;
                        }
                        {
                            auto* var = vars->texture_variable( "u_tex_2" ) ;
                            var->set( "the_scene.2" ) ;
                            //var->set( "checker_board" ) ;
                        }
                        {
                            auto* var = vars->texture_variable( "u_tex_3" ) ;
                            var->set( "the_scene.depth" ) ;
                            //var->set( "checker_board" ) ;
                        }
                    }

                    rc.add_variable_set( std::move( vars ) ) ;
                }

                _rc_map = std::move( rc ) ;
                _wid_async.async().configure( _rc_map ) ;
                _wid_async2.async().configure( _rc_map ) ;
            }
            
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) 
        { 
            auto const dif = std::chrono::duration_cast< std::chrono::microseconds >( __clock_t::now() - _tp ) ;
            _tp = __clock_t::now() ;

            float_t const dt = float_t( double_t( dif.count() ) / std::chrono::microseconds::period().den ) ;
            
            if( value > 1.0f ) value = 0.0f ;
            value += natus::math::fn<float_t>::fract( dt ) ;

            {
                static float_t t = 0.0f ;
                t += dt * 0.1f ;

                if( t > 1.0f ) t = 0.0f ;
                
                static natus::math::vec3f_t tr ;
                tr.x( 1.0f * natus::math::fn<float_t>::sin( t * 2.0f * 3.14f ) ) ;
            }

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

            // clear futures
            {
                auto iter = _tasks.begin();
                while( iter != _tasks.end() )
                {
                    if( iter->wait_for( std::chrono::milliseconds(0) ) == std::future_status::ready )
                    {
                        iter = _tasks.erase( iter ) ;
                        continue ;
                    }
                    ++iter ;
                }
            }

            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) 
        { 
            static float_t v = 0.0f ;
            v += 0.01f ;
            if( v > 1.0f ) v = 0.0f ;

            static __clock_t::time_point tp = __clock_t::now() ;

            size_t const num_object = _render_objects.size() ;

            float_t const dt = float_t ( double_t( std::chrono::duration_cast< std::chrono::milliseconds >( __clock_t::now() - tp ).count() ) / 1000.0 ) ;

            // per frame update of variables
            for( auto & rc : _render_objects )
            {
                rc->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
                {
                    {
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "view" ) ;
                        var->set( _camera_0.mat_view() ) ;
                    }

                    {
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "proj" ) ;
                        var->set( _camera_0.mat_proj() ) ;
                    }

                    {
                        auto* var = vs->data_variable< natus::math::vec4f_t >( "color" ) ;
                        var->set( natus::math::vec4f_t( v, 0.0f, 1.0f, 0.5f ) ) ;
                    }

                    {
                        static float_t  angle = 0.0f ;
                        angle = ( ( (dt/10.0f)  ) * 2.0f * natus::math::constants<float_t>::pi() ) ;
                        if( angle > 2.0f * natus::math::constants<float_t>::pi() ) angle = 0.0f ;
                        
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "world" ) ;
                        natus::math::m3d::trafof_t trans( var->get() ) ;

                        natus::math::m3d::trafof_t rotation ;
                        rotation.rotate_by_axis_fr( natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), angle ) ;
                        
                        trans.transform_fl( rotation ) ;

                        var->set( trans.get_transformation() ) ;
                    }
                } ) ;
            }

            tp = __clock_t::now() ;

            // render the root render state sets render object
            // this will set the root render states
            {
                _wid_async.async().use( _root_render_states ) ;
                _wid_async2.async().use( _root_render_states ) ;
            }

            // use the framebuffer
            {
                _wid_async.async().use( _fb, true, true, false ) ;
                _wid_async2.async().use( _fb, true, true, false ) ;
            }

            for( size_t i=0; i<_render_objects.size(); ++i )
            {
                natus::graphics::backend_t::render_detail_t detail ;
                detail.start = 0 ;
                //detail.num_elems = 3 ;
                detail.varset = 0 ;
                //detail.render_states = _render_states ;
                _wid_async.async().render( _render_objects[i], detail ) ;
                _wid_async2.async().render( _render_objects[i], detail ) ;
            }

            // un-use the framebuffer
            {
                _wid_async.async().use( natus::graphics::framebuffer_object_t() ) ;
                _wid_async2.async().use( natus::graphics::framebuffer_object_t() ) ;
            }

            // render the root render state sets render object
            // this will set the root render states
            {
                _wid_async.async().use( natus::graphics::state_object_t(), 10 ) ;
                _wid_async2.async().use( natus::graphics::state_object_t(), 10 ) ;
            }

            // perform mapping
            _wid_async.async().render( _rc_map ) ;
            _wid_async2.async().render( _rc_map ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::gfx::imgui_view_t imgui )
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "Reconfig Objects" ) ;

            if( ImGui::Button( "Reconfig Image" ) )
            {
                _tasks.emplace_back( std::async( std::launch::async, [=] ( void_t )
                {
                    // image configuration
                    {
                        natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                        auto fitem = mod_reg->import_from( natus::io::location_t( "images.test.png" ), _db ) ;
                        natus::format::image_item_res_t ii = fitem.get() ;
                        if( ii.is_valid() )
                        {
                            natus::graphics::image_t img = *ii->img ;

                            natus::graphics::image_object_res_t _img_ = natus::graphics::image_object_t( "loaded_image", std::move( img ) )
                                .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                                .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                                .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                                .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );


                            _wid_async.async().configure( _img_ ) ;
                            _wid_async2.async().configure( _img_ ) ;
                        }
                    }
                    
                    std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;

                    // image configuration 
                    {
                        natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                        auto fitem = mod_reg->import_from( natus::io::location_t( "images.checker.png" ), _db ) ;
                        natus::format::image_item_res_t ii = fitem.get() ;
                        if( ii.is_valid() )
                        {
                            natus::graphics::image_t img = *ii->img ;

                            natus::graphics::image_object_res_t _img_ = natus::graphics::image_object_t( "loaded_image", std::move( img ) )
                                .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                                .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                                .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                                .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );


                            _wid_async.async().configure( _img_ ) ;
                            _wid_async2.async().configure( _img_ ) ;
                        }
                    }
                } ) ) ;

                _tasks.emplace_back( std::async( std::launch::async, [=] ( void_t )
                {
                    
                } ) ) ;
            }

            if( ImGui::Button( "Reconfig Geometry" ) )
            {
                _tasks.emplace_back( std::async( std::launch::async, [=] ( void_t )
                {
                    // tet
                    {
                        natus::geometry::tetra::input_params ip ;
                        ip.scale = natus::math::vec3f_t( 1.0f ) ;

                        natus::geometry::polygon_mesh_t tm ;
                        natus::geometry::tetra::make( &tm, ip ) ;

                        natus::geometry::flat_tri_mesh_t ftm ;
                        tm.flatten( ftm ) ;

                        auto vb = natus::graphics::vertex_buffer_t()
                            .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                            .add_layout_element( natus::graphics::vertex_attribute::normal, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                            .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                            .resize( ftm.get_num_vertices() ).update<vertex>( [&] ( vertex* array, size_t const ne )
                        {
                            for( size_t i = 0; i < ne; ++i )
                            {
                                array[ i ].pos = ftm.get_vertex_position_3d( i ) ;
                                array[ i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                                array[ i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                            }
                        } );

                        auto ib = natus::graphics::index_buffer_t().
                            set_layout_element( natus::graphics::type::tuint ).resize( ftm.indices.size() ).
                            update<uint_t>( [&] ( uint_t* array, size_t const ne )
                        {
                            for( size_t i = 0; i < ne; ++i ) array[ i ] = ftm.indices[ i ] ;
                        } ) ;

                        natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "geometry",
                            natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                        _wid_async.async().configure( geo ) ;
                        _wid_async2.async().configure( geo ) ;
                    }

                    std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;

                    // cube
                    {
                        natus::geometry::cube_t::input_params ip ;
                        ip.scale = natus::math::vec3f_t( 1.0f ) ;
                        ip.tess = 100 ;

                        natus::geometry::tri_mesh_t tm ;
                        natus::geometry::cube_t::make( &tm, ip ) ;

                        natus::geometry::flat_tri_mesh_t ftm ;
                        tm.flatten( ftm ) ;

                        auto vb = natus::graphics::vertex_buffer_t()
                            .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                            .add_layout_element( natus::graphics::vertex_attribute::normal, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                            .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                            .resize( ftm.get_num_vertices() ).update<vertex>( [&] ( vertex* array, size_t const ne )
                        {
                            for( size_t i = 0; i < ne; ++i )
                            {
                                array[ i ].pos = ftm.get_vertex_position_3d( i ) ;
                                array[ i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                                array[ i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                            }
                        } );

                        auto ib = natus::graphics::index_buffer_t().
                            set_layout_element( natus::graphics::type::tuint ).resize( ftm.indices.size() ).
                            update<uint_t>( [&] ( uint_t* array, size_t const ne )
                        {
                            for( size_t i = 0; i < ne; ++i ) array[ i ] = ftm.indices[ i ] ;
                        } ) ;

                        natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "geometry",
                            natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                        _wid_async.async().configure( geo ) ;
                        _wid_async2.async().configure( geo ) ;
                    }
                } ) ) ;
            }

            if( ImGui::Button( "Reconfig Object Shader" ) )
            {
                _tasks.emplace_back( std::async( std::launch::async, [=] ( void_t )
                {
                    natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                    auto fitem2 = mod_reg->import_from( natus::io::location_t( "shaders.just_render.nsl" ), _db ) ;

                    // do the config
                    {
                        natus::format::nsl_item_res_t ii = fitem2.get() ;
                        if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ) ) ;
                    }

                    natus::nsl::generatable_t res = natus::nsl::dependency_resolver_t().resolve(
                        _ndb, natus::nsl::symbol( "just_render" ) ) ;

                    if( res.missing.size() != 0 )
                    {
                        natus::log::global_t::warning( "We have missing symbols." ) ;
                        for( auto const& s : res.missing )
                        {
                            natus::log::global_t::status( s.expand() ) ;
                        }
                    }

                    auto const sc = natus::graphics::nsl_bridge_t().create(
                        natus::nsl::generator_t( std::move( res ) ).generate() ).set_name( "just_render" ) ;

                    _wid_async.async().configure( sc ) ;
                    _wid_async2.async().configure( sc ) ;
                } ) ) ;
            }

            ImGui::End() ;
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_shutdown( void_t ) 
        { return natus::application::result::ok ; }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}
