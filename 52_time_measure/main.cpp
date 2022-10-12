

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/geometry/3d/cube.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>

#include <natus/nsl/parser.h>
#include <natus/nsl/database.hpp>
#include <natus/nsl/dependency_resolver.hpp>
#include <natus/nsl/generator_structs.hpp>

#include <natus/tool/imgui/custom_widgets.h>

#include <natus/profile/macros.h>

#include <random>
#include <thread>

//
namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

    private:

        natus::application::util::app_essentials_t _ae ;

    private: // geometry
    public:

        test_app( void_t ) noexcept 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            auto view1 = this_t::create_window( "A Render Window Default", wi ) ;
            auto view2 = this_t::create_window( "A Render Window Additional", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _ae = natus::application::util::app_essentials_t( 
                natus::graphics::async_views_t( { view1.async(), view2.async() } ) ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
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


        //*****************************************************************************
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

        //*****************************************************************************
        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_audio( audio_data_in_t ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Audio Clock" ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            return natus::application::result::ok ; 
        }

        // for time measuring
        float_t _max_value = 1.0f ;
        std::array< float_t, 200 > _data_items ;
        void_t add_data_item( float_t  const d ) noexcept
        {
            _max_value = 0.0f ;
            for( size_t i=1; i<_data_items.size(); ++i )
            {
                _data_items[i-1] = _data_items[i] ;
                _max_value = std::max( _max_value, _data_items[i] ) ;
            }
            _data_items[_data_items.size()-1] = d ;
            _max_value = std::max( _max_value, d ) ;
            _max_value *= 1.5f ;
        }

        //*****************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            // change properties of cubes
            {
                static __clock_t::time_point tp = __clock_t::now() ;

                float_t const dt = float_t ( double_t( std::chrono::duration_cast< std::chrono::milliseconds >( __clock_t::now() - tp ).count() ) / 1000.0 ) ;
                tp = __clock_t::now() ;

                this_t::add_data_item( dt ) ;
            }

            _ae.on_graphics_begin( rd ) ;
            _ae.on_graphics_end( 100 ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            if( ImGui::Begin("Time Measure Control") )
            {
                // display data items
                {
                    ImGui::PlotHistogram("Histogram", _data_items.data(), _data_items.size(), 0, NULL, 0.0f, _max_value, ImVec2(0, 80.0f));
                }
            }

            ImGui::End() ;
        }

        virtual natus::application::result on_shutdown( void_t ) noexcept 
        { 
            _ae.on_shutdown() ;
            return natus::application::result::ok ; 
        }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}
