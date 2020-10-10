
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/imgui/imgui.h>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/audio/backend/oal/oal.h>

#include <natus/device/layouts/three_mouse.hpp>
#include <natus/device/global.h>

#include <natus/math/dsp/fft.hpp>

#include <thread>

//
// This file just test the direct capture of sound data through the OpenAL backend.
// In later versions, this is done differently.
//

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        natus::gfx::imgui_res_t _imgui ;

        float_t _demo_width = 10.0f ;
        float_t _demo_height = 10.0f ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _fullscreen = false ;
        bool_t _vsync = true ;

        natus::audio::oal_backend_t _audio ;

        natus::audio::capture_object_res_t _capture = natus::audio::capture_object_t(
            natus::audio::capture_type::what_u_hear, natus::audio::channels::mono ) ;

        bool_t _captured_rendered = true ;
        natus::ntd::vector< float_t > _captured ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "An Imgui Rendering Test", wi ) ;
            _wid_async.first.fullscreen( _fullscreen ) ;
            _wid_async.first.vsync( _vsync ) ;

            _imgui = natus::gfx::imgui_res_t( natus::gfx::imgui_t() ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = ::std::move( rhv._wid_async ) ;
            _imgui = ::std::move( rhv._imgui ) ;

            _dev_mouse = ::std::move( rhv._dev_mouse ) ;
            _dev_ascii = ::std::move( rhv._dev_ascii ) ;

            _audio = std::move( rhv._audio ) ;
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

            _imgui->init( _wid_async.second ) ;

            _audio.configure( _capture ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            natus::gfx::imgui_t::window_data_t wd ;
            wd.width = wei.w ;
            wd.height = wei.h ;
            _imgui->update(wd) ;

            _demo_width = float_t( wei.w ) ;
            _demo_height = float_t( wei.h ) ;
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

            

            /*{
                static natus::math::dsp::fft<float_t>::samples_t smps ;
                static natus::math::dsp::fft<float_t> fft ;

                size_t const nsmps = _sds->nsmps ;
                _sds->nsmps = 0 ;

                smps.resize( nsmps ) ;
                for( size_t i = 0; i < nsmps; ++i )
                {
                    smps[ i ] = _sds->smps[ i ] ;
                }

                _sds->fft_render.resize( fft.n() ) ;
                if( fft.update( smps[ std::slice( 0, nsmps, 1 ) ], _sds->smps_freqs ) )
                {
                    for( size_t i = 0; i < fft.n(); ++i )
                    {
                        _sds->fft_render[ i ] = ( float_t ) std::abs( _sds->smps_freqs[ i ] ) /
                            float_t( fft.n() ) ;
                    }

                    fft.sort_abs( _sds->smps_freqs[ std::slice( 0, _sds->smps_freqs.size(), 1 ) ],
                        _sds->smps_sorted ) ;

                    for( auto& item : _sds->smps_sorted )
                    {
                        item.first = item.first / float_t( fft.n() ) ;
                    }

                }
            }*/
            

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_audio( natus::application::app_t::audio_data_in_t )
        {
            _audio.capture( _capture ) ;
            if( _capture->size() > 0  )
            {
                _capture->copy_samples_to( _captured ) ;
                _captured_rendered = false ;
            }
            
            //natus::log::global_t::status( std::to_string(_captured.size()) ) ;
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_render( natus::application::app_t::render_data_in_t ) 
        {
            _imgui->begin() ;
            _imgui->execute( [&] ( ImGuiContext* ctx )
            {
                ImGui::SetCurrentContext( ctx ) ;

                
                bool_t open = true ;
                //ImGui::SetWindowSize( ImVec2( { _demo_width*0.5f, _demo_height*0.5f } ) ) ;
                ImGui::ShowDemoWindow( &open ) ;
                
                ImGui::Begin("Capture") ;

                auto const mm =  _capture->minmax() ;

                static int func_type = 0, display_count = (int)_captured.size() ;

                //float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
                //ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr), 0, 0, -1.0f, 1.0f, ImVec2( 100, 80)  );
                ImGui::PlotLines("Samples", _captured.data(), _captured.size(), 0, 0, mm.x(), mm.y(), ImVec2( ImGui::GetWindowWidth(), 100.0f) );
                
                ImGui::End() ;
            } ) ;

            _imgui->render( _wid_async.second ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_shutdown( void_t ) 
        { return natus::application::result::ok ; }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;

    natus::memory::global_t::dump_to_std() ;

    return 0 ;
}
