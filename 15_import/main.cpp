
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/io/database.h>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        natus::graphics::render_state_sets_res_t _render_states = natus::graphics::render_state_sets_t() ;
        natus::graphics::render_object_res_t _rc = natus::graphics::render_object_t() ;
        natus::graphics::geometry_object_res_t _gconfig = natus::graphics::geometry_object_t() ;
        natus::graphics::image_object_res_t _imgconfig = natus::graphics::image_object_t() ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;
        
        typedef ::std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::io::database_res_t _db ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "A Render Window", wi/*, {natus::graphics::backend_type::gl3}*/ ) ;
            _db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = std::move( rhv._wid_async ) ;
            _camera_0 = std::move( rhv._camera_0 ) ;
            _gconfig = std::move( rhv._gconfig ) ;
            _rc = std::move( rhv._rc) ;
            _db = std::move( rhv._db ) ;
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual natus::application::result on_init( void_t )
        { 
            {
                _camera_0.orthographic( 2.0f, 2.0f, 1.0f, 1000.0f ) ;
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
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem = mod_reg->import_from( natus::io::location_t( "images.test.png" ), _db ) ;
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
            }

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
                            out vec4 out_color ;
                            in vec2 var_tx0 ;

                            uniform sampler2D u_tex ;
                        
                            void main()
                            {    
                                out_color = texture( u_tex, var_tx0 ) ;
                            } )" ) ) ;
                    
                    sc.insert( natus::graphics::backend_type::gl3, ::std::move(ss) ) ;
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
                            out vec4 out_color ;
                            in vec2 var_tx0 ;

                            uniform sampler2D u_tex ;
                        
                            void main()
                            {    
                                out_color = texture( u_tex, var_tx0 ) ;
                            } )" ) ) ;

                    sc.insert( natus::graphics::backend_type::es3, ::std::move(ss) ) ;
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
                                VS_OUTPUT output = (VS_OUTPUT)0 ;
                                //output.Pos = mul( Pos, World ) ;
                                output.pos = mul( in_pos, u_view ) ;
                                output.pos = mul( output.pos, u_proj ) ;
                                output.tx = in_tx ;
                                return output;
                            } )" ) ).

                        set_pixel_shader( natus::graphics::shader_t( R"(
                            // texture and sampler needs to be on the same slot.
                            
                            Texture2D u_tex : register( t0 );
                            SamplerState smp_u_tex : register( s0 );
                            
                            struct VS_OUTPUT
                            {
                                float4 Pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            };

                            float4 PS( VS_OUTPUT input ) : SV_Target
                            {
                                return u_tex.Sample( smp_u_tex, input.tx ) ;
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
                _rc->link_geometry( "quad" ) ;
                _rc->link_shader( "quad" ) ;
            }

            // add variable set 0
            {
                natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                {
                    auto * var = vars->texture_variable( "u_tex" ) ;
                    var->set( "loaded_image" ) ;
                }

                _rc->add_variable_set( ::std::move( vars ) ) ;
            }

            _wid_async.async().configure( _rc ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) 
        { 
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) 
        { 
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
            } ) ;

            {
                natus::graphics::backend_t::render_detail_t detail ;
                detail.start = 0 ;
                detail.render_states = _render_states ;
                _wid_async.async().render( _rc, detail ) ;
            }
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
