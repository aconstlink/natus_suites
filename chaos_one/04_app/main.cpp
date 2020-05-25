
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
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
        natus::gpu::async_id_res_t _geo = natus::gpu::async_id_t() ;
        natus::gpu::async_id_res_t _rconfig = natus::gpu::async_id_t() ;
        natus::gpu::variable_set_res_t _vars = natus::gpu::variable_set_t() ;

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
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual natus::application::result on_init( void_t )
        { 
            // geometry configuration
            {
                struct vertex { natus::math::vec3f_t pos ; } ;

                auto vb = natus::gpu::vertex_buffer_t().add_layout_element( natus::gpu::vertex_attribute::position,
                    natus::gpu::type::tfloat, natus::gpu::type_struct::vec3 ).resize( 4 )
                    .update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    // fill vertex buffer
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

                natus::gpu::geometry_configuration_t config( "quad",
                    natus::gpu::primitive_type::triangles,
                    ::std::move( vb ), ::std::move( ib ) ) ;

                _wid_async.second.configure( _geo, ::std::move( config ) ) ;
            }

            // render configuration
            {
                natus::gpu::render_configuration rc( "quad_a" ) ;

                {
                    auto vs = natus::gpu::vertex_shader_t( R"(
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
                        } )" )
                        .add_vertex_binding( natus::gpu::vertex_attribute::position, "in_pos" )
                        .add_input_binding( natus::gpu::binding_point::view_matrix, "u_view" )
                        .add_input_binding( natus::gpu::binding_point::projection_matrix, "u_proj" ) ;

                    rc.set_shader( vs ) ;
                }

                {
                    natus::gpu::pixel_shader_t ps( R"(
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
                        }
                         )" ) ;
                    rc.set_shader( ps ) ;
                }
                rc.set_geometry( "quad" ) ;
                natus::gpu::render_configurations_t rcs( natus::gpu::backend_type::gl3, 
                    natus::gpu::render_configuration_res_t( ::std::move(rc) ) ) ;

                _wid_async.second.configure( _rconfig, rcs ) ;

                // test second configure, meaning reconfigure
                _wid_async.second.configure( _rconfig, rcs ) ;
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

        virtual natus::application::result on_update( void_t ) 
        { 
            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 10 ) ) ;
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_render( void_t ) 
        { 
            {
                static float_t value = 0.0f ;
                if( value > 1.0f ) value = 0.0f ;
                value += 0.005f ;

                auto* var = _vars->data_variable< natus::math::vec4f_t >( "u_color" ) ;
                var->set( natus::math::vec4f_t( value , 1.0f-value, value*0.5f, 1.0f ) ) ;
            }

            {
                _wid_async.second.render( _rconfig ) ;
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
    natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;

    return 0 ;
}
