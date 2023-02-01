
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
        
        natus::graphics::array_object_res_t _gpu_data ;

        natus::graphics::state_object_res_t _root_render_states ;

        // will capture the streamed out data
        natus::graphics::streamout_object_res_t _soo ;

        // render object for doing stream out
        natus::graphics::render_object_res_t _ro_so ;

        // render object for rendering original geometry
        natus::graphics::render_object_res_t _ro ;

        // render object for rendering streamed out geometry
        natus::graphics::render_object_res_t _ro_sec ;

        natus::graphics::geometry_object_res_t _go ;
        natus::graphics::geometry_object_res_t _go_pts ;

        natus::graphics::variable_set_res_t _vs0 ;

        struct render_vertex { natus::math::vec4f_t pos ; natus::math::vec4f_t color ; } ;
        struct sim_vertex { natus::math::vec4f_t pos ; natus::math::vec4f_t vel ; natus::math::vec4f_t accel; natus::math::vec4f_t force ;} ;

        natus::math::vec4f_t _particle_bounds = natus::math::vec4f_t( -1000.0f, -1000.0f, 1000.0f, 1000.0f ) ;
        size_t _num_particles = 1000 ;

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
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .resize( 4 * _num_particles ).update<render_vertex>( [=] ( render_vertex* array, size_t const ne )
                {
                    for( size_t i=0; i<_num_particles; ++i )
                    {
                        size_t const idx = i * 4 ;
                        array[ idx + 0 ].pos = natus::math::vec4f_t( -0.5f, -0.5f, 0.0f, 1.0f ) ;
                        array[ idx + 1 ].pos = natus::math::vec4f_t( -0.5f, +0.5f, 0.0f, 1.0f ) ;
                        array[ idx + 2 ].pos = natus::math::vec4f_t( +0.5f, +0.5f, 0.0f, 1.0f ) ;
                        array[ idx + 3 ].pos = natus::math::vec4f_t( +0.5f, -0.5f, 0.0f, 1.0f ) ;

                        array[ idx + 0 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ idx + 1 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ idx + 2 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ idx + 3 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    }
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).resize( 6 * _num_particles ).
                    update<uint_t>( [&] ( uint_t* array, size_t const ne )
                {
                    for( uint_t i=0; i< uint_t(_num_particles); ++i )
                    {
                        uint_t const vidx = i * 4 ;
                        uint_t const idx = i * 6 ;

                        array[ idx + 0 ] = vidx + 0 ;
                        array[ idx + 1 ] = vidx + 1 ;
                        array[ idx + 2 ] = vidx + 2 ;

                        array[ idx + 3 ] = vidx + 0 ;
                        array[ idx + 4 ] = vidx + 2 ;
                        array[ idx + 5 ] = vidx + 3 ;
                    }
                } ) ;

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "quad",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;
                _go = std::move( geo ) ;
            }

            // geometry configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color1, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color2, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .resize(_num_particles ).update<sim_vertex>( [=] ( sim_vertex* array, size_t const ne )
                {
                    for( size_t i=0; i<_num_particles; ++i )
                    {
                        float_t const mass = float_t((i%10)+1)/10.0f ;
                        array[ i ].pos = natus::math::vec4f_t( 0.0f, 0.0f, 0.0f, mass )  + natus::math::vec4f_t( _particle_bounds.x()*2.0f, 0.0f, 0.0f, 0.0f ) ;
                        array[ i ].vel = natus::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                        array[ i ].accel = natus::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                        array[ i ].force = natus::math::vec4f_t( 0.0f, -9.81f, 0.0f, 1.0f ) ;
                    }
                } );

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "points",
                    natus::graphics::primitive_type::points, std::move( vb ) ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;
                _go = std::move( geo ) ;
            }

            // stream out object configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color1, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color2, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 ) ;

                _soo = natus::graphics::streamout_object_t( "compute", std::move( vb ) ).resize( _num_particles ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a ) { a.configure( _soo ) ; } ) ;
            }

            // shader configuration
            {
                natus::ntd::string_t const nsl_shader = R"(
                    config render_original
                    {
                        vertex_shader
                        {
                            in vec4_t pos : position ;
                            in vec4_t color : color ;

                            out vec4_t pos : position ;
                            out vec4_t color : color ;

                            uint_t vid : vertex_id ;

                            mat4_t u_proj : projection ;
                            mat4_t u_view : view ;
                            mat4_t u_world : world ;

                            // will contain the transform feedback data
                            data_buffer_t u_data ;

                            void main()
                            {
                                int_t idx = vid / 4 ;
                                vec4_t pos = fetch_data( u_data, (idx << 2) + 0 ) ; 
                                //vec4_t vel = fetch_data( u_data, (idx << 2) + 1 ) ;

                                pos = vec4_t( (pos).xyz, 1.0 ) ;
                                out.color = in.color ;
                                out.pos = u_proj * u_view * ((u_world * in.pos)+ vec4_t(pos.xyz,0.0)) ;
                            }
                        }

                        pixel_shader
                        {
                            in vec4_t color : color ;
                            out vec4_t color : color ;

                            void main()
                            {
                                out.color = in.color ;
                            }
                        }
                    }
                )" ;

                _ae.process_shader( nsl_shader ) ;
            }

            // shader configuration for stream out
            {
                natus::ntd::string_t const nsl_shader = R"(
                    config stream_out
                    {
                        vertex_shader
                        {
                            inout vec4_t pos : position ;
                            inout vec4_t vel : color0 ;
                            inout vec4_t accel : color1 ;
                            inout vec4_t force : color2 ;
                            
                            uint_t vid : vertex_id ;

                            vec4_t u_bounds ;
                            float_t u_dt ;

                            void main()
                            {
                                float_t dt = u_dt ;
                                float_t mass = in.pos.w ;
                                vec3_t acl = in.force.xyz / mass + in.accel.xyz ;
                                vec3_t vel = acl * dt + in.vel.xyz ;
                                vec3_t pos = vel * dt + in.pos.xyz ;

                                if( any( less_than( pos, as_vec3( u_bounds.x ) ) ) || 
                                    any( greater_than( pos, as_vec3( u_bounds.z ) ) ) )
                                {
                                    pos = vec3_t( float_t(vid%30)*20-300.0, 200.0, 0.0 ) ;
                                    pos.y += float_t( (vid / 30) * 15 ) ;

                                    vel = as_vec3( 0.0 ) ;
                                    acl = as_vec3( 0.0 ) ;
                                }
                                
                                out.pos = vec4_t( pos, in.pos.w ) ;
                                out.vel = vec4_t( vel, 0.0 ) ;
                                out.accel = as_vec4(0.0) ; //vec4_t( acl, 0.0 ) ;
                                out.force = in.force ;
                            }
                        }

                        // 1. automatically enable streamout when no pixel shader is present.
                        // 2. the layout elements of the vertex buffer bound to the streamout object used 
                        //      MUST MATCH the number of output variables of this shader.
                    }
                )" ;

                // needs to handle 
                // - output binding
                // - feedback mode
                _ae.process_shader( nsl_shader ) ;
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
                    
                    // associate variable with streamout object
                    {
                        auto* var = vars->array_variable_streamout( "u_data" ) ;
                        var->set( "compute" ) ;
                    }
                    _vs0 = vars ;
                    rc.add_variable_set( std::move( vars ) ) ;
                }

                _ro = std::move( rc ) ;
            }

            // the stream out render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "stream_out" ) ;

                {
                    rc.link_geometry( "points", "compute" ) ;
                    rc.link_shader( "stream_out" ) ;
                }

                // add variable set
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    {}
                    rc.add_variable_set( std::move( vars ) ) ;
                }
                _ro_so = std::move( rc ) ;
            }
            
            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                a.configure( _ro ) ;
                a.configure( _ro_so ) ;
            } ) ;


            // do initial stream out pass for filling buffer
            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                a.use( _soo ) ;
                a.render( _ro_so ) ;
                a.unuse( natus::graphics::backend::unuse_type::streamout ) ;
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

            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                // do stream out
                a.use( _soo ) ;
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.feed_from_streamout = true ;
                    a.render( _ro_so, detail ) ;
                }
                a.unuse( natus::graphics::backend::unuse_type::streamout ) ;
            } ) ;

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
                    auto m = natus::math::mat4f_t(natus::math::with_identity()) * 10.0f ;
                    m[15] = 1.0f ;
                    auto * var = _vs0->data_variable<natus::math::mat4f_t>("u_world") ;
                    var->set( m ) ;
                }
            } ) ;
            
            _ro_so->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
            {
                {
                    auto * var = vs->data_variable<natus::math::float_t>("u_dt") ;
                    var->set( float_t( rd.sec_dt ) ) ;
                }
                {
                    auto * var = vs->data_variable<natus::math::vec4f_t>("u_bounds") ;
                    var->set( _particle_bounds ) ;
                }
            } ) ;

            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                #if 0
                // render original from geometry
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.feed_from_streamout = false ;
                    a.render( _ro, detail ) ;
                }
                #endif
                #if 1
                // render streamed out
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.feed_from_streamout = false ;
                    a.render( _ro, detail ) ;
                }
                #endif
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
    return natus::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}






