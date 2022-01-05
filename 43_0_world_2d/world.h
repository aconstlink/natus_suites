
#pragma once

#include <natus/collide/2d/bounds/aabb.hpp>
#include <natus/math/vector/vector2.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/ntd/vector.hpp>
#include <natus/core/types.hpp>

namespace uniform_grid
{
    using namespace natus::core::types ;
    class ij_id
    {
        natus_this_typedefs( ij_id ) ;

    private: 

        // x,y,z : i, j, id
        natus::math::vec3ui_t _ij_id = natus::math::vec3ui_t( uint_t(-1) ) ;

    public:

        ij_id( void_t ) noexcept {}
        ij_id( uint_t const i, uint_t const j, uint_t const id ) { _ij_id = natus::math::vec3ui_t( i, j, id ) ; }
        ij_id( natus::math::vec2ui_t const ij, uint_t const id ) { _ij_id = natus::math::vec3ui_t( ij, id ) ; }
        ij_id( natus::math::vec3ui_t const ij_id_ ) { _ij_id = ij_id_ ; }
        ij_id( this_cref_t rhv ) noexcept { _ij_id = rhv._ij_id ; }
        ij_id( this_rref_t rhv ) noexcept { _ij_id = rhv._ij_id ; }
        ~ij_id( void_t ) noexcept {}

        natus::math::vec2ui_t get_ij( void_t ) const noexcept
        {
            return _ij_id.xy() ;
        }

        size_t get_id( void_t ) const noexcept
        {
            return _ij_id.z() ;
        }

        bool_t operator == ( natus::math::vec2ui_t const ij ) const noexcept
        {
            return this_t::get_ij().equal( ij ).all() ;

        }

        bool_t operator == ( size_t const id ) const noexcept
        {
            return this_t::get_id() == id ;
        }

    };
    natus_typedef( ij_id ) ;
}

namespace uniform_grid
{
    using namespace natus::core::types ;

    struct dimensions
    {
        natus_this_typedefs( dimensions ) ;

    private:

        /// in regions per grid
        natus::math::vec2ui_t regions_per_grid ;

        /// in cells per region
        natus::math::vec2ui_t cells_per_region ;

        /// in pixels per cell
        natus::math::vec2ui_t pixels_per_cell ;

    public:

        dimensions( void_t ) noexcept {}

        dimensions( natus::math::vec2ui_cref_t pre_grid, 
            natus::math::vec2ui_cref_t per_region, natus::math::vec2ui_cref_t per_cell ) noexcept
        {
            regions_per_grid = pre_grid ;
            cells_per_region = per_region ;
            pixels_per_cell = per_cell ;
        }

        dimensions( this_cref_t rhv ) noexcept
        {
            regions_per_grid = rhv.regions_per_grid ;
            cells_per_region = rhv.cells_per_region ;
            pixels_per_cell = rhv.pixels_per_cell ;
        }

        this_ref_t operator = ( this_cref_t rhv ) noexcept
        {
            regions_per_grid = rhv.regions_per_grid ;
            cells_per_region = rhv.cells_per_region ;
            pixels_per_cell = rhv.pixels_per_cell ;
            return *this ;
        }

        natus::math::vec2ui_t get_cells_global( void_t ) const noexcept
        {
            return regions_per_grid * cells_per_region ;
        }

        ij_id calc_cell_ij_id( natus::math::vec2ui_cref_t ij ) const noexcept
        {
            auto const x = ij.x() % cells_per_region.x() ;
            auto const y = ij.y() % cells_per_region.y() ;

            return ij_id( natus::math::vec2ui_t(x, y), x + y * cells_per_region.x() ) ;
        }

        //**************************************************************************
        natus::math::vec2i_t transform_to_center( natus::math::vec2ui_cref_t pos ) const noexcept
        {
            auto const dims = pixels_per_cell * cells_per_region * regions_per_grid ;
            auto const dims_half = dims / natus::math::vec2ui_t( 2 ) ;

            return pos - dims_half ;
        }

        //**************************************************************************
        /// @param pos in pixels from the center of the field
        natus::math::vec2ui_t calc_region_ij( natus::math::vec2i_cref_t pos ) const noexcept
        {
            auto const cdims = pixels_per_cell * cells_per_region ;
            auto const cdims_half = cdims / natus::math::vec2ui_t( 2 ) ;

            auto const dims = cdims * regions_per_grid ;
            auto const dims_half = dims / natus::math::vec2ui_t( 2 ) ;

            natus::math::vec2ui_t const pos_global = dims_half + pos ;

            return pos_global / cdims ;
        }

        //**************************************************************************
        /// @param pos in pixels from the center of the field
        natus::math::vec2ui_t calc_cell_ij_global( natus::math::vec2i_cref_t pos ) const noexcept
        {
            auto const cdims = pixels_per_cell * cells_per_region ;
            auto const cdims_half = cdims / natus::math::vec2ui_t( 2 ) ;

            auto const dims = cdims * regions_per_grid ;
            auto const dims_half = dims / natus::math::vec2ui_t( 2 ) ;

            natus::math::vec2ui_t const pos_global = dims_half + pos ;

            return pos_global / pixels_per_cell ;
        }

