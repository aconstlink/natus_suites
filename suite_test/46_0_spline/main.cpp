
#include <natus/math/vector/vector2.hpp>
#include <natus/math/spline/linear_bezier_spline.hpp>
#include <natus/math/spline/quadratic_bezier_spline.hpp>

#include <natus/log/global.h>

namespace this_file
{
    static natus::ntd::string_t print( natus::math::vec2f_cref_t v ) noexcept
    {
        return "(" + std::to_string( v.x() ) + "; " + std::to_string( v.y() ) + ")" ;
    }
}

int main( int argc, char ** argv )
{
    natus::log::global_t::status("************* TEST 1 ***********************") ;
    {
        typedef natus::math::linear_bezier_spline< float_t > spline_t ;
        spline_t s = spline_t( {0.0f, 5.0f, 13.0f, 10.0f} ) ;

        float_t const step = (1.0f / float_t( s.num_control_points() - 1 ))*0.5f ;

        for( size_t i=0; i<(s.num_control_points()*2)-1; ++i )
        {
            float_t const t = i * step ;
            natus::log::global_t::status( "s( " + std::to_string( t ) + ") = " + std::to_string( s( t ) ) ) ;
        }
    }

    natus::log::global_t::status("************* TEST 2 ***********************") ;
    {
        typedef natus::math::linear_bezier_spline< natus::math::vec2f_t > spline_t ;
        spline_t s = spline_t( { 
            natus::math::vec2f_t(0.0f, 1.0f), natus::math::vec2f_t(-5.0f, 5.0f), 
            natus::math::vec2f_t(10.0f, -15.0f), natus::math::vec2f_t(100.0f, 40.5f) } ) ;

        float_t const step = (1.0f / float_t( s.num_control_points() - 1 ))*0.5f ;

        for( size_t i=0; i<(s.num_control_points()*2)-1; ++i )
        {
            float_t const t = i * step ;
            natus::log::global_t::status( "s( " + std::to_string( t ) + ") = " + this_file::print( s( t ) ) ) ;
        }
    }

    {
        natus::math::quadratic_bezier_spline< natus::math::vec2f_t > s ;

    }

    return 0 ;
}
