
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/profiling/macros.h>

#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>
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

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
            _wid_async2 = this_t::create_window( "A Render Window", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 }) ;

            _wid_async.window().position( 50, 50 ) ;
            _wid_async.window().resize( 800, 800 ) ;
            _wid_async2.window().position( 50 + 800, 50 ) ;
            _wid_async2.window().resize( 800, 800 ) ;
            #else
            _wid_async = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::es3, natus::graphics::backend_type::d3d11 } ) ;
            #endif
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
                    rss.polygon_s.ss.ff = natus::graphics::front_face::clock_wise ;
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

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "cube",
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
                    array[ 1 ] = 2 ;
                    array[ 2 ] = 1 ;

                    array[ 3 ] = 0 ;
                    array[ 4 ] = 3 ;
                    array[ 5 ] = 2 ;
                } ) ;

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "quad",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _wid_async.async().configure( geo ) ;
                _wid_async2.async().configure( geo ) ;
                _geometries.emplace_back( std::move( geo ) ) ;
            }

            // image configuration
            {
                natus::graphics::image_t img = natus::graphics::image_t( natus::graphics::image_t::dims_t( 100, 100 ) )
                    .update( [&]( natus::graphics::image_ptr_t, natus::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                {
                    typedef natus::math::vector4< uint8_t > rgba_t ;
                    auto* data = reinterpret_cast< rgba_t * >( data_in ) ;

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

                _imgconfig = natus::graphics::image_object_t( "checker_board", ::std::move( img ) )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                    .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                    .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );

                _wid_async.async().configure( _imgconfig ) ;
                _wid_async2.async().configure( _imgconfig ) ;
            }

            
            // shader configuration
            {
                natus::graphics::shader_object_t sc( "just_render" ) ;

                // shaders : ogl 3.0
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec3 in_pos ;
                            in vec3 in_nrm ;
                            in vec2 in_tx ;
                            out vec3 var_nrm ;
                            out vec2 var_tx0 ;
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;

                            void main()
                            {
                                vec3 pos = in_pos ;
                                pos.xyz *= 10.0 ;
                                var_tx0 = in_tx ;
                                gl_Position = u_proj * u_view * u_world * vec4( pos, 1.0 ) ;
                                var_nrm = normalize( u_world * vec4( in_nrm, 0.0 ) ).xyz ;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 140
                            #extension GL_ARB_separate_shader_objects : enable
                            #extension GL_ARB_explicit_attrib_location : enable
                            in vec2 var_tx0 ;
                            in vec3 var_nrm ;
                            layout(location = 0 ) out vec4 out_color0 ;
                            layout(location = 1 ) out vec4 out_color1 ;
                            layout(location = 2 ) out vec4 out_color2 ;
                            uniform sampler2D u_tex ;
                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color0 = u_color * texture( u_tex, var_tx0 ) ;
                                out_color1 = vec4( var_nrm, 1.0 ) ;
                                out_color2 = vec4( 
                                    vec3( dot( normalize( var_nrm ), normalize( vec3( 1.0, 1.0, 0.5) ) ) ), 
                                    1.0 ) ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::backend_type::gl3, std::move( ss ) ) ;
                }

                // shaders : es 3.0
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            in vec3 in_pos ;
                            in vec3 in_nrm ;
                            in vec2 in_tx ;
                            out vec3 var_nrm ;
                            out vec2 var_tx0 ;
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;

                            void main()
                            {
                                var_tx0 = in_tx ;
                                vec3 pos = in_pos ;
                                pos.xyz *= 10.0 ;
                                gl_Position = u_proj * u_view * u_world * vec4( pos, 1.0 ) ;
                                var_nrm = normalize( u_world * vec4( in_nrm, 0.0 ) ).xyz ;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump float ;
                            in vec2 var_tx0 ;
                            in vec3 var_nrm ;
                            layout(location = 0 ) out vec4 out_color0 ;
                            layout(location = 1 ) out vec4 out_color1 ;
                            layout(location = 2 ) out vec4 out_color2 ;
                            uniform sampler2D u_tex ;
                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color0 = u_color * texture( u_tex, var_tx0 ) ;
                                out_color1 = vec4( var_nrm, 1.0 ) ;
                                out_color2 = vec4( 
                                    vec3( dot( normalize( var_nrm ), normalize( vec3( 1.0, 1.0, 0.5) ) ) ), 
                                    1.0 ) ;
                            })" ) ) ;

                    sc.insert( natus::graphics::backend_type::es3, std::move( ss ) ) ;
                }

                // shaders : hlsl 11(5.0)
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            cbuffer ConstantBuffer : register( b0 ) 
                            {
                                matrix u_proj ;
                                matrix u_view ;
                                matrix u_world ;
                            }

                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION;
                                float3 nrm : NORMAL;
                                float2 tx : TEXCOORD0;
                            };

                            VS_OUTPUT VS( float4 in_pos : POSITION, float3 in_nrm : NORMAL, float2 in_tx : TEXCOORD0 )
                            {
                                VS_OUTPUT output = (VS_OUTPUT)0;                                
                                //output.Pos = mul( Pos, World );
                                float4 pos = in_pos * float4( 10.0, 10.0, 10.0, 1.0 ) ;
                                output.pos = mul( pos, u_world );
                                output.pos = mul( output.pos, u_view );
                                output.pos = mul( output.pos, u_proj );
                                output.tx = in_tx ;
                                output.nrm = mul( float4( in_nrm, 0.0 ), u_world ) ;
                                return output;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            // texture and sampler needs to be on the same slot.
                            
                            Texture2D u_tex : register( t0 );
                            SamplerState smp_u_tex : register( s0 );
                            
                            float4 u_color ;

                            struct VS_OUTPUT
                            {
                                float4 Pos : SV_POSITION;
                                float3 nrm : NORMAL;
                                float2 tx : TEXCOORD0;
                            };

                            struct MRT_OUT 
                            {
                                float4 color1 : SV_Target0 ;
                                float4 color2 : SV_Target1 ;
                                float4 color3 : SV_Target2 ;
                            } ;

                            MRT_OUT PS( VS_OUTPUT input )
                            {
                                MRT_OUT o = (MRT_OUT)0;
                                o.color1 = u_tex.Sample( smp_u_tex, input.tx ) * u_color ;
                                o.color2 = float4( input.nrm, 1.0f ) ;
                                o.color3 = dot( normalize( input.nrm ), normalize( float3( 1.0f, 1.0f, 1.0f ) ) ) ;
                                return o ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::backend_type::d3d11, std::move( ss ) ) ;
                }

                // configure more details
                {
                    sc
                        .add_vertex_input_binding( natus::graphics::vertex_attribute::position, "in_pos" )
                        .add_vertex_input_binding( natus::graphics::vertex_attribute::normal, "in_nrm" )
                        .add_vertex_input_binding( natus::graphics::vertex_attribute::texcoord0, "in_tx" )
                        .add_input_binding( natus::graphics::binding_point::view_matrix, "u_view" )
                        .add_input_binding( natus::graphics::binding_point::projection_matrix, "u_proj" ) ;
                }

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
                    rc.link_geometry( "cube" ) ;
                    rc.link_shader( "just_render" ) ;
                }

                // add variable set 
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    {
                        auto* var = vars->data_variable< natus::math::vec4f_t >( "u_color" ) ;
                        var->set( natus::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                    }

                    {
                        auto* var = vars->data_variable< float_t >( "u_time" ) ;
                        var->set( 0.0f ) ;
                    }

                    {
                        auto* var = vars->texture_variable( "u_tex" ) ;
                        var->set( "checker_board" ) ;
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

                        auto* var = vars->data_variable< natus::math::mat4f_t >( "u_world" ) ;
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

                // shader configuration
                {
                    natus::graphics::shader_object_t sc( "post_blit" ) ;

                    // shaders : ogl 3.0
                    {
                        natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                            set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec3 in_pos ;
                            in vec2 in_tx ;
                            out vec2 var_tx ;
                            
                            void main()
                            {
                                var_tx = in_tx ;
                                gl_Position = vec4( sign( in_pos ), 1.0 ) ;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec2 var_tx ;
                            out vec4 out_color ;
                            uniform sampler2D u_tex_0 ;
                            uniform sampler2D u_tex_1 ;
                            uniform sampler2D u_tex_2 ;
                            uniform sampler2D u_tex_3 ;
                        
                            void main()
                            {   
                                out_color = vec4(0.5,0.5,0.5,1.0) ;

                                if( var_tx.x < 0.5 && var_tx.y > 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.0, 0.5 ) ) * 2.0 ; 
                                    out_color = texture( u_tex_0, tx ) ; 
                                }
                                else if( var_tx.x > 0.5 && var_tx.y > 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.5, 0.5 ) ) * 2.0 ; 
                                    out_color = texture( u_tex_1, tx ) ; 
                                }
                                else if( var_tx.x > 0.5 && var_tx.y < 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.5, 0.0 ) ) * 2.0 ; 
                                    out_color = vec4( texture( u_tex_2, tx ).xyz, 1.0 ); 
                                }
                                else if( var_tx.x < 0.5 && var_tx.y < 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.0, 0.0 ) ) * 2.0 ; 
                                    out_color = vec4( vec3(pow( texture( u_tex_3, tx ).r,2.0)), 1.0 ); 
                                }
                            } )" ) ) ;

                        sc.insert( natus::graphics::backend_type::gl3, std::move( ss ) ) ;
                    }

                    // shaders : es 3.0
                    {
                        natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                            set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            in vec3 in_pos ;
                            in vec2 in_tx ;
                            out vec2 var_tx0 ;

                            void main()
                            {
                                var_tx0 = in_tx ;
                                gl_Position = vec4( sign(in_pos), 1.0 ) ;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump float ;
                            in vec2 var_tx0 ;
                            uniform sampler2D u_tex_0 ;
                            uniform sampler2D u_tex_1 ;
                            uniform sampler2D u_tex_2 ;
                            uniform sampler2D u_tex_3 ;
                        
                            void main()
                            {   
                                out_color = vec4(0.5,0.5,0.5,1.0) ;

                                if( var_tx.x < 0.5 && var_tx.y > 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.0, 0.5 ) ) * 2.0 ; 
                                    out_color = texture( u_tex_0, tx ) ; 
                                }
                                else if( var_tx.x > 0.5 && var_tx.y > 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.5, 0.5 ) ) * 2.0 ; 
                                    out_color = texture( u_tex_1, tx ) ; 
                                }
                                else if( var_tx.x > 0.5 && var_tx.y < 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.5, 0.0 ) ) * 2.0 ; 
                                    out_color = vec4( texture( u_tex_2, tx ).xyz, 1.0 ); 
                                }
                                else if( var_tx.x < 0.5 && var_tx.y < 0.5 )
                                {
                                    vec2 tx = (var_tx - vec2( 0.0, 0.0 ) ) * 2.0 ; 
                                    out_color = vec4( vec3(pow( texture( u_tex_3, tx ).r,2.0)), 1.0 ); 
                                }
                            } )" ) ) ;

                        sc.insert( natus::graphics::backend_type::es3, std::move( ss ) ) ;
                    }

                    // shaders : hlsl 11(5.0)
                    {
                        natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                            set_vertex_shader( natus::graphics::shader_t( R"(
                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            };

                            VS_OUTPUT VS( float4 in_pos : POSITION, float2 in_tx : TEXCOORD0 )
                            {
                                VS_OUTPUT output = (VS_OUTPUT)0;
                                output.pos = float4( sign( in_pos.xy ), 0.0f, 1.0f ) ;
                                output.tx = in_tx ;
                                return output;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            // texture and sampler needs to be on the same slot.
                            
                            Texture2D u_tex_0 : register( t0 );
                            SamplerState smp_u_tex_0 : register( s0 );

                            Texture2D u_tex_1 : register( t1 );
                            SamplerState smp_u_tex_1 : register( s1 );

                            Texture2D u_tex_2 : register( t2 );
                            SamplerState smp_u_tex_2 : register( s2 );

                            Texture2D u_tex_3 : register( t3 );
                            SamplerState smp_u_tex_3 : register( s3 );

                            struct VS_OUTPUT
                            {
                                float4 Pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            };

                            float4 PS( VS_OUTPUT input ) : SV_Target
                            {
                                float4 color = float4(0.0f,0.0f,0.0f,1.0f) ;
                                float2 tx = float2( input.tx.x, input.tx.y ) ;
                                if( tx.x < 0.5f && tx.y > 0.5f )
                                {
                                    tx = ( tx - float2( 0.0f, 0.5f ) ) * 2.0f ;
                                    tx = float2( tx.x, 1.0 - tx.y ) ;
                                    color = u_tex_0.Sample( smp_u_tex_0, tx ) ;
                                }
                                else if( tx.x > 0.5f && tx.y > 0.5f )
                                {
                                    tx = ( tx - float2( 0.5f, 0.5f ) ) * 2.0f ;
                                    tx = float2( tx.x, 1.0 - tx.y ) ;
                                    color = u_tex_1.Sample( smp_u_tex_1, tx ) ;
                                }
                                else if( tx.x > 0.5f && tx.y < 0.5f )
                                {
                                    tx = ( tx - float2( 0.5f, 0.0f ) ) * 2.0f ;
                                    tx = float2( tx.x, 1.0 - tx.y ) ;
                                    color = u_tex_2.Sample( smp_u_tex_2, tx ) ;
                                }
                                else if( tx.x < 0.5f && tx.y < 0.5f )
                                {
                                    tx = ( tx - float2( 0.0f, 0.0f ) ) * 2.0f ;
                                    tx = float2( tx.x, 1.0 - tx.y ) ;
                                    color = u_tex_3.Sample( smp_u_tex_3, tx ).rrra ;
                                }
                                return color ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::backend_type::d3d11, std::move( ss ) ) ;
                    }

                    // configure more details
                    {
                        sc
                            .add_vertex_input_binding( natus::graphics::vertex_attribute::position, "in_pos" )
                            .add_vertex_input_binding( natus::graphics::vertex_attribute::texcoord0, "in_tx" ) ;
                    }

                    _wid_async.async().configure( sc ) ;
                    _wid_async2.async().configure( sc ) ;
                }

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

                //_camera_0.translate_by( tr ) ;
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
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "u_view" ) ;
                        var->set( _camera_0.mat_view() ) ;
                    }

                    {
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "u_proj" ) ;
                        var->set( _camera_0.mat_proj() ) ;
                    }

                    {
                        auto* var = vs->data_variable< natus::math::vec4f_t >( "u_color" ) ;
                        var->set( natus::math::vec4f_t( v, 0.0f, 1.0f, 0.5f ) ) ;
                    }

                    {
                        static float_t  angle = 0.0f ;
                        angle = ( ( (dt/10.0f)  ) * 2.0f * natus::math::constants<float_t>::pi() ) ;
                        if( angle > 2.0f * natus::math::constants<float_t>::pi() ) angle = 0.0f ;
                        
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "u_world" ) ;
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