        //**************************************************************************
        /// @param pos in pixels from the center of the field
        natus::math::vec2ui_t calc_cell_ij_local( natus::math::vec2i_cref_t pos ) const noexcept
        {
            return this_t::calc_cell_ij_global( pos ) % cells_per_region ;
        }

        //**************************************************************************
        /// pass global cell ij and get region ij and local cell ij
        void global_to_local( natus::math::vec2ui_in_t global_ij, natus::math::vec2ui_out_t region_ij, 
            natus::math::vec2ui_out_t local_ij ) const noexcept
        {
            region_ij = global_ij / cells_per_region ;
            local_ij = global_ij % cells_per_region ;
        }

        //**************************************************************************
        /// world space region bounding box
        natus::collide::n2d::aabbf_t calc_region_bounds( uint_t const ci, uint_t const cj ) const noexcept
        {
            natus::math::vec2ui_t const ij( ci, cj ) ;

            auto const cdims = pixels_per_cell * cells_per_region ;
            auto const cdims_half = cdims / natus::math::vec2ui_t( 2 ) ;

            auto const dims = cdims * regions_per_grid ;
            auto const dims_half = dims / natus::math::vec2ui_t(2) ;

            natus::math::vec2i_t const start = natus::math::vec2i_t(dims_half).negated() ;

            natus::math::vec2f_t const min = start + cdims * ( ij + natus::math::vec2ui_t( 0 ) ) ;
            natus::math::vec2f_t const max = start + cdims * ( ij + natus::math::vec2ui_t( 1 ) ) ;

            return natus::collide::n2d::aabbf_t( min, max ) ;
        }

        //**************************************************************************
        natus::collide::n2d::aabbf_t calc_cell_bounds( uint_t const ci, uint_t const cj ) const noexcept
        {
            return natus::collide::n2d::aabbf_t() ;
        }

        //**************************************************************************
        /// returns the amount of cells for passed pixels
        natus::math::vec2ui_t pixels_to_cells( natus::math::vec2ui_cref_t v ) const noexcept
        {
            return v / pixels_per_cell ;
        }

        /// stores the area of involved regions and cells
        /// the area is determined by the four corners
        struct regions_and_cells
        {
            natus::math::vec2ui_t regions[4] ;
            natus::math::vec2ui_t cells[4] ;

            natus::math::vec2ui_t cell_dif( void_t ) const 
            {
                return cells[ 2 ] - cells[ 0 ] ;
            }

            natus::math::vec2ui_t cell_min( void_t ) const
            {
                return cells[ 0 ] ;
            }

            natus::math::vec2ui_t region_dif( void_t ) const
            {
                return regions[ 2 ] - regions[ 0 ] ;
            }

            natus::math::vec2ui_t region_min( void_t ) const
            {
                return regions[ 0 ] ;
            }
        };
        natus_typedef( regions_and_cells ) ;

        //**************************************************************************
        // arguments are in pixels
        regions_and_cells_t calc_regions_and_cells( natus::math::vec2i_cref_t pos, natus::math::vec2ui_cref_t radius ) const noexcept
        {
            regions_and_cells_t od ;
            
            // cells
            {
                auto const r = this_t::pixels_to_cells( radius ) ;
                auto const ij = this_t::calc_cell_ij_global( pos ) ;
                auto const min = ij - natus::math::vec2ui_t( ij.min_ed( r ) ) ;
                auto const max = (ij + natus::math::vec2ui_t( r )).min( this_t::get_cells_global() ) ;

                od.cells[ 0 ] = min ;
                od.cells[ 1 ] = natus::math::vec2ui_t( min.x(), max.y() ) ;
                od.cells[ 2 ] = max ;
                od.cells[ 3 ] = natus::math::vec2ui_t( max.x(), min.y() ) ;
            }

            // regions
            {
                for( size_t i=0; i<4; ++i )
                {
                    od.regions[i] = od.cells[ i ] / cells_per_region ;
                }
            }

            return od ;
        }

    };
    natus_typedef( dimensions ) ;
}



namespace uniform_grid
{
    using namespace natus::core::types ;

    template< class T >
    struct data
    {
        /// from cells ij index
        ij_id index ;
        T t ;
    };

    class cell
    {
        // material id
        // collision boxes
    };

    class region
    {
        // pre-rendered texture 
        natus::ntd::vector< data< cell > > cells ;
    };

    class grid
    {
        natus_this_typedefs( grid ) ;

        natus::ntd::vector< data< region > > _regions ;
        uniform_grid::dimensions_t _dims ;

    public:

        grid( void_t ) noexcept {}
        grid( uniform_grid::dimensions_cref_t dims ) noexcept : _dims( dims ){}
        ~grid( void_t ) noexcept {}

        uniform_grid::dimensions_cref_t get_dims( void_t ) const noexcept
        {
            return _dims ;
        }


    };
    natus_typedef( grid ) ;
}