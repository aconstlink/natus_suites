

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/primitive/primitive_render_2d.h> 
#include <natus/profiling/macros.h>
#include <natus/graphics/variable/variable_set.hpp>

#include <natus/concurrent/parallel_for.hpp>

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

#include <random>
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

    enum class variation_type
    {
        fixed,
        random
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
        
    protected:

        typedef std::function< float_t ( void_t ) > variation_funk_t ;

    private:

        variation_type _mass_vt = variation_type::fixed ;
        variation_type _avt = variation_type::fixed ;
        variation_type _acl_vt = variation_type::fixed ;
        variation_type _vel_vt = variation_type::fixed ;
        

        variation_funk_t _mass_funk ;
        variation_funk_t _age_funk ;
        variation_funk_t _acl_funk ;
        variation_funk_t _vel_funk ;

        // in seconds
        float_t _age = 0.0f ;

        

        std::mt19937 _generator ;

    public:

        emitter( void_t ) noexcept
        {
            this_t::set_age_variation_type( _avt ) ;
            this_t::set_acceleration_variation_type( _acl_vt ) ;
            this_t::set_velocity_variation_type( _vel_vt ) ;
        }

        emitter( this_cref_t ) = delete ;
        emitter( this_rref_t rhv ) noexcept
        {
            this_t::operator=( std::move( rhv ) ) ;
        }
        virtual ~emitter( void_t ) noexcept {}

        this_ref_t operator = ( this_cref_t ) = delete ;
        this_ref_t operator = ( this_rref_t rhv ) noexcept
        {
            _amount = rhv._amount ;
            _rate = rhv._rate ;
            _mass = rhv._mass ;
            _pos = rhv._pos ;
            _dir = rhv._dir ;
            _acl = rhv._acl ;
            _vel = rhv._vel ;
            _slt = rhv._slt ;
            _sdt = rhv._sdt ;
            
            
            _age = rhv._age ;
            _generator = std::move( rhv._generator ) ;
            
            this_t::set_mass_variation_type( rhv._mass_vt ) ;
            this_t::set_age_variation_type( rhv._avt ) ;
            this_t::set_acceleration_variation_type( rhv._acl_vt ) ;
            this_t::set_velocity_variation_type( rhv._vel_vt ) ;
            return *this ;
        }

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
        void_t set_direction( natus::math::vec2f_cref_t v ) noexcept { _dir = v.normalized() ; }
        void_t set_velocity( float_t const v ) noexcept { _vel = v ; }
        void_t set_acceleration( float_t const v ) noexcept { _acl = v ; }
        void_t set_spawn_location_type( spawn_location_type const spt ) noexcept { _slt = spt ; }
        void_t set_spawn_distribution_type( spawn_distribution_type const sdt ) noexcept { _sdt = sdt ; }

        natus::math::vec2f_cref_t get_position( void_t ) const noexcept{ return _pos ; }
        float_t get_velocity( void_t ) const noexcept{ return _vel ; }
        float_t get_acceleration( void_t ) const noexcept{ return _acl ; }
        natus::math::vec2f_cref_t get_direction( void_t ) const noexcept{ return _dir ; }
        spawn_location_type get_spawn_location_type( void_t ) const noexcept { return _slt ; }
        spawn_distribution_type get_spawn_distribution_type( void_t ) const noexcept { return _sdt ; }

        variation_type get_mass_variation_type( void_t ) const noexcept { return _mass_vt ; }
        variation_type get_age_variation_type( void_t ) const noexcept { return _avt ; }
        variation_type get_acceleration_variation_type( void_t ) const noexcept { return _acl_vt ; }
        variation_type get_velocity_variation_type( void_t ) const noexcept { return _vel_vt ; }

        variation_funk_t get_mass_funk( void_t ) const noexcept { return _mass_funk ; }
        variation_funk_t get_age_funk( void_t ) const noexcept { return _age_funk ; }
        variation_funk_t get_acceleration_funk( void_t ) const noexcept { return _acl_funk ; }
        variation_funk_t get_velocity_funk( void_t ) const noexcept { return _vel_funk ; }

        float_t random_real_number( float_t const a, float_t const b ) noexcept
        {
            std::uniform_real_distribution<float_t> distribution(a, b) ;
            return distribution( _generator ) ;
        }

        void_t set_mass_variation_type( variation_type const avt ) noexcept 
        { 
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_mass() ;
            } ;

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_mass() * 0.2f, this_t::get_mass() ) ;
                } ;
            }
            _mass_vt = avt ; 
            _mass_funk = funk ;
        }

        void_t set_age_variation_type( variation_type const avt ) noexcept 
        {             
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_age() ;
            };

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_age() * 0.2f, this_t::get_age() ) ;
                } ;
            }

            _avt = avt ; 
            _age_funk = funk ;
        }

        void_t set_acceleration_variation_type( variation_type const avt ) noexcept 
        { 
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_acceleration() ;
            } ;

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_acceleration() * 0.2f, this_t::get_acceleration() ) ;
                } ;
            }

            _acl_vt = avt ; 
            _acl_funk = funk ;
        }

        void_t set_velocity_variation_type( variation_type const avt ) noexcept 
        {
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_velocity() ;
            } ;

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_velocity() * 0.2f, this_t::get_velocity() ) ;
                } ;
            }

            _vel_vt = avt ; 
            _vel_funk = funk ;
        }
        

    public:

        // the default function returns amount particles per rate * second
        virtual size_t calc_emits( size_t const emitted, float_t const secs ) const noexcept 
        {
            float_t const s = secs * this_t::get_rate() ;
            size_t const tq = size_t( s - natus::math::fn<float_t>::fract( s ) ) ;
            size_t const eq = emitted / this_t::get_amount() ;

            return tq >= eq ? this_t::get_amount() : 0 ;
        }

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


        variation_type _radius_vt = variation_type::fixed ;
        variation_type _angle_vt = variation_type::fixed ;

        emitter::variation_funk_t _radius_funk ;
        emitter::variation_funk_t _angle_funk ;

    public:

        radial_emitter( void_t ) noexcept 
        {
            this_t::set_angle_variation_type( _angle_vt ) ;
            this_t::set_radius_variation_type( _radius_vt ) ;
        }

        radial_emitter( this_cref_t ) = delete ;
        radial_emitter( this_rref_t rhv ) noexcept : emitter( std::move( rhv ) )
        {
            this_t::operator=( std::move( rhv ) ) ;
        }
        virtual ~radial_emitter( void_t ) noexcept{}
        
        this_ref_t operator = ( this_cref_t ) = delete ;
        this_ref_t operator = ( this_rref_t rhv ) noexcept
        {
            _radius = rhv._radius ;
            _angle = rhv._angle ;

            this_t::set_radius_variation_type( rhv._radius_vt ) ;
            this_t::set_angle_variation_type( rhv._angle_vt ) ;

            return *this ;
        }

        void_t set_radius_variation_type( variation_type const avt ) noexcept 
        { 
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_radius() ;
            } ;

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_radius() * 0.2f, this_t::get_radius() ) ;
                } ;
            }
            _radius_vt = avt ; 
            _radius_funk = funk ;
        }

        void_t set_angle_variation_type( variation_type const avt ) noexcept 
        { 
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_angle() ;
            } ;

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_angle() * 0.2f, this_t::get_angle() ) ;
                } ;
            }
            _angle_vt = avt ; 
            _angle_funk = funk ;
        }

        variation_type get_radius_variation_type( void_t ) const noexcept { return _radius_vt ; }
        variation_funk_t get_radius_funk( void_t ) const noexcept { return _radius_funk ; }
        variation_type get_angle_variation_type( void_t ) const noexcept { return _angle_vt ; }
        variation_funk_t get_angle_funk( void_t ) const noexcept { return _angle_funk ; }

    public:

        float_t get_radius( void_t ) const noexcept { return _radius ; }
        void_t set_radius( float_t const r ) noexcept { _radius = std::abs( r ) ; }

        float_t get_angle( void_t ) const noexcept { return _angle ; }
        void_t set_angle( float_t const v ) noexcept { _angle = v ; }

    public:

        virtual void_t emit( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) noexcept 
        {
            float_t const angle = this_t::get_angle_funk()() ;

            float_t const angle_step = ( 2.0f * angle ) / float_t(n-1) ;
            natus::math::mat2f_t uniform = natus::math::mat2f_t::rotation( angle_step ) ;

            natus::math::vec2f_t dir = natus::math::mat2f_t::rotation( n == 1 ? 0.0f : -angle ) * 
                this_t::get_direction() ;

            for( size_t i=beg; i<beg+n; ++i )
            {
                particle_t p ;
                
                p.age = this_t::get_age_funk()() ;
                p.mass = this_t::get_mass_funk()() ;
                p.pos = this_t::get_position() + dir * this_t::get_radius_funk()() ;
                p.vel = dir * this_t::get_velocity_funk()() ;
                p.acl = dir * this_t::get_acceleration_funk()() ;

                particles[ i ] = p ;

                dir = uniform * dir ;
            }
        }
    };
    natus_res_typedef( radial_emitter ) ;

    class line_emitter : public emitter
    {
        natus_this_typedefs( line_emitter ) ;

    private:

        /// distance from origin on the line
        float_t _parallel_dist = 0.0f ;
        
        /// distance away from line in ortho direction
        float_t _ortho_dist = 0.0f ;
        
        variation_type _parallel_vt = variation_type::fixed ;
        variation_type _ortho_vt = variation_type::fixed ;

        emitter::variation_funk_t _parallel_funk ;
        emitter::variation_funk_t _ortho_funk ;

    public:

        line_emitter( void_t ) noexcept 
        {
            this_t::set_parallel_variation_type( _parallel_vt ) ;
            this_t::set_ortho_variation_type( _ortho_vt ) ;
        }
        line_emitter( this_cref_t ) = delete ;
        line_emitter( this_rref_t rhv ) noexcept : emitter( std::move( rhv ) )
        {
            this_t::operator=( std::move( rhv ) ) ;
        }
        virtual ~line_emitter( void_t ) noexcept{}

        this_ref_t operator = ( this_cref_t ) = delete ;
        this_ref_t operator = ( this_rref_t rhv ) noexcept
        {
            _parallel_dist = rhv._parallel_dist ;
            _ortho_dist = rhv._ortho_dist ;

            this_t::set_parallel_variation_type( rhv._parallel_vt ) ;
            this_t::set_ortho_variation_type( rhv._ortho_vt ) ;
            return *this ;
        }

        void_t set_parallel_variation_type( variation_type const avt ) noexcept 
        { 
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_parallel_distance() ;
            } ;

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_parallel_distance() * 0.2f, this_t::get_parallel_distance() ) ;
                } ;
            }
            _parallel_vt = avt ; 
            _parallel_funk = funk ;
        }

        void_t set_ortho_variation_type( variation_type const avt ) noexcept 
        { 
            variation_funk_t funk = [=]( void_t )
            {
                return this_t::get_ortho_distance() ;
            } ;

            if( avt == nps::variation_type::random )
            {
                funk = [=]( void_t )
                {
                    return this_t::random_real_number( this_t::get_ortho_distance() * 0.2f, this_t::get_ortho_distance() ) ;
                } ;
            }
            _ortho_vt = avt ; 
            _ortho_funk = funk ;
        }

        variation_type get_parallel_variation_type( void_t ) const noexcept { return _parallel_vt ; }
        variation_funk_t get_parallel_funk( void_t ) const noexcept { return _parallel_funk ; }
        variation_type get_ortho_variation_type( void_t ) const noexcept { return _ortho_vt ; }
        variation_funk_t get_ortho_funk( void_t ) const noexcept { return _ortho_funk ; }

    public:

        float_t get_parallel_distance( void_t ) const noexcept { return _parallel_dist ; }
        void_t set_parallel_distance( float_t const r ) noexcept { _parallel_dist = std::abs( r ) ; }

        float_t get_ortho_distance( void_t ) const noexcept { return _ortho_dist ; }
        void_t set_ortho_distance( float_t const v ) noexcept { _ortho_dist = v ; }

    public:

        virtual void_t emit( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) noexcept 
        {
            float_t const parallel = this_t::get_parallel_funk()() ;
            float_t const step = (2.0f * parallel) / float_t(n-1) ;

            natus::math::vec2f_t ortho_dir = this_t::get_direction() ; 
            natus::math::vec2f_t parallel_dir( ortho_dir.y(), -ortho_dir.x() ) ; 

            natus::math::vec2f_t pos = this_t::get_position() + parallel_dir * parallel ;

            #if 0
            for( size_t i=beg; i<beg+n; ++i )
            {
                particle_t p ;
                
                p.age = this_t::get_age_funk()() ;
                p.mass = this_t::get_mass_funk()() ;
                p.pos = pos + ortho_dir * this_t::get_ortho_funk()() ;
                p.vel = ortho_dir * this_t::get_velocity_funk()() ;
                p.acl = ortho_dir * this_t::get_acceleration_funk()() ;

                particles[ i ] = p ;

                pos += parallel_dir.negated() * step ;
            }
            #else
            natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>( beg, beg+n ),
                [&]( natus::concurrent::range_1d<size_t> const & r )
                {
                    for( size_t i=r.begin(); i<r.end(); ++i )
                    {
                        particle_t p ;
                
                        p.age = this_t::get_age_funk()() ;
                        p.mass = this_t::get_mass_funk()() ;
                        p.pos = pos + ortho_dir * this_t::get_ortho_funk()() ;
                        p.vel = ortho_dir * this_t::get_velocity_funk()() ;
                        p.acl = ortho_dir * this_t::get_acceleration_funk()() ;

                        particles[ i ] = p ;

                        pos += parallel_dir.negated() * step ;
                    }
                } ) ;
            #endif
        }
    };
    natus_res_typedef( line_emitter ) ;

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

    class constant_force_field : public force_field
    {
        natus_this_typedefs( constant_force_field ) ;

    private:

        natus::math::vec2f_t _force ;

    public:

        void_t set_force( natus::math::vec2f_cref_t v ) noexcept
        {
            _force = v ;
        }

    public:

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            #if 0
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                p.force += _force ;
            }
            #else
            natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>( beg, beg+n ),
                [&]( natus::concurrent::range_1d<size_t> const & r )
                {
                    for( size_t i=r.begin(); i<r.end(); ++i )
                    {
                        nps::particle_ref_t p = particles[i] ;
                        if( !this_t::is_inside( p ) ) continue ;
                        p.force += _force ;
                    }
                } ) ;
            #endif
        }
    } ;
    natus_res_typedef( constant_force_field ) ;

    class acceleration_field : public force_field
    {
        natus_this_typedefs( acceleration_field ) ;

    private:

        natus::math::vec2f_t _accl ;

    public:

        acceleration_field( void_t ) noexcept {}
        acceleration_field( natus::math::vec2f_cref_t accl ) noexcept : _accl( accl ) {}

        void_t set_acceleration( natus::math::vec2f_cref_t v ) noexcept
        {
            _accl = v ;
        }

    public:

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            #if 0
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                p.force += _accl * p.mass ;
            }
            #else
            natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>( beg, beg+n ),
                [&]( natus::concurrent::range_1d<size_t> const & r )
                {
                    for( size_t i=r.begin(); i<r.end(); ++i )
                    {
                        nps::particle_ref_t p = particles[i] ;
                        if( !this_t::is_inside( p ) ) continue ;
                        p.force += _accl * p.mass ;
                    }
                } ) ;
            #endif
        }
    } ;
    natus_res_typedef( acceleration_field ) ;

    class friction_force_field : public force_field
    {
        natus_this_typedefs( friction_force_field ) ;

    private:

        float_t _friction = 0.4f ;
        
    public:

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            #if 0
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                p.force -= natus::math::vec2f_t( _friction ) * p.force ;
            }
            #else
            natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>( beg, beg+n ),
                [&]( natus::concurrent::range_1d<size_t> const & r )
                {
                    for( size_t i=r.begin(); i<r.end(); ++i )
                    {
                        nps::particle_ref_t p = particles[i] ;
                        if( !this_t::is_inside( p ) ) continue ;
                        p.force -= natus::math::vec2f_t( _friction ) * p.force ;
                    }
                } ) ;
            #endif
        }
    } ;
    natus_res_typedef( friction_force_field ) ;

    class viscosity_force_field : public force_field
    {
        natus_this_typedefs( viscosity_force_field ) ;

    private:

        float_t _friction = 0.1f ;
        
    public:

        viscosity_force_field( void_t ) noexcept {}
        viscosity_force_field( float_t const friction ) noexcept { _friction = friction ; }

    public:

        virtual void_t apply( size_t const beg, size_t const n, natus::ntd::vector< particle_t > & particles ) const noexcept 
        {
            #if 0
            for( size_t i=beg; i<beg+n; ++i )
            {
                nps::particle_ref_t p = particles[i] ;
                if( !this_t::is_inside( p ) ) continue ;
                p.force -= natus::math::vec2f_t( _friction ) * p.vel  ;
            }
            #else
            natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>( beg, beg+n ),
                [&]( natus::concurrent::range_1d<size_t> const & r )
                {
                    for( size_t i=r.begin(); i<r.end(); ++i )
                    {
                        nps::particle_ref_t p = particles[i] ;
                        if( !this_t::is_inside( p ) ) continue ;
                        p.force -= natus::math::vec2f_t( _friction ) * p.vel  ;
                    }
                } ) ;
            #endif
        }
    } ;
    natus_res_typedef( viscosity_force_field ) ;

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
                typedef natus::concurrent::range_1d<size_t> range_t ;
                auto const & range = range_t( 0, _particles.size() ) ;

                natus::concurrent::parallel_for<size_t>( range, [&]( range_t const & r )
                {
                    for( size_t e=r.begin(); e<r.end(); ++e )
                    {
                        particle_t & p = _particles[e] ;
                        p.age -= dt ;
                    }
                } ) ;
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
                    if( e.emit == 0 ) continue ;
                    e.emt->emit( begin, e.emit, _particles ) ; begin += e.emit ;
                    e.emit = 0 ;
                }
            }
            
            // reset particle physics here so 
            // value can be read elsewhere during a 
            // cycle of physics
            {
                for( auto & p : _particles )
                {
                    //p.force.negate();//natus::math::vec2f_t() ;
                    //p.acl = natus::math::vec2f_t() ;
                    //p.vel = natus::math::vec2f_t() ;
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
                typedef natus::concurrent::range_1d<size_t> range_t ;
                auto const & range = range_t( 0, _particles.size() ) ;

                natus::concurrent::parallel_for<size_t>( range, [&]( range_t const & r )
                {
                    for( size_t e=r.begin(); e<r.end(); ++e )
                    {
                        particle_t & p = _particles[e] ;
                        p.acl = p.force / p.mass ;
                        p.vel += natus::math::vec2f_t( dt ) * p.acl ;
                        p.pos += natus::math::vec2f_t( dt ) * p.vel ;
                    }
                } ) ;
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

    struct test_particle_effect
    {
        natus_this_typedefs( test_particle_effect ) ;

        particle_system_t flakes ;
        emitter_res_t current_emitter ;
        radial_emitter_res_t emitter ;
        line_emitter_res_t lemitter ;
        constant_force_field_res_t wind ;
        acceleration_field_res_t g ;
        friction_force_field_res_t friction ;
        viscosity_force_field_res_t viscosity ;

        test_particle_effect( void_t ) 
        {
            wind = nps::constant_force_field_t() ;
            g = nps::acceleration_field_t( natus::math::vec2f_t( 0.0f, -9.81f ) ) ;
            friction = nps::friction_force_field_t() ;
            viscosity = nps::viscosity_force_field_t( 0.01f ) ;

            //flakes.attach_force_field( wind ) ;
            flakes.attach_force_field( g ) ;
            //flakes.attach_force_field( viscosity ) ;

            wind->set_force( natus::math::vec2f_t( -10.0f, 0.0f ) ) ;

            {
                emitter = nps::radial_emitter_t() ;
                emitter->set_mass( 1.0049f )  ;
                emitter->set_age( 2.5f ) ;
                emitter->set_amount( 10 ) ;
                emitter->set_rate( 2.0f ) ;
                emitter->set_angle( 0.3f ) ;
                emitter->set_velocity( 200.0f ) ;
                emitter->set_direction( natus::math::vec2f_t( 0.0f,1.0f ) ) ;
            }

            {
                lemitter = nps::line_emitter_t() ;
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

        void_t set_wind_velocity( natus::math::vec2f_cref_t v ) noexcept
        {
            wind->set_force( v ) ;
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
                    pr->draw_circle( 4, 10, p.pos, p.mass, 
                        natus::math::vec4f_t(1.0f), 
                        natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f) ) ;
                    #elif 1
                    natus::math::vec2f_t const pos = p.pos ;
                    float_t const s = p.mass ;

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
    natus_typedef( test_particle_effect ) ;
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

        nps::test_particle_effect_t _spe ;

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

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei )
        {
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ),
                float_t(wei.w) / float_t(wei.h), 1.0f, 1000.0f ) ;

            natus::math::vec2f_t const target = natus::math::vec2f_t(800, 600) ; 
            natus::math::vec2f_t const window = natus::math::vec2f_t( float_t(wei.w), float_t(wei.h) ) ;

            natus::math::vec2f_t const ratio = window / target ;

            _camera_0.orthographic( wei.w, wei.h, 0.1f, 1000.0f ) ;

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

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t  ) 
        {
            _pr->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;

            // draw particle effects
            {
                _spe.render( _pr ) ;
                _pr->draw_circle( 0, 20, _spe.emitter->get_position(), _spe.emitter->get_radius(), 
                    natus::math::vec4f_t(0.0f, 0.5f,0.0f,0.1f),  natus::math::vec4f_t(0.0f,0.5f,0.0f,0.1f)) ;
            }

            // draw extend
            {
                natus::math::vec2f_t p0 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(-0.5f,-0.5f) ;
                natus::math::vec2f_t p1 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(-0.5f,+0.5f) ;
                natus::math::vec2f_t p2 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(+0.5f,+0.5f) ;
                natus::math::vec2f_t p3 = natus::math::vec2f_t() + _extend * natus::math::vec2f_t(+0.5f,-0.5f) ;

                natus::math::vec4f_t color0( 1.0f, 1.0f, 1.0f, 0.0f ) ;
                natus::math::vec4f_t color1( 1.0f, 1.0f, 1.0f, 1.0f ) ;

                _pr->draw_rect( 50, p0, p1, p2, p3, color0, color1 ) ;
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

        virtual natus::application::result on_tool( natus::tool::imgui_view_t imgui )
        {
            if( !_do_tool ) return natus::application::result::no_imgui ;

            ImGui::Begin( "View Control" ) ;
            {
                float_t data[2] = {_extend.x(), _extend.y() } ;
                ImGui::SliderFloat2( "Extend", data, 0.0f, 1000.0f, "%f", 1.0f ) ;
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

            if( item_current == 0 )
            {
                if( item_changed ) 
                {
                    _spe.current_emitter = _spe.emitter ;
                    _spe.flakes.attach_emitter( _spe.current_emitter ) ;
                }

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
                

                {
                    float_t v = _spe.emitter->get_angle() ;
                    if( ImGui::VSliderFloat("Angle", ImVec2(50,100), &v, 0.0f, 3.14f ) )
                    {
                        _spe.emitter->set_angle( v ) ;
                    }
                }

                ImGui::SameLine() ;
                {
                    float_t v = _spe.emitter->get_velocity() ;
                    if( ImGui::VSliderFloat("Velocity", ImVec2(50,100), &v, 0.0f, 1000.0f ) )
                    {
                        _spe.emitter->set_velocity( v ) ;
                    }
                }

                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_mass_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Mass Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_mass_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_mass_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_age_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Age Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_age_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_age_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_acceleration_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Acceleration Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_acceleration_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_acceleration_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_velocity_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Velocity Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_velocity_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_velocity_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_radius_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Radius Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_radius_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_radius_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_angle_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Angle Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.emitter->set_angle_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.emitter->set_angle_variation_type( nps::variation_type::random ) ;
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
                    auto dir = _spe.lemitter->get_direction() ;
                    if( natus::tool::custom_imgui_widgets::direction( "dir", dir ) )
                    {
                        _spe.lemitter->set_direction( dir ) ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.lemitter->get_rate() ;
                    if( ImGui::VSliderFloat("Rate", ImVec2(50,100), &v, 1.0f, 30.0f ) )
                    {
                        _spe.lemitter->set_rate( v ) ;
                        _spe.flakes.clear() ;
                    }
                }
                ImGui::SameLine() ;
                {
                    int_t v = _spe.lemitter->get_amount() ;
                    if( ImGui::VSliderInt("Amount", ImVec2(50,100), &v, 1, 200 ) )
                    {
                        _spe.lemitter->set_amount( v ) ;
                        _spe.flakes.clear() ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.lemitter->get_age() ;
                    if( ImGui::VSliderFloat("Age", ImVec2(50,100), &v, 1.0f, 10.0f ) )
                    {
                        _spe.lemitter->set_age( v ) ;
                    }
                }
                
                {
                    float_t v = _spe.lemitter->get_ortho_distance() ;
                    if( ImGui::VSliderFloat("Ortho Dist", ImVec2(50,100), &v, -400.0f, 400.0f ) )
                    {
                        _spe.lemitter->set_ortho_distance( v ) ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.lemitter->get_parallel_distance() ;
                    if( ImGui::VSliderFloat("Parallel Dist", ImVec2(50,100), &v, 1.0f, 400.0f ) )
                    {
                        _spe.lemitter->set_parallel_distance( v ) ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.lemitter->get_velocity() ;
                    if( ImGui::VSliderFloat("Velocity", ImVec2(50,100), &v, 0.0f, 1000.0f ) )
                    {
                        _spe.lemitter->set_velocity( v ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_mass_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Mass Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_mass_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_mass_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_age_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Age Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_age_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_age_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_acceleration_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Acceleration Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_acceleration_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_acceleration_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_velocity_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Velocity Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_velocity_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_velocity_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_ortho_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Ortho Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_ortho_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_ortho_variation_type( nps::variation_type::random ) ;
                    }
                }
                {
                    const char* items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_parallel_variation_type() == nps::variation_type::fixed ? 0 : 1 ;
                    if( item_changed = ImGui::Combo( "Parallel Variation", &rand_item, items, IM_ARRAYSIZE(items) ) )
                    {
                        if( rand_item == 0 ) _spe.lemitter->set_parallel_variation_type( nps::variation_type::fixed ) ;
                        else if( rand_item == 1 ) _spe.lemitter->set_parallel_variation_type( nps::variation_type::random ) ;
                    }
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
