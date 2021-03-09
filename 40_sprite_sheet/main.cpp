

#include <natus/application/global.h>
#include <natus/application/app.h>

#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/format/natus/natus_module.h>

#include <natus/io/database.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/sprite/sprite_render_2d.h>
#include <natus/gfx/util/quad.h>

#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profiling/macros.h>

#include <natus/geometry/mesh/polygon_mesh.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>
#include <natus/geometry/3d/tetra.h>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/3d/transformation.hpp>

#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::graphics::async_views_t _graphics ;

        natus::graphics::state_object_res_t _root_render_states ;
        natus::graphics::framebuffer_object_res_t _fb = natus::graphics::framebuffer_object_t() ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _do_tool = false ;

        natus::gfx::quad_res_t _quad ;
        natus::gfx::sprite_render_2d_res_t _sr ;
        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::io::database_res_t _db ;

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
            auto view1 = this_t::create_window( "A Render Window", wi ) ;
            auto view2 = this_t::create_window( "A Render Window", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _graphics = natus::graphics::async_views_t( { view1.async(), view2.async() } ) ;
            #elif 0
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
            _graphics = natus::graphics::async_views_t( { view1.async() } ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi ) ;
            _graphics = natus::graphics::async_views_t( { view1.async() } ) ;
            #endif

            _db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _camera_0 = std::move( rhv._camera_0 ) ;
            _graphics = std::move( rhv._graphics ) ;
            _sr = std::move( rhv._sr ) ;
            _db = std::move( rhv._db ) ; 

            _sr = std::move( rhv._sr ) ;
            _quad = std::move( rhv._quad ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ),
                float_t(wei.w) / float_t(wei.h), 1.0f, 1000.0f ) ;

            _camera_0.orthographic( float_t(wei.w), float_t(wei.h), 0.1f, 1000.0f ) ;

            return natus::application::result::ok ;
        }

    private:

        virtual natus::application::result on_init( void_t )
        { 
            natus::device::global_t::system()->search( [&] ( natus::device::idevice_res_t dev_in )
            {
                if( natus::device::three_device_res_t::castable( dev_in ) )
                {
                    _dev_mouse = dev_in ;
                }
                else if( natus::device::ascii_device_res_t::castable( dev_in ) )
                {
                    _dev_ascii = dev_in ;
                }
            } ) ;

            if( !_dev_mouse.is_valid() )
            {
                natus::log::global_t::status( "no three mosue found" ) ;
            }

            if( !_dev_ascii.is_valid() )
            {
                natus::log::global_t::status( "no ascii keyboard found" ) ;
            }

            {
                _camera_0.look_at( natus::math::vec3f_t( 0.0f, 0.0f, -50.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }

            uint_t const fb_width = 1024 ;
            uint_t const fb_height = 512 ;

            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "root_render_states" ) ;

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

                _root_render_states = std::move( so ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }
            
            // import natus file
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto item = mod_reg->import_from( natus::io::location_t( "sprite_sheet.natus" ), _db ) ;

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
                            futures.emplace_back( mod_reg->import_from( l, _db ) ) ;
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

                    _graphics.for_each( [&]( natus::graphics::async_view_t a )
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

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _fb ) ;
                } ) ;
            }

            // prepare sprite render
            {
                _sr = natus::gfx::sprite_render_2d_res_t( natus::gfx::sprite_render_2d_t() ) ;
                _sr->init( "debug_sprite_render", "image_array", _graphics ) ;
            }
            
            // prepare quad
            {
                _quad = natus::gfx::quad_res_t( natus::gfx::quad_t("post_map") ) ;
                _quad->set_texture("the_scene.0") ;
                _quad->init( _graphics ) ;
                
            }
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_device( natus::application::app_t::device_data_in_t ) 
        {
            {
                natus::device::layouts::ascii_keyboard_t ascii( _dev_ascii ) ;
                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f8 ) ==
                    natus::device::components::key_state::released )
                {
                }
                else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f9 ) ==
                    natus::device::components::key_state::released )
                {
                }
                else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f2 ) ==
                    natus::device::components::key_state::released )
                {
                    _do_tool = !_do_tool ;
                }
            }
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rdi ) 
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
                    _sr->draw( 0, 
                        pos, 
                        natus::math::mat2f_t().identity(),
                        natus::math::vec2f_t(10000.0f),
                        rect.rect,  
                        sheet, rect.pivot, 
                        natus::math::vec4f_t(1.0f) ) ;
                    break ;
                }
            }

            ani_time += rdi.milli_dt ;
            
            #if 0
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( _root_render_states ) ;
                } ) ;
            }

            
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( natus::graphics::state_object_t(), 10 ) ;
                } ) ;
            }
            #endif

            {
                _sr->set_view_proj( natus::math::mat4f_t().identity(), _camera_0.mat_proj() ) ;

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( _fb ) ;
                    a.use( _root_render_states ) ;
                } ) ;

                _sr->prepare_for_rendering() ;
                _sr->render( 0 ) ;
                //_pr->render( 1 ) ;
                //_pr->render( 2 ) ;
                //_pr->render( 3 ) ;
                //_pr->render( 4 ) ;

                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( natus::graphics::state_object_res_t() ) ;
                    a.use( natus::graphics::framebuffer_object_res_t() ) ;
                } ) ;
                
            }
            
            _quad->render( _graphics ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui )
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "do something" ) ;

            ImGui::End() ;
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
