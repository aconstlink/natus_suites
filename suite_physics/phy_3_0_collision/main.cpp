

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
#include <natus/collide/2d/hit_tests.hpp>

#include <random>
#include <thread>

namespace this_file
{
    using namespace natus::core ;
    using namespace natus::core::types ;

    //****************************************************************************************
    class physic_property
    {
        float_t _mass = 10.0f ;
        natus::math::vec2f_t _pos ;
        natus::math::vec2f_t _vel ; // linear velocity
        natus::math::vec2f_t _force ; // linear force


        float_t _inertia = 0.0f ; // angular mass
        float_t _tau = 0.0f ; // angular force
        float_t _omega = 0.0f ; // angular velocity
        float_t _alpha = 0.0f ; // angular displacement

    public:

        natus::math::vec2f_cref_t get_force( void_t ) const noexcept{ return _force ; }
        void_t set_force( natus::math::vec2f_cref_t f ) noexcept{ _force = f ; }
        void_t add_force( natus::math::vec2f_cref_t f ) noexcept{ _force += f ; }

        natus::math::vec2f_cref_t get_velocity( void_t ) const noexcept{ return _vel ; }
        void_t set_velocity( natus::math::vec2f_cref_t v ) noexcept{ _vel = v ; }

        natus::math::vec2f_cref_t get_position( void_t ) const noexcept{ return _pos ; }
        void_t set_position( natus::math::vec2f_cref_t v ) noexcept{ _pos = v ; }

        void_t reset_force( void_t ) noexcept { 
            _force = natus::math::vec2f_t() ;
            _inertia = 0.0f ;
            _tau = 0.0f ;
        }

        float_t get_mass( void_t ) const noexcept { return _mass ; }
        void_t set_mass( float_t const m ) noexcept { _mass = m ; }

        void_t set_angular_velocity( natus::math::vec2f_cref_t v, natus::math::vec2f_cref_t prel ) noexcept        
        {
            float_t const d2 = prel.length2() ;
            float_t const omega = prel.cross_as_scalar( v ) ;
            _omega = omega ;
        }

        natus::math::vec2f_t get_angular( void_t ) const noexcept
        {
            return natus::math::vec2f_t( _omega, _alpha ) ;
        }

        // apply force at point relative (prel) to cur position.
        void_t apply_angular_force_at( natus::math::vec2f_cref_t f, natus::math::vec2f_cref_t prel ) noexcept
        {
            float_t const d2 = prel.length2() ;
            float_t const I = std::max( d2 * _mass, 0.01f ) ;
            float_t const tau = prel.cross_as_scalar( f ) ;
            
            _inertia += I ;
            _tau += tau ;
        }

        // generate a velocity vector
        natus::math::vec2f_t angular_velocity( natus::math::vec2f_cref_t n ) const noexcept
        {
            return n.ortho() * natus::math::vec2f_t( _omega ) ;
        }

        float_t compute_inertia( float_t const distance ) const noexcept
        {
            return distance * distance * _mass ;
        }

        void_t integrate( float_t const dt ) noexcept
        {
            // linear moment
            {
                auto const a = _force / _mass ;
                _vel = _vel + a * dt ;
                _pos = _pos + _vel * dt ;
            }

            // angular moment
            {
                float_t const a = std::abs( _inertia ) > 0.0001f ? _tau / _inertia : 0.0f ;
                _omega = a * dt + _omega ;
                _alpha = _omega * dt + _alpha ;
            }
        }
    };
    natus_typedef( physic_property ) ;

    //****************************************************************************************
    class shape_property
    {

        // unit: meters
        float_t _radius = .1f ;

    public:

        float_t get_radius( void_t ) const noexcept { return _radius ; }
        void_t set_radius( float_t const r ) noexcept { _radius = r ; }
    };
    natus_typedef( shape_property ) ;

    //****************************************************************************************
    class physics_object
    {
        physic_property_t _phy_prop ;
        
        // levels of shape
        // coars to fine
        shape_property_t _shp_props[4] ; 

    public:

        physic_property_cref_t get_physic( void_t ) const noexcept { return _phy_prop ; }
        physic_property_ref_t get_physic( void_t ) noexcept { return _phy_prop ; }

