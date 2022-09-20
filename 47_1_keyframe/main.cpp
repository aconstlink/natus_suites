



#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/math/animation/keyframe_sequence.hpp>

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

    private:

        natus::application::util::app_essentials_t _ae ;

    public:

        test_app( void_t ) 
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

        //*****************************************************************************
        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;
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
            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _ae.on_graphics_begin( rd ) ;

            auto pr3 = _ae.get_prim_render_3d() ;

            static size_t time = 0 ;
            time += rd.milli_dt ;

            // 
            {
                typedef natus::math::cubic_hermit_spline< natus::math::vec3f_t > spline_t ;
                typedef natus::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

                keyframe_sequence_t kf ;

                kf.insert( keyframe_sequence_t::keyframe_t( 0, natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 1000, natus::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 4000, natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 2000, natus::math::vec3f_t( 0.0f, 0.0f, 1.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 3000, natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ) ) ) ;

                size_t ltime = time % kf.back().get_time() ;

                natus::math::vec4f_t color = natus::math::vec4f_t( kf(ltime), 1.0f ) ;

                pr3->draw_circle( 
                    ( _ae.get_camera_0()->get_transformation()  ).get_rotation_matrix(), natus::math::vec3f_t(), 30.0f, color, color, 10 ) ;
            }

            // 
            {
                natus::math::vec3f_t const df( 100.0f ) ;

                typedef natus::math::cubic_hermit_spline< natus::math::vec3f_t > spline_t ;
                typedef natus::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

                typedef natus::math::linear_bezier_spline< float_t > splinef_t ;
                typedef natus::math::keyframe_sequence< splinef_t > keyframe_sequencef_t ;

                keyframe_sequence_t kf ;

                natus::math::vec3f_t const p0 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( -1.0f,-1.0f, 1.0f) ;
                natus::math::vec3f_t const p1 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( -1.0f,1.0f, -1.0f) ;
                natus::math::vec3f_t const p2 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( 1.0f,1.0f, 1.0f) ;
                natus::math::vec3f_t const p3 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( 1.0f,-1.0f, -1.0f) ;

                kf.insert( keyframe_sequence_t::keyframe_t( 0, p0 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 1000, p1 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 2000, p2 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 3000, p3 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 4000, p0 ) ) ;


                keyframe_sequencef_t kf2 ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 0, 30.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 1000, 50.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 2000, 20.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 3000, 60.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 10000, 10.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 11000, 30.0f ) ) ;


                size_t const ltime = time % kf.back().get_time() ;
                size_t const ltime2 = time % kf2.back().get_time() ;

                natus::math::vec4f_t color = natus::math::vec4f_t(1.0f) ;

                pr3->draw_circle( 
                    ( _ae.get_camera_0()->get_transformation()  ).get_rotation_matrix(), kf( ltime ), kf2( ltime2 ), color, color, 10 ) ;
            }

            _ae.on_graphics_end( 100 ) ;

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
