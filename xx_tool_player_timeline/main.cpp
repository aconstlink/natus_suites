
#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/tool/imgui/player_controller.h>
#include <natus/tool/imgui/timeline.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/profile/macros.h>

#include <natus/math/utility/angle.hpp>

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

        natus::tool::player_controller_t _pc ;
        natus::tool::timeline_t _tl0 ;
        natus::tool::timeline_t _tl1 ;
        natus::tool::timeline_t _tl2 ;

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

            _pc = natus::tool::player_controller_t() ;
            _tl0 = natus::tool::timeline() ;
            _tl1 = natus::tool::timeline() ;
            _tl2 = natus::tool::timeline() ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
            _pc = std::move( rhv._pc ) ;
            _tl0 = std::move( rhv._tl0 ) ;
            _tl1 = std::move( rhv._tl1 ) ;
            _tl2 = std::move( rhv._tl2 ) ;
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
                { "myapp" }, { natus::io::path_t( DATAPATH ), "./working", "data" }
            } ;

            _ae.init( is ) ;

            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_device( natus::application::app_t::device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;
            NATUS_PROFILING_COUNTER_HERE( "Device Clock" ) ;

            return natus::application::result::ok ; 
        }

        size_t _milli_delta = 0 ;

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd_in ) noexcept 
        { 
            _milli_delta = rd_in.milli_dt ;

            _ae.on_graphics_begin( rd_in ) ; 
            _ae.on_graphics_end( 10 ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            static bool_t do_play = false ;
            static size_t max_milli = 100000 ;
                
            static size_t milli_test = 0 ;
            
            if( do_play ) milli_test += _milli_delta ;
            if( milli_test > max_milli ) milli_test = 0 ;

            {
                ImGui::Begin( "Player Controller" ) ;
                auto const res = _pc.do_tool( "player", td.imgui ) ;
                switch( res ) 
                {
                case natus::tool::player_controller::player_state::play:
                    do_play = true ;
                    break ;
                case natus::tool::player_controller::player_state::stop:
                    do_play = false ;
                    milli_test = 0 ;
                    break ;
                case natus::tool::player_controller::player_state::pause:
                    do_play = false ;
                    break ;
                }
                ImGui::End() ;
            }

            {
                natus::tool::time_info_t ti ;
                ti.cur_milli = milli_test ;
                ti.max_milli = max_milli ;

                ImGui::Begin( "Timeline" ) ;
                if( _tl0.begin( ti, td.imgui ) )
                {
                    // user draw in timeline?
                    //ImGui::Text( "test 1" ) ;
                    _tl0.end() ;
                }

                if( _tl1.begin( ti, td.imgui ) )
                {
                    // user draw in timeline?
                    //ImGui::Text( "test 1" ) ;
                    _tl1.end() ;
                }

                if( _tl2.begin( ti, td.imgui ) )
                {
                    // user draw in timeline?
                    //ImGui::Text( "test 1" ) ;
                    _tl2.end() ;
                }
                ImGui::End() ;

                milli_test = ti.cur_milli ;
            }

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
