

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/gfx/sprite/sprite_render_2d.h>

#include <natus/profile/macros.h>

#include <natus/physics/particle_system.h>
#include <natus/physics/force_fields.hpp>

#include <natus/collide/2d/bounds/aabb.hpp>
#include <natus/collide/2d/bounds/circle.hpp>
#include <natus/collide/2d/hit_test/hit_test_aabb_circle.hpp>

#include <random>
#include <thread>

namespace this_file
{
    using namespace natus::core ;
    using namespace natus::core::types ;

    //****************************************************************************************
    class physic_property
    {
        // orientation
        
        float_t _mass = 10.0f ;
        natus::math::vec2f_t _pos ;
        natus::math::vec2f_t _vel ;
        natus::math::vec2f_t _force ;

    public:

        natus::math::vec2f_cref_t get_force( void_t ) const noexcept{ return _force ; }
        void_t set_force( natus::math::vec2f_cref_t f ) noexcept{ _force = f ; }
        void_t add_force( natus::math::vec2f_cref_t f ) noexcept{ _force += f ; }

        natus::math::vec2f_cref_t get_velocity( void_t ) const noexcept{ return _vel ; }
        void_t set_velocity( natus::math::vec2f_cref_t v ) noexcept{ _vel = v ; }

        natus::math::vec2f_cref_t get_position( void_t ) const noexcept{ return _pos ; }
        void_t set_position( natus::math::vec2f_cref_t v ) noexcept{ _pos = v ; }

        void_t reset_force( void_t ) noexcept { _force = natus::math::vec2f_t() ; }

        float_t get_mass( void_t ) const noexcept { return _mass ; }
        void_t set_mass( float_t const m ) noexcept { _mass = m ; }

        void_t integrate( float_t const dt ) noexcept
        {
            auto const a = (_force ) / _mass ;
            _vel = _vel + a * dt ;
            _pos = _pos + _vel * dt ;
        }
    };
    natus_typedef( physic_property ) ;

    //****************************************************************************************
    class shape_property
    {
    };
    natus_typedef( shape_property ) ;

    //****************************************************************************************
    class object
    {
        physic_property_t _phy_prop ;
        shape_property_t _shp_prop ;

    public:

        physic_property_cref_t get_physic( void_t ) const noexcept { return _phy_prop ; }
        physic_property_ref_t get_physic( void_t ) noexcept { return _phy_prop ; }

    };
    natus_typedef( object ) ;

    //****************************************************************************************
    // shoots objects
    class object_cannon
    {
        natus::math::vec2f_t _position ;
        natus::math::vec2f_t _direction = natus::math::vec2f_t(-100.0f, 0.0f ) ;
        float_t _mag = 10.0f ;
        float_t _mass = 10.0f ;

    public:

        natus::math::vec2f_t get_position( void_t ) const noexcept
        {
            return _position ;
        }

        natus::math::vec2f_t get_direction( void_t ) const noexcept
        {
            return _direction ;
        }

        natus::math::vec2f_t gen_velocity( void_t ) const noexcept
        {
            return _direction ;
        }

        natus::math::vec2f_t gen_force( void_t ) const noexcept
        {
            return _direction.normalized() * _mag ;
        }

    public:

        float_t get_mass( void_t ) const noexcept { return _mass ; }
        void_t set_mass( float_t const m ) noexcept { _mass = m ;}

        void_t set_direction( natus::math::vec2f_cref_t dir ) noexcept
        {
            _direction = dir ;
        }

        void_t set_position( natus::math::vec2f_cref_t pos ) noexcept
        {
            _position = pos ;
        }

        void_t add_magnitude( float_t const f ) noexcept { _mag += f ; }
        void_t set_magnitude( float_t const f ) noexcept { _mag = f ; }

    public:

        this_file::object_t gen_object( void_t ) const noexcept
        {
            auto const o = this_file::object_t() ;

            return o ;
        }
    };
    natus_typedef( object_cannon ) ;

    //****************************************************************************************
    class ground_object
    {
    public:
        natus_typedefs( natus::collide::n2d::aabb<float_t>, box ) ;

    private:

        box_t _bound ;

    public:

        box_t get_box( void_t ) const noexcept { return _bound ; }
        void_t set_box( box_cref_t b ) noexcept { _bound = b ; }

    };
    natus_typedef( ground_object ) ;

    //****************************************************************************************
    //
    //
    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;

    private:

        object_cannon_t _obj_cannon ;
        natus::ntd::vector< ground_object_t > _ground ;
        natus::ntd::vector< object_t > _objects ;
        natus::collide::n2d::aabbf_t _bound ;
        
        // pixel per meter
        float_t _ppm = 50.0f ;

    public:

