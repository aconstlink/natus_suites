
#pragma once

#include <natus/math/vector/vector2.hpp>

namespace world
{
    using namespace natus::core::types ;

    // G : granularity of a sector in units
    template< uint_t G >
    class coordinate
    {
        natus_this_typedefs( coordinate ) ;

    private:

        // Section id
        natus::math::vec2i_t _ij ;

        // relative position to sector origin
        // could also be a direction, depending on the operation done
        natus::math::vec2f_t _vec ;

    private:
        
        // evaluate internal parameters to be
        // correct sector coordinate
        // 1. there can not be any negative positions(except for relative coords)
        // 2. there can not be any positions greater than the granularity
        this_ref_t evaluate( void_t ) noexcept
        {
            natus::math::vec2f_t const tmp = _vec + natus::math::vec2f_t( this_t::granularity_half() ) ;
            natus::math::vec2i_t const ij_min = tmp.less_than( natus::math::vec2i_t(0) ).select( natus::math::vec2i_t(-1), natus::math::vec2i_t(0) ) ;
            natus::math::vec2i_t const ij_rel = (natus::math::vec2i_t( tmp ) / this_t::granularity()) + ij_min ;
            
            _ij = _ij + ij_rel ;
            _vec = (ij_rel * this_t::granularity()).negate() + _vec ;

            return *this ;
        }

    public:

        coordinate( void_t ) noexcept {}
        coordinate( this_cref_t rhv ) noexcept 
        {
            _ij = rhv._ij ;
            _vec = rhv._vec ;
        }

        ~coordinate( void_t ) noexcept {}


    private:

        coordinate( natus::math::vec2ui_cref_t ij ) noexcept
        {
            _ij = ij ;
        }

        coordinate( natus::math::vec2f_cref_t pos ) noexcept
        {
            _vec = pos ;
        }

        coordinate( natus::math::vec2ui_cref_t ij, natus::math::vec2f_cref_t pos ) noexcept
        {
            _ij = ij ;
            _vec = pos ;
        }

    public:

        static this_t from_position( natus::math::vec2f_cref_t pos ) noexcept
        {
            return this_t( pos ).evaluate() ;
        }

        static this_t from_sector( natus::math::vec2ui_cref_t ij ) noexcept
        {
            return this_t( ij ).evaluate() ;
        }

        static this_t from_direction( natus::math::vec2f_cref_t pos ) noexcept
        {
            return this_t( pos ) ;
        }

        static this_t from_rel_sector( natus::math::vec2ui_cref_t ij ) noexcept
        {
            return this_t( ij ) ;
        }

        static this_t zero( void_t ) noexcept
        {
            return this_t( natus::math::vec2ui_t(0) ) ;
        }

        this_t origin( void_t ) const noexcept
        {
            return this_t( natus::math::vec2ui_t( _ij ) ) ;
        }

    public:

        static natus::math::vec2i_t granularity( void_t ) noexcept
        {
            return natus::math::vec2i_t( G ) ;
        }

        static natus::math::vec2i_t granularity_half( void_t ) noexcept
        {
            return natus::math::vec2i_t( G ) >> natus::math::vec2i_t( 1 ) ;
        }

    public:

        this_ref_t operator = ( this_cref_t rhv ) noexcept
        {
            _ij = rhv._ij ;
            _vec = rhv._vec ;
            return this_t::evaluate() ;
        }

        this_ref_t operator = ( natus::math::vec2f_cref_t f ) noexcept
        {
            return *this = this_t( f ) ;
        }

        this_cref_t operator += ( natus::math::vec2f_cref_t f ) const noexcept
        {
            _vec += f ;
            return this_t::evaluate() ;
        }
        
        this_t operator + ( natus::math::vec2f_cref_t f ) const noexcept
        {
            return this_t( _ij, _vec + f ).evaluate() ;
        }

        this_t operator - ( natus::math::vec2f_cref_t f ) const noexcept
        {
            return this_t( _ij, _vec - f ) ;
        }

        this_t operator + ( natus::math::vec2ui_cref_t ij ) const noexcept
        {
            return this_t( _ij + ij, _vec ).evaluate() ;
        }

        this_t operator + ( this_cref_t c ) const noexcept
        {
            return this_t( _ij + c._ij, _vec + c._vec ).evaluate() ;
        }

        this_t operator - ( this_cref_t c ) const noexcept
        {
            auto const ij_dif = _ij - c._ij ;
            auto const v = _vec + natus::math::vec2f_t( ij_dif * this_t::granularity() ) ;
            return this_t( ij_dif, v - c._vec ) ;
        }

    public:

        natus::math::vec2ui_t get_ij( void_t ) const noexcept
        {
            return _ij ;
        }

        natus::math::vec2f_t get_vector( void_t ) const noexcept
        {
            return _vec ;
        }
    };
    natus_typedefs( coordinate<100>, coord_2d ) ;
}