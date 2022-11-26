
#include "coordinate.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/gfx/sprite/sprite_render_2d.h>

#include <natus/profile/macros.h>

#include <random>
#include <thread>

// this test checks out how we can do custom coordinates for
// "infinite" space traversal of game objects 
// This is done by using a custom coordinate that divides
// space into float managable pieces (sectors) of a huge space where
// each sector will get an i,j coordinate and a relative 2d float vector.
// All calculation is done by transforming certain objects into sector 
// space.
namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;
        world::coord_2d_t _ccamera_0 ;

        world::coord_2d_t _ccur_mouse ;
        world::coord_2d_t _ccur_mouse_global ;

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
        test_app( this_rref_t rhv ) noexcept : app( ::std::move( rhv ) ) 
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
            natus::application::util::app_essentials_t::init_struct is = 
            {
                { "myapp" }, 
                { natus::io::path_t( DATAPATH ), "./working", "data" }
            } ;

            _ae.init( is ) ;
            
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;

            {
                natus::device::layouts::three_mouse_t mouse( _ae.get_mouse_dev() ) ;
                auto const rel = mouse.get_local() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;

                {
                    world::coord_2d_t a = rel * (_ae.get_window_dims() * natus::math::vec2f_t(0.5f) ) ;
                    _ccur_mouse_global = a + _ccamera_0 ;
                    _ccur_mouse = a ;
                }
            }

            // move camera with mouse
            {
                natus::device::layouts::three_mouse_t mouse( _ae.get_mouse_dev() ) ;
                static auto m_rel_old = natus::math::vec2f_t() ;
                auto const m_rel = mouse.get_local() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;

                if( mouse.is_pressing( natus::device::layouts::three_mouse::button::left ) )
                {
                    _ccamera_0 = _ccamera_0 + (m_rel-m_rel_old).negated() * 100.0f ;
                    _ae.get_camera_0()->translate_to( natus::math::vec3f_t( _ccamera_0.get_vector().x(), _ccamera_0.get_vector().y(), _ae.get_camera_0()->get_position().z() ) ) ;
                }

                m_rel_old = m_rel ;
            }

            return natus::application::result::ok ; 
        }

        //***********************************************************************************
        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        //***********************************************************************************
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;
            return natus::application::result::ok ; 
        }

        //***********************************************************************************
        void_t draw_sections( void_t ) noexcept
        {
            auto pr = _ae.get_prim_render() ;

            natus::math::vec2f_t const eh = natus::math::vec2ui_t( _ae.get_extent() ) >> natus::math::vec2ui_t( 1 ) ;

            auto const p0 = _ccamera_0 + natus::math::vec2f_t( -eh.x(), -eh.y() ) ;
            auto const p1 = _ccamera_0 + natus::math::vec2f_t( -eh.x(), +eh.y() ) ;
            auto const p2 = _ccamera_0 + natus::math::vec2f_t( +eh.x(), +eh.y() ) ;
            auto const p3 = _ccamera_0 + natus::math::vec2f_t( +eh.x(), -eh.y() ) ;

            world::coord_2d_t const cc = _ccamera_0 ;
            world::coord_2d_t const c0 = p0.origin() ;
            world::coord_2d_t const c1 = p2.origin() + natus::math::vec2ui_t(1) ;

            auto const rel = c1 - c0 ;
            auto const num_sectors = rel.get_ij()  ;

            // vertical lines
            {
                auto const off = world::coord_2d_t::granularity_half() ;
                natus::math::vec2f_t const start = (c0 - cc.origin()).euclidean() - off ;
                natus::math::vec2f_t const adv = world::coord_2d_t::granularity() ;

                natus::math::vec2f_t p0( start ) ;
                natus::math::vec2f_t p1( start + natus::math::vec2f_t( 0.0f, rel.euclidean().y() ) ) ;

                for( uint_t i = 0 ; i < num_sectors.x() + 1; ++i )
                {
                    pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.0f,0.0f,1.0f,1.0f) ) ;
                    p0 = p0 + natus::math::vec2f_t( adv.x(), 0.0f ) ;
                    p1 = p1 + natus::math::vec2f_t( adv.x(), 0.0f ) ;
                }
            }

            // y lines / horizontal lines
            {
                auto const off = world::coord_2d_t::granularity_half() ;
                natus::math::vec2f_t const start = (c0 - cc.origin()).euclidean() - off ;
                natus::math::vec2f_t const adv = world::coord_2d_t::granularity() ;

                natus::math::vec2f_t p0( start ) ;
                natus::math::vec2f_t p1( start + natus::math::vec2f_t( rel.euclidean().x(), 0.0f ) ) ;
                        
                for( uint_t i = 0 ; i < num_sectors.y() + 1; ++i )
                {
                    pr->draw_line( 0, p0, p1, natus::math::vec4f_t(0.0f,0.0f,1.0f,1.0f) ) ;
                    p0 = p0 + natus::math::vec2f_t( 0.0f, adv.y() ) ;
                    p1 = p1 + natus::math::vec2f_t( 0.0f, adv.y() ) ;
                }
            }
        }

        //***********************************************************************************
        void_t draw_mouse( void_t ) noexcept
        {
            auto pr = _ae.get_prim_render() ;

            world::coord_2d_t const cc = _ccamera_0 ;
            world::coord_2d_t const c0 = _ccur_mouse_global ;
            world::coord_2d_t const c1 = _ccur_mouse ;
            natus::math::vec2f_t const p0 = (c0.origin() - cc.origin()).euclidean() ;
            natus::math::vec2f_t const p1 = (c1 + cc - cc.origin()).euclidean() ;

            pr->draw_line( 1, p0, p1, natus::math::vec4f_t(0.0f,1.0f,0.0f,1.0f) ) ;
        }

        //***********************************************************************************
        void_t draw_camera( void_t ) noexcept
        {
            auto pr = _ae.get_prim_render() ;

            world::coord_2d_t const cc = _ccamera_0 ;
            world::coord_2d_t const c0 = _ccamera_0 ;
            natus::math::vec2f_t const p0 = (c0 - cc.origin()).euclidean() ;
            natus::math::vec2f_t const p1 = (c0.origin() - cc.origin()).euclidean() ;

            pr->draw_circle( 1, 2, p0, 2.0f, natus::math::vec4f_t(0.0f,1.0f,0.0f,1.0f), natus::math::vec4f_t(0.0f,1.0f,0.0f,1.0f) ) ;
            pr->draw_line( 1, p0, p1, natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f) ) ;
        }

        //***********************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _ae.on_graphics_begin( rd ) ;

            auto pr = _ae.get_prim_render() ;
            auto tr = _ae.get_text_render() ;

            _ae.get_camera_0()->orthographic() ;
            
            {
                this_t::draw_sections() ;
                this_t::draw_mouse() ;
                this_t::draw_camera() ;
            }

            _ae.on_graphics_end( 100 ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        //***********************************************************************************
        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            natus::tool::custom_imgui_widgets::text_overlay( "mouse_control", "Use left mouse button to move", 1 ) ;

            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            if( ImGui::Begin( "Mouse Info" ) )
            {
                // mouse coords
                {
                    auto const tmp = _ccur_mouse_global ;
                    {
                    
                        uint_t data[2] = {tmp.get_ij().x(), tmp.get_ij().y() } ;
                        ImGui::Text( "i: %d, j: %d", data[0], data[1] ) ;
                    }

                    {
                        float_t data[2] = {tmp.get_vector().x(), tmp.get_vector().y() } ;
                        ImGui::Text( "x: %f, y: %f", data[0], data[1] ) ;
                    }
                }

                ImGui::End() ;
            }

            if( ImGui::Begin( "Camera Info" ) )
            {
                // mouse coords
                {
                    auto const tmp = _ccamera_0 ;
                    {
                    
                        uint_t data[2] = {tmp.get_ij().x(), tmp.get_ij().y() } ;
                        ImGui::Text( "i: %d, j: %d", data[0], data[1] ) ;
                    }

                    {
                        float_t data[2] = {tmp.get_vector().x(), tmp.get_vector().y() } ;
                        ImGui::Text( "x: %f, y: %f", data[0], data[1] ) ;
                    }
                }

                ImGui::End() ;
            }
            

            return natus::application::result::ok ;
        }

        //***********************************************************************************
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