        shape_property_cref_t get_shape( size_t const i = 0 ) const noexcept { return _shp_props[i] ; }
        shape_property_ref_t get_shape( size_t const i = 0 ) noexcept { return _shp_props[i] ; }
    };
    natus_typedef( physics_object ) ;

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

        this_file::physics_object_t gen_object( void_t ) const noexcept
        {
            auto const o = this_file::physics_object_t() ;

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
        float_t _material_coeff = 0.8f ;

    public:

        box_t get_box( void_t ) const noexcept { return _bound ; }
        void_t set_box( box_cref_t b ) noexcept { _bound = b ; }

        void_t set_friction_coeff( float_t const f ) noexcept { _material_coeff = f ; }
        float_t get_friction_coeff( void_t ) const noexcept { return _material_coeff ; }

    };
    natus_typedef( ground_object ) ;

    //****************************************************************************************
    class physics_system
    {
        natus_this_typedefs( physics_system ) ;

    public:

        struct normal
        {/*
            normal( void_t ) noexcept {}
            normal( normal const & n ) noexcept : pos( n.pos ), nrm(n.nrm ){}
            ~normal( void_t ) noexcept {}
            */
            natus::math::vec2f_t pos ;
            natus::math::vec2f_t nrm ;
        };

        struct contact_point 
        {
            // requires objects
            float_t time = -1.0f ;
            natus::math::vec2f_t p ;
            natus::math::vec2f_t p2 ;
        } ;

    public:

        natus_typedefs( natus::ntd::vector< normal >, normals ) ;
        natus_typedefs( natus::ntd::vector< contact_point >, contacts ) ;

    private:
        
        normals_t _normals ;
        contacts_t _contacts ;


    public:

        void_t add_normal( natus::math::vec2f_cref_t p, natus::math::vec2f_cref_t n ) noexcept
        {
            
            _normals.emplace_back( normal( { p, n } ) ) ;
        }

        normals_cref_t normals( void_t ) const noexcept
        {
            return _normals ;
        }

        contacts_cref_t contacts( void_t ) const noexcept
        {
            return _contacts ;
        }

        void_t add_contact_point( float_t const secs, natus::math::vec2f_cref_t p, natus::math::vec2f_cref_t p2 ) noexcept
        {
            this_t::contact_point cp ;
            cp.p = p ;
            cp.p2 = p2 ;
            cp.time = secs ;
            
            _contacts.emplace_back( cp ) ;
        }

        void_t clear_normals( void_t ) noexcept { _normals.clear() ; }

        void_t update( float_t const dt ) noexcept
        {
            size_t n = _contacts.size() ;
            for( size_t i=0; i<n; ++i )
            {
                _contacts[i].time -= dt ;

                if( _contacts[i].time > 0.0f ) continue ;
                
                _contacts[i].p = _contacts[n-1].p ;
                _contacts[i].p2 = _contacts[n-1].p2 ;
                _contacts[i].time = _contacts[n-1].time ;

                --n ;
                --i ;
            }

            _contacts.resize( n ) ;
        }
    };
    natus_typedef( physics_system ) ;

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
        natus::ntd::vector< physics_object_t > _objects ;
        natus::collide::n2d::aabbf_t _bound ;
        
        // pixel per meter
        float_t _ppm = 50.0f ;

