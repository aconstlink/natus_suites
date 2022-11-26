

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/gfx/sprite/sprite_render_2d.h>

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

        natus::gfx::sprite_render_2d_res_t _pr ;

    public:

        test_app( void_t ) noexcept
        {
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
            _pr = std::move( rhv._pr ) ;
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
                _ae.get_camera_0()->look_at( natus::math::vec3f_t( 0.0f, 60.0f, -50.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }
            
            // image configuration
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                natus::format::future_item_t items[4] = 
                {
                    mod_reg->import_from( natus::io::location_t( "images.industrial.industrial.v2.png" ), _ae.db() ),
                    mod_reg->import_from( natus::io::location_t( "images.Paper-Pixels-8x8.Enemies.png" ), _ae.db() ),
                    mod_reg->import_from( natus::io::location_t( "images.Paper-Pixels-8x8.Player.png" ), _ae.db() ),
                    mod_reg->import_from( natus::io::location_t( "images.Paper-Pixels-8x8.Tiles.png" ), _ae.db() )
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

            // prepare sprite render
            {
                _pr = natus::gfx::sprite_render_2d_res_t( natus::gfx::sprite_render_2d_t() ) ;
                _pr->init( "debug_sprite_render", "image_array", _ae.graphics() ) ;
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
            _ae.on_graphics_begin( rd ) ;

            static float_t inc = 0.0f ;

            natus::math::vec2f_t pos( -1.0f, 0.5f ) ;
            for( size_t i=0; i<1159; ++i )
            {
                _pr->draw( 0, 
                    pos + natus::math::vec2f_t( 0.0f, -0.4f ), 
                    natus::math::mat2f_t( std::sin(inc*2.0f), std::cos(inc), natus::math::rotation_matrix() ),
                    natus::math::vec2f_t(0.05f*inc),
                    natus::math::vec4f_t(0.3f,0.1f,0.35f,0.14f),  
                    1 ) ;

                pos += natus::math::vec2f_t( float_t(4) / 1000.0f, 
                    std::sin((float_t(i)/50.0f)*2.0f*natus::math::constants<float_t>::pi())*0.05f) ;
            }

            pos = natus::math::vec2f_t( 1.0f, -0.5f ) ;

            for( size_t i=0; i<1154; ++i )
            {
                size_t const idx = 999 - i ;
                _pr->draw( 1, 
                    pos + natus::math::vec2f_t( 0.0f, -0.4f ), 
                    natus::math::mat2f_t().identity(),
                    natus::math::vec2f_t(0.1f),
                    natus::math::vec4f_t(0.1f,0.1f,0.2f,0.2f), 
                    0 ) ;

                pos -= natus::math::vec2f_t( (float_t(i) / 1000.0f), 0.0f ) ;
            }

            _pr->draw( 2, 
                    natus::math::vec2f_t( 0.0f, -0.0f ), 
                    natus::math::mat2f_t().identity(),
                    natus::math::vec2f_t(0.5f),
                    natus::math::vec4f_t(0.0f,0.0f,0.4f,0.4f), 
                    3 ) ;

            inc += 0.01f  ;
            inc = natus::math::fn<float_t>::mod( inc, 1.0f ) ;
            
            _pr->prepare_for_rendering() ;
            _ae.on_graphics_end( 10, [&]( size_t const l )
            {
                _pr->render( l ) ;
            } ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

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
