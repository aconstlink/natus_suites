

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/format/natus/natus_module.h>

#include <natus/tool/imgui/custom_widgets.h>

#include <natus/profile/macros.h>

#include <natus/gfx/sprite/sprite_render_2d.h>
#include <natus/gfx/util/quad.h>

#include <natus/math/interpolation/interpolate.hpp>

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

        struct sprite_sheet
        {
            struct animation
            {
                struct sprite
                {
                    size_t idx ;
                    size_t begin ;
                    size_t end ;
                };
                size_t duration ;
                natus::ntd::vector< sprite > sprites ;

            };
            natus::ntd::vector< animation > animations ;

            struct sprite
            {
                natus::math::vec4f_t rect ;
                natus::math::vec2f_t pivot ;
            };
            natus::ntd::vector< sprite > rects ;
        };
        natus::ntd::vector< sprite_sheet > _sheets ;

        

    public:

        test_app( void_t ) 
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
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
            _pr = std::move( rhv._pr ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        //**********************************************************************************************************
        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;
            _ae.get_camera_0()->orthographic() ;

            return natus::application::result::ok ;
        }

    private:

        //**********************************************************************************************************
        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::application::util::app_essentials_t::init_struct is = 
            {
                { "myapp" }, { natus::io::path_t( DATAPATH ), "./working", "data" }
            } ;

            _ae.init( is ) ;

            {
                _ae.get_camera_0()->look_at( natus::math::vec3f_t( 0.0f, 0.0f, -50.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }
            
            // import natus file
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto item = mod_reg->import_from( natus::io::location_t( "sprite_sheet.natus" ), _ae.db() ) ;

                natus::format::natus_item_res_t ni = item.get() ;
                if( ni.is_valid() )
                {
                    natus::format::natus_document_t doc = std::move( ni->doc ) ;

                    // taking all slices
                    natus::graphics::image_t imgs ;

                    // load images
                    {
                        natus::ntd::vector< natus::format::future_item_t > futures ;
                        for( auto const & ss : doc.sprite_sheets )
                        {
                            auto const l = natus::io::location_t::from_path( natus::io::path_t(ss.image.src) ) ;
                            futures.emplace_back( mod_reg->import_from( l, _ae.db() ) ) ;
                        }

                    
                        for( size_t i=0; i<doc.sprite_sheets.size(); ++i )
                        {
                            natus::format::image_item_res_t ii = futures[i].get() ;
                            if( ii.is_valid() )
                            {
                                imgs.append( *ii->img ) ;
                            }

                            sprite_sheet ss ;
                            _sheets.emplace_back( ss ) ;
                        }
                    }

                    // make sprite animation infos
                    {
                        // as an image array is used, the max dims need to be
                        // used to compute the particular rect infos
                        auto dims = imgs.get_dims() ;
                    
                        size_t i=0 ;
                        for( auto const & ss : doc.sprite_sheets )
                        {
                            auto & sheet = _sheets[i++] ;

                            for( auto const & s : ss.sprites )
                            {
                                natus::math::vec4f_t const rect = 
                                    (natus::math::vec4f_t( s.animation.rect ) + 
                                        natus::math::vec4f_t(0.5f,0.5f, 0.5f, 0.5f))/ 
                                    natus::math::vec4f_t( dims.xy(), dims.xy() )  ;

                                natus::math::vec2f_t const pivot =
                                    natus::math::vec2f_t( s.animation.pivot ) / 
                                    natus::math::vec2f_t( dims.xy() ) ;

                                sprite_sheet::sprite s_ ;
                                s_.rect = rect ;
                                s_.pivot = pivot ;

                                sheet.rects.emplace_back( s_ ) ;
                            }


                            for( auto const & a : ss.animations )
                            {
                                sprite_sheet::animation a_ ;

                                size_t tp = 0 ;
                                for( auto const & f : a.frames )
                                {
                                    auto iter = std::find_if( ss.sprites.begin(), ss.sprites.end(), 
                                        [&]( natus::format::natus_document_t::sprite_sheet_t::sprite_cref_t s )
                                    {
                                        return s.name == f.sprite ;
                                    } ) ;

                                    size_t const d = std::distance( ss.sprites.begin(), iter ) ;
                                    sprite_sheet::animation::sprite s_ ;
                                    s_.begin = tp ;
                                    s_.end = tp + f.duration ;
                                    s_.idx = d ;
                                    a_.sprites.emplace_back( s_ ) ;

                                    tp = s_.end ;
                                }
                                a_.duration = tp ;
                                sheet.animations.emplace_back( std::move( a_ ) ) ;
                            }
                        }
                    }

                    natus::graphics::image_object_res_t ires = natus::graphics::image_object_t( 
                        "image_array", std::move( imgs ) )
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
            }

            // prepare sprite render
            {
                _pr = natus::gfx::sprite_render_2d_res_t( natus::gfx::sprite_render_2d_t() ) ;
                _pr->init( "debug_sprite_render", "image_array",  _ae.graphics() ) ;
            }
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        //**********************************************************************************************************
        virtual natus::application::result on_device( natus::application::app_t::device_data_in_t dd ) noexcept 
        {
            _ae.on_device( dd ) ;
            return natus::application::result::ok ; 
        }

        //**********************************************************************************************************
        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }

        //**********************************************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rdi ) noexcept
        { 
            _ae.on_graphics_begin( rdi ) ;

            {
                size_t const sheet = 0 ;
                size_t const ani = 0 ;
                static size_t ani_time = 0 ;
                if( ani_time > _sheets[sheet].animations[ani].duration ) ani_time = 0 ;

                for( auto const & s : _sheets[sheet].animations[ani].sprites )
                {
                    if( ani_time >= s.begin && ani_time < s.end )
                    {
                        auto const & rect = _sheets[sheet].rects[s.idx] ;
                        natus::math::vec2f_t pos( -0.0f, 0.0f ) ;
                        _pr->draw( 0, 
                            pos, 
                            natus::math::mat2f_t().identity(),
                            10.0f,
                            rect.rect,  
                            sheet, rect.pivot, 
                            natus::math::vec4f_t(1.0f) ) ;
                        break ;
                    }
                }

                ani_time += rdi.micro_dt / 1000 ;
            }

            _ae.on_graphics_end( 5, [&]( size_t const l )
            {
                _pr->prepare_for_rendering() ;
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
