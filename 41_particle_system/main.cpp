

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/primitive/primitive_render_2d.h> 

#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profiling/macros.h>

#include <natus/geometry/mesh/polygon_mesh.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>
#include <natus/geometry/3d/tetra.h>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/3d/transformation.hpp>

#include <thread>

namespace nps
{
    using namespace natus::core::types ;

    struct particle
    {
        float_t age = 1.0f ; // in seconds
            
        float_t mass = 1.0f ;

        natus::math::vec2f_t force ;

        natus::math::vec2f_t pos ;
        natus::math::vec2f_t vel ;
        natus::math::vec2f_t acl ;
    };
    natus_typedef( particle ) ;

    enum class spawn_location_type
    {
        area,
        border
    } ;

    enum class spawn_distribution_type
    {
        uniform,
        random
    };

    enum class age_variation_type
    {
        random,
        fixed
    };

    class emitter
    {
        natus_this_typedefs( emitter ) ;

    private:

        // amount of particles per interval 
        size_t _amount = 1 ;
        // times per second <=> interval
        float_t _rate = 1.0f ;

        float_t _mass = 1.0f ;

        natus::math::vec2f_t _pos ;
        natus::math::vec2f_t _dir = natus::math::vec2f_t( 1.0f, 0.0f ) ;

        float_t _acl = 0.0f ;
        float_t _vel = 0.0f ;

        spawn_location_type _slt = spawn_location_type::border ;
        spawn_distribution_type _sdt = spawn_distribution_type::uniform ;
        age_variation_type _avt = age_variation_type::fixed ;

        // in seconds
        float_t _age = 0.0f ;

    public:

        float_t get_age( void_t ) const noexcept { return _age ; }
        void_t set_age( float_t const s ) noexcept { _age = s ; }

        size_t get_amount( void_t ) const noexcept { return _amount ; }
        void_t set_amount( size_t const s ) noexcept { _amount = s ; }

        float_t get_rate( void_t ) const noexcept { return _rate ; }
        void_t set_rate( float_t const s ) noexcept { _rate = std::max( 1.0f, std::abs(s) ) ; }

        void_t set_mass( float_t const m ) noexcept { _mass = m ; }
        float_t get_mass( void_t ) const noexcept { return _mass ; }

        void_t set_position( natus::math::vec2f_cref_t v ) noexcept { _pos = v ; }
        void_t set_direction( natus::math::vec2f_cref_t v ) noexcept { _dir = v ; }
        void_t set_velocity( float_t const v ) noexcept { _vel = v ; }
        void_t set_acceleration( float_t const v ) noexcept { _acl = v ; }
        void_t set_spawn_location_type( spawn_location_type const spt ) noexcept { _slt = spt ; }
        void_t set_spawn_distribution_type( spawn_distribution_type const sdt ) noexcept { _sdt = sdt ; }
        void_t set_age_variation_type( age_variation_type const avt ) noexcept { _avt = avt ; }

        natus::math::vec2f_cref_t get_position( void_t ) const noexcept{ return _pos ; }
        float_t get_velocity( void_t ) const noexcept{ return _vel ; }
        float_t get_acceleration( void_t ) const noexcept{ return _acl ; }
        natus::math::vec2f_cref_t get_direction( void_t ) const noexcept{ return _dir ; }
        spawn_location_type get_spawn_location_type( void_t ) const noexcept { return _slt ; }
        spawn_distribution_type get_spawn_distribution_type( void_t ) const noexcept { return _sdt ; }
        age_variation_type get_age_variation_type( void_t ) const noexcept { return _avt ; }

    public:

        virtual size_t calc_emits( size_t const emitted, float_t const secs ) const noexcept = 0 ;
        virtual void_t emit( size_t const, size_t const, natus::ntd::vector< particle_t > & ) noexcept = 0 ;
    };
    natus_res_typedef( emitter ) ;

    
    class radial_emitter : public emitter
    {
        natus_this_typedefs( radial_emitter ) ;