        test_app( void_t ) noexcept
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
        virtual ~test_app( void_t ) noexcept {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;
            _ae.get_camera_0()->orthographic() ;
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
            _ae.enable_mouse_control( false ) ;

            // init ground obstacles
            {
                {
                    this_file::ground_object_t go ;
                    auto const b = this_file::ground_object_t::box_t( natus::math::vec2f_t(-400.0f, -500.0f), natus::math::vec2f_t(500.0f, -300.0f) );
                    go.set_box( b ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(-400.0f, -400.0f), natus::math::vec2f_t(-350.0f, 400.0f) ) ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(-120.0f, -120.0f), natus::math::vec2f_t( -90.0f, -90.0f) ) ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(-400.0f, 200.0f), natus::math::vec2f_t( -50.0f, 230.0f) ) ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(100.0f, -400.0f), natus::math::vec2f_t( 250.0f, -100.0f) ) ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(400.0f, -400.0f), natus::math::vec2f_t( 450.0f, 400.0f) ) ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(0.0f, -300.0f), natus::math::vec2f_t( 50.0f, -200.0f) ) ) ;
                    _ground.emplace_back( go ) ;
                }
            }

            {
                _bound = natus::collide::n2d::aabbf_t( natus::math::vec2f_t(-1000.0f), natus::math::vec2f_t(1000.0f) ) ;
            }

            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;

            // mouse
            {
                natus::device::layouts::three_mouse_t mouse( _ae.get_mouse_dev() ) ;

                if( mouse.is_pressing( natus::device::layouts::three_mouse::button::right ) )
                {
                    _obj_cannon.set_position( _ae.get_cur_mouse_pos() ) ;
                }

                if( mouse.is_pressing( natus::device::layouts::three_mouse::button::left ) )
                {
                    _obj_cannon.set_direction( _ae.get_cur_mouse_pos() - _obj_cannon.get_position() ) ;
                }
            }

            // keyboard
            {
                natus::device::layouts::ascii_keyboard_t keyboard( _ae.get_ascii_dev() ) ;
                
                if( keyboard.get_state( natus::device::layouts::ascii_keyboard::ascii_key::space ) == 
                    natus::device::components::key_state::pressing )
                {
                    _obj_cannon.add_magnitude( 100.0f ) ;
                }
                else if( keyboard.get_state( natus::device::layouts::ascii_keyboard::ascii_key::space ) == 
                    natus::device::components::key_state::released )
                {
                    // shoot object
                    this_file::object_t obj ;
                    obj.get_physic().set_force( _obj_cannon.gen_force() * _obj_cannon.get_mass() ) ;
                    obj.get_physic().set_position( _obj_cannon.get_position() / _ppm ) ;
                    obj.get_physic().set_mass( _obj_cannon.get_mass() ) ;
                    _objects.emplace_back( obj ) ;

                    _obj_cannon.set_magnitude( 10.0f ) ;
                }
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            // apply base force
            // e.g. gravity, wind, air friction
            {
                for( auto & o : _objects )
                {
                    auto & phy = o.get_physic() ;
                    phy.add_force( natus::math::vec2f_t( 0.0f, -9.81f * phy.get_mass() ) ) ;
                }
            }

            // apply collision force
            // e.g friction, drag, impluse, rotation
            {
            }

            // integrate
            {
                float_t const dt = ud.sec_dt ;
                for( auto & o : _objects )
                {
                    auto & phy = o.get_physic() ;
                    phy.integrate( dt ) ;
                }
            }

            // check bounds
            {
                for( size_t i=0; i<_objects.size(); ++i )
                {
                    auto & o = _objects[i] ;

                    if( !_bound.is_inside( o.get_physic().get_position() )  )
                    {
                        _objects[i] = _objects[_objects.size()-1] ;
                        _objects.resize( _objects.size()-1) ;
                    }
                }
            }

            // reset force
            {
                for( auto & o : _objects )
                {
                    auto & phy = o.get_physic() ;
                    phy.reset_force() ;
                }
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _ae.on_graphics_begin( rd ) ;

            auto pr = _ae.get_prim_render() ;

            // draw ground
            {
                for( auto const & g : _ground )
                {
                    natus::math::vec2f_t points[4] ;
                    g.get_box().get_points( points ) ;
                    pr->draw_rect( 5, points[0], points[1], points[2], points[3], 
                        natus::math::vec4f_t( .1f, .1f, .1f, 1.0f ), natus::math::vec4f_t(.1f, .1f, .1f, 1.0f) ) ;
                }
            }

            // draw cannon
            {
                {
                    auto const p0 = _obj_cannon.get_position() ;
                    auto const p1 = p0 + _obj_cannon.gen_force() * 0.1f;
                    pr->draw_circle( 10, 10, p0, _obj_cannon.get_mass(), natus::math::vec4f_t(1.0f),natus::math::vec4f_t(1.0f) ) ;
                    pr->draw_line( 10, p0, p1, natus::math::vec4f_t(1.0f, 0.0f, 0.0, 1.0f) ) ;
                }

                {
                    auto const p0 = _obj_cannon.get_position() ;
                    auto const p1 = p0 + _obj_cannon.gen_force().normalize() * 10.0f ;
                    pr->draw_line( 11, p0, p1, natus::math::vec4f_t(0.0f, 1.0f, 0.0, 1.0f) ) ;
                }
            }

            // draw objects
            {
                for( auto & o : _objects )
                {
                    auto const p0 = o.get_physic().get_position() * _ppm ;
                    pr->draw_circle( 11, 10, p0, o.get_physic().get_mass(), natus::math::vec4f_t(1.0f),natus::math::vec4f_t(1.0f) ) ;
                }
            }

            _ae.on_graphics_end( 100 ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Control" ) ;
            {
                float_t m = _obj_cannon.get_mass() ;
                ImGui::SliderFloat( "Mass", &m, 1.0f, 100.0f ) ;
                _obj_cannon.set_mass( m ) ;
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
