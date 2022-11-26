

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

        natus::graphics::state_object_res_t _fb_render_states ;
        natus::application::util::app_essentials_t _ae ;

        natus::graphics::framebuffer_object_res_t _fb = natus::graphics::framebuffer_object_t() ;

        natus::gfx::quad_res_t _quad ;
        natus::gfx::sprite_render_2d_res_t _sr ;
        
        natus::ntd::vector< natus::gfx::sprite_sheet_t > _sheets ;
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
            _sr = std::move( rhv._sr ) ;
            _quad = std::move( rhv._quad ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;
            _ae.get_camera_0()->orthographic() ;
            return natus::application::result::ok ;
        }

    private:

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

            uint_t const fb_width = 1024 ;
            uint_t const fb_height = 512 ;
            
            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "framebuffer_render_states" ) ;

                {
                    natus::graphics::render_state_sets_t rss ;

                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = false ;

                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = natus::graphics::fill_mode::fill ;

                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.clear_color = natus::math::vec4f_t(0.4f) ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear  = true ;
                   
                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = natus::math::vec4ui_t( 0, 0, fb_width, fb_height ) ;

                    so.add_render_state_set( rss ) ;
                }

                _fb_render_states = std::move( so ) ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _fb_render_states ) ;
                } ) ;
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

                            natus::gfx::sprite_sheet ss ;
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

                                natus::gfx::sprite_sheet::sprite s_ ;
                                s_.rect = rect ;
                                s_.pivot = pivot ;

                                sheet.rects.emplace_back( s_ ) ;
                            }

                            natus::ntd::map< natus::ntd::string_t, size_t > object_map ;
                            for( auto const & a : ss.animations )
                            {
                                size_t obj_id = 0 ;
                                {
                                    auto iter = object_map.find( a.object ) ;
                                    if( iter != object_map.end() ) obj_id = iter->second ;
                                    else 
                                    {
                                        obj_id = sheet.objects.size() ;
                                        sheet.objects.emplace_back( natus::gfx::sprite_sheet::object { a.object, {} } ) ;
                                    }
                                }

                                natus::gfx::sprite_sheet::animation a_ ;

                                size_t tp = 0 ;
                                for( auto const & f : a.frames )
                                {
                                    auto iter = std::find_if( ss.sprites.begin(), ss.sprites.end(), 
                                        [&]( natus::format::natus_document_t::sprite_sheet_t::sprite_cref_t s )
                                    {
                                        return s.name == f.sprite ;
                                    } ) ;

                                    size_t const d = std::distance( ss.sprites.begin(), iter ) ;
                                    natus::gfx::sprite_sheet::animation::sprite s_ ;
                                    s_.begin = tp ;
                                    s_.end = tp + f.duration ;
                                    s_.idx = d ;
                                    a_.sprites.emplace_back( s_ ) ;

                                    tp = s_.end ;
                                }
                                a_.duration = tp ;
                                a_.name = a.name ;
                                sheet.objects[obj_id].animations.emplace_back( std::move( a_ ) ) ;
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

            // framebuffer
            {
                _fb = natus::graphics::framebuffer_object_t( "the_scene" ) ;
                _fb->set_target( natus::graphics::color_target_type::rgba_uint_8, 1 ).
                    resize( fb_width, fb_height ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _fb ) ;
                } ) ;
            }

            // prepare sprite render
            {
                _sr = natus::gfx::sprite_render_2d_res_t( natus::gfx::sprite_render_2d_t() ) ;
                _sr->init( "debug_sprite_render", "image_array", _ae.graphics() ) ;
            }
            
            // prepare quad
            {
                _quad = natus::gfx::quad_res_t( natus::gfx::quad_t("post_map") ) ;
                _quad->init( _ae.graphics() ) ;
                _quad->set_texture("the_scene.0") ;
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
            //NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        natus::math::vec2f_t _pos = natus::math::vec2f_t( -400.0f, -100.0f ) ;

        //**********************************************************************************************************
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t pd ) noexcept
        { 
            natus::log::global_t::status( "physics: " + std::to_string( pd.micro_dt ) ) ;

            static float_t inter = 0.0f ;
            float_t v = natus::math::interpolation<float_t>::linear( 0.0f, 1.0f, inter ) *2.0f-1.0f ;
            v *= 400 ;

            _pos = natus::math::vec2f_t( v, 0.0f ) ;
            //_pos += natus::math::vec2f_t( 200.0f, 0.0f ) * pd.sec_dt ;
            //_pos += natus::math::vec2f_t( 200.0f, 0.0f ) * 0.008f ;
            if( _pos.x() > 400.0f ) _pos.x( -400.0f ) ;

            inter += pd.sec_dt * 0.4f ;
            if( inter > 1.0f ) inter = 0.0f ;

            //NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;

            
            return natus::application::result::ok ; 
        }

        //**********************************************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rdi ) noexcept 
        { 
            natus::log::global_t::status( "graphics: " + std::to_string( rdi.micro_dt ) ) ;
            _sr->set_view_proj( _ae.get_camera_0()->mat_view(), _ae.get_camera_0()->mat_proj() ) ;

            size_t const sheet = 0 ;
            size_t const ani = 0 ;
            static size_t ani_time = 0 ;
            if( ani_time > _sheets[sheet].objects[0].animations[ani].duration ) ani_time = 0 ;

            for( auto const & s : _sheets[sheet].objects[0].animations[ani].sprites )
            {
                if( ani_time >= s.begin && ani_time < s.end )
                {
                    auto const & rect = _sheets[sheet].rects[s.idx] ;
                    natus::math::vec2f_t pos( -0.0f, 0.0f ) ;
                    _sr->draw( 0, 
                        pos, 
                        natus::math::mat2f_t().identity(),
                        natus::math::vec2f_t(1000.0f),
                        rect.rect,  
                        sheet, rect.pivot, 
                        natus::math::vec4f_t(1.0f) ) ;
                    break ;
                }
            }

            ani_time += rdi.micro_dt / 1000 ;
            
            #if 1
            // physics driven
            {
                auto & s = _sheets[0].objects[0].animations[0].sprites[0] ;

                auto const & rect = _sheets[sheet].rects[s.idx] ;
                natus::math::vec2f_t pos( -0.0f, 0.0f ) ;
                _sr->draw( 0, 
                    _pos,
                    natus::math::mat2f_t().identity(),
                    natus::math::vec2f_t(2000.0f),
                    rect.rect,  
                    sheet, rect.pivot, 
                    natus::math::vec4f_t(1.0f) ) ;
            }

            // animation driven
            {
                static float_t inter = 0.0f ;
                float_t v = natus::math::interpolation<float_t>::linear( 0.0f, 1.0f, inter ) *2.0f-1.0f ;
                v *= 400 ;

                auto & s = _sheets[0].objects[0].animations[0].sprites[0] ;

                auto const & rect = _sheets[sheet].rects[s.idx] ;
                natus::math::vec2f_t pos( -0.0f, 0.0f ) ;
                _sr->draw( 0, 
                    natus::math::vec2f_t( v, -200.0f ),
                    natus::math::mat2f_t().identity(),
                    natus::math::vec2f_t(2000.0f),
                    rect.rect,  
                    sheet, rect.pivot, 
                    natus::math::vec4f_t(1.0f) ) ;

                inter += rdi.sec_dt * 0.4f ;
                //inter += 0.016f * 0.4f ;
                if( inter > 1.0f ) inter = 0.0f ;
            }
            #endif

            #if 0
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

            
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
            }
            #endif

            {
                _sr->set_view_proj( natus::math::mat4f_t().identity(), _ae.get_camera_0()->mat_proj() ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( _fb ) ;
                    a.push( _fb_render_states ) ;
                } ) ;

                _sr->prepare_for_rendering() ;
                _sr->render( 0 ) ;
                //_pr->render( 1 ) ;
                //_pr->render( 2 ) ;
                //_pr->render( 3 ) ;
                //_pr->render( 4 ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                    a.unuse( natus::graphics::backend::unuse_type::framebuffer ) ;
                } ) ;
            }
            
            _ae.on_graphics_begin( rdi ) ;
            _quad->render( _ae.graphics() ) ;
            _ae.on_graphics_end( 100 ) ;

            //NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        //**********************************************************************************************************
        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            return natus::application::result::ok ;
        }

        //**********************************************************************************************************
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
