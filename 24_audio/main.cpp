
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
// Generate Sound and Play
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

        natus::audio::result_res_t _play_res = natus::audio::result_t(natus::audio::result::initial) ;
        natus::audio::buffer_object_res_t _play = natus::audio::buffer_object_t() ;

        natus::audio::execution_options _eo = natus::audio::execution_options::undefined ;

    public:

        test_app( void_t )
        {
            natus::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "Generate Some Audio", wi ) ;
            _wid_async.first.fullscreen( _fullscreen ) ;
            _wid_async.first.vsync( _vsync ) ;

            _imgui = natus::gfx::imgui_res_t( natus::gfx::imgui_t() ) ;

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

            _play = std::move( rhv._play ) ;
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

            if( !_dev_mouse.is_valid() ) natus::log::global_t::status( "no three mouse found" ) ;
            if( !_dev_ascii.is_valid() ) natus::log::global_t::status( "no ascii keyboard found" ) ;

            _imgui->init( _wid_async.second ) ;

            //
            // prepare the audio buffer for playing
            //
            {
                _play = natus::audio::buffer_object_res_t( natus::audio::buffer_object_t( "gen.sine" ) ) ;
                
                natus::audio::channels channels = natus::audio::channels::stereo ;

                size_t const sample_rate = 96000 ;
                size_t const num_channels = natus::audio::to_number( channels ) ;
                size_t const num_seconds = 4 ;

                size_t const freq = 100 ;

                {
                    natus::ntd::vector< float_t > floats( sample_rate * num_seconds * num_channels ) ;

                    for( size_t i = 0; i < floats.size()/num_channels; ++i )
                    {
                        size_t const idx = i * num_channels ;
                        double_t const a = 0.3f ;
                        double_t const f = natus::math::fn<double_t>::mod( 
                            double_t( i ) / double_t( sample_rate-num_channels  ), 1.0 ) ;
                        
                        double_t const value = a * std::sin( freq * f * 2.0 * natus::math::constants<double_t>::pi() ) ;
                        
                        floats[ idx + 0 ] = float_t( value ) ;
                        floats[ idx + 1 ] = float_t( value ) ;
                    }
                    _play->set_samples( channels, sample_rate, floats ) ;
                }
                _audio.configure( _play ) ;
                _eo = natus::audio::execution_options::play ;
            }
            
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
                    _wid_async.first.fullscreen( _fullscreen ) ;
                }
                else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f9 ) ==
                    natus::device::components::key_state::released )
                {
                    _vsync = !_vsync ;
                    _wid_async.first.vsync( _vsync ) ;
                }
            }

            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_audio( natus::application::app_t::audio_data_in_t )
        {
            if( *_play_res == natus::audio::result::initial )
            {
                natus::audio::backend::execute_detail ed ;
                ed.to = _eo ;
                _audio.execute( _play, ed, _play_res ) ;
            }
            else if( *_play_res == natus::audio::result::ok )
            {
                natus::log::global_t::status("status ok") ;
                *_play_res = natus::audio::result::idle ;
            }

            NATUS_PROFILING_COUNTER_HERE( "Audio Clock" ) ;

            return natus::application::result::ok ;
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t )
        {
            _imgui->begin() ;
            _imgui->execute( [&] ( ImGuiContext* ctx )
            {
                ImGui::SetCurrentContext( ctx ) ;

                ImGui::SetNextWindowSize( ImVec2( 200, 200 ) ) ;
                ImGui::Begin( "Audio" ) ;

                if( ImGui::Button( "Play" ) )
                {
                    *_play_res = natus::audio::result::initial ;
                    _eo = natus::audio::execution_options::play ;
                }
                else if( ImGui::Button( "Stop" ) )
                {
                    *_play_res = natus::audio::result::initial ;
                    _eo = natus::audio::execution_options::stop ;
                }

                ImGui::End() ;
            } ) ;

            _imgui->render( _wid_async.second ) ;

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
