
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
                    array[ 0 ].pos = natus::math::vec4f_t(1.0f,1.0f,0.0f,1.0f) * natus::math::vec4f_t( -0.5f, -0.5f, 0.0f, 1.0f ) ;
                    array[ 1 ].pos = natus::math::vec4f_t(1.0f,1.0f,0.0f,1.0f) *  natus::math::vec4f_t( -0.5f, +0.5f, 0.0f, 1.0f ) ;
                    array[ 2 ].pos = natus::math::vec4f_t(1.0f,1.0f,0.0f,1.0f) * natus::math::vec4f_t( +0.5f, +0.5f, 0.0f, 1.0f ) ;
                    array[ 3 ].pos = natus::math::vec4f_t(1.0f,1.0f,0.0f,1.0f) * natus::math::vec4f_t( +0.5f, -0.5f, 0.0f, 1.0f ) ;

                    array[ 0 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    array[ 1 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    array[ 2 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    array[ 3 ].color = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                } );

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "points",
                    natus::graphics::primitive_type::points, std::move( vb ) ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;
                _go = std::move( geo ) ;
            }

            // shader configuration
            {
                natus::ntd::string_t const nsl_shader = R"(
                    config render_original
                    {
                        vertex_shader
                        {
                            inout vec4_t pos : position ;
                            inout vec4_t color : color ;

                            mat4_t u_proj : projection ;
                            mat4_t u_view : view ;
                            mat4_t u_world : world ;
                            
                            void main()
                            {
                                out.pos = vec4_t( in.pos.xyz, 1.0 ) ;
                                out.pos = u_proj * u_view * u_world * out.pos ;
                                out.color = in.color ;
                            }
                        }

                        geometry_shader
                        {
                            in points ;
                            out triangles[ max_verts = 6 ] ;

                            inout vec4_t pos : position ;
                            inout vec4_t color : color ;

                            void main()
                            {
                                for( int i=0; i<in.length(); i++ )
                                {
                                    {
                                        out.pos = vec4_t( in[i].pos.xy + vec2_t( -10.5, -10.5 ), 0.0, in[i].pos.w ) ;
                                        out.color = in[i].color ;
                                        emit_vertex() ;
                                    }
                                    {
                                        out.pos =  vec4_t( in[i].pos.xy + vec2_t( -10.5, 10.5 ), 0.0, in[i].pos.w ) ;
                                        out.color = in[i].color ;
                                        emit_vertex() ;
                                    }
                                    {
                                        out.pos =  vec4_t( in[i].pos.xy + vec2_t( +10.5, +10.5 ), 0.0, in[i].pos.w ) ;
                                        out.color = in[i].color ;
                                        emit_vertex() ;
                                    }
                                    end_primitive() ;

                                    {
                                        out.pos =  vec4_t( in[i].pos.xy + vec2_t( -10.5, -10.5 ), 0.0, in[i].pos.w ) ;
                                        out.color = in[i].color ;
                                        emit_vertex() ;
                                    }
                                    {
                                        out.pos =  vec4_t( in[i].pos.xy + vec2_t( 10.5, 10.5 ), 0.0, in[i].pos.w ) ;
                                        out.color = in[i].color ;
                                        emit_vertex() ;
                                    }
                                    {
                                        out.pos =  vec4_t( in[i].pos.xy + vec2_t( 10.5, -10.5 ), 0.0, in[i].pos.w ) ;
                                        out.color = in[i].color ;
                                        emit_vertex() ;
                                    }
                                    end_primitive() ;
                                }
                            }
                        }

                        pixel_shader
                        {
                            inout vec4_t color : color ;

                            void main()
                            {
                                out.color = in.color ;
                            }
                        }
                    }
                )" ;

                _ae.process_shader( nsl_shader ) ;
            }


            // the original geometry render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "quad" ) ;

                {
                    rc.link_geometry( "points" ) ;
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

            //_ae.get_camera_0()->translate_by( natus::math::vec3f_t(0.0f,0.0f,-10.0f) ) ;

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
    return natus::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}






