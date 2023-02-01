

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/tool/imgui/custom_widgets.h>

#include <natus/profile/macros.h>

#include <natus/math/utility/angle.hpp>

#include <random>
#include <thread>

// this test uses the mouse, generates a ray from it and does object 
// picking with about 100 2d objects in a 3d scene.
namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;
        natus::ntd::vector< natus::math::vec2f_t > _points ;

    private:

        float_t _fov = natus::math::angle<float_t>::degree_to_radian( 45.0f ) ;
        float_t _far = 1000.0f ;
        float_t _near = 0.1f ;
        float_t _aspect = 1.0f ;

        bool_t _update_camera = true ;

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
        test_app( this_rref_t rhv ) noexcept : app( std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _aspect = float_t(wei.h) / float_t(wei.w) ;
            _update_camera = true ;

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

            

            {
                srand( uint_t( time(NULL) ) ) ;
                for( size_t i=0; i<100; ++i )
                {
                    float_t const x = float_t((rand() % 100)-50)/50.0f ;
                    float_t const y = float_t((rand() % 100)-50)/50.0f ;

                    _points.emplace_back( natus::math::vec2f_t( x, y ) ) ;
                }
            }

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

            float_t const radius = 3.0f ;

            // draw points
            {
                for( size_t i=0; i<_points.size(); ++i )
                {
                    auto const p = _points[i] * _ae.get_window_dims() ;
                    _ae.get_prim_render()->draw_circle( 3, 10, p, radius, 
                        natus::math::vec4f_t(1.0f), natus::math::vec4f_t(1.0f) ) ;
                }
            }

            // test pick points
            {
                auto const ray = _ae.get_camera_0()->get_camera()->create_ray_norm( _ae.get_cur_mouse_pos_nrm() ) ;
                auto const plane = natus::math::vec3f_t(0.0f,0.0f,-1.0f) ;
                float_t const lambda = - ray.get_origin().dot( plane ) / ray.get_direction().dot( plane ) ;

                // point on plane
                natus::math::vec2f_t const pop = ray.point_at( lambda ).xy() ;
                //_pr->draw_circle( 4, 10, pop, radius, 
                 //           natus::math::vec4f_t(1.0f,1.0f,0.0f,1.0f), natus::math::vec4f_t(1.0f) ) ;

                for( size_t i=0; i<_points.size(); ++i )
                {
                    auto const p = _points[i] * _ae.get_window_dims() ;
                    auto const b = (p - pop).length() < radius*1.0f ;  
                    if( b )
                    {
                        _ae.get_prim_render()->draw_circle( 4, 10, p, radius, 
                            natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f), natus::math::vec4f_t(1.0f) ) ;
                    }
                }
            }

            #if 0

            // test ray generation
            {
                auto const ray = _camera_0.get_camera()->create_ray_norm( natus::math::vec2f_t() ) ;

                _pr3->draw_circle( 
                    _camera_0.get_transformation().get_transformation(), 
                    ray.get_origin() + ray.get_direction() * 200.0f, 3.0f, 
                    natus::math::vec4f_t(1), natus::math::vec4f_t(1), 10 ) ;
            }

            {
                auto const ray = _camera_0.get_camera()->create_ray_norm( natus::math::vec2f_t(-1.0f) ) ;

                _pr3->draw_circle( 
                    _camera_0.get_transformation().get_transformation(), 
                    ray.get_origin() + ray.get_direction() * 200.0f, 3.0f, 
                    natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f), natus::math::vec4f_t(1), 10 ) ;
            }

            {
                auto const ray = _camera_0.get_camera()->create_ray_norm( natus::math::vec2f_t(1.0f) ) ;

                _pr3->draw_circle( 
                    _camera_0.get_transformation().get_transformation(), 
                    ray.get_origin() + ray.get_direction() * 200.0f, 3.0f, 
                    natus::math::vec4f_t(0.0f,1.0f,0.0f,1.0f), natus::math::vec4f_t(1), 10 ) ;
            }
            
            {
                auto const ray = _camera_0.get_camera()->create_ray_norm( _cur_mouse_nrm ) ;

                _pr3->draw_circle( 
                    _camera_0.get_transformation().get_transformation(), 
                    ray.get_origin() + ray.get_direction() * 200.0f, 3.0f, 
                    natus::math::vec4f_t(0.0f,1.0f,0.0f,1.0f), natus::math::vec4f_t(1), 10 ) ;
            }

            #endif

            _ae.on_graphics_end( 100 ) ;
            

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Control" ) ;
            
            {
                {
                    bool_t b = _ae.get_camera_0()->is_perspective() ;
                    if( ImGui::Checkbox( "Perspective", &b ) && !_ae.get_camera_0()->is_perspective() )
                    {
                        _ae.get_camera_0()->perspective_fov( _fov ) ;
                    }
                }

                {
                    bool_t b = _ae.get_camera_0()->is_orthographic() ;
                    if( ImGui::Checkbox( "Orthographic", &b ) && !_ae.get_camera_0()->is_orthographic() )
                    {
                        _ae.get_camera_0()->orthographic() ;
                    }
                }

                if( _ae.get_camera_0()->is_perspective() )
                {
                    float_t tmp = _aspect ;
                    ImGui::SliderFloat( "Aspect", &tmp, 0.0f, 5.0f ) ;
                    ImGui::SliderFloat( "Field of View", &_fov, 0.0f, natus::math::constants<float_t>::pi() ) ;
                    ImGui::SliderFloat( "Near", &_near, 0.0f, _far ) ;
                    ImGui::SliderFloat( "Far", &_far, _near, 10000.0f ) ;

                    _ae.get_camera_0()->set_near_far( _near, _far ) ;
                }
                else if( _ae.get_camera_0()->is_orthographic() )
                {
                    ImGui::SliderFloat( "Near", &_near, 0.0f, _far ) ;
                    ImGui::SliderFloat( "Far", &_far, _near, 10000.0f ) ;
                }

                if( _ae.get_camera_0()->is_perspective() )
                {
                    _ae.get_camera_0()->perspective_fov( _fov ) ;
                }
                else if( _ae.get_camera_0()->is_orthographic() )
                {
                    _ae.get_camera_0()->orthographic() ;
                }

                _update_camera = false ;
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
    return natus::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}
