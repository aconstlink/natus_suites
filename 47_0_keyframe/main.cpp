
#include <natus/math/vector/vector2.hpp>
#include <natus/math/animation/linear_keyframe_sequence.hpp>
#include <natus/math/animation/keyframe_sequence.hpp>
#include <natus/math/spline/cubic_hermit_spline.hpp>

#include <natus/log/global.h>

int main( int argc, char ** argv )
{
    {
        natus::math::linear_keyframe_sequence< float_t > kf ;

        kf.insert( natus::math::keyframe< float_t >( 0, 0.0f ) ) ;
        kf.insert( natus::math::keyframe< float_t >( 1000, 1.0f ) ) ;
        kf.insert( natus::math::keyframe< float_t >( 3000, 25.0f ) ) ;
        kf.insert( natus::math::keyframe< float_t >( 2000, 10.0f ) ) ;

        natus::log::global_t::status( std::to_string( kf( 0 ) ) ) ;
        natus::log::global_t::status( std::to_string( kf( 1000 ) ) ) ;
        natus::log::global_t::status( std::to_string( kf( 2000 ) ) ) ;
        natus::log::global_t::status( std::to_string( kf( 3000 ) ) ) ;
    }

    {
        natus::math::linear_keyframe_sequence< natus::math::vec2f_t > kf ;

        kf.insert( natus::math::keyframe< natus::math::vec2f_t >( 0, natus::math::vec2f_t(0.0f) ) ) ;
        kf.insert( natus::math::keyframe< natus::math::vec2f_t >( 1000, natus::math::vec2f_t(1.0f, 0.0f) ) ) ;
        kf.insert( natus::math::keyframe< natus::math::vec2f_t >( 3000, natus::math::vec2f_t(1.0f, 0.5f) ) ) ;
        kf.insert( natus::math::keyframe< natus::math::vec2f_t >( 2000, natus::math::vec2f_t(0.0f, 1.0f ) ) ) ;

        natus::log::global_t::status( std::to_string( kf( 0 ).x() ) + ":" + std::to_string( kf( 0 ).y() ) ) ;
        natus::log::global_t::status( std::to_string( kf( 1000 ).x() ) + ":" + std::to_string( kf( 1000 ).y() ) ) ;
        natus::log::global_t::status( std::to_string( kf( 2000 ).x() ) + ":" + std::to_string( kf( 2000 ).y() ) ) ;
        natus::log::global_t::status( std::to_string( kf( 3000 ).x() ) + ":" + std::to_string( kf( 3000 ).y() ) ) ;
    }

    {
        typedef natus::math::cubic_hermit_spline<float_t> spline_t ;
        typedef natus::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

        keyframe_sequence_t kf ;
        kf.insert( keyframe_sequence_t::keyframe_t( 0, 0.0f ) ) ;
        kf.insert( keyframe_sequence_t::keyframe_t( 1000, 1.0f ) ) ;
        kf.insert( keyframe_sequence_t::keyframe_t( 4000, 25.0f ) ) ;
        kf.insert( keyframe_sequence_t::keyframe_t( 2000, 10.0f ) ) ;

        natus::log::global_t::status( std::to_string( kf( 0 ) ) ) ;
        natus::log::global_t::status( std::to_string( kf( 1000 ) ) ) ;
        natus::log::global_t::status( std::to_string( kf( 2000 ) ) ) ;
        natus::log::global_t::status( std::to_string( kf( 3000 ) ) ) ;
        natus::log::global_t::status( std::to_string( kf( 4000 ) ) ) ;
    }

    return 0 ;
}
