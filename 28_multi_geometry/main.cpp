
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
        
        natus::graphics::render_object_res_t _ro ;

        struct vertex { natus::math::vec4f_t pos ; natus::math::vec4f_t col ; } ;

        int_t _max_textures = 3 ;
        int_t _used_texture = 0 ;

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
                natus::ntd::vector< natus::io::location_t > shader_locations = {
                    natus::io::location_t( "shaders.test_multi_geometry.nsl" )
                };

                natus::application::util::app_essentials_t::init_struct is = 
                {
                    { "myapp" }, 
                    { natus::io::path_t( DATAPATH ), "./working", "data" },
                    shader_locations
                } ;

                _ae.init( is ) ;
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

                    array[ 0 ].col = natus::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 1 ].col = natus::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 2 ].col = natus::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 3 ].col = natus::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
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
            }

            // geometry configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::vertex_attribute::color0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .resize( 9 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = natus::math::vec4f_t( -0.25f, +0.50f, 0.0f, 1.0f ) ;
                    array[ 1 ].pos = natus::math::vec4f_t( +0.25f, +0.50f, 0.0f, 1.0f ) ;
                    array[ 2 ].pos = natus::math::vec4f_t( +0.50f, +0.25f, 0.0f, 1.0f ) ;
                    array[ 3 ].pos = natus::math::vec4f_t( +0.50f, -0.25f, 0.0f, 1.0f ) ;
                    array[ 4 ].pos = natus::math::vec4f_t( +0.25f, -0.50f, 0.0f, 1.0f ) ;
                    array[ 5 ].pos = natus::math::vec4f_t( -0.25f, -0.50f, 0.0f, 1.0f ) ;
                    array[ 6 ].pos = natus::math::vec4f_t( -0.50f, -0.25f, 0.0f, 1.0f ) ;
                    array[ 7 ].pos = natus::math::vec4f_t( -0.50f, +0.25f, 0.0f, 1.0f ) ;
                    array[ 8 ].pos = natus::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ) ;

                    array[ 0 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 1 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 2 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 3 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 4 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 5 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 6 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 7 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 8 ].col = natus::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).resize( 24 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 8 ;

                    array[ 3 ] = 1 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 8 ;

                    array[ 6 ] = 2 ;
                    array[ 7 ] = 3 ;
                    array[ 8 ] = 8 ;

                    array[ 9 ] = 3 ;
                    array[ 10 ] = 4 ;
                    array[ 11 ] = 8 ;

                    array[ 12 ] = 4 ;
                    array[ 13 ] = 5 ;
                    array[ 14 ] = 8 ;

                    array[ 15 ] = 5 ;
                    array[ 16 ] = 6 ;
                    array[ 17 ] = 8 ;

                    array[ 18 ] = 6 ;
                    array[ 19 ] = 7 ;
                    array[ 20 ] = 8 ;

                    array[ 21 ] = 7 ;
                    array[ 22 ] = 0 ;
                    array[ 23 ] = 8 ;
                } ) ;

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "geometry",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;     
            }

            // the quad render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "quad" ) ;

                {
                    rc.link_geometry( {"quad", "geometry"} ) ;
                    rc.link_shader( "test_multi_geometry" ) ;
                }
               
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    rc.add_variable_set( std::move( vars ) ) ;
                }

                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
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

            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        { 
            _ro->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
            {
                {
                    auto * var = vs->data_variable<natus::math::mat4f_t>("u_view") ;
                    var->set( _ae.get_camera_0()->get_camera()->get_view_matrix() ) ;
                }
                {
                    auto * var = vs->data_variable<natus::math::mat4f_t>("u_proj") ;
                    var->set( _ae.get_camera_0()->get_camera()->get_proj_matrix() ) ;
                }

                {
                    auto const of = float_t(i) * 2.0 - 1.0 ;
                    natus::math::m3d::trafof_t t ;
                    t.scale_fl( 100.0f ) ;
                    t.translate_fl( natus::math::vec3f_t( 100.0f * (float_t(i) * 2.0f - 1.0f), 0.0f, 0.0f ) ) ;

                    auto * var = vs->data_variable<natus::math::mat4f_t>("u_world") ;
                    var->set( t.get_transformation() ) ;
                }
            } ) ;

            _ae.on_graphics_begin( rd ) ;

            _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
            {
                // geometry 0
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    detail.geo = 0 ;
                    a.render( _ro, detail ) ;
                }

                // geometry 1
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.varset = 1 ;
                    detail.geo = 1 ;
                    a.render( _ro, detail ) ;
                }
            } ) ;


            _ae.on_graphics_end( 10 ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Test Control" ) ;

            if( ImGui::SliderInt( "Use Texture", &_used_texture, 0, _max_textures ) )
            {
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
    return natus::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}






