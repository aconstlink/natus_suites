
#include <natus/math/vector/vector2.hpp>
#include <natus/math/spline/linear_bezier_spline.hpp>
#include <natus/math/spline/quadratic_bezier_spline.hpp>
#include <natus/math/matrix/matrix2.hpp>
#include <natus/math/quaternion/quaternion4.hpp>
#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/fn.hpp>

#include <natus/log/global.h>

using namespace natus::core::types ;

int main( int argc, char ** argv )
{
    // test rotation direction
    {
        float_t const a = natus::math::angle<float_t>::degree_to_radian(90.0f) ;
        natus::math::quat4f_t const q( a, natus::math::vec3f_t(0.0f, 0.0f, 1.0f) ) ;

        auto const v = q * natus::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ;
        int bp = 0 ;
    }

    // test matrix rotation
    {
        float_t const a = natus::math::angle<float_t>::degree_to_radian(90.0f) ;
        natus::math::mat2f_t const r = natus::math::mat2f_t::make_rotation_matrix( a ) ;

        auto const v = r * natus::math::vec2f_t( 1.0f, 0.0f ) ;
        int bp = 0 ;
    }

    // little test scope here
    {
        auto z_comp_of_cross3= [=]( natus::math::vec2f_cref_t a, natus::math::vec2f_cref_t b )
        {
            return a.x()*b.y() - a.y()*b.x() ;
        } ;

        natus::math::vec2f_t const a = natus::math::vec2f_t(1.0f,0.0f).normalize() ;
        natus::math::vec2f_t const b = natus::math::vec2f_t(0.7f,0.7f).normalize() ;
        natus::math::vec2f_t const c = natus::math::vec2f_t(-0.7f,0.7f).normalize() ;
        natus::math::vec2f_t const d = natus::math::vec2f_t(-0.7f,-0.7f).normalize() ;
        natus::math::vec2f_t const e = natus::math::vec2f_t(0.7f,-0.7f).normalize() ;

        // cos of angle
        float_t const c_ab = a.dot( b ) ;
        float_t const c_ac = a.dot( c ) ;
        float_t const c_ad = a.dot( d ) ;
        float_t const c_ae = a.dot( e ) ;

        // sin of angle
        float_t const s_ab = z_comp_of_cross3( a, b ) ;
        float_t const s_ac = z_comp_of_cross3( a, c ) ;
        float_t const s_ad = z_comp_of_cross3( a, d ) ;
        float_t const s_ae = z_comp_of_cross3( a, e ) ;

        // both from top compact
        auto const dc_ab = a.dot_cross(b) ;
        auto const dc_ac = a.dot_cross(c) ;
        auto const dc_ad = a.dot_cross(d) ;
        auto const dc_ae = a.dot_cross(e) ;

        // arccos values
        float_t const ac_ab = std::acos( c_ab ) ;
        float_t const ac_ac = std::acos( c_ac ) ;
        float_t const ac_ad = std::acos( c_ad ) ;
        float_t const ac_ae = std::acos( c_ae ) ;
                
        float_t const sig_ab = dc_ab.sign().y() ;
        float_t const sig_ac = dc_ac.sign().y() ;
        float_t const sig_ad = dc_ad.sign().y() ;
        float_t const sig_ae = dc_ae.sign().y() ;

        // indices into the offset vector
        uint_t const i_ab = 1 - uint_t( natus::math::fn<float_t>::nnv_to_pnv( sig_ab ) ) ;
        uint_t const i_ac = 1 - uint_t( natus::math::fn<float_t>::nnv_to_pnv( sig_ac ) ) ;
        uint_t const i_ad = 1 - uint_t( natus::math::fn<float_t>::nnv_to_pnv( sig_ad ) ) ;
        uint_t const i_ae = 1 - uint_t( natus::math::fn<float_t>::nnv_to_pnv( sig_ae ) ) ;

        natus::math::vec2f_t const offsets( 0, natus::math::constants<float_t>::pix2() ) ;

        // full angles
        float_t const full_ab = ac_ab * sig_ab + offsets[ i_ab ] ;
        float_t const full_ac = ac_ac * sig_ac + offsets[ i_ac ] ;
        float_t const full_ad = ac_ad * sig_ad + offsets[ i_ad ] ;
        float_t const full_ae = ac_ae * sig_ae + offsets[ i_ae ] ;

        // atan2
        float_t const at_ab = std::atan2( dc_ab.y(), dc_ab.x() ) ;
        float_t const at_ac = std::atan2( dc_ac.y(), dc_ac.x() ) ;
        float_t const at_ad = std::atan2( dc_ad.y(), dc_ad.x() ) ;
        float_t const at_ae = std::atan2( dc_ae.y(), dc_ae.x() ) ;

        // in degrees for better readibility
        float_t const a_ab = natus::math::angle<float_t>::radian_to_degree( full_ab ) ;
        float_t const a_ac = natus::math::angle<float_t>::radian_to_degree( full_ac ) ;
        float_t const a_ad = natus::math::angle<float_t>::radian_to_degree( full_ad ) ;
        float_t const a_ae = natus::math::angle<float_t>::radian_to_degree( full_ae ) ;

        // with atan2 result -> does not give the full angle 
        float_t const a2_ab = natus::math::angle<float_t>::radian_to_degree( at_ab ) ;
        float_t const a2_ac = natus::math::angle<float_t>::radian_to_degree( at_ac ) ;
        float_t const a2_ad = natus::math::angle<float_t>::radian_to_degree( at_ad ) ;
        float_t const a2_ae = natus::math::angle<float_t>::radian_to_degree( at_ae ) ;

        // using the vector extension function
        float_t const a3_ab = natus::math::angle<float_t>::radian_to_degree( natus::math::vec2fe_t::full_angle(a, b) ) ;
        float_t const a3_ac = natus::math::angle<float_t>::radian_to_degree( natus::math::vec2fe_t::full_angle(a, c) ) ;
        float_t const a3_ad = natus::math::angle<float_t>::radian_to_degree( natus::math::vec2fe_t::full_angle(a, d) ) ;
        float_t const a3_ae = natus::math::angle<float_t>::radian_to_degree( natus::math::vec2fe_t::full_angle(a, e) ) ;

        // using the vector extension function
        // full_angle is invariant. 
        float_t const a4_ab = natus::math::angle<float_t>::radian_to_degree( natus::math::vec2fe_t::full_angle(a, b) ) ;
        float_t const a4_ba = natus::math::angle<float_t>::radian_to_degree( natus::math::vec2fe_t::full_angle(b, a) ) ;

        int bp = 0 ;
    }

    // test cos
    {
        float_t const a = natus::math::angle<float_t>::radian_to_degree(std::acos( -0.1f )) ;
        float_t const b = natus::math::angle<float_t>::radian_to_degree(std::acos( +0.1f )) ;
        int bp=0; 
    }
    return 0 ;
}
