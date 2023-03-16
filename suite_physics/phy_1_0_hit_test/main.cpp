

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

    enum class shape_type
    {
        unknown,
        ray,
        circle,
        aabb
        // obb
        // circle_tree
        // aabb_tree
    };

    //****************************************************************************************
    class shape_property
    {

    public:

        shape_property( void_t ) noexcept {}
        virtual ~shape_property( void_t ) noexcept {}

        virtual shape_type get_type( void_t ) const noexcept = 0 ;
        virtual void_t translate_to( natus::math::vec2f_cref_t p ) noexcept = 0 ;
        virtual natus::math::vec2f_t get_position( void_t ) const noexcept = 0 ;
    };
    natus_res_typedef( shape_property ) ;

    //****************************************************************************************
    class circle_shape_property : public shape_property
    {
        natus_this_typedefs( circle_shape_property ) ;

        natus::collide::n2d::circlef_t _circle ;

    public:

        circle_shape_property( void_t ) noexcept {}
        circle_shape_property( natus::collide::n2d::circlef_cref_t c ) noexcept : _circle( c ) {}
        circle_shape_property( this_cref_t rhv ) noexcept : _circle( rhv._circle ) {}
        virtual ~circle_shape_property( void_t ) noexcept {}

        natus::collide::n2d::circlef_cref_t get_circle( void_t ) const noexcept { return _circle ; }
        natus::collide::n2d::circlef_ref_t get_circle( void_t ) noexcept { return _circle ; }
        void_t set_circle( natus::collide::n2d::circlef_cref_t c ) noexcept { _circle = c ; }

        virtual shape_type get_type( void_t ) const noexcept { return shape_type::circle ; }
        virtual void_t translate_to( natus::math::vec2f_cref_t p ) noexcept { _circle.set_center( p ) ; }
        virtual natus::math::vec2f_t get_position( void_t ) const noexcept { return _circle.get_center() ; }
    };
    natus_res_typedef( circle_shape_property ) ;

    //****************************************************************************************
    class aabb_shape_property : public shape_property
    {
        natus_this_typedefs( aabb_shape_property ) ;

    public:

        natus_typedefs( natus::collide::n2d::aabbf_t, box ) ;

    private:

        box_t _box ;

    public:

        aabb_shape_property( void_t ) noexcept {}
        aabb_shape_property( box_cref_t b ) noexcept : _box( b ) {}
        aabb_shape_property( this_cref_t rhv ) noexcept : _box( rhv._box ) {}
        virtual ~aabb_shape_property( void_t ) noexcept {}

        box_cref_t get_box( void_t ) const noexcept { return _box ; }
        box_ref_t get_box( void_t ) noexcept { return _box ; }
        void_t set_box( box_cref_t b ) noexcept { _box = b ; }

        virtual shape_type get_type( void_t ) const noexcept { return shape_type::aabb ; }
        virtual void_t translate_to( natus::math::vec2f_cref_t p ) noexcept { _box.translate_to_position( p ) ; }
        virtual natus::math::vec2f_t get_position( void_t ) const noexcept { return _box.get_center() ; }
    };
    natus_res_typedef( aabb_shape_property ) ;


    //****************************************************************************************
    class ray_shape_property : public shape_property
    {
        natus_this_typedefs( ray_shape_property ) ;

    public:

        natus_typedefs( natus::math::ray2f_t, ray ) ;

    private:

        ray_t _ray ;

    public:

        ray_shape_property( void_t ) noexcept {}
        ray_shape_property( ray_cref_t r ) noexcept : _ray( r ) {}
        ray_shape_property( this_cref_t rhv ) noexcept : _ray( rhv._ray ) {}
        virtual ~ray_shape_property( void_t ) noexcept {}

        ray_cref_t get_ray( void_t ) const noexcept { return _ray ; }
        ray_ref_t get_ray( void_t ) noexcept { return _ray ; }
        void_t set_ray( ray_cref_t r ) noexcept { _ray = r ; }

        virtual shape_type get_type( void_t ) const noexcept { return shape_type::ray ; }
        virtual void_t translate_to( natus::math::vec2f_cref_t p ) noexcept { _ray.translate_to( p ) ; }
        virtual natus::math::vec2f_t get_position( void_t ) const noexcept { return _ray.get_origin() ; }
    };
    natus_res_typedef( ray_shape_property ) ;

    //****************************************************************************************
    //
    //
    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;

    private:

        natus::ntd::vector< shape_property_res_t > _shapes ;
        shape_property_res_t _mouse_shape ;
        
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
        test_app( this_rref_t rhv ) noexcept : app( std::move( rhv ) ) 
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

            // mouse shape
            {
                natus::math::vec2f_t const pos ;
                float_t const r = 0.3f ;
                _mouse_shape = this_file::circle_shape_property_res_t( 
                    this_file::circle_shape_property_t( natus::collide::n2d::circlef_t( pos, r ) ) ) ;
            }

            // init obstacles
            {
                {
                    natus::math::vec2f_t const pos( -5.0f, -2.0f ) ;
                    float_t const r = 1.0f ;
                    _shapes.emplace_back( this_file::aabb_shape_property_res_t( 
                        this_file::aabb_shape_property_t( this_file::aabb_shape_property_t::box_t( pos, r ) ) ) ) ;
                }

                {
                    natus::math::vec2f_t const pos( -2.0f, -2.0f ) ;
                    float_t const r = 1.0f ;
                    _shapes.emplace_back( this_file::circle_shape_property_res_t( 
                    this_file::circle_shape_property_t( natus::collide::n2d::circlef_t( pos, r ) ) ) ) ;
                }
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
                    auto const cur_mouse_pos = _ae.get_cur_mouse_pos() / _ppm ;

                    if( _mouse_shape->get_type() == this_file::shape_type::ray )
                    {
                        this_file::ray_shape_property_res_t c = _mouse_shape ;
                        auto const dir = ( cur_mouse_pos - _mouse_shape->get_position() ).normalize() ;
                        c->get_ray().set_direction( dir ) ;
                    }
                }

                if( mouse.is_pressing( natus::device::layouts::three_mouse::button::left ) )
                {
                    _mouse_shape->translate_to( _ae.get_cur_mouse_pos() / _ppm ) ;
                }
            }

            // keyboard
            {
                natus::device::layouts::ascii_keyboard_t keyboard( _ae.get_ascii_dev() ) ;
                
                if( keyboard.get_state( natus::device::layouts::ascii_keyboard::ascii_key::space ) == 
                    natus::device::components::key_state::pressing )
                {
                }
                else if( keyboard.get_state( natus::device::layouts::ascii_keyboard::ascii_key::space ) == 
                    natus::device::components::key_state::released )
                {
                }
            }

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            return natus::application::result::ok ; 
        }

        struct hit_test_result
        {
            bool_t valid = false ;
            this_file::shape_type st0 = this_file::shape_type::unknown ;
            this_file::shape_type st1 = this_file::shape_type::unknown ;
            natus::collide::hit_test_type htt = natus::collide::hit_test_type::unknown ;

            size_t num_contacts = 0 ;
            natus::math::vec2f_t contact_points[3] ;
            natus::math::vec3f_t contact_normals[3] ;
        };

        typedef std::function< hit_test_result ( this_file::shape_property_res_t, this_file::shape_property_res_t ) > hit_test_funk_t ;
        static hit_test_funk_t demux( this_file::shape_type const s0, this_file::shape_type const s1 ) noexcept
        {
            hit_test_funk_t unknown = [&]( this_file::shape_property_res_t, this_file::shape_property_res_t ){ return hit_test_result() ;} ;

            hit_test_funk_t circle_ray = [=]( this_file::shape_property_res_t r0, this_file::shape_property_res_t r1 )
            { 
                this_file::circle_shape_property_res_t o0 = r0 ;
                this_file::ray_shape_property_res_t o1 = r1 ;

                natus::math::vec2f_t dist ;

                hit_test_result ret ;
                ret.valid = true ;
                ret.st0 = s0 ;
                ret.st1 = s1 ;
                ret.htt = natus::collide::n2d::hit_tests<float_t>::ray_circle( o1->get_ray(), o0->get_circle(), dist ) ;

                ret.num_contacts = 2 ;

                ret.contact_points[0] = o1->get_ray().point_at( dist.x() ) ;
                ret.contact_normals[0] = o0->get_circle().calculate_normal_distance_to_bound_for( ret.contact_points[0] ) ;

                ret.contact_points[1] = o1->get_ray().point_at( dist.y() ) ;
                ret.contact_normals[1] = o0->get_circle().calculate_normal_distance_to_bound_for( ret.contact_points[1] ) ;
                
                return ret ;
            } ;
            hit_test_funk_t ray_circle = [=]( this_file::shape_property_res_t r0, this_file::shape_property_res_t r1 )
            {
                return circle_ray( r1, r0 ) ;
            } ;

            hit_test_funk_t circle_circle = [&]( this_file::shape_property_res_t r0, this_file::shape_property_res_t r1 )
            { 
                this_file::circle_shape_property_res_t o0 = r0 ;
                this_file::circle_shape_property_res_t o1 = r1 ;

                hit_test_result ret ;
                ret.valid = true ;
                ret.st0 = s0 ;
                ret.st1 = s1 ;
                ret.htt = natus::collide::n2d::hit_tests<float_t>::circle_circle_overlap( o1->get_circle(), o0->get_circle() ) ;
                
                auto const nrm = o1->get_circle().calculate_normal_distance_to_bound_for( o0->get_circle().get_center() ) ;
                ret.num_contacts = 1 ;
                ret.contact_normals[0] = nrm ;
                ret.contact_points[0] = o1->get_circle().get_center() + nrm.xy() * nrm.z() ;

                return ret ;
            } ;
            hit_test_funk_t aabb_aabb = [&]( this_file::shape_property_res_t, this_file::shape_property_res_t )
            { 
                return hit_test_result() ;
            } ;
            hit_test_funk_t circle_aabb = [&]( this_file::shape_property_res_t r0, this_file::shape_property_res_t r1 )
            { 
                this_file::circle_shape_property_res_t o0 = r0 ;
                this_file::aabb_shape_property_res_t o1 = r1 ;

                hit_test_result ret ;
                ret.valid = true ;
                ret.st0 = s0 ;
                ret.st1 = s1 ;
                ret.htt = natus::collide::n2d::hit_tests<float_t>::aabb_circle( o1->get_box(), o0->get_circle() ) ;
                auto const nrm = o1->get_box().calculate_normal_for( o0->get_circle().get_center() ) ;
                ret.num_contacts = 1 ;
                ret.contact_normals[0] = nrm ;
                ret.contact_points[0] = o0->get_circle().get_center() + nrm.xy() * 
                    nrm.dot( natus::math::vec3f_t( o0->get_circle().get_center() - o1->get_box().get_center(), 1.0f ) ) ;

                return ret ;
            } ;
            hit_test_funk_t aabb_circle = [&]( this_file::shape_property_res_t r0, this_file::shape_property_res_t r1 ){ return circle_aabb(r0, r1) ;} ;

            switch( s0 )
            {
            case this_file::shape_type::ray:
            {
                switch( s1 )
                {
                case this_file::shape_type::circle:
                    return ray_circle ;
                case this_file::shape_type::aabb:
                    return unknown ;
                case this_file::shape_type::ray:
                    return unknown ;
                }
                break ;
            }
            case this_file::shape_type::circle:
            {
                switch( s1 )
                {
                case this_file::shape_type::circle:
                    return circle_circle ;
                case this_file::shape_type::aabb:
                    return circle_aabb ;
                case this_file::shape_type::ray:
                    return circle_ray ;
                }
                break ;
            }
            case this_file::shape_type::aabb:
            {
                switch( s1 )
                {
                case this_file::shape_type::circle:
                    return aabb_circle ;
                case this_file::shape_type::aabb:
                    return aabb_aabb ;
                case this_file::shape_type::ray:
                    return unknown ;
                }
                break ;
            }
            }

            return unknown ;
        }

        // this one is used for visual hit test notification
        size_t _num_htr = 0 ;
        hit_test_result _htr[10] ;

        //***************************************************************************************************************
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            _num_htr = 0 ;

            auto const s0 = _mouse_shape->get_type() ;

            for( auto & s : _shapes )
            {
                auto const s1 = s->get_type() ;
                auto res = demux( s0, s1 )( _mouse_shape, s ) ;
                if( res.htt == natus::collide::hit_test_type::intersect ||
                    res.htt == natus::collide::hit_test_type::overlap ||
                    res.htt == natus::collide::hit_test_type::inside )
                {
                    _htr[_num_htr++] = res ;
                }
            }
            return natus::application::result::ok ; 
        }

        //***************************************************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _ae.on_graphics_begin( rd ) ;

            auto pr = _ae.get_prim_render() ;

            // draw shapes
            for( auto & s : _shapes )
            {
                switch( s->get_type() )
                {
                case this_file::shape_type::circle: 
                {
                    this_file::circle_shape_property_res_t sr = s ;

                    auto const p0 = sr->get_circle().get_center() * _ppm ;
                    pr->draw_circle( 5, 20, p0, sr->get_circle().get_radius()*_ppm, 
                        natus::math::vec4f_t(0.2f,0.2f,0.2f,1.0f),natus::math::vec4f_t(1.0f) ) ;

                    break ;
                }
                case this_file::shape_type::aabb: 
                {
                    this_file::aabb_shape_property_res_t sr = s ;

                    natus::math::vec2f_t points[4] ;
                    sr->get_box().get_points( points ) ;
                    pr->draw_rect( 5, points[0]*_ppm, points[1]*_ppm, points[2]*_ppm, points[3]*_ppm, 
                        natus::math::vec4f_t( 0.2f, 0.2f, 0.2f, 1.0f ), natus::math::vec4f_t(1.0f) ) ;
                    break ;
                }
                default: break ;
                }
            }

            // draw mouse
            {
                switch( _mouse_shape->get_type() )
                {
                case this_file::shape_type::circle: 
                {
                    this_file::circle_shape_property_res_t sr = _mouse_shape ;

                    auto const p0 = sr->get_circle().get_center() * _ppm ;
                    pr->draw_circle( 7, 20, p0, sr->get_circle().get_radius()*_ppm, 
                        natus::math::vec4f_t(0.3f,0.3f,0.3f,1.0f),natus::math::vec4f_t(1.0f) ) ;

                    break ;
                }
                case this_file::shape_type::aabb: 
                {
                    this_file::aabb_shape_property_res_t sr = _mouse_shape ;

                    natus::math::vec2f_t points[4] ;
                    sr->get_box().get_points( points ) ;
                    pr->draw_rect( 5, points[0]*_ppm, points[1]*_ppm, points[2]*_ppm, points[3]*_ppm, 
                        natus::math::vec4f_t( 0.3f, 0.3f, 0.3f, 1.0f ), natus::math::vec4f_t(1.0f) ) ;
                    break ;
                }
                case this_file::shape_type::ray: 
                {
                    this_file::ray_shape_property_res_t sr = _mouse_shape ;

                    // draw little circle around ray origin
                    {
                        auto const p0 = sr->get_ray().get_origin() * _ppm ;
                        pr->draw_circle( 7, 20, p0, 10.0f, 
                            natus::math::vec4f_t(0.3f,0.3f,0.3f,1.0f),natus::math::vec4f_t(1.0f) ) ;
                    }

                    // draw direction of ray
                    {
                        auto const p0 = sr->get_ray().get_origin() * _ppm ;
                        auto const p1 = p0 + sr->get_ray().get_direction() * 80.0f ;
                        pr->draw_line( 7, p0, p1, natus::math::vec4f_t(0.0f, 1.0f, 0.0, 1.0f) ) ;
                    }
                }

                default: break ;
                }
            }

            // draw contact points
            {
                for( size_t h=0; h<_num_htr; ++h )
                {
                    for( size_t i=0; i<_htr[h].num_contacts; ++i )
                    {
                        // draw normal
                        {
                            auto const p0 = _htr[h].contact_points[i] * _ppm ;
                            auto const p1 = p0 + _htr[h].contact_normals[i].xy() * 1.0f * _ppm ;
                            pr->draw_line( 15, p0, p1, natus::math::vec4f_t(0.0f, 1.0f, 0.0, 1.0f) ) ;
                        }
                    }
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
                const char* items[] = { "Circle", "Ray" };
                static int item_current = 0;
                if( ImGui::Combo( "Mouse Shape", &item_current, items, IM_ARRAYSIZE(items) ) )
                {
                    if( items[item_current] == "Circle" && _mouse_shape->get_type() != this_file::shape_type::circle )
                    {
                        natus::math::vec2f_t const pos = _mouse_shape->get_position() ;
                        float_t const r = 0.3f ;
                        _mouse_shape = this_file::circle_shape_property_res_t( 
                            this_file::circle_shape_property_t( natus::collide::n2d::circlef_t( pos, r ) ) ) ;
                    }
                    else if( items[item_current] == "Ray" && _mouse_shape->get_type() != this_file::shape_type::ray)
                    {
                        natus::math::vec2f_t const pos = _mouse_shape->get_position() ;
                        natus::math::vec2f_t const dir = natus::math::vec2f_t(1.0f, 0.0f ) ;

                        _mouse_shape = this_file::ray_shape_property_res_t( 
                            this_file::ray_shape_property_t( natus::math::ray2f_t( pos, dir ) ) ) ;
                    }
                }
            }

            if( _mouse_shape->get_type() == this_file::shape_type::circle )
            {
                ImGui::Text( "Mouse Shape is: %s", "Circle" ) ;

                this_file::circle_shape_property_res_t c = _mouse_shape ;
                float_t r = c->get_circle().get_radius() ;
                ImGui::SliderFloat( "Radius", &r, 0.01f, 2.0f ) ;
                c->get_circle().set_radius( r ) ;
            }
            else if( _mouse_shape->get_type() == this_file::shape_type::circle )
            {
                ImGui::Text( "Mouse Shape is: %s", "Ray" ) ;
            }

            {
                //ImGui::Text( "Hit test type: %s", natus::collide::to_string( _htr.htt ).c_str() ) ;
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