    private:

        /// radius of 0 is a point emitter
        float_t _radius = 0.0f ;

        // around direction in [-angle, angle]
        float_t _angle = 0.0f ;

    public:

        float_t get_radius( void_t ) const noexcept { return _radius ; }
        void_t set_radius( float_t const r ) noexcept { _radius = std::abs( r ) ; }

        float_t get_angle( void_t ) const noexcept { return _angle ; }
        void_t set_angle( float_t const v ) noexcept { _angle = v ; }

    public:

        virtual size_t calc_emits( size_t const emitted, float_t const secs ) const noexcept 
        {
            float_t const s = secs * this_t::get_rate() ;
            size_t const tq = size_t( s - natus::math::fn<float_t>::fract( s ) ) ;
            size_t const eq = emitted / this_t::get_amount() ;

            return tq >= eq ? this_t::get_amount() : 0 ;
        }

        virtual void_t emit( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) noexcept 
        {            
            if( n == 0 ) return ;

            float_t const angle_step = ( 2.0f * this_t::get_angle()) / float_t(n-1) ;
            natus::math::mat2f_t uniform = natus::math::mat2f_t::rotation( angle_step ) ;

            natus::math::vec2f_t dir = natus::math::mat2f_t::rotation( n == 1 ? 0.0f : -this_t::get_angle() ) * 
                this_t::get_direction() ;

            for( size_t i=beg; i<beg+n; ++i )
            {
                particle_t p ;
                
                p.age = this_t::get_age() ;
                p.mass = this_t::get_mass() ;
                p.pos = this_t::get_position() + dir * this_t::get_radius() ;
                p.vel = dir * this_t::get_velocity() ;
                p.acl = dir * this_t::get_acceleration() ;

                particles[ i ] = p ;

                dir = uniform * dir ;
            }
        }
    };
    natus_res_typedef( radial_emitter ) ;

    class force_field
    {
    private:
                
        natus::math::vec2f_t _pos ;
        float_t _radius = -1.0f ;
        float_t _radius2 = 0.0f ;

    public:

        void_t set_radius( float_t const r ) noexcept { _radius = r ; _radius2 = r * r ; }
        float_t get_radius( void_t ) const noexcept { return _radius ; }

        bool_t is_inside( particle_in_t p ) const noexcept
        {
            return _radius < 0.0f || (p.pos - _pos).length2() < _radius2 ;
        }

    public:

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept = 0 ;
    } ;
    natus_res_typedef( force_field ) ;

    class constant_velocity_field : public force_field
    {
        natus_this_typedefs( constant_velocity_field ) ;

    private:

        natus::math::vec2f_t _vel ;

    public:

