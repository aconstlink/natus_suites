
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/imgui/imgui.h>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/profiling/macros.h>
#include <natus/audio/buffer.hpp>

#include <natus/device/layouts/three_mouse.hpp>
#include <natus/device/global.h>

#include <natus/math/dsp/fft.hpp>

#include <thread>
#include <array>

//
// This test prints the captured "What U Hear" samples using Imgui.
// The program uses the async audio system.
//

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;

        natus::audio::async_access_t _audio ;

        natus::gfx::imgui_res_t _imgui ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _fullscreen = false ;
        bool_t _vsync = true ;

        natus::audio::capture_object_res_t _capture = natus::audio::capture_object_t() ;

        bool_t _captured_rendered = true ;
        natus::audio::buffer_t _captured = natus::audio::buffer_t( 48000 ) ;
        natus::ntd::vector< float_t > _frequencies0 ;
        natus::ntd::vector< float_t > _frequencies1 ;
        natus::ntd::vector< float_t > _freq_bands ;

        natus::graphics::state_object_res_t _root_render_states ;

    public:

        test_app( void_t )
        {
            natus::application::app::window_info_t wi ;
            wi.w = 1200 ;
            wi.h = 400 ;
            _wid_async = this_t::create_window( "Frequencies", wi, { natus::graphics::backend_type::d3d11 } ) ;
            _wid_async.window().fullscreen( _fullscreen ) ;
            _wid_async.window().vsync( _vsync ) ;

            _imgui = natus::gfx::imgui_res_t( natus::gfx::imgui_t() ) ;

            _freq_bands.resize( 14 ) ;

            _audio = this_t::create_audio_engine() ;
        }

        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) )
        {
            _wid_async = ::std::move( rhv._wid_async ) ;
            _imgui = ::std::move( rhv._imgui ) ;

            _dev_mouse = ::std::move( rhv._dev_mouse ) ;
            _dev_ascii = ::std::move( rhv._dev_ascii ) ;

            _audio = std::move( rhv._audio ) ;

            _frequencies0 = std::move( rhv._frequencies0 ) ;
            _freq_bands = std::move( rhv._freq_bands ) ;
        }

        virtual ~test_app( void_t )
        {}

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

            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "root_render_states" ) ;


                {
                    natus::graphics::render_state_sets_t rss ;
                    
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::counter_clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = natus::graphics::fill_mode::fill ;
                    
                    rss.blend_s.do_change = true ;
                    rss.blend_s.ss.do_activate = true ;
                    rss.blend_s.ss.src_blend_factor = natus::graphics::blend_factor::src_alpha ;
                    rss.blend_s.ss.dst_blend_factor = natus::graphics::blend_factor::one_minus_src_alpha ;
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _wid_async.async().configure( _root_render_states ) ;
            }

            _imgui->init( _wid_async.async() ) ;

            _audio.configure( natus::audio::capture_type::what_u_hear, _capture ) ;

            return natus::application::result::ok ;
        }

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            natus::gfx::imgui_t::window_data_t wd ;
            wd.width = wei.w ;
            wd.height = wei.h ;
            _imgui->update( wd ) ;

            return natus::application::result::ok ;
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t )
        {
            _imgui->update( _dev_mouse ) ;
            _imgui->update( _dev_ascii ) ;

            {
                natus::device::layouts::ascii_keyboard_t ascii( _dev_ascii ) ;
                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f8 ) ==
                    natus::device::components::key_state::released )
                {
                    _fullscreen = !_fullscreen ;
                    _wid_async.window().fullscreen( _fullscreen ) ;
                }
                else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f9 ) ==
                    natus::device::components::key_state::released )
                {
                    _vsync = !_vsync ;
                    _wid_async.window().vsync( _vsync ) ;
                }
            }

            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_audio( natus::application::app_t::audio_data_in_t )
        {
            _frequencies1.resize( _frequencies0.size() ) ;
            for( size_t i = 0; i < _frequencies0.size(); ++i )
            {
                _frequencies1[ i ] = _frequencies0[ i ] ;
            }
            _audio.capture( _capture ) ;

            _capture->append_samples_to( _captured ) ;
            _capture->copy_frequencies_to( _frequencies0 ) ;
            _frequencies1.resize( _frequencies0.size() ) ;

            NATUS_PROFILING_COUNTER_HERE( "Audio Clock" ) ;

            return natus::application::result::ok ;
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t )
        {
            _wid_async.async().use( _root_render_states ) ;

            _imgui->begin() ;
            _imgui->execute( [&] ( ImGuiContext* ctx )
            {
                ImGui::SetCurrentContext( ctx ) ;

                ImGui::Begin( "Capture" ) ;

                // print wave form
                {
                    auto const mm = _capture->minmax() ;
                    ImGui::PlotLines( "Waveform", _captured.data(), (int_t)_captured.size(), 0, 0, mm.x(), mm.y(), ImVec2( ImGui::GetWindowWidth() - 100.0f , 100.0f ) );
                }

                // print frequencies
                {
                    float_t max_value = std::numeric_limits<float_t>::min() ;
                    for( size_t i = 0 ; i < _frequencies0.size(); ++i )
                        max_value = std::max( _frequencies0[ i ], max_value ) ;

                    static float_t smax_value = 0.0f ;
                    float_t const mm = ( max_value + smax_value ) * 0.5f;
                    ImGui::PlotHistogram( "Frequencies", _frequencies0.data(), (int_t)_frequencies0.size()/4, 0, 0, 0.0f, mm, ImVec2( ImGui::GetWindowWidth() - 100.0f , 100.0f ) );
                    smax_value = max_value ;
                }

                // tried some sort of peaking, but sucks.
                if( _frequencies0.size() > 0 )
                {
                    natus::ntd::vector< float_t > difs( _frequencies0.size() ) ;
                    for( size_t i = 0; i < _frequencies0.size(); ++i )
                    {
                        difs[ i ] = _frequencies0[ i ] - _frequencies1[ i ] ;
                        difs[ i ] = difs[ i ] < 0.00001f ? 0.0f : difs[ i ] ;
                    }

                    float_t max_value = std::numeric_limits<float_t>::min() ;
                    for( size_t i = 0; i < 30/*difs.size()*/; ++i )
                        max_value = std::max( difs[ i ], max_value ) ;

                    static float_t smax_value = 0.0f ;
                    float_t const mm = ( max_value + smax_value ) * 0.5f;
                    ImGui::PlotHistogram( "Difs", difs.data(), 30/*difs.size()*/, 0, 0, 0.0f, mm, ImVec2( ImGui::GetWindowWidth()-100.0f , 100.0f ) );
                    smax_value = max_value ;
                }
                
                ImGui::End() ;
            } ) ;

            _imgui->render( _wid_async.async() ) ;

            _wid_async.async().use( natus::graphics::state_object_t() ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ;
        }

        virtual natus::application::result on_shutdown( void_t )
        {
            return natus::application::result::ok ;
        }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char** argv )
{
    natus::application::global_t::create_application(
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;

    natus::memory::global_t::dump_to_std() ;

    return 0 ;
}
