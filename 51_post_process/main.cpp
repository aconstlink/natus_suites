

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
        natus::ntd::vector< natus::graphics::geometry_object_res_t > _geometries ;
        natus::ntd::vector< natus::graphics::render_object_res_t > _render_objects ;

    public:

        test_app( void_t ) noexcept 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            auto view1 = this_t::create_window( "A Render Window Default", wi ) ;
            auto view2 = this_t::create_window( "A Render Window Additional", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _ae = natus::application::util::app_essentials_t( 
                natus::graphics::async_views_t( { view1.async(), view2.async() } ) ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
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

        //*****************************************************************************
        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::application::util::app_essentials_t::init_struct is = 
            {
                { "myapp" }, 
                { natus::io::path_t( DATAPATH ), "./working", "data" }
            } ;

            _ae.init( is ) ;
            
            {
                _ae.get_camera_0()->look_at( natus::math::vec3f_t( 0.0f, 60.0f, -50.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
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
                            out vec4_t color1 : color1 ;
                            out vec4_t color2 : color2 ;

                            void main()
                            {
                                float_t light = dot( normalize( in.nrm ), normalize( vec3_t( 1.0, 1.0, 0.5) ) ) ;
                                out.color0 = color ' light ;
                                out.color1 = vec4_t( in.nrm, 1.0 ) ;
                                out.color2 = vec4_t( light, light, light , 1.0 ) ;
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
            NATUS_PROFILING_COUNTER_HERE( "Audio Clock" ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
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
            _ae.on_graphics_begin( rd ) ;
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

            tp = __clock_t::now() ;

            {
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    //a.use(_fb) ;
                    //a.push( _root_render_states ) ;
                } ) ;
            }

            

            for( size_t i=0; i<_render_objects.size(); ++i )
            {
                natus::graphics::backend_t::render_detail_t detail ;
                detail.start = 0 ;
                //detail.num_elems = 3 ;
                detail.varset = 0 ;
                //detail.render_states = _render_states ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.render( _render_objects[i], detail ) ;
                } ) ;
            }

            {
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    //a.use(_fb) ;
                    //a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
            }

            // un-use the framebuffer
            {
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.unuse( natus::graphics::backend_t::unuse_type::framebuffer ) ;
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
                
            }

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
            _ae.on_graphics_end( 100 ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;
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
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}
