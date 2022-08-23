

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/primitive/primitive_render_2d.h> 

#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profile/macros.h>

#include <natus/physics/particle_system.h>
#include <natus/physics/force_fields.hpp>

#include <natus/geometry/mesh/polygon_mesh.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>
#include <natus/geometry/3d/tetra.h>
#include <natus/math/utility/fn.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/3d/transformation.hpp>

#include <random>
#include <thread>



namespace this_file
{
    using namespace natus::core ;

    struct test_particle_effect
    {
        natus_this_typedefs( test_particle_effect ) ;

        natus::physics::particle_system_t flakes ;
        natus::physics::emitter_res_t current_emitter ;
        natus::physics::radial_emitter_res_t emitter ;
        natus::physics::line_emitter_res_t lemitter ;
        natus::physics::acceleration_field_res_t g ;
        natus::physics::friction_force_field_res_t friction ;
        natus::physics::viscosity_force_field_res_t viscosity ;
        natus::physics::sin_velocity_field_res_t wind ;

        test_particle_effect( void_t ) 
        {
            g = natus::physics::acceleration_field_t( natus::math::vec2f_t( 0.0f, -9.81f ) ) ;
            friction = natus::physics::friction_force_field_t() ;
            viscosity = natus::physics::viscosity_force_field_t( 0.01f ) ;
            wind = natus::physics::sin_velocity_field_t( 1.1f, 10.1f, 0.0f ) ;

            //flakes.attach_force_field( g ) ;
            //flakes.attach_force_field( viscosity ) ;
            flakes.attach_force_field( wind ) ;


            {
                emitter = natus::physics::radial_emitter_t() ;
                emitter->set_mass( 1.0049f )  ;
                emitter->set_age( 2.5f ) ;
                emitter->set_amount( 10 ) ;
                emitter->set_rate( 2.0f ) ;
                emitter->set_angle( 0.3f ) ;
                emitter->set_velocity( 200.0f ) ;
                emitter->set_direction( natus::math::vec2f_t( 0.0f,1.0f ) ) ;
            }

            {
                lemitter = natus::physics::line_emitter_t() ;
                lemitter->set_mass( 1.0049f ) ;
                lemitter->set_age( 2.0f ) ;
                lemitter->set_amount( 10 ) ;
                lemitter->set_rate( 2.0f ) ;
                lemitter->set_velocity( 200.0f ) ;
                lemitter->set_direction( natus::math::vec2f_t( 0.0f,1.0f ) ) ;
            }

            current_emitter = emitter ;
            flakes.attach_emitter( current_emitter ) ;
        }

        test_particle_effect( this_cref_t ) = delete ;
        test_particle_effect( this_rref_t rhv ) noexcept
        {
            flakes = std::move( rhv.flakes ) ;
            emitter = std::move( rhv.emitter ) ;
            lemitter = std::move( rhv.lemitter ) ;
            wind = std::move( rhv.wind ) ;
            g = std::move( rhv.g ) ;
            current_emitter = std::move( rhv.current_emitter ) ;
        }

        this_ref_t operator = ( this_rref_t rhv ) noexcept 
        {
            flakes = std::move( rhv.flakes ) ;
            emitter = std::move( rhv.emitter ) ;
            lemitter = std::move( rhv.lemitter ) ;
            wind = std::move( rhv.wind ) ;
            g = std::move( rhv.g ) ;
            current_emitter = std::move( rhv.current_emitter ) ;
            return *this ;
        }

        

        void_t update( float_t const dt ) noexcept
        {
            flakes.update( dt ) ;
        }

