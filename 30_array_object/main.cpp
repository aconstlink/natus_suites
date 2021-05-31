
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/util/quad.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/graphics/variable/variable_set.hpp>

#include <natus/concurrent/parallel_for.hpp>
#include <natus/device/layouts/ascii_keyboard.hpp>
#include <natus/device/layouts/three_mouse.hpp>
#include <natus/device/global.h>

#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>

#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/3d/transformation.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>

#include <natus/profiling/macros.h>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::graphics::async_views_t _graphics ;
        
        natus::graphics::image_object_res_t _imgconfig = natus::graphics::image_object_t() ;
        natus::graphics::array_object_res_t _gpu_data = natus::graphics::array_object_t() ;

        natus::graphics::state_object_res_t _root_render_states ;
        natus::ntd::vector< natus::graphics::render_object_res_t > _render_objects ;
        natus::ntd::vector< natus::graphics::geometry_object_res_t > _geometries ;

        natus::graphics::framebuffer_object_res_t _fb = natus::graphics::framebuffer_object_t() ;
        natus::gfx::quad_res_t _quad ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec3f_t nrm ; natus::math::vec2f_t tx ; } ;
        struct vertex2 { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;
        
        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        natus::gfx::pinhole_camera_t _camera_0 ;

        int_t _max_objects = 0 ;
        int_t _num_objects_rnd = 0 ;

        natus::math::vec4ui_t _fb_dims = natus::math::vec4ui_t( 0, 0, 1280, 768 ) ;

        bool_t _do_tool = true ;

        natus::device::ascii_device_res_t _dev_ascii ;

    public:

        test_app( void_t ) 
        {
            srand (time(NULL));

            natus::application::app::window_info_t wi ;
            #if 1
            app::window_async_t wid_async = this_t::create_window( "A Render Window", wi ) ;
            app::window_async_t wid_async2 = this_t::create_window( "A Render Window", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 }) ;

            wid_async.window().position( 50, 50 ) ;
            wid_async.window().resize( 800, 800 ) ;
            wid_async2.window().position( 50 + 800, 50 ) ;
            wid_async2.window().resize( 800, 800 ) ;

            _graphics = natus::graphics::async_views_t( {wid_async.async(), wid_async2.async()} ) ;
            #elif 0
            app::window_async_t wid_async = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
            _graphics = natus::graphics::async_views_t( {wid_async.async()} ) ;
            #elif 1
            app::window_async_t wid_async = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::d3d11, natus::graphics::backend_type::gl3 } ) ;
            _graphics = natus::graphics::async_views_t( {wid_async.async()} ) ;
            #else
            app::window_async_t wid_async = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::es3, natus::graphics::backend_type::d3d11 } ) ;
            _graphics = natus::graphics::async_views_t( {wid_async.async()} ) ;
            #endif
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _graphics = std::move( rhv._graphics ) ;
            _camera_0 = std::move( rhv._camera_0 ) ;
            _geometries = std::move( rhv._geometries ) ;
            _render_objects = std::move( rhv._render_objects ) ;
            _fb = std::move( rhv._fb ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ),
                float_t(wei.w) / float_t(wei.h), 1.0f, 10000.0f ) ;

            return natus::application::result::ok ;
        }

    private:

        virtual natus::application::result on_init( void_t )
        { 
            {
                _camera_0.look_at( natus::math::vec3f_t( 2500.0f, 1000.0f, 1000.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }

            natus::device::global_t::system()->search( [&] ( natus::device::idevice_res_t dev_in )
            {
                if( natus::device::three_device_res_t::castable( dev_in ) )
                {
                }
                else if( natus::device::ascii_device_res_t::castable( dev_in ) )
                {
                    _dev_ascii = dev_in ;
                }
            } ) ;

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

            size_t const num_objects_x = 500 ;
            size_t const num_objects_y = 500 ;
            size_t const num_objects = num_objects_x * num_objects_y ;
            _max_objects = num_objects  ;
            _num_objects_rnd = int_t(std::min( size_t(40000), size_t(num_objects / 2) )) ;

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
                    .resize( ftm.get_num_vertices() * _max_objects ).update<vertex>( [&] ( vertex* array, size_t const ne )
                {
                    natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>(0, _max_objects),
                        [&]( natus::concurrent::range_1d<size_t> const & r )
                    {
                        for( size_t o=r.begin(); o<r.end(); ++o )
                        {
                            size_t const base = o * ftm.get_num_vertices() ;
                            for( size_t i=0; i<ftm.get_num_vertices(); ++i )
                            {
                                array[ base + i ].pos = ftm.get_vertex_position_3d( i ) ;
                                array[ base + i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                                array[ base + i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                            }
                        }
                    } ) ;
                    
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).
                    resize( ftm.indices.size() * _max_objects ).
                    update<uint_t>( [&] ( uint_t* array, size_t const ne )
                {
                    natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>(0, _max_objects),
                        [&]( natus::concurrent::range_1d<size_t> const & r )
                    {
                        for( size_t o=r.begin(); o<r.end(); ++o )
                        {
                            size_t const vbase = o * ftm.get_num_vertices() ;
                            size_t const ibase = o * ftm.indices.size() ;
                            for( size_t i = 0; i < ftm.indices.size(); ++i ) 
                            {
                                array[ ibase + i ] = ftm.indices[ i ] + vbase ;
                            }
                        }
                    } ) ;
                } ) ;

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "cubes",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;
                _geometries.emplace_back( std::move( geo ) ) ;
            }

            // array
            {
                struct the_data
                {
                    natus::math::vec4f_t pos ;
                    natus::math::vec4f_t col ;
                };

                float_t scale = 20.0f ;
                natus::graphics::data_buffer_t db = natus::graphics::data_buffer_t()
                    .add_layout_element( natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 ) ;

                _gpu_data = natus::graphics::array_object_t( "object_data", std::move( db ) ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _gpu_data ) ;
                } ) ;
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

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _imgconfig ) ;
                } ) ;
            }

            
            // shader configuration
            {
                natus::graphics::shader_object_t sc( "just_render" ) ;

                // shaders : ogl 3.1
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec3 in_pos ;
                            in vec3 in_nrm ;
                            in vec2 in_tx ;
                            out vec3 var_nrm ;
                            out vec2 var_tx0 ;
                            out vec4 var_col ;
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;
                            uniform samplerBuffer u_data ;

                            void main()
                            {
                                int idx = gl_VertexID / 24 ;
                                vec4 pos_scl = texelFetch( u_data, (idx *2) + 0 ) ;
                                vec4 col = texelFetch( u_data, (idx *2) + 1 ) ;
                                var_col = col ;
                                vec4 pos = vec4(in_pos * vec3( pos_scl.w ),1.0 )  ;
                                pos = u_world * pos + vec4(pos_scl.xyz*(pos_scl.w*2),0.0) ;
                                gl_Position = u_proj * u_view * pos ;

                                var_tx0 = in_tx ;
                                var_nrm = normalize( u_world * vec4( in_nrm, 0.0 ) ).xyz ;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 140
                            #extension GL_ARB_separate_shader_objects : enable
                            #extension GL_ARB_explicit_attrib_location : enable
                            in vec2 var_tx0 ;
                            in vec3 var_nrm ;
                            in vec4 var_col ;
                            layout(location = 0 ) out vec4 out_color0 ;
                            layout(location = 1 ) out vec4 out_color1 ;
                            layout(location = 2 ) out vec4 out_color2 ;
                            uniform sampler2D u_tex ;
                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color0 = u_color * texture( u_tex, var_tx0 ) * var_col ;
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
                            out vec4 var_col ;
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;
                            uniform sampler2D u_data ;

                            void main()
                            {
                                int idx = gl_VertexID / 24 ;
                                ivec2 wh = textureSize( u_data, 0 ) ;
                                vec4 pos_scl = texelFetch( u_data, 
                                     ivec2( ((idx*2) % wh.x), ((idx*2) / wh.x) ), 0 ) ;
                                var_col = texelFetch( u_data, 
                                     ivec2( (((idx*2)+1) % wh.x), (((idx*2)+1) / wh.x) ), 0 ) ;
                                var_tx0 = in_tx ;
                                vec4 pos = vec4(in_pos * vec3( pos_scl.w ),1.0 )  ;
                                pos = u_world * pos ;
                                pos += vec4(pos_scl.xyz*vec3(pos_scl.w*2.0f),0.0) ;
                                gl_Position = u_proj * u_view * pos ;
                                var_nrm = normalize( u_world * vec4( in_nrm, 0.0 ) ).xyz ;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump float ;
                            in vec2 var_tx0 ;
                            in vec3 var_nrm ;
                            in vec4 var_col ;
                            layout(location = 0 ) out vec4 out_color0 ;
                            layout(location = 1 ) out vec4 out_color1 ;
                            layout(location = 2 ) out vec4 out_color2 ;
                            uniform sampler2D u_tex ;
                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color0 = u_color * texture( u_tex, var_tx0 ) * var_col ;
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
                                float4x4 u_proj ;
                                float4x4 u_view ;
                                float4x4 u_world ;
                            }

                            struct VS_INPUT
                            {
                                uint in_id: SV_VertexID ;
                                float4 in_pos : POSITION ; 
                                float3 in_nrm : NORMAL ;
                                float2 in_tx : TEXCOORD0 ;
                            } ;
                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION;
                                float3 nrm : NORMAL;
                                float2 tx : TEXCOORD0;
                                float4 col : COLOR0;
                            };
                            
                            Buffer< float4 > u_data ;

                            VS_OUTPUT VS( VS_INPUT input )
                            {
                                VS_OUTPUT output = (VS_OUTPUT)0;
                                int idx = input.in_id / 24 ;
                                float4 pos_scl = u_data.Load( (idx * 2) + 0 ) ;
                                float4 col = u_data.Load( (idx * 2) + 1 ) ;
                                output.col = col ;
                                float4 pos = input.in_pos * float4( pos_scl.www, 1.0 ) ;
                                output.pos = mul( pos, u_world );
                                
                                output.pos = output.pos + float4( pos_scl.xyz*pos_scl.www*2.0f, 0.0f ) ;
                                //output.pos = output.pos * float4( pos_scl.www*2.0f, 1.0f ) ;
                                output.pos = mul( output.pos, u_view );
                                output.pos = mul( output.pos, u_proj );
                                output.tx = input.in_tx ;
                                output.nrm = mul( float4( input.in_nrm, 0.0 ), u_world ) ;
                                return output;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            // texture and sampler needs to be on the same slot.
                            
                            Texture2D u_tex : register( t0 );
                                SamplerState smp_u_tex : register( s0 );

                            cbuffer ConstantBuffer : register( b0 ) 
                            {
                                float4 u_color ;
                            }                            

                            

                            struct VS_OUTPUT
                            {
                                float4 Pos : SV_POSITION;
                                float3 nrm : NORMAL;
                                float2 tx : TEXCOORD0;
                                float4 col : COLOR0;
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
                                o.color1 = u_tex.Sample( smp_u_tex, input.tx ) * u_color * input.col ;
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

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( sc ) ;
                } ) ;
            }

            // the cubes render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "cubes" ) ;

                {
                    rc.link_geometry( "cubes" ) ;
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
                        auto* var = vars->array_variable( "u_data" ) ;
                        var->set( "object_data" ) ;
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
                _fb->set_target( natus::graphics::color_target_type::rgba_uint_8, 3 )
                    .set_target( natus::graphics::depth_stencil_target_type::depth32 )
                    .resize( _fb_dims.z(), _fb_dims.w() ) ;

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _fb ) ;
                } ) ;
            }

            {
                _quad = natus::gfx::quad_res_t( natus::gfx::quad_t("post.quad") ) ;
                _quad->set_texture("the_scene.0") ;
                _quad->init( _graphics ) ;
            }
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }
        
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ) noexcept
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

            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) 
        { 
            static float_t v = 0.0f ;
            v += 0.01f ;
            if( v > 1.0f ) v = 0.0f ;


            size_t const num_object = _render_objects.size() ;
            float_t const dt = rd.sec_dt ;

            {
                static float_t  angle_ = 0.0f ;
                angle_ += ( ( ((dt))  ) * 2.0f * natus::math::constants<float_t>::pi() ) / 1.0f ;
                if( angle_ > 4.0f * natus::math::constants<float_t>::pi() ) angle_ = 0.0f ;

                float_t s = 5.0f * std::sin(angle_) ;

                struct the_data
                {
                    natus::math::vec4f_t pos ;
                    natus::math::vec4f_t col ;
                };
                
                _gpu_data->data_buffer().resize( _num_objects_rnd ).
                update< the_data >( [&]( the_data * array, size_t const ne )
                {
                    size_t const w = 80 ;
                    size_t const h = 80 ;
                    
                    #if 0 // serial for
                    for( size_t e=0; e<std::min( size_t(_num_objects_rnd), ne ); ++e )
                    {
                        size_t const x = e % w ;
                        size_t const y = (e / w) % h ;
                        size_t const z = (e / w) / h ;

                        natus::math::vec4f_t pos(
                            float_t(x) - float_t(w/2),
                            float_t(z) - float_t(w/2),//10.0f * std::sin( y + angle_ ),
                            float_t( y ) - float_t(w/2),
                            30.0f
                        ) ;

                        array[e].pos = pos ;

                        float_t c = float_t( rand() % 255 ) /255.0f ;
                        array[e].col = natus::math::vec4f_t ( 0.1f, c, (c) , 1.0f ) ;
                    }

                    #else // parallel for

                    typedef natus::concurrent::range_1d<size_t> range_t ;
                    auto const & range = range_t( 0, std::min( size_t(_num_objects_rnd), ne ) ) ;

                    natus::concurrent::parallel_for<size_t>( range, [&]( range_t const & r )
                    {
                        for( size_t e=r.begin(); e<r.end(); ++e )
                        {
                            size_t const x = e % w ;
                            size_t const y = (e / w) % h ;
                            size_t const z = (e / w) / h ;

                            natus::math::vec4f_t pos(
                                float_t(x) - float_t(w/2),
                                float_t(z) - float_t(w/2),//10.0f * std::sin( y + angle_ ),
                                float_t( y ) - float_t(w/2),
                                30.0f
                            ) ;

                            array[e].pos = pos ;

                            float_t c = float_t( rand() % 255 ) /255.0f ;
                            array[e].col = natus::math::vec4f_t ( 0.1f, c, (c) , 1.0f ) ;
                        }
                    } ) ;

                    #endif

                } ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _gpu_data ) ;
                } ) ; 
            }

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
                        angle = ( ( (dt/1.0f)  ) * 2.0f * natus::math::constants<float_t>::pi() ) ;
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

             // use the framebuffer
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( _fb ) ;
                } ) ;
            }

            // render the root render state sets render object
            // this will set the root render states
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

           

            for( size_t i=0; i<_render_objects.size(); ++i )
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.start = 0 ;
                    detail.num_elems = _num_objects_rnd * 36 ;
                    detail.varset = 0 ;
                    //detail.render_states = _render_states ;
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

            _quad->render( _graphics ) ;
            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_device( device_data_in_t ) 
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

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui )
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "Config" ) ;

            if( ImGui::SliderInt( "Objects", &_num_objects_rnd, 0, _max_objects  ) )
            {
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