        void_t set_velocity( natus::math::vec2f_cref_t v ) noexcept
        {
            _vel = v ;
        }

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                //p.vel = _vel ;
            }
            
        }
    } ;
    natus_res_typedef( constant_velocity_field ) ;

    class constant_acceleration_field : public force_field
    {
        natus_this_typedefs( constant_acceleration_field ) ;

    private:

        natus::math::vec2f_t _acl ;

    public:

        void_t set_acceleration( natus::math::vec2f_cref_t v ) noexcept
        {
            _acl = v ;
        }

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                p.acl = _acl ;
            }
        }
    } ;
    natus_res_typedef( constant_acceleration_field ) ;

    class constant_force_field : public force_field
    {
        natus_this_typedefs( constant_force_field ) ;

    private:

        natus::math::vec2f_t _force ;

        void_t set_force( natus::math::vec2f_cref_t v ) noexcept
        {
            _force = v ;
        }

    public:

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                p.force = _force ;
            }
        }
    } ;
    natus_res_typedef( constant_force_field ) ;

    class friction_force_field : public force_field
    {
        natus_this_typedefs( friction_force_field ) ;

    private:

        float_t _friction = 0.4f ;
        
    public:

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                p.force -= natus::math::vec2f_t( _friction ) * p.force ;
            }
        }
    } ;
    natus_res_typedef( friction_force_field ) ;

    class particle_system
    {
        natus_this_typedefs( particle_system ) ;

    public:
        
        particle_system( void_t ) {}
        particle_system( this_cref_t rhv ) noexcept = delete ;
        particle_system( this_rref_t rhv ) noexcept 
        {
            _particles = std::move( rhv._particles ) ;
            _forces = std::move( rhv._forces ) ;
            _emitter = std::move( rhv._emitter ) ;
        }

        this_ref_t operator = ( this_cref_t ) = delete ;
        this_ref_t operator = ( this_rref_t rhv ) noexcept 
        {
            _particles = std::move( rhv._particles ) ;
            _forces = std::move( rhv._forces ) ;
            _emitter = std::move( rhv._emitter ) ;
            return *this ;
        }

    private:

        // particle array
        natus::ntd::vector< particle_t > _particles ;

        struct emitter_data
        {
            emitter_res_t emt ;

            // time since attached
            float_t seconds ;

            size_t emitted ;

            size_t emit ;
        };
        natus_typedef( emitter_data ) ;

        // emitter array
        natus::ntd::vector< emitter_data_t > _emitter ;

        struct force_field_data
        {
            force_field_res_t ff ;
        };
        natus_typedef( force_field_data ) ;

        // force fields
        natus::ntd::vector< force_field_data_t > _forces ;

    public:

        void_t attach_emitter( emitter_res_t emt ) noexcept
        {
            auto iter = std::find_if( _emitter.begin(), _emitter.end(), [&]( emitter_data_cref_t d )
            {
                return d.emt == emt ;
            } ) ;
            if( iter != _emitter.end() ) return ;

            _emitter.push_back( { emt, 0, 0, 0 } ) ;
        }

        void_t detach_emitter( emitter_res_t emt ) noexcept
        {
            auto iter = std::find_if( _emitter.begin(), _emitter.end(), [&]( emitter_data_cref_t d )
            {
                return d.emt == emt ;
            } ) ;
            if( iter == _emitter.end() ) return ;

            _emitter.erase( iter ) ;
        }

        void_t attach_force_field( force_field_res_t ff ) noexcept
        {
            auto iter = std::find_if( _forces.begin(), _forces.end(), [&]( force_field_data_cref_t d )
            {
                return d.ff == ff ;
            } ) ;
            if( iter != _forces.end() ) return ;
            _forces.push_back( { ff } ) ;
        }

        void_t detach_force_field( force_field_res_t ff ) noexcept
        {
            auto iter = std::find_if( _forces.begin(), _forces.end(), [&]( force_field_data_cref_t d )
            {
                return d.ff == ff ;
            } ) ;
            if( iter == _forces.end() ) return ;

            _forces.erase( iter ) ;
        }

        void_t clear( void_t ) noexcept
        {
            _particles.clear() ;
            for( auto & e : _emitter )
            {
                e.emitted = 0 ;
                e.emit = 0 ;
                e.seconds = 0.0f ;
            }
        }
    public: 

        void_t update( float_t const dt ) noexcept 
        {
            // update particle
            {
                for( auto & p : _particles )
                {
                    p.age -= dt ;
                }
            }

            // reorder particles
            {
                size_t cur = 0 ;
                size_t last = _particles.size() ;
                for( size_t i=0; i<last; ++i )
                {
                    auto const & p = _particles[i] ;
                    if( p.age <= 0.0f )
                    {
                        _particles[i--] = _particles[ --last ] ;
                    }
                }
                _particles.resize( last ) ;
            }

            // emit new particles
            {
                size_t new_emit = 0 ;
                for( auto & e : _emitter )
                {
                    auto const emit = e.emt->calc_emits( e.emitted, e.seconds ) ;
                    e.seconds += dt ;
                    e.emitted += emit ;
                    e.emit = emit ;
                    new_emit += emit ;
                }

                size_t begin = _particles.size() ;
                _particles.resize( _particles.size() + new_emit ) ;

                for( auto & e : _emitter )
                {
                    e.emt->emit( begin, e.emit, _particles ) ; begin += e.emit ;
                    e.emit = 0 ;
                }
            }
            
            // do force fields
            {
                for( auto & f : _forces )
                {
                    f.ff->apply( 0, _particles.size(), _particles ) ;
                }
            }
            
            // do physics
            {
                for( auto & p : _particles )
                {
                    p.acl += p.force / p.mass ;
                    p.vel += natus::math::vec2f_t( dt ) * p.acl ;
                    p.pos += natus::math::vec2f_t( dt ) * p.vel ;
                }
            }
            
        }

        typedef std::function< void_t ( natus::ntd::vector< particle_t > const &  ) > on_particles_funk_t ;
        void_t on_particles( on_particles_funk_t funk )
        {
            funk( _particles ) ;
        }
    };
    natus_res_typedef( particle_system ) ;

    // rocket that opens and throws bombs
    struct rocket_particle_effect
    {
        particle_system_res_t smoke ;
        particle_system_res_t fire ;
        particle_system_res_t rocket ;
        particle_system_res_t bombs ;
        particle_system_res_t explosions ;
    } ;

    struct rain_particle_effect
    {
        particle_system_res_t rain ;
        particle_system_res_t droplet ;
        particle_system_res_t splash ;
        
    } ;

    struct snow_particle_effect
    {
        natus_this_typedefs( snow_particle_effect ) ;

        particle_system_t flakes ;
        radial_emitter_res_t emitter ;
        constant_velocity_field_res_t wind ;
        constant_acceleration_field_res_t g ;

        snow_particle_effect( void_t ) 
        {
            emitter = nps::radial_emitter_t() ;
            wind = nps::constant_velocity_field_t() ;
            g = nps::constant_acceleration_field_t() ;

            flakes.attach_emitter( emitter ) ;
            flakes.attach_force_field( wind ) ;
            flakes.attach_force_field( g ) ;

            g->set_acceleration( natus::math::vec2f_t( 0.0f, -9.81f ) ) ;

            emitter->set_mass( 0.0049f )  ;
            emitter->set_age( 8.0f ) ;
            emitter->set_amount( 10 ) ;
            emitter->set_rate( 2.0f ) ;
            emitter->set_angle( 0.3f ) ;
            emitter->set_velocity( 20.0f ) ;
            emitter->set_direction( natus::math::vec2f_t(0.0f,1.0f) ) ;
        }

        snow_particle_effect( this_cref_t ) = delete ;
        snow_particle_effect( this_rref_t rhv ) noexcept
        {
            flakes = std::move( rhv.flakes ) ;
            emitter = std::move( rhv.emitter ) ;
            wind = std::move( rhv.wind ) ;
            g = std::move( rhv.g ) ;
        }

        this_ref_t operator = ( this_rref_t rhv ) noexcept 
        {
            flakes = std::move( rhv.flakes ) ;
            emitter = std::move( rhv.emitter ) ;
            wind = std::move( rhv.wind ) ;
            g = std::move( rhv.g ) ;
            return *this ;
        }

        void_t set_wind_velocity( natus::math::vec2f_cref_t v ) noexcept
        {
            wind->set_velocity( v ) ;
        }

        void_t update( float_t const dt ) noexcept
        {
            flakes.update( dt ) ;
        }

        void_t render( natus::gfx::primitive_render_2d_res_t pr )
        {
            flakes.on_particles( [&]( natus::ntd::vector< particle_t > const & particles )
            {
                size_t i=0 ;
                for( auto & p : particles )
                {
                    #if 0
                    pr->draw_circle( 4, 10, p.pos/25.0f, p.mass*5.0f, 
                        natus::math::vec4f_t(1.0f), 
                        natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f) ) ;
                    #elif 1
                    natus::math::vec2f_t const pos = p.pos/25.0f ;
                    float_t const s = p.mass*3.0f*0.5f ;

                    natus::math::vec2f_t points[] = 
                    {
                        pos + natus::math::vec2f_t(-s, -s), 
                        pos + natus::math::vec2f_t(-s, +s),  
                        pos + natus::math::vec2f_t(+s, +s),
                        pos + natus::math::vec2f_t(+s, -s)
                    } ;
                    float_t const alpha = p.age / emitter->get_age() ;
                    pr->draw_rect( ++i % 50, 
                        points[0], points[1], points[2], points[3],
                        natus::math::vec4f_t(0.0f, 0.0f, 0.5f, alpha), 
                        natus::math::vec4f_t(1.0f,0.0f,0.0f,alpha) ) ;
                    #endif
                }
            } ) ;
        }
    } ;
    natus_typedef( snow_particle_effect ) ;
}