        physics_system_t _phy_sys ;

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
                float_t const ms = 1.5f ;
                #if 1
                {
                    this_file::ground_object_t go ;
                    auto const b = this_file::ground_object_t::box_t( natus::math::vec2f_t(-4.0f, -5.0f) * ms, natus::math::vec2f_t(5.0f, -3.0f) * ms )  ;
                    go.set_box( b ) ;
                    go.set_friction_coeff( 0.2f ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(-4.0f, -4.0f)* ms, natus::math::vec2f_t(-3.5f, 4.0f)* ms ) ) ;
                    go.set_friction_coeff( 0.4f ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(-1.2f, -1.2f)* ms, natus::math::vec2f_t( -.9f, -.9f)* ms ) ) ;
                    go.set_friction_coeff( 0.4f ) ;
                    _ground.emplace_back( go ) ;
                }
                #endif
                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(-4.0f, 2.0f)* ms, natus::math::vec2f_t( -.5f, 2.3f)* ms ) ) ;
                    go.set_friction_coeff( 0.6f ) ;
                    _ground.emplace_back( go ) ;
                }
                #if 1
                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(1.0f, -4.0f)* ms, natus::math::vec2f_t( 2.5f, -1.0f)* ms ) ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(4.0f, -4.0f)* ms, natus::math::vec2f_t( 4.5f, 4.0f)* ms ) ) ;
                    _ground.emplace_back( go ) ;
                }

                {
                    this_file::ground_object_t go ;
                    go.set_box( this_file::ground_object_t::box_t( natus::math::vec2f_t(0.0f, -3.0f)* ms, natus::math::vec2f_t( .50f, -2.0f)* ms ) ) ;
                    _ground.emplace_back( go ) ;
                }
                #endif
            }

            {
                _bound = natus::collide::n2d::aabbf_t( natus::math::vec2f_t(-100.0f), natus::math::vec2f_t(100.0f) ) ;
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
                    this_file::physics_object_t obj ;
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
            _phy_sys.update( ud.sec_dt ) ;
            _phy_sys.clear_normals() ;

            // apply base force
            // e.g. gravity, wind, air friction
            {
                for( auto & o : _objects )
                {
                    auto & phy = o.get_physic() ;
                    phy.add_force( natus::math::vec2f_t( 0.0f, -9.81f * phy.get_mass() ) ) ;
                }
            }

            // collision detect
            {
                for( auto & b : _ground )
                {
                    for( auto & o : _objects )
                    {
                        auto & phy = o.get_physic() ;
                        auto const p = phy.get_position() ;

                        auto const n = b.get_box().calculate_normal_for( p ) ;
                        _phy_sys.add_normal( b.get_box().get_center(), n.xy() ) ;
                    }
                }
            }

            // apply collision force
            // e.g friction, drag, impluse, rotation
            {
                for( auto & b : _ground )
                {
                    for( auto & o : _objects )
                    {
                        auto & shp = o.get_shape() ;
                        auto & phy = o.get_physic() ;

                        auto p = phy.get_position() ;

                        float_t const radius = shp.get_radius() ;
                        natus::collide::n2d::circle<float_t> c( p, radius ) ;
                        natus::collide::n2d::aabb<float_t> a( b.get_box() ) ;

                        auto const res = natus::collide::n2d::hit_tests<float_t>::aabb_circle( a, c ) ;
                        if( res == natus::collide::hit_test_type::intersect || 
                            res == natus::collide::hit_test_type::inside )
                        {
                            auto const old_pos = phy.get_position() ;
                            auto const old_vel = phy.get_velocity() ;

                            auto const n = b.get_box().calculate_normal_for( old_pos ) ;
                            auto const d = n.dot( natus::math::vec3f_t( old_pos - b.get_box().get_center(), 1.0f) ) ;

                            auto const pen_depth = radius - d ;

                            // should not happen
                            if (pen_depth < 0.0f)
                            {
                                int pb = 0 ;
                                continue ;
                            }
                            

                            auto new_pos = old_pos ;
                            auto new_vel = old_vel ;

                            // repair collision
                            {
                                auto const repair = pen_depth * (n.xy() * 1.2f);

                                new_pos = old_pos + repair ;
                                new_vel = natus::math::vec2fe_t::reflect( n, old_vel ) ;
                            }

                            auto const n_vel_paral = n.xy().dot( new_vel ) * n.xy() ;
                            auto const n_vel_ortho = new_vel - n.xy().dot( new_vel ) * n.xy() ;

                            // all contact forces
                            natus::math::vec2f_t forces ;


                            // linear friction
                            {
                                forces += -(1.0f-b.get_friction_coeff()) * ((new_vel) * phy.get_mass()  / ud.sec_dt) ;
                            }

                            natus::math::vec2f_t cp ;

                            // determin contact point
                            {
                                auto const pen_depth = d ;
                                auto const repair = pen_depth * (n * 1.0f);
                                cp = phy.get_position() - repair ;
                            }

                            // angular friction
                            {
                                auto const v = -phy.angular_velocity( n ) * 0.2f  ;
                                //new_vel += v ;
                            }
                            

                            // apply forces 
                            {
                                auto const rel = cp - phy.get_position() ;
                                phy.apply_angular_force_at( forces, rel ) ;
                                phy.add_force( forces ) ;
                            }

                            if( phy.get_velocity().length() > 1.0f )
                                _phy_sys.add_contact_point(0.5f, cp, cp + n * 10.5f );

                            phy.set_position( new_pos ) ;
                            //phy.set_velocity( natus::math::vec2f_t() ) ;
                            phy.set_velocity( new_vel ) ;
                            phy.set_angular_velocity( new_vel, cp - phy.get_position() ) ;
                        }
                    }
                }
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

        //***************************************************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _ae.on_graphics_begin( rd ) ;

            auto pr = _ae.get_prim_render() ;

            // draw ground
            {
                for( auto const & g : _ground )
                {
                    float_t const c = g.get_friction_coeff() ;

                    natus::math::vec2f_t points[4] ;
                    g.get_box().get_points( points ) ;
                    pr->draw_rect( 5, points[0]*_ppm, points[1]*_ppm, points[2]*_ppm, points[3]*_ppm, 
                        natus::math::vec4f_t( c, c, c, 1.0f ), natus::math::vec4f_t(1.0f) ) ;
                }
            }

            // draw cannon
            {
                {
                    auto const p0 = _obj_cannon.get_position() ;
                    auto const p1 = p0 + _obj_cannon.gen_force() * 0.1f;
                    pr->draw_circle( 10, 10, p0, 2.0f, natus::math::vec4f_t(1.0f),natus::math::vec4f_t(1.0f) ) ;
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
                    auto const & pp = o.get_physic() ;
                    auto const & sp = o.get_shape() ;

                    // draw shape
                    {
                        auto const p0 = pp.get_position() * _ppm ;
                        pr->draw_circle( 11, 10, p0, sp.get_radius()*_ppm, natus::math::vec4f_t(1.0f),natus::math::vec4f_t(1.0f) ) ;
                    }

                    // draw properties : Linear Velocity
                    {
                        auto const p0 = pp.get_position() * _ppm ;
                        auto const p1 = p0 + pp.get_velocity() * 2.0f ;
                        pr->draw_line( 12, p0, p1, natus::math::vec4f_t(0.0f, 1.0f, 0.0, 1.0f) ) ;
                    }

                    // draw properties : angular Velocity
                    {
                        auto const rot = natus::math::mat2f_t::rotation( pp.get_angular().y() ) ;
                        auto const r = rot * natus::math::vec2f_t(0.0f, 15.0f ) ;

                        auto const p0 = pp.get_position() * _ppm ;
                        auto const p1 = p0 + r ;

                        pr->draw_line( 12, p0, p1, natus::math::vec4f_t(1.0f, 0.0f, 0.0, 1.0f) ) ;

                    }
                }
            }

            // draw some physics
            {

                for( auto const & cp : _phy_sys.contacts() )
                {
                    // draw shape
                    {
                        auto const p0 = cp.p * _ppm ;
                        pr->draw_circle( 11, 10, p0, 2.0f, natus::math::vec4f_t(1.0f),natus::math::vec4f_t(1.0f) ) ;
                    }
                    {
                        auto const p0 = cp.p * _ppm ;
                        auto const p1 = p0 + (cp.p2 - cp.p) ;

                        pr->draw_line( 12, p0, p1, natus::math::vec4f_t(0.0f, 1.0f, 0.0, 1.0f) ) ;
                    }
                }

                

                for( auto const & n : _phy_sys.normals() )
                {
                    {
                        auto const p0 = n.pos * _ppm ;
                        auto const p1 = p0 + n.nrm * 10.0f ;
                        pr->draw_line( 11, p0, p1, natus::math::vec4f_t(0.0f, 1.0f, 0.0, 1.0f) ) ;
                    }
                }
                #if 0
                for( auto const & g : _ground )
                {
                    for( auto & o : _objects )
                    {
                        auto const p0 = g.get_box().get_center() ;
                        auto const p1 = o.get_physic().get_position() * _ppm ;
                        pr->draw_line( 11, p0, p1, natus::math::vec4f_t(0.0f, 1.0f, 0.0, 1.0f) ) ;
                    }
                }
                #endif
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