        void_t render( natus::gfx::primitive_render_2d_res_t pr )
        {
            flakes.on_particles( [&]( natus::ntd::vector< natus::physics::particle_t > const & particles )
            {
                size_t i=0 ;

                natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>( 0, particles.size() ),
                    [&]( natus::concurrent::range_1d<size_t> const & r )
                {
                    for( size_t i=r.begin(); i<r.end(); ++i )
                    {
                        auto & p = particles[i] ;

                        #if 0
                        pr->draw_circle( 4, 10, p.pos, p.mass, 
                            natus::math::vec4f_t(1.0f), 
                            natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f) ) ;
                        #elif 1
                        natus::math::vec2f_t const pos = p.pos ;
                        float_t const s = p.mass * 1.0f;

                        natus::math::vec2f_t points[] = 
                        {
                            pos + natus::math::vec2f_t(-s, -s), 
                            pos + natus::math::vec2f_t(-s, +s),  
                            pos + natus::math::vec2f_t(+s, +s),
                            pos + natus::math::vec2f_t(+s, -s)
                        } ;
                        float_t const life = p.age / emitter->get_age() ;
                        float_t const alpha = natus::math::fn<float_t>::smooth_pulse( life, 0.1f, 0.7f ) ;
                        pr->draw_rect( i % 50, 
                            points[0], points[1], points[2], points[3],
                            natus::math::vec4f_t(0.0f, 0.0f, 0.5f, alpha), 
                            natus::math::vec4f_t(1.0f,0.0f,0.0f,alpha) ) ;
                        #endif
                    }
                } ) ;

                
            } ) ;
        }
    } ;
    natus_typedef( test_particle_effect ) ;

    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::graphics::async_views_t _graphics ;

        natus::graphics::state_object_res_t _root_render_states ;
                
        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        bool_t _do_tool = true ;
        
    private: // particle system

        
        natus::gfx::primitive_render_2d_res_t _pr ;

        this_file::test_particle_effect_t _spe ;

        bool_t _draw_debug = false ;

    private:

        natus::math::vec2f_t _extend = natus::math::vec2f_t( 100, 100 ) ;

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
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
            _graphics = natus::graphics::async_views_t( { view1.async() } ) ;
            #endif
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _camera_0 = std::move( rhv._camera_0 ) ;
            _graphics = std::move( rhv._graphics ) ;
            _spe = std::move( rhv._spe ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei ) noexcept
        {
            _camera_0.set_dims( float_t(wei.w), float_t(wei.h), 0.1f, 1000.0f ) ;
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ) ) ;

            natus::math::vec2f_t const target = natus::math::vec2f_t(800, 600) ; 
            natus::math::vec2f_t const window = natus::math::vec2f_t( float_t(wei.w), float_t(wei.h) ) ;

            natus::math::vec2f_t const ratio = window / target ;

            _camera_0.orthographic() ;

            _extend = target * (ratio.x() < ratio.y() ? ratio.xx() : ratio.yy()) ;

            return natus::application::result::ok ;
        }

    private:

        virtual natus::application::result on_init( void_t ) noexcept
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
                _camera_0.look_at( natus::math::vec3f_t( 0.0f, 0.0f, -10.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }

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

                   
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }
            
            // prepare primitive
            {
                _pr = natus::gfx::primitive_render_2d_res_t( natus::gfx::primitive_render_2d_t() ) ;
                _pr->init( "particle_prim_render", _graphics ) ;
            }

            // init particles
            {
               
            }
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_device( device_data_in_t ) noexcept 
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

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            

            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            //std::this_thread::sleep_for( std::chrono::milliseconds(5) ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            // do update all particle effects
            {
                _spe.update( ud.sec_dt ) ;
            }

            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t  ) noexcept 
        {
            _pr->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;

            // draw particle effects
            {
                _spe.render( _pr ) ;

                if( _draw_debug )
                {
                    _pr->draw_circle( 0, 20, _spe.emitter->get_position(), _spe.emitter->get_radius(), 
                        natus::math::vec4f_t(0.0f, 0.5f,0.0f,0.1f),  natus::math::vec4f_t(0.0f,0.5f,0.0f,0.1f)) ;
                
                    {
                        auto const points = _spe.flakes.get_extend_rect() ;
                        _pr->draw_rect( 0, points[0], points[1], points[2], points[3], 
                            natus::math::vec4f_t(0.0f), 
                            natus::math::vec4f_t(0.5f) ) ;
                    }
                }
            }

            // draw extend of aspect
            if( _draw_debug )
            {
                natus::math::vec2f_t p0 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(-0.5f,-0.5f) ;
                natus::math::vec2f_t p1 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(-0.5f,+0.5f) ;
                natus::math::vec2f_t p2 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(+0.5f,+0.5f) ;
                natus::math::vec2f_t p3 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(+0.5f,-0.5f) ;

                natus::math::vec4f_t color0( 1.0f, 1.0f, 1.0f, 0.0f ) ;
                natus::math::vec4f_t color1( 1.0f, 1.0f, 1.0f, 1.0f ) ;

                _pr->draw_rect( 50, p0, p1, p2, p3, color0, color1 ) ;
            }

            // draw particles extend
            {

            }

            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

            // render all
            {
                _pr->prepare_for_rendering() ;
                for( size_t i=0; i<100+1; ++i )
                {
                    _pr->render( i ) ;
                }
            }
            
           
            
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
            }

            

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui ) noexcept
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "View Control" ) ;
            {
                float_t data[2] = {_extend.x(), _extend.y() } ;
                ImGui::SliderFloat2( "Extend", data, 0.0f, 1000.0f, "%f" ) ;
                _extend.x( data[0] ) ; _extend.y( data[1] ) ;
            }
            ImGui::End() ;

            ImGui::Begin( "Control Particle System" ) ;

            static int item_current = 0 ;
            bool_t item_changed = false ;
            {
                const char* items[] = { "Radial", "Line" };
                if( item_changed = ImGui::Combo("Emitter", &item_current, items, IM_ARRAYSIZE(items) ) )
                {
                    _spe.flakes.detach_emitter( _spe.current_emitter ) ;
                }
            }

            {
                ImGui::Checkbox( "Draw Debug", &_draw_debug ) ;
            }

            if( item_current == 0 )
            {
                if( item_changed ) 
                {
                    _spe.current_emitter = _spe.emitter ;
                    _spe.flakes.attach_emitter( _spe.current_emitter ) ;
                }

                {
                    float_t v = _spe.emitter->get_radius() ;
                    if( ImGui::SliderFloat("Radius", &v, 0.0f, 100.0f ) )
                    {
                        _spe.emitter->set_radius( v ) ;
                    }
                }
                {
                    float_t v = _spe.emitter->get_angle() ;
                    if( ImGui::SliderFloat("Angle", &v, 0.0f, 3.14f ) )
                    {
                        _spe.emitter->set_angle( v ) ;
                    }
                }

                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_radius_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Radius Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_radius_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_radius_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_angle_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Angle Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_angle_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_angle_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
            }
            else if( item_current == 1 )
            {
                if( item_changed ) 
                {
                    _spe.current_emitter = _spe.lemitter ;
                    _spe.flakes.attach_emitter( _spe.current_emitter ) ;
                }
                
                {
                    float_t v = _spe.lemitter->get_ortho_distance() ;
                    if( ImGui::SliderFloat("Ortho Dist", &v, -400.0f, 400.0f ) )
                    {
                        _spe.lemitter->set_ortho_distance( v ) ;
                    }
                }
                {
                    float_t v = _spe.lemitter->get_parallel_distance() ;
                    if( ImGui::SliderFloat("Parallel Dist", &v, 1.0f, 400.0f ) )
                    {
                        _spe.lemitter->set_parallel_distance( v ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_ortho_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Ortho Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_ortho_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_ortho_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_parallel_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Parallel Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_parallel_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_parallel_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
            }

            {
                {
                    auto dir = _spe.current_emitter->get_direction() ;
                    if( natus::tool::custom_imgui_widgets::direction( "dir", dir ) )
                    {
                        _spe.current_emitter->set_direction( dir ) ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.current_emitter->get_rate() ;
                    if( ImGui::VSliderFloat("Rate", ImVec2(50,100), &v, 1.0f, 100.0f ) )
                    {
                        _spe.current_emitter->set_rate( v ) ;
                        _spe.flakes.clear() ;
                    }
                }
                ImGui::SameLine() ;
                {
                    int_t v = int_t( _spe.current_emitter->get_amount() ) ;
                    if( ImGui::VSliderInt("Amount", ImVec2(50,100), &v, 1, 100 ) )
                    {
                        _spe.current_emitter->set_amount( v ) ;
                        _spe.flakes.clear() ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.current_emitter->get_age() ;
                    if( ImGui::VSliderFloat("Age", ImVec2(50,100), &v, 1.0f, 10.0f ) )
                    {
                        _spe.current_emitter->set_age( v ) ;
                    }
                }
                ImGui::SameLine() ;
                

                ImGui::SameLine() ;
                {
                    float_t v = _spe.current_emitter->get_velocity() ;
                    if( ImGui::VSliderFloat("Velocity", ImVec2(50,100), &v, 0.0f, 1000.0f ) )
                    {
                        _spe.current_emitter->set_velocity( v ) ;
                    }
                }

                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_mass_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Mass Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.current_emitter->set_mass_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.current_emitter->set_mass_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_age_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Age Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.current_emitter->set_age_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.current_emitter->set_age_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_acceleration_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Acceleration Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.current_emitter->set_acceleration_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.current_emitter->set_acceleration_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_velocity_variation_type() == natus::physics::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Velocity Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.current_emitter->set_velocity_variation_type( natus::physics::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.current_emitter->set_velocity_variation_type( natus::physics::variation_type::random ) ;
                    }
                }
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