namespace this_file
{
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

        bool_t _do_tool = false ;
        
    private: // particle system

        
        natus::gfx::primitive_render_2d_res_t _pr ;

        nps::snow_particle_effect_t _spe ;

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

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ),
                float_t(wei.w) / float_t(wei.h), 1.0f, 1000.0f ) ;

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
                _camera_0.look_at( natus::math::vec3f_t( 0.0f, 60.0f, -50.0f ),
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

        virtual natus::application::result on_device( device_data_in_t ) 
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

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) 
        { 
            // do update all particle effects
            {
                _spe.update( ud.sec_dt ) ;
            }

            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t  ) 
        {
            // draw particle effects
            {
                _spe.render( _pr ) ;
                _pr->draw_circle( 0, 20, _spe.emitter->get_position(), _spe.emitter->get_radius()/25.0f, 
                    natus::math::vec4f_t(1.0f, 0.0f,0.0f,0.2f),  natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f)) ;
            }

            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( _root_render_states ) ;
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
                    a.use( natus::graphics::state_object_t(), 10 ) ;
                } ) ;
            }

            

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui )
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "Control Particle System" ) ;

            {
                auto dir = _spe.emitter->get_direction() ;
                if( natus::tool::custom_imgui_widgets::direction( "dir", dir ) )
                {
                    _spe.emitter->set_direction( dir ) ;
                }
            }
            ImGui::SameLine() ;
            {
                float_t v = _spe.emitter->get_rate() ;
                if( ImGui::VSliderFloat("Rate", ImVec2(50,100), &v, 1.0f, 10.0f ) )
                {
                    _spe.emitter->set_rate( v ) ;
                    _spe.flakes.clear() ;
                }
            }
            ImGui::SameLine() ;
            {
                int_t v = _spe.emitter->get_amount() ;
                if( ImGui::VSliderInt("Amount", ImVec2(50,100), &v, 1, 100 ) )
                {
                    _spe.emitter->set_amount( v ) ;
                    _spe.flakes.clear() ;
                }
            }
            ImGui::SameLine() ;
            {
                float_t v = _spe.emitter->get_age() ;
                if( ImGui::VSliderFloat("Age", ImVec2(50,100), &v, 1.0f, 10.0f ) )
                {
                    _spe.emitter->set_age( v ) ;
                }
            }
            ImGui::SameLine() ;
            {
                float_t v = _spe.emitter->get_radius() ;
                if( ImGui::VSliderFloat("Radius", ImVec2(50,100), &v, 0.0f, 100.0f ) )
                {
                    _spe.emitter->set_radius( v ) ;
                }
            }

            ImGui::End() ;
            return natus::application::result::ok ;
        }

        virtual natus::application::result on_shutdown( void_t ) 
        { return natus::application::result::ok ; }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}
