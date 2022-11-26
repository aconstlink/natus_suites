

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/tool/imgui/custom_widgets.h>

#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/degree.hpp>
#include <natus/math/spline/linear_bezier_spline.hpp>
#include <natus/math/spline/quadratic_bezier_spline.hpp>
#include <natus/math/spline/cubic_bezier_spline.hpp>
#include <natus/math/spline/cubic_hermit_spline.hpp>

#include <natus/profile/macros.h>

#include <random>
#include <thread>

// this tests uses(copy) what has been done in the last test
// and adds textured quads to each region.
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

        virtual ~test_app( void_t ) {}

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
            auto lr3 = _ae.get_line_render_3d() ;
            
            natus::math::vec3f_t off( 0.0f, 0.0f, 0.0f) ;

            // draw splines #1
            {
                off += natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ;

                typedef natus::math::quadratic_bezier_spline<natus::math::vec3f_t> spline_t ;

                spline_t s( {
                    natus::math::vec3f_t(-50.0f, 0.0f, 0.0f) + off,
                    natus::math::vec3f_t(-25.0f, 50.0f, 0.0f) + off,
                    natus::math::vec3f_t(-0.0f, 50.0f, 0.0f) + off,
                    natus::math::vec3f_t(50.0f, 0.0f, -0.0f) + off,
                    natus::math::vec3f_t(100.0f, -50.0f, 0.0f) + off,
                    natus::math::vec3f_t(150.0f, 0.0f, 0.0f) + off,
                    natus::math::vec3f_t(200.0f, -100.0f, 0.0f) + off,
                    natus::math::vec3f_t(250.0f, 0.0f, 0.0f) + off
                }, spline_t::init_type::construct  ) ;
                
                static size_t change_idx = 1 ;

                // change control point
                {
                    static float_t t = 0.0f ;
                    t += rd.sec_dt * 0.5f  ;
                    if( t > 1.0f ) 
                    {
                        change_idx = ++change_idx < s.ncp() ? change_idx : 0 ;
                        t = 0.0f ;
                    }

                    float_t f = t * natus::math::constants<float_t>::pix2() ;
                    natus::math::vec3f_t dif( 
                        natus::math::fn<float_t>::cos( f ), 
                        natus::math::fn<float_t>::sin( f ) * natus::math::fn<float_t>::cos( f ), 
                        natus::math::fn<float_t>::sin( f ) ) ;
                    
                    auto const cp = s.get_control_point( change_idx ) + dif * 30.0f ;
                    s.change_point( change_idx, cp ) ;
                }


                natus::math::linear_bezier_spline<natus::math::vec3f_t> ls = s.control_points() ;

                size_t const samples = 100 ;
                for( size_t i=0; i<(samples>>1); ++i )
                {
                    float_t const frac0 = float_t((i<<1)+0) / float_t(samples-1) ;
                    float_t const frac1 = float_t((i<<1)+1) / float_t(samples-1) ;

                    {
                        auto const p0 = s( frac0 ) ;
                        auto const p1 = s( frac1 ) ;

                        lr3->draw( p0, p1, natus::math::vec4f_t( 1.0f, 1.0f, 0.4f, 1.0f )  ) ;
                    }

                    {
                        auto const p0 = ls( frac0 ) ;
                        auto const p1 = ls( frac1 ) ;

                        lr3->draw( p0, p1, natus::math::vec4f_t( 1.0f, 0.5f, 0.4f, 1.0f )  ) ;
                    }
                }

                {
                    s.for_each_control_point( [&]( size_t const i, natus::math::vec3f_cref_t p )
                    {
                        auto const r = natus::math::m3d::trafof_t::rotation_by_axis( natus::math::vec3f_t(1.0f,0.0f,0.0f), 
                                natus::math::angle<float_t>::degree_to_radian(90.0f) ) ;

                        natus::math::vec4f_t color( 1.0f ) ;
                        if( i == change_idx ) color = natus::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ;

                        pr3->draw_circle( 
                            ( _ae.get_camera_0()->get_transformation()  ).get_rotation_matrix(), p, 
                            2.0f, color, color, 20 ) ;
                    } ) ;
                }
            }

            // draw splines #2
            {
                off += natus::math::vec3f_t( 0.0f, 150.0f, 0.0f ) ;

                typedef natus::math::cubic_hermit_spline<natus::math::vec3f_t> spline_t ;

                spline_t::init_by_catmull_rom_t const id { spline_t::points_t {
                    natus::math::vec3f_t(-50.0f, 0.0f, 0.0f) + off,
                    natus::math::vec3f_t(-25.0f, 50.0f, 0.0f) + off,
                    natus::math::vec3f_t(-0.0f, 0.0f, 0.0f) + off,
                    natus::math::vec3f_t(50.0f, 0.0f, -0.0f) + off,
                    natus::math::vec3f_t(100.0f, -50.0f, 0.0f) + off,
                    natus::math::vec3f_t(150.0f, 0.0f, 0.0f) + off,
                    natus::math::vec3f_t(200.0f, -100.0f, 0.0f) + off,
                    natus::math::vec3f_t(250.0f, 0.0f, 0.0f) + off }
                } ;

                spline_t s( id ) ;
                
                static size_t change_idx = 1 ;
                
                // change control point
                {
                    static float_t t = 0.0f ;
                    t += rd.sec_dt * 0.5f  ;
                    if( t > 1.0f ) 
                    {
                        change_idx = ++change_idx < s.ncp() ? change_idx : 0 ;
                        t = 0.0f ;
                    }

                    float_t f = t * natus::math::constants<float_t>::pix2() ;
                    natus::math::vec3f_t dif = natus::math::vec3f_t( 
                        natus::math::fn<float_t>::cos( f ), 
                        natus::math::fn<float_t>::sin( f ), 
                        natus::math::fn<float_t>::cos( f ) ).abs() ;
                    
                    auto cp = s.get_control_point( change_idx );
                    

                    if( change_idx % 2 == 0 )
                    {
                        auto const p = cp.p + dif * 30.0f ;
                        cp.p = p ;
                    }
                    else 
                    {
                        cp.lt = cp.lt + dif * 50.0f ;
                        cp.rt = cp.lt ;
                    }

                    s.change_control_point( change_idx, cp ) ;
                    
                }

                natus::math::linear_bezier_spline<natus::math::vec3f_t> ls = s.control_points() ;

                size_t const samples = 100 ;
                for( size_t i=0; i<(samples>>1); ++i )
                {
                    float_t const frac0 = float_t((i<<1)+0) / float_t(samples-1) ;
                    float_t const frac1 = float_t((i<<1)+1) / float_t(samples-1) ;

                    {
                        auto const p0 = s( frac0 ) ;
                        auto const p1 = s( frac1 ) ;

                        lr3->draw( p0, p1, natus::math::vec4f_t( 1.0f, 1.0f, 0.4f, 1.0f )  ) ;
                    }

                    {
                        auto const p0 = ls( frac0 ) ;
                        auto const p1 = ls( frac1 ) ;

                        lr3->draw( p0, p1, natus::math::vec4f_t( 1.0f, 0.5f, 0.4f, 1.0f )  ) ;
                    }
                }

                {
                    s.for_each_control_point( [&]( size_t const i, spline_t::control_point_cref_t p )
                    {
                        natus::math::vec4f_t color( 1.0f ) ;
                        if( i == change_idx ) color = natus::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ;

                        _ae.get_prim_render_3d()->draw_circle( 
                            ( _ae.get_camera_0()->get_transformation()  ).get_rotation_matrix(), p.p, 
                            2.0f, color, color, 20 ) ;

                        {
                            auto const p0 = p.p ;
                            auto const p1 = p.p - p.lt.normalized() * 10.0f ;
                            lr3->draw( p0, p1, natus::math::vec4f_t( 1.0f, 0.5f, 0.4f, 1.0f )  ) ;
                            pr3->draw_circle( 
                                ( _ae.get_camera_0()->get_transformation()  ).get_rotation_matrix(), p1, 
                                1.0f, color, color, 20 ) ;
                        }

                        {
                            auto const p0 = p.p ;
                            auto const p1 = p.p + p.rt.normalized() * 10.0f ;
                            lr3->draw( p0, p1 , natus::math::vec4f_t( 1.0f, 0.5f, 0.4f, 1.0f )  ) ;
                            pr3->draw_circle( 
                                ( _ae.get_camera_0()->get_transformation()  ).get_rotation_matrix(), p1, 
                                1.0f, color, color, 20 ) ;
                        }
                    } ) ;
                }
            }

            _ae.on_graphics_end( 100 ) ;
                       

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Control and Info" ) ;
            
            {
                ImGui::Text( "mx: %f, my: %f", _ae.get_cur_mouse_pos().x(), _ae.get_cur_mouse_pos().y() ) ;
                //_cur_mouse
            }

            ImGui::End() ;

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
