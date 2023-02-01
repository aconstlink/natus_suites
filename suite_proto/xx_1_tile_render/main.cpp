

#include "tile_render.h"

#include <natus/ntd/vector.hpp>

int main( int argc, char ** argv )
{
    proto::tile_render_2d_t tir ;

    tir.init( "test", 200, 200 ) ;


    size_t const num_tiles = 100 ;
    natus::ntd::vector< proto::tile_render_2d_t::tile_res_t > tiles( num_tiles ) ;
    
    for( size_t i=0; i<num_tiles; ++i )
    {
        tiles[i] = tir.acquire_tile( i ) ;
    }

    for( size_t i=0; i<num_tiles; ++i )
    {
        tiles[i]->draw( [&]( proto::tile_render_2d_t::tile_t::items_ref_t items )
        {
            size_t const num_items = 32 * 32 ;
            items.resize( num_items ) ;

            for( size_t j=0; j<num_items; ++j )
            {
                proto::tile_render_2d_t::tile_t::item item ;
                item.layer = 0 ;
                item.mid = 0 ;
                item.pos = natus::math::vec2f_t() ;
                item.scale = natus::math::vec2f_t() ;
                item.color = natus::math::vec4f_t() ;

                items[ j ] = item ;
            }

        } ) ;
    }

    tir.prepare_for_rendering() ;
    for( size_t l=0; l<100; ++l )
    {
        tir.render( l ) ;
    }

    for( size_t i=0; i<num_tiles; ++i )
    {
        tir.release_tile( tiles[i] ) ;
    }
}
