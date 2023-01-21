
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

        natus::graphics::state_object_res_t _root_render_states ;

        natus::graphics::render_object_res_t _ro ;

        natus::graphics::geometry_object_res_t _go ;

        natus::graphics::variable_set_res_t _vs0 ;

        struct vertex { natus::math::vec4f_t pos ; natus::math::vec4f_t color ; } ;
        
    public:

        test_app( void_t ) noexcept
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
        test_app( this_rref_t rhv ) noexcept : app( ::std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
        }
        virtual ~test_app( void_t ) noexcept 
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
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = natus::math::vec4f_t( -0.5f, -0.5f, 0.0f, 1.0f ) ;
                    array[ 1 ].pos = natus::math::vec4f_t( -0.5f, +0.5f, 0.0f, 1.0f ) ;
                    array[ 2 ].pos = natus::math::vec4f_t( +0.5f, +0.5f, 0.0f, 1.0f ) ;
                    array[ 3 ].pos = natus::math::vec4f_t( +0.5f, -0.5f, 0.0f, 1.0f ) ;

                    array[ 0 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    array[ 1 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    array[ 2 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    array[ 3 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
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

            // shader configuration
            {
                natus::graphics::shader_object_t sc( "render_original" ) ;

                // shaders : ogl 3.1
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 400 core
                            in vec4 in_pos ;
                            in vec4 in_color ;

                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;
                            
                            out VS_GS_VERTEX
                            {
                                vec4 color ;
                            } vout ;

                            void main()
                            {
                                
                                (gl_Position) = u_proj * u_view * u_world * in_pos ;
                                vout.color = in_color ;

                            } )" ) ).

                        set_geometry_shader( natus::graphics::shader_t( R"(
                            #version 400 core
                            
                            layout( triangles ) in ;
                            layout( triangle_strip, max_vertices = 3 ) out ;

                            in VS_GS_VERTEX
                            {
                                vec4 color ;
                            } vin[] ;

                            out GS_FS_VERTEX
                            {
                                vec4 color ;
                            } vout ;

                            void main()
                            {
                                for( int i=0; i<gl_in.length(); ++i )
                                {
                                    vout.color = vin[i].color ;
                                    gl_Position = gl_in[i].gl_Position ;
                                    EmitVertex() ;
                                }
                                EndPrimitive() ;
                            } )" )).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 400 core
    
                            in GS_FS_VERTEX
                            {
                                vec4 color ;
                            } fsin ;

                            layout( location = 0 ) out vec4 out_color ;

                            void main()
                            {
                                out_color = fsin.color ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::shader_api_type::glsl_1_4, std::move( ss ) ) ;
                }

                // shaders : es 3.0
                {
                    natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                        set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            in vec4 in_pos ;
                            in vec4 in_color ;
                            out vec4 var_color ;

                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;
                            
                            void main()
                            {
                                var_color = in_color ;
                                gl_Position = u_proj * u_view * u_world * vec4( in_pos, 1.0 ) ;

                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump int ;
                            precision mediump float ;
                            precision mediump sampler2DArray ;

                            in vec4 var_color ;
                            out vec4 out_color ;

                            void main()
                            {
                                out_color = var_color ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::shader_api_type::glsles_3_0, std::move( ss ) ) ;
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
                                float4 in_pos : POSITION ; 
                                float4 in_color : COLOR ;
                            } ;

                            struct VSGS_DATA
                            {
                                float4 pos : SV_POSITION ;
                                float4 col : COLOR ;
                            };

                            VSGS_DATA VS( VS_INPUT input )
                            {
                                VSGS_DATA output = (VSGS_DATA)0 ;

                                output.pos = mul( input.in_pos, u_world ) ;
                                output.pos = mul( output.pos, u_view ) ;
                                output.pos = mul( output.pos, u_proj ) ;
                                output.col = input.in_color ;
                                return output;
                            } )" ) ).
                        
                        set_geometry_shader( natus::graphics::shader_t( R"(
                            
                            cbuffer ConstantBuffer : register( b0 ) 
                            {}

                            struct VSGS_DATA
                            {
                                float4 pos : SV_POSITION ;
                                float4 col : COLOR ;
                            };
                            
                            struct GSPS_DATA
                            {
                                float4 pos : SV_POSITION;
                                float4 col : COLOR ;
                            } ;

                            [maxvertexcount(3)]
                            void GS( triangle VSGS_DATA input[3], inout TriangleStream<GSPS_DATA> tri_stream ) 
                            {
                                GSPS_DATA output ;
                                for( int i=0; i<3; ++i )
                                {
                                    output.pos = input[i].pos ;
                                    output.col = input[i].col ;
                                    tri_stream.Append( output ) ;    
                                }
                                tri_stream.RestartStrip() ;
                            } )" ) ) .

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            
                            cbuffer ConstantBuffer : register( b0 ) 
                            {}

                            struct GSPS_DATA
                            {
                                float4 pos : SV_POSITION;
                                float4 col : COLOR ;
                            } ;

                            float4 PS( GSPS_DATA input ) : SV_Target0
                            {
                                return input.col ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::shader_api_type::hlsl_5_0, std::move( ss ) ) ;
                }

                // configure more details
                {
                    sc
                        .add_vertex_input_binding( natus::graphics::vertex_attribute::position, "in_pos" )
                        .add_vertex_input_binding( natus::graphics::vertex_attribute::color0, "in_color" )
                        .add_input_binding( natus::graphics::binding_point::world_matrix, "u_world" )
                        .add_input_binding( natus::graphics::binding_point::view_matrix, "u_view" )
                        .add_input_binding( natus::graphics::binding_point::projection_matrix, "u_proj" ) ;
                }

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( sc ) ;
                } ) ;
            }

            // the original geometry render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "quad" ) ;

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "render_original" ) ;
                }

                // add variable set 0
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    
                    {}
                    _vs0 = vars ;
                    rc.add_variable_set( std::move( vars ) ) ;
                }

                _ro = std::move( rc ) ;
            }
            
            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                a.configure( _ro ) ;
            } ) ;

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
                    auto * var = _vs0->data_variable<natus::math::mat4f_t>("u_view") ;
                    var->set( _ae.get_camera_0()->get_camera()->get_view_matrix() ) ;
                }
                {
                    auto * var = _vs0->data_variable<natus::math::mat4f_t>("u_proj") ;
                    var->set( _ae.get_camera_0()->get_camera()->get_proj_matrix() ) ;
                }
                {
                    auto m = natus::math::mat4f_t(natus::math::with_identity()) * 100.0f ;
                    m[15] = 1.0f ;
                    auto * var = _vs0->data_variable<natus::math::mat4f_t>("u_world") ;
                    var->set( m ) ;
                }
            } ) ;

            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                a.render( _ro ) ;
            } ) ;

            _ae.on_graphics_end( 10 ) ;

            //NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Test Control" ) ;

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






