
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/format/module_registry.hpp>
#include <natus/format/future_items.hpp>
#include <natus/format/global.h>

#include <natus/profile/macros.h>

#include <random>
#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;
        
        natus::graphics::array_object_res_t _gpu_data = natus::graphics::array_object_t() ;

        natus::graphics::state_object_res_t _root_render_states ;

        natus::graphics::render_object_res_t _ro ;
        natus::graphics::geometry_object_res_t _go ;

        natus::graphics::variable_set_res_t _vs0 ;
        natus::graphics::variable_set_res_t _vs1 ;

        struct vertex { natus::math::vec3f_t pos ; } ;
        
        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;


        int_t _max_textures = 3 ;
        int_t _used_texture = 0 ;


    public:

        test_app( void_t ) 
        {
            srand( uint_t( time(NULL) ) ) ;

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
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
            _gpu_data = std::move( rhv._gpu_data ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;

            return natus::application::result::ok ;
        }

    private:

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            {
                

                natus::application::util::app_essentials_t::init_struct is = 
                {
                    { "myapp" }, 
                    { natus::io::path_t( DATAPATH ), "./working", "data" }
                } ;

                _ae.init( is ) ;
            }

            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "root_render_states" ) ;

                {
                    natus::graphics::render_state_sets_t rss ;

                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = true ;

                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = false ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = natus::graphics::fill_mode::fill ;

                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                   
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }

            // geometry configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = natus::math::vec3f_t( -0.5f, -0.5f, 0.0f ) ;
                    array[ 1 ].pos = natus::math::vec3f_t( -0.5f, +0.5f, 0.0f ) ;
                    array[ 2 ].pos = natus::math::vec3f_t( +0.5f, +0.5f, 0.0f ) ;
                    array[ 3 ].pos = natus::math::vec3f_t( +0.5f, -0.5f, 0.0f ) ;
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
                } ) ;                
                _go = std::move( geo ) ;
            }

            // image configuration
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                natus::format::future_item_t items[4] = 
                {
                    mod_reg->import_from( natus::io::location_t( "images.1.png" ), _ae.db() ),
                    mod_reg->import_from( natus::io::location_t( "images.2.png" ), _ae.db() ),
                    mod_reg->import_from( natus::io::location_t( "images.3.png" ), _ae.db() ),
                    mod_reg->import_from( natus::io::location_t( "images.4.png" ), _ae.db() )
                } ;

                // taking all slices
                natus::graphics::image_t img ;

                // load each slice into the image
                for( size_t i=0; i<4; ++i )
                {
                    natus::format::image_item_res_t ii = items[i].get() ;
                    if( ii.is_valid() )
                    {
                        img.append( *ii->img ) ;
                    }
                }

                natus::graphics::image_object_res_t ires = natus::graphics::image_object_t( 
                    "image_array", std::move( img ) )
                    .set_type( natus::graphics::texture_type::texture_2d_array )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                    .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                    .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( ires ) ;
                } ) ;
            }
            
            // shader configuration
            {
                natus::graphics::shader_object_t sc( "test_variable_array" ) ;

                // shaders : ogl 3.1
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec3 in_pos ;
                            out vec2 var_tx ;

                            uniform int u_quad ; // in [0,1] left or right quad
                            
                            void main()
                            {
                                vec2 offset[2] = vec2[2]( vec2(-0.5, 0.0), vec2(0.5,0.0) ) ;
                                gl_Position = vec4( in_pos.xy * vec2(0.85) + offset[u_quad], 0.0, 1.0 ) ;
                                var_tx = sign( in_pos.xy ) * vec2( 0.5 ) + vec2( 0.5 ) ;

                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 140
                            #extension GL_ARB_separate_shader_objects : enable
                            #extension GL_ARB_explicit_attrib_location : enable
                            
                            in vec2 var_tx ;

                            layout(location = 0 ) out vec4 out_color ;

                            uniform sampler2DArray u_tex ;
                            
                            uniform int u_quad ; // in [0,1] left or right quad
                            uniform int u_texture ; // in [0,3] choosing the sampler in u_tex

                            void main()
                            {    
                                vec2 uv = fract( var_tx * 2.0 ) ;
                                int quadrant = int( dot( floor(var_tx*2.0), vec2(1,2) ) ) ;
                                int idx = u_quad * u_texture + quadrant * ( 1 - u_quad ) ;
                                //out_color = vec4( floor(var_tx*2.0), 0.0, 1.0 ) ;
                                out_color = texture( u_tex, vec3( uv, float(idx) ) ) ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::shader_api_type::glsl_1_4, std::move( ss ) ) ;
                }

                // shaders : es 3.0
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump int ;
                            in vec3 in_pos ;
                            out vec2 var_tx ;

                            uniform int u_quad ; // in [0,1] left or right quad

                            void main()
                            {
                                vec2 offset[2] = vec2[2]( vec2(-0.5, 0.0), vec2(0.5,0.0) ) ;
                                gl_Position = vec4( in_pos.xy * vec2(0.85) + offset[u_quad], 0.0, 1.0 ) ;
                                var_tx = sign( in_pos.xy ) * vec2( 0.5 ) + vec2( 0.5 ) ;

                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump int ;
                            precision mediump float ;
                            precision mediump sampler2DArray ;

                            in vec2 var_tx ;

                            out vec4 out_color ;

                            uniform sampler2DArray u_tex ;

                            uniform int u_quad ; // in [0,1] left or right quad
                            uniform int u_texture ; // in [0,3] choosing the sampler in u_tex

                            void main()
                            {
                                vec2 uv = fract( var_tx * 2.0 ) ;
                                int quadrant = int( dot( floor(var_tx*2.0), vec2(1,2) ) ) ;
                                int idx = u_quad * u_texture + quadrant * ( 1 - u_quad ) ;
                                //out_color = vec4( floor(var_tx*2.0), 0.0, 1.0 ) ;
                                out_color = texture( u_tex, vec3( uv, float(idx) ) ) ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::shader_api_type::glsles_3_0, std::move( ss ) ) ;
                }

                // shaders : hlsl 11(5.0)
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            cbuffer ConstantBuffer : register( b0 ) 
                            {
                                int u_quad ; // in [0,1] left or right quad
                            }

                            struct VS_INPUT
                            {
                                float4 in_pos : POSITION ; 
                            } ;

                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION ;
                                float2 tx : TEXCOORD0 ;
                            };

                            VS_OUTPUT VS( VS_INPUT input )
                            {
                                VS_OUTPUT output = (VS_OUTPUT)0 ;

                                float2 offset[2] = { float2(-0.5, 0.0), float2(0.5,0.0) };
                                float2 pos = input.in_pos.xy * float2(0.85, 0.85) + offset[u_quad] ;
                                output.pos = float4( pos, 0.0, 1.0 ) ;
                                output.tx = sign( input.in_pos.xy ) * float2(0.5,0.5) + float2(0.5,0.5);

                                return output;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            
                            Texture2DArray u_tex : register( t0 ) ;
                            SamplerState smp_u_tex : register( s0 ) ;

                            cbuffer ConstantBuffer : register( b0 ) 
                            {
                                int u_quad ; // in [0,1] left or right quad
                                int u_texture ; // in [0,3] choosing the sampler in u_tex
                            }

                            struct VS_OUTPUT
                            {
                                float4 Pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            } ;

                            float4 PS( VS_OUTPUT input ) : SV_Target0
                            {
                                float2 uv = frac( input.tx * 2.0 ) ;
                                int quadrant = int( dot( floor(input.tx*float2(2,2)), float2(1,2) ) ) ;
                                //return float4( uv, 0.0, 1.0 ) ; 
                                int idx = u_quad * u_texture + quadrant *(1-u_quad) ;
                                return u_tex.Sample( smp_u_tex, float3( uv, float(idx)) ) ;
                                //return float4( uv, 0.0, 1.0) ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::shader_api_type::hlsl_5_0, std::move( ss ) ) ;
                }

                // configure more details
                {
                    sc
                        .add_vertex_input_binding( natus::graphics::vertex_attribute::position, "in_pos" )
                        .add_input_binding( natus::graphics::binding_point::view_matrix, "u_view" )
                        .add_input_binding( natus::graphics::binding_point::projection_matrix, "u_proj" ) ;
                }

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( sc ) ;
                } ) ;
            }

            // the quad render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "quad" ) ;

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "test_variable_array" ) ;
                }

                // add variable set 0
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    
                    {
                        auto* var = vars->texture_variable( "u_tex" ) ;
                        var->set( "image_array" ) ;
                    }

                    {
                        auto * var = vars->data_variable<int32_t>("u_quad" ) ;
                        var->set( 0 ) ;
                    }

                    {
                        auto * var = vars->data_variable<int32_t>("u_texture" ) ;
                        var->set( 0 ) ;
                    }

                    _vs0 = vars ;
                    rc.add_variable_set( std::move( vars ) ) ;
                }

                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    
                    {
                        auto* var = vars->texture_variable( "u_tex" ) ;
                        var->set( "image_array" ) ;
                    }

                    {
                        auto * var = vars->data_variable<int32_t>("u_quad" ) ;
                        var->set( 1 ) ;
                    }

                    {
                        auto * var = vars->data_variable<int32_t>("u_texture" ) ;
                        var->set( 0 ) ;
                    }

                    _vs1 = vars ;
                    rc.add_variable_set( std::move( vars ) ) ;
                }
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( rc ) ;
                } ) ;
                _ro = std::move( rc ) ;
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
            _ae.on_update( ud ) ;
            //NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        { 
            _ae.on_graphics_begin( rd ) ;

            _ro->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
            {
                {
                    auto* var = vs->data_variable< int32_t >( "u_texture" ) ;
                    var->set( _used_texture ) ;
                }
            } ) ;

            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                // left quad
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    a.render( _ro, detail ) ;
                }
                // right quad
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.varset = 1 ;
                    a.render( _ro, detail ) ;
                }
            } ) ;

            _ae.on_graphics_end( 10 ) ;

            //NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Test Control" ) ;

            if( ImGui::SliderInt( "Use Texture", &_used_texture, 0, _max_textures ) ){} 

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
    return natus::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}






