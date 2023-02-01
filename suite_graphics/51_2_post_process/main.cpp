

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/geometry/3d/cube.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>

#include <natus/nsl/parser.h>
#include <natus/nsl/database.hpp>
#include <natus/nsl/dependency_resolver.hpp>
#include <natus/nsl/generator_structs.hpp>

#include <natus/tool/imgui/custom_widgets.h>

#include <natus/profile/macros.h>

#include <random>
#include <thread>

//
namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

    private:

        natus::application::util::app_essentials_t _ae ;

    private: // geometry

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec3f_t nrm ; natus::math::vec2f_t tx ; } ;
        struct vertex2 { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;

        natus::ntd::vector< natus::graphics::geometry_object_res_t > _geometries ;
        natus::ntd::vector< natus::graphics::render_object_res_t > _render_objects ;
        natus::graphics::state_object_res_t _root_render_states ;
        natus::graphics::state_object_res_t _post_render_states ;
        
        natus::graphics::framebuffer_object_res_t _fb[2] = {
        natus::graphics::framebuffer_object_t(),
        natus::graphics::framebuffer_object_t() } ;
        natus::graphics::render_object_res_t _rc_post = natus::graphics::render_object_t() ;
        natus::graphics::render_object_res_t _rc_map = natus::graphics::render_object_t() ;

        natus::math::vec4ui_t _fb_dims = natus::math::vec4ui_t( 0, 0, 1280, 768 ) ;

        natus::math::vec3f_t _dark_color = natus::math::vec3f_t( 0.0f, 0.0f, 0.4f ) ;
        natus::math::vec3f_t _light_color = natus::math::vec3f_t( 0.5f, 0.2f, 0.1f ) ;
    public:

        test_app( void_t ) noexcept 
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
            _geometries = std::move( rhv._geometries ) ;
            _render_objects = std::move( rhv._render_objects ) ;
            _fb[0] = std::move( rhv._fb[0] ) ;
            _fb[1] = std::move( rhv._fb[1] ) ;
            _rc_map = std::move( rhv._rc_map ) ;
        }

        virtual ~test_app( void_t ) noexcept 
        {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;
            return natus::application::result::ok ;
        }        

    private:

        //*****************************************************************************
        void_t init_cubes( void_t ) noexcept
        {
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

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "cube",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;

                _geometries.emplace_back( std::move( geo ) ) ;
            }

            {
                natus::ntd::string_t const nsl_shader = R"(
                    config just_render
                    {
                        vertex_shader
                        {
                            mat4_t proj : projection ;
                            mat4_t view : view ;
                            mat4_t world : world ;
                            
                            in vec3_t pos : position ;
                            in vec3_t nrm : normal ;
                            in vec2_t tx : texcoord ;

                            out vec4_t pos : position ;
                            out vec2_t tx : texcoord ;
                            out vec3_t nrm : normal ;

                            void main()
                            {
                                vec3_t pos = in.pos ;
                                pos.xyz = pos.xyz * 10.0 ;
                                out.tx = in.tx ;
                                out.pos = proj * view * world * vec4_t( pos, 1.0 ) ;
                                out.nrm = normalize( world * vec4_t( in.nrm, 0.0 ) ).xyz ;
                            }
                        }

                        pixel_shader
                        {
                            vec4_t color ;

                            in vec2_t tx : texcoord ;
                            in vec3_t nrm : normal ;
                            out vec4_t color0 : color0 ;
                            //out vec4_t color1 : color1 ;
                            //out vec4_t color2 : color2 ;

                            void main()
                            {
                                float_t light = dot( normalize( in.nrm ), normalize( vec3_t( 1.0, 1.0, 0.5) ) ) ;
                                out.color0 = vec4_t( as_vec3( light ), 1.0 ) ;
                                //out.color1 = vec4_t( in.nrm, 1.0 ) ;
                                //out.color2 = vec4_t( light, light, light , 1.0 ) ;
                            }
                        }
                    }
                )" ;

                _ae.process_shader( nsl_shader ) ;
            }

            // the rendering objects
            size_t const num_object = 30 ;
            for( size_t i=0; i<num_object;++i )
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( 
                    "object." + std::to_string(i)  ) ;

                {
                    rc.link_geometry( "cube" ) ;
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
                
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( rc ) ;
                } ) ;

                _render_objects.emplace_back( std::move( rc ) ) ;
            }
        }

        //*****************************************************************************
        void_t init_post_process( void_t ) noexcept
        {
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
                
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                }) ;
                
                _geometries.emplace_back( std::move( geo ) ) ;
            }

            // framebuffer
            {
                for( size_t i=0; i<2; ++i )
                {
                    _fb[i] = natus::graphics::framebuffer_object_t( "post." + std::to_string(i) ) ;
                    _fb[i]->set_target( natus::graphics::color_target_type::rgba_uint_8, 1 )
                        .set_target( natus::graphics::depth_stencil_target_type::depth32 )
                        .resize( _fb_dims.z(), _fb_dims.w() ) ;
                
                }

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    for( size_t i=0; i<2; ++i ) a.configure( _fb[i] ) ;
                } ) ;
            }

            // post framebuffer render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "post" ) ;

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "post.color_grade" ) ;
                }

                // add variable set 
                {
                    {
                        natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                        {
                            {
                                auto* var = vars->texture_variable( "u_tex_0" ) ;
                                var->set( "post.0.0" ) ;
                                //var->set( "checker_board" ) ;
                            }

                            {
                                auto* var = vars->data_variable< natus::math::vec4f_t >( "u_dark_color" ) ;
                                var->set( natus::math::vec4f_t( _dark_color, 0.0f) ) ;
                            }

                            {
                                auto* var = vars->data_variable< natus::math::vec4f_t >( "u_light_color" ) ;
                                var->set( natus::math::vec4f_t(_light_color, 0.0f) ) ;
                            }

                            {
                                auto* var = vars->data_variable< uint_t >( "u_sobel_type" ) ;
                                var->set( 0 ) ;
                            }
                        }

                        rc.add_variable_set( std::move( vars ) ) ;
                    }
                }

                _rc_post = std::move( rc ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _rc_post ) ;
                } ) ;
            }

            // map framebuffer render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "map" ) ;

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "post.map" ) ;
                }

                // add variable set 
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    {
                        {
                            auto* var = vars->texture_variable( "u_tex_0" ) ;
                            var->set( "post.0.0" ) ;
                        }
                        {
                            auto* var = vars->texture_variable( "u_tex_1" ) ;
                            var->set( "post.1.0" ) ;
                        }
                        {
                            auto* var = vars->data_variable< float_t >( "u_split" ) ;
                            var->set( 0.5f ) ;
                        }
                    }

                    rc.add_variable_set( std::move( vars ) ) ;
                }

                _rc_map = std::move( rc ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _rc_map ) ;
                } ) ;
            }
        }

        //*****************************************************************************
        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::application::util::app_essentials_t::init_struct is = 
            {
                { "myapp" }, 
                { natus::io::path_t( DATAPATH ), "./working", "data" },
                { natus::io::location_t( "shaders.post_process.nsl" ) }
            } ;

            _ae.init( is ) ;
            
            {
                _ae.get_camera_0()->look_at( natus::math::vec3f_t( 0.0f, 600.0f, -500.0f ),
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

                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                    rss.clear_s.ss.clear_color = natus::math::vec4f_t( 0.2f, 0.2f, 0.2f, 1.0f ) ;
                   
                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = _fb_dims ;

                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }

            // post render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "post_render_states" ) ;

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
                    rss.clear_s.ss.do_activate = false ;
                    rss.clear_s.ss.do_color_clear = false ;
                    rss.clear_s.ss.do_depth_clear = false ;
                    rss.clear_s.ss.clear_color = natus::math::vec4f_t( 0.2f, 0.2f, 0.2f, 1.0f ) ;
                   
                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = _fb_dims ;

                    so.add_render_state_set( rss ) ;
                }

                _post_render_states = std::move( so ) ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _post_render_states ) ;
                } ) ;
            }

            this_t::init_cubes() ;
            this_t::init_post_process() ;

            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_audio( audio_data_in_t ) noexcept 
        { 
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            _ae.on_update( ud ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            // change properties of cubes
            {
                static float_t v = 0.0f ;
                v += 0.01f ;
                if( v > 1.0f ) v = 0.0f ;

                static __clock_t::time_point tp = __clock_t::now() ;

                size_t const num_object = _render_objects.size() ;

                float_t const dt = float_t ( double_t( std::chrono::duration_cast< std::chrono::milliseconds >( __clock_t::now() - tp ).count() ) / 1000.0 ) ;
                tp = __clock_t::now() ;

                // per frame update of variables
                for( auto & rc : _render_objects )
                {
                    rc->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
                    {
                        {
                            auto* var = vs->data_variable< natus::math::mat4f_t >( "view" ) ;
                            var->set( _ae.get_camera_0()->mat_view() ) ;
                        }

                        {
                            auto* var = vs->data_variable< natus::math::mat4f_t >( "proj" ) ;
                            var->set( _ae.get_camera_0()->mat_proj() ) ;
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
            }

            // render scene into framebuffer color target 0
            {
                {
                    _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.use( _fb[0] ) ;
                        a.push( _root_render_states ) ;
                    } ) ;
                }

                for( size_t i=0; i<_render_objects.size(); ++i )
                {
                    _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.render( _render_objects[i] ) ;
                    } ) ;
                }

                {
                    _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.pop( natus::graphics::backend::pop_type::render_state ) ;
                        a.unuse( natus::graphics::backend_t::unuse_type::framebuffer ) ;
                    } ) ;
                }
            }

            // render into framebuffer color target 0 for post
            {
                {
                    _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.use( _fb[1] ) ;
                        a.push( _post_render_states ) ;
                        {
                            natus::graphics::backend::render_detail_t rd ;
                            rd.varset = 0 ;
                            a.render( _rc_post, rd ) ;
                        }
                        a.pop( natus::graphics::backend::pop_type::render_state ) ;
                        a.unuse( natus::graphics::backend_t::unuse_type::framebuffer ) ;
                    } ) ;
                }
            }

            _ae.on_graphics_begin( rd ) ;
            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                {
                    natus::graphics::backend::render_detail_t rd ;
                    rd.varset = 0 ;
                    a.render( _rc_map, rd ) ;
                }
            } ) ;
            
            _ae.on_graphics_end( 100 ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            if( ImGui::Begin("Post Processing Control") )
            {
                static int item_current = 0;
                {
                    const char* items[] = { "color_grade", "box", "gauss", "sobel", "laplace" };
                    if( ImGui::ListBox("postprocess", &item_current, items, IM_ARRAYSIZE(items), 4) )
                    {
                        natus::ntd::string_t shader = "post.color_grade" ;

                        switch( item_current )
                        {
                        default:
                        case 0 : shader = "post.color_grade" ; break ;
                        case 1 : shader = "post.box" ; break ;
                        case 2 : shader = "post.gauss" ; break ;
                        case 3 : shader = "post.sobel" ; break ;
                        case 4 : shader = "post.laplace" ; break ;
                        }

                        _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                        {
                            _rc_post->link_shader( shader ) ;
                            a.configure( _rc_post ) ;
                        } ) ;
                    }
                }

                if( item_current == 0 )
                {
                    {
                        float_t col[3] = { _dark_color.x(),_dark_color.y(), _dark_color.z() };
                        ImGui::ColorEdit3( "Shadow", col ) ;
                        _dark_color = natus::math::vec3f_t( col[0], col[1], col[2]  ) ;
                    }
                    {
                        float_t col[3] = { _light_color.x(),_light_color.y(), _light_color.z() };
                        ImGui::ColorEdit3( "Bright", col ) ;
                        _light_color = natus::math::vec3f_t( col[0], col[1], col[2]  ) ;
                    }

                    // change post
                    {
                        _rc_post->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
                        {
                            {
                                auto* var = vs->data_variable< natus::math::vec4f_t >( "u_dark_color" ) ;
                                var->set( natus::math::vec4f_t(_dark_color, 0.0f) ) ;
                            }

                            {
                                auto* var = vs->data_variable< natus::math::vec4f_t >( "u_light_color" ) ;
                                var->set( natus::math::vec4f_t(_light_color, 0.0f) ) ;
                            }
                        } ) ;
                    }
                }
                else if( item_current == 3 )
                {
                    static int_t sobel_type = 0 ;
                    if( ImGui::SliderInt( "solbel type", &sobel_type, 0, 3 ) )
                    {
                        _rc_post->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
                        {
                            {
                                auto* var = vs->data_variable< uint_t >( "u_sobel_type" ) ;
                                var->set( uint_t( sobel_type ) ) ;
                            }
                        } ) ;
                    }
                }

                // options for mapping shader
                {
                    _rc_map->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
                    {
                        static float_t split = 0.5f ;
                        if( ImGui::SliderFloat( "split", &split, 0.0f, 1.0f ) )
                        {
                            auto* var = vs->data_variable< float_t >( "u_split" ) ;
                            var->set( split ) ;
                        }
                    } ) ;
                }
                
                ImGui::End() ;
            }

            return natus::application::result::ok ;
        }

        virtual natus::application::result on_shutdown( void_t ) noexcept 
        { 
            _ae.on_shutdown() ;
            return natus::application::result::ok ; 
        }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}
