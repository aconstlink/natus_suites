
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gpu/variable/variable_set.hpp>
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

        app::wid_async_t _wid_async ;
        
        natus::gpu::async_id_res_t _rconfig = natus::gpu::async_id_t() ;
        natus::gpu::variable_set_res_t _vars = natus::gpu::variable_set_t() ;
        
        struct vertex { natus::math::vec3f_t pos ; } ;
        natus::gpu::async_id_res_t _geo = natus::gpu::async_id_t() ;
        natus::gpu::geometry_configuration_res_t _gconfig ;

        typedef ::std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        natus::gfx::pinhole_camera_t _camera ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = ::std::move( rhv._wid_async ) ;
            _geo = ::std::move( rhv._geo ) ;
            _rconfig = ::std::move( rhv._rconfig ) ;
            _camera = ::std::move( rhv._camera ) ;
            _gconfig = ::std::move( rhv._gconfig ) ;
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual natus::application::result on_init( void_t )
        { 
            {
                _camera.orthographic( 2.0f, 2.0f, 1.0f, 1000.0f ) ;
            }

            // geometry configuration
            {
                auto vb = natus::gpu::vertex_buffer_t().add_layout_element( natus::gpu::vertex_attribute::position,
                    natus::gpu::type::tfloat, natus::gpu::type_struct::vec3 ).resize( 4 )
                    .update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = natus::math::vec3f_t( -0.5f, -0.5f, 0.0f ) ;
                    array[ 1 ].pos = natus::math::vec3f_t( -0.5f, +0.5f, 0.0f ) ;
                    array[ 2 ].pos = natus::math::vec3f_t( +0.5f, +0.5f, 0.0f ) ;
                    array[ 3 ].pos = natus::math::vec3f_t( +0.5f, -0.5f, 0.0f ) ;
                } );

                auto ib = natus::gpu::index_buffer_t().
                    set_layout_element( natus::gpu::type::tuint ).resize( 6 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 2 ;

                    array[ 3 ] = 0 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 3 ;
                } ) ;

                _gconfig = natus::gpu::geometry_configuration_t( "quad",
                    natus::gpu::primitive_type::triangles,
                    ::std::move( vb ), ::std::move( ib ) ) ;

                _wid_async.second.configure( _geo, _gconfig ) ;
            }

            {
                natus::gpu::render_configuration_t rc( "quad_a" ) ;

                // shaders : ogl 3.0
                {
                    natus::gpu::shader_set_t ss = natus::gpu::shader_set_t().

                        set_vertex_shader( natus::gpu::shader_t( R"(
                            #version 140
                            in vec3 in_pos ;
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;

                            //out vertex_data
                            //{
                            //    vec2 tx ;
                            //} vso ;

                            void main()
                            {
                                gl_Position = u_proj * u_view * vec4( in_pos, 1.0 ) ;
                            } )" ) ).

                        set_pixel_shader( natus::gpu::shader_t( R"(
                            #version 140
                            //layout( location = 0 ) out vec4 out_color ;
                            out vec4 out_color ;

                            //in vertex_data
                            //{
                            //  vec4 color ;
                            //} psi ;

                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color = u_color ;
                            } )" ) ) ;
                    
                    rc.insert( natus::gpu::backend_type::gl3, ::std::move(ss) ) ;
                }

                // shaders : es 3.0
                {
                    natus::gpu::shader_set_t ss = natus::gpu::shader_set_t().

                        set_vertex_shader( natus::gpu::shader_t( R"(
                            #version 300 es
                            in vec3 in_pos ;
                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;

                            //out vertex_data
                            //{
                            //    vec2 tx ;
                            //} vso ;

                            void main()
                            {
                                gl_Position = u_proj * u_view * vec4( in_pos, 1.0 ) ;
                            } )" ) ).
                        
                        set_pixel_shader( natus::gpu::shader_t( R"(
                            #version 300 es
                            precision mediump float ;
                            //layout( location = 0 ) out vec4 out_color ;
                            out vec4 out_color ;

                            //in vertex_data
                            //{
                            //  vec4 color ;
                            //} psi ;

                            uniform vec4 u_color ;
                        
                            void main()
                            {    
                                out_color = u_color ;
                            } )" ) ) ;

                    rc.insert( natus::gpu::backend_type::es3, ::std::move(ss) ) ;
                }

                // configure more details
                {
                    rc.add_vertex_input_binding( natus::gpu::vertex_attribute::position, "in_pos" )
                        .add_input_binding( natus::gpu::binding_point::view_matrix, "u_view" )
                        .add_input_binding( natus::gpu::binding_point::projection_matrix, "u_proj" ) ;

                    rc.link_geometry( "quad" ) ;
                }

                _wid_async.second.configure( _rconfig, rc ) ;
            }

            {
                natus::gpu::variable_set_res_t vars = natus::gpu::variable_set_t() ;
                {
                    auto* var = vars->data_variable< natus::math::vec4f_t >( "u_color" ) ;
                    var->set( natus::math::vec4f_t( 1.0f ) ) ;
                }

                {
                    auto* var = vars->data_variable< natus::math::vec4f_t >( "u_color" ) ;
                    var->set( natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ) ;
                }

                {
                    auto* var = vars->data_variable< float_t >( "u_time" ) ;
                    var->set( 0.0f ) ;
                }

                {
                    //vars->texture_variable( "tex_name", "shader_var_name" ) ;
                }                
                _wid_async.second.connect( _rconfig, vars ) ;

                _vars = ::std::move( vars ) ;
            }

            return natus::application::result::ok ; 
        }

        float value = 0.0f ;
        size_t ucount = 0 ;

        virtual natus::application::result on_update( void_t ) 
        { 
            
            auto const dif = ::std::chrono::duration_cast< ::std::chrono::microseconds >( __clock_t::now() - _tp ) ;
            _tp = __clock_t::now() ;


            float_t const dt = float_t( double_t( dif.count() ) / ::std::chrono::microseconds::period().den ) ;
            
            if( value > 1.0f ) value = 0.0f ;
            value += natus::math::fn<float_t>::fract( dt ) ;

            {
                static float_t t = 0.0f ;
                t += dt * 0.1f ;

                if( t > 1.0f ) t = 0.0f ;
                
                static natus::math::vec3f_t tr ;
                tr.x( 1.0f * natus::math::fn<float_t>::sin( t * 2.0f * 3.14f ) ) ;

                _camera.translate_by( tr ) ;
            }

            ucount++ ;

            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 1 ) ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_render( void_t ) 
        { 
            // per frame update 
            {
                auto* var = _vars->data_variable< natus::math::vec4f_t >( "u_color" ) ;
                var->set( natus::math::vec4f_t( value, 1.0f-value, ::std::abs( value*2.0f-1.0f ), 1.0f ) ) ;
            }

            // per frame update 
            {
                auto* var = _vars->data_variable< natus::math::mat4f_t >( "u_view" ) ;
                var->set( _camera.mat_view() ) ;
            }

            // per frame update 
            {
                auto* var = _vars->data_variable< natus::math::mat4f_t >( "u_proj" ) ;
                var->set( _camera.mat_proj() ) ;
            }

            // per frame update 
            {
                static float_t v = 0.0f ;
                v += 0.01f ;
                if( v > 1.0f ) v = 0.0f ;

                _gconfig->vertex_buffer().update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    float_t const x = natus::math::fn<float_t>::abs( natus::math::fn<float_t>::sin( v * 3.14 * 2.0f ) * 0.5 ) ;

                    array[ 0 ].pos = natus::math::vec3f_t( -x, -0.5f, 0.0f ) ;
                    array[ 1 ].pos = natus::math::vec3f_t( -0.5f, +0.5f, 0.0f ) ;
                    array[ 2 ].pos = natus::math::vec3f_t( +x, +0.5f, 0.0f ) ;
                    array[ 3 ].pos = natus::math::vec3f_t( +0.5f, -0.5f, 0.0f ) ;
                } );
                _wid_async.second.update( _geo, _gconfig ) ;
            }

            _wid_async.second.render( _rconfig ) ;

            // print calls from run-time per second
            {
                static __clock_t::time_point tp = __clock_t::now() ;

                auto const dif = ::std::chrono::duration_cast< ::std::chrono::microseconds >( __clock_t::now() - tp ) ;
                tp = __clock_t::now() ;

                float_t const sec = float_t( double_t( dif.count() ) / ::std::chrono::microseconds::period().den ) ;

                static size_t count = 0 ;
                count++ ;
                static float this_sec = 0.0f ;
                this_sec += sec ;
                if( this_sec > 1.0 )
                {
                    natus::log::global_t::status( "Update Count: " + ::std::to_string( ucount ) ) ;
                    natus::log::global_t::status( "Render Count: " + ::std::to_string( count ) ) ;
                    this_sec = 0.0f ;
                    count = 0 ;
                    ucount = 0 ;
                }
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_shutdown( void_t ) 
        { return natus::application::result::ok ; }
    };
    natus_soil_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}
