
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/util/quad.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/profile/macros.h>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    // this test shows how to use the framebuffer
    // it uses a framebuffer with two outputs.
    // fb0 : checkerboard with texture color * user color
    // fb1 : checkerboard with texture color filtered red
    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        natus::graphics::render_object_res_t _rc = natus::graphics::render_object_t() ;
        natus::graphics::geometry_object_res_t _gconfig = natus::graphics::geometry_object_t() ;
        natus::graphics::image_object_res_t _imgconfig = natus::graphics::image_object_t() ;
        natus::graphics::framebuffer_object_res_t _fb = natus::graphics::framebuffer_object_t() ;

        natus::graphics::state_object_res_t _root_render_states ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;
        
        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::math::vec4ui_t _fb_dims = natus::math::vec4ui_t(0, 0, 800, 800) ;

        natus::gfx::quad_res_t _quad ;

        natus::math::vec2f_t _window_dims = natus::math::vec2f_t( 1.0f ) ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
            #else
            _wid_async = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
            #endif
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = std::move( rhv._wid_async ) ;
            _camera_0 = std::move( rhv._camera_0 ) ;
            _gconfig = std::move( rhv._gconfig ) ;
            _rc = std::move( rhv._rc) ;
            _fb = std::move( rhv._fb ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei ) noexcept
        {
            _window_dims = natus::math::vec2f_t( float_t(wei.w), float_t(wei.h) ) ;
            return natus::application::result::ok ;
        }

    private:

        //********************************************************************
        virtual natus::application::result on_init( void_t ) noexcept
        { 
            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "root_render_states" ) ;

                {
                    natus::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = false ;

                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = _fb_dims ;

                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _wid_async.async().configure( _root_render_states ) ;
            }

            // geometry configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
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

                _gconfig = natus::graphics::geometry_object_t( "quad",
                    natus::graphics::primitive_type::triangles, ::std::move( vb ), ::std::move( ib ) ) ;

                _wid_async.async().configure( _gconfig ) ;
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
                    .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest ) ;

                _wid_async.async().configure( _imgconfig ) ;
            }

            // the rendering effect
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "quad" ) ;

                // shader configuration
                {
                    natus::graphics::shader_object_t sc( "quad" ) ;

                    // shaders : ogl 3.0
                    {
                        natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                            set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec3 in_pos ;
                            in vec2 in_tx ;
                            out vec2 var_tx0 ;
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            
                            void main()
                            {
                                var_tx0 = in_tx ;
                                gl_Position = u_proj * u_view * vec4( in_pos, 1.0 ) ;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 140
                            #extension GL_ARB_separate_shader_objects : enable
                            #extension GL_ARB_explicit_attrib_location : enable
                            in vec2 var_tx0 ;
                            layout(location = 0 ) out vec4 out_color0 ;
                            layout(location = 1 ) out vec4 out_color1 ;
                            uniform sampler2D u_tex ;
                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color0 = u_color * texture( u_tex, var_tx0 ) ;
                                out_color1 = vec4(1.0,0.0,0.0,1.0) * texture( u_tex, var_tx0 ) ;
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
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;

                            void main()
                            {
                                var_tx0 = in_tx ;
                                gl_Position = u_proj * u_view * vec4( in_pos, 1.0 ) ;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump float ;
                            in vec2 var_tx0 ;
                            layout( location = 0 ) out vec4 out_color ;
                            layout( location = 1 ) out vec4 out_color1 ;
                            uniform sampler2D u_tex ;
                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color = u_color * texture( u_tex, var_tx0 ) ;
                                out_color1 = vec4(1.0,0.0,0.0,1.0) * texture( u_tex, var_tx0 ) ;
                            } )" ) ) ;

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
                            }

                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            };

                            VS_OUTPUT VS( float4 in_pos : POSITION, float2 in_tx : TEXCOORD0 )
                            {
                                VS_OUTPUT output = (VS_OUTPUT)0;                                
                                //output.Pos = mul( Pos, World );
                                output.pos = mul( in_pos, u_view );
                                output.pos = mul( output.pos, u_proj );
                                output.tx = in_tx ;
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
                                float2 tx : TEXCOORD0;
                            };

                            struct MRT_OUT 
                            {
                                float4 color1 : SV_Target0 ;
                                float4 color2 : SV_Target1 ;
                            } ;

                            MRT_OUT PS( VS_OUTPUT input )
                            {
                                MRT_OUT o = (MRT_OUT)0;
                                o.color1 = u_tex.Sample( smp_u_tex, input.tx ) * u_color ;
                                o.color2 = float4(1.0f,0.0f,0.0f,1.0f) * u_tex.Sample( smp_u_tex, input.tx ) ;
                                return o ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::backend_type::d3d11, std::move( ss ) ) ;
                    }

                    // configure more details
                    {
                        sc
                            .add_vertex_input_binding( natus::graphics::vertex_attribute::position, "in_pos" )
                            .add_vertex_input_binding( natus::graphics::vertex_attribute::texcoord0, "in_tx" )
                            .add_input_binding( natus::graphics::binding_point::view_matrix, "u_view" )
                            .add_input_binding( natus::graphics::binding_point::projection_matrix, "u_proj" ) ;
                    }

                    _wid_async.async().configure( sc ) ;
                }

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "quad" ) ;
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

                    rc.add_variable_set( std::move( vars ) ) ;
                }

                _rc = std::move( rc ) ;
                _wid_async.async().configure( _rc ) ;
            }


            // framebuffer
            {
                _fb = natus::graphics::framebuffer_object_t( "the_scene" ) ;
                _fb->set_target( natus::graphics::color_target_type::rgba_uint_8, 2 )
                    .resize( _fb_dims.z(), _fb_dims.w() ) ;

                _wid_async.async().configure( _fb ) ;
            }

            // prepare quad
            {
                _quad = natus::gfx::quad_res_t( natus::gfx::quad_t("post_map") ) ;
                
                natus::graphics::async_views_t graphics = natus::graphics::async_views_t({
                    _wid_async.async()}) ;
                _quad->init( graphics ) ;
                _quad->set_texture("the_scene.0") ;
                
            }
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        //********************************************************************
        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        //********************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) noexcept 
        { 
            static float_t v = 0.0f ;
            v += 0.01f ;
            if( v > 1.0f ) v = 0.0f ;

            // per frame update of variables
            _rc->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
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
            } ) ;

            // use the framebuffer
            {
                _wid_async.async().use( _fb ) ;
                _wid_async.async().push( _root_render_states ) ;
            }

            {
                natus::graphics::backend_t::render_detail_t detail ;
                detail.start = 0 ;
                detail.varset = 0 ;
                _wid_async.async().render( _rc, detail ) ;
            }

            // un-use the framebuffer
            {
                _wid_async.async().pop( natus::graphics::backend::pop_type::render_state ) ;
                _wid_async.async().unuse( natus::graphics::backend::unuse_type::framebuffer ) ;
            }

            {
                natus::graphics::async_views_t graphics = natus::graphics::async_views_t( {
                    _wid_async.async()}) ;
                _quad->render( graphics ) ;
            }

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        //********************************************************************
        bool_t _do_tool = true ;
        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t ) noexcept
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "Control and Info" ) ;

            {
                static int_t data = 0 ;
                if( ImGui::SliderInt( "Framebuffer", &data, 0, 1, "%i", 0 ) )
                {
                    _quad->set_texture( "the_scene." + std::to_string( data ) ) ;
                }
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
