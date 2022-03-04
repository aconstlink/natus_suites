#pragma once

#include <natus/gfx/sprite/sprite_render_2d.h>
#include <natus/gfx/util/quad.h>
#include <natus/concurrent/mrsw.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/vector/vector2.hpp>
#include <natus/core/types.hpp>

namespace proto
{
    using namespace natus::core::types ;


    // 1. render material into region (not implemented)
    // 2. render multiple regions per framebuffer
    // 3. render finished regions to screen
    class tile_render_2d
    {
        natus_this_typedefs( tile_render_2d ) ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;

    private:

        natus::ntd::string_t _name ;

        natus::graphics::async_views_t _asyncs ;

        natus::graphics::geometry_object_res_t _gconfig ;

        natus::graphics::render_object_t _rc_quad ;

        natus::graphics::state_object_t _so_tiles ;

        natus::gfx::quad_res_t _quad ;

        natus::math::mat4f_t _view ;
        natus::math::mat4f_t _proj ;

        // the meaning of location here is the location
        // in the arrays.
        struct tile_location
        {
            natus_this_typedefs( tile_location ) ;

            // framebuffer id
            // on which framebuffer is the region rendered
            size_t fid = size_t( -1 ) ;

            // tile data id
            size_t tid = size_t( -1 ) ;

            // variable set id
            size_t vs_id = size_t( -1 ) ;

            bool_t used = false ;

            this_ref_t operator = ( this_cref_t rhv ) noexcept
            {
                tid = rhv.tid ;
                fid = rhv.fid ;
                used = rhv.used ;
                return *this ;
            }
        };
        natus_typedef( tile_location ) ;

        natus::ntd::vector< tile_location_t > _tile_locations ;
        natus::ntd::vector< natus::graphics::framebuffer_object_res_t > _fbs ;

        // the tile data stores 
        // - where to render the tile into the framebuffer
        // - where to sample from the framebuffer texture
        struct tile_data
        {
            // viewport in framebuffer
            natus::math::vec4ui_t viewport ;
            // framebuffer texture coords for rendering
            natus::math::vec4f_t texcoords ;
        };
        natus_typedef( tile_data ) ;
        natus::ntd::vector< tile_data_t > _tile_datas ;

        

        natus::math::vec2ui_t _tile_dims = natus::math::vec2ui_t( 6, 6 ) ;

        // regions to be rendered

    public:

        tile_render_2d( natus::graphics::async_views_t a ) noexcept : _asyncs( a )  {}
        tile_render_2d( this_cref_t ) noexcept = delete ;
        tile_render_2d( this_rref_t rhv ) noexcept { _asyncs = std::move( rhv._asyncs ) ; }
        ~tile_render_2d( void_t ) noexcept {}

    public:

        // init renderer with region width and height
        void_t init( natus::ntd::string_cref_t name, size_t const region_w, size_t const region_h ) noexcept 
        {
            _name = name ;

            size_t const w = region_w * _tile_dims.x() ;
            size_t const h = region_h * _tile_dims.y() ;

            {
                size_t const num_fbs = 6 ;
                size_t const num_tiles = _tile_dims.x() * _tile_dims.y() ;

                _fbs.resize( num_fbs ) ;
                _tile_locations.resize( num_fbs * num_tiles ) ;
                _tile_datas.resize( num_tiles ) ;

                for( size_t i = 0; i < num_fbs; ++i )
                {
                    for( size_t j=0; j<num_tiles; ++j )
                    {
                        _tile_locations[ i * num_tiles + j ].tid = j ;
                        _tile_locations[ i * num_tiles + j ].fid = i ;
                    }

                    // framebuffer
                    {
                        auto const tmp = name + ".framebuffer." + std::to_string( i ) ;
                        natus::graphics::framebuffer_object_res_t fb = natus::graphics::framebuffer_object_t( tmp ) ;
                        fb->set_target( natus::graphics::color_target_type::rgba_uint_8, 2 ).resize( w, h ) ;

                        _asyncs.for_each( [&]( natus::graphics::async_view_t a )
                        {
                            a.configure( fb ) ;
                        } ) ;

                        _fbs[i] = fb ;
                    }
                }

                {
                    size_t i = 0 ;
                    for( size_t y = 0 ; y < _tile_dims.y(); ++y )
                    {
                        for( size_t x = 0 ; x < _tile_dims.x(); ++x )
                        {
                            natus::math::vec2ui_t const p0 ( (x+0) * region_w, (y+0) * region_h ) ;
                            natus::math::vec2ui_t const p1 ( (x+1) * region_w, (y+1) * region_h ) ;

                            // where to write into the framebuffer texture
                            natus::math::vec4ui_t const viewport( p0.x(), p0.y(), region_w, region_h ) ;
                            // where to read from the framebuffer texture
                            natus::math::vec4f_t const tex_coords( 
                                float_t( p0.x() )/float_t(w), float_t(p0.y())/float_t(h),
                                float_t( p1.x() )/float_t(w), float_t(p1.y())/float_t(h) ) ;
                            
                            _tile_datas[ i++ ] = { viewport, tex_coords } ;
                        }
                    }
                }
            }

            // state object and render state sets
            {
                size_t const num_tiles = _tile_dims.x() * _tile_dims.y() ;

                natus::graphics::state_object_t so = natus::graphics::state_object_t( name + ".tile_states" ) ;
                for( size_t i=0; i < num_tiles; ++i )
                {
                    natus::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = false ;

                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = _tile_datas[ i ].viewport ;

                    so.add_render_state_set( rss ) ;
                }

                _so_tiles = std::move( so ) ;
                _asyncs.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _so_tiles ) ;
                } ) ;
            }

            // geometry configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = natus::math::vec3f_t( -0.5f, -0.5f, 0.0f ) ;
                    array[ 1 ].pos = natus::math::vec3f_t( -0.5f, +0.5f, 0.0f ) ;
                    array[ 2 ].pos = natus::math::vec3f_t( +0.5f, +0.5f, 0.0f ) ;
                    array[ 3 ].pos = natus::math::vec3f_t( +0.5f, -0.5f, 0.0f ) ;

                    array[ 0 ].tx = natus::math::vec2f_t( -0.0f, -0.0f ) ;
                    array[ 1 ].tx = natus::math::vec2f_t( -0.0f, +1.0f ) ;
                    array[ 2 ].tx = natus::math::vec2f_t( +1.0f, +1.0f ) ;
                    array[ 3 ].tx = natus::math::vec2f_t( +1.0f, -0.0f ) ;
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).resize( 6 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 2 ;

                    array[ 3 ] = 0 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 3 ;
                } ) ;

                _gconfig = natus::graphics::geometry_object_t( name + ".region_quad",
                    natus::graphics::primitive_type::triangles, ::std::move( vb ), ::std::move( ib ) ) ;

                _asyncs.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _gconfig ) ;
                } ) ;
            }

            // test quad into framebuffer
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( name + ".test" ) ;

                // shader configuration
                {
                    natus::graphics::shader_object_t sc( name + ".test" ) ;

                    // shaders : ogl 3.0
                    {
                        natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                            set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec3 in_pos ;
                            in vec2 in_tx ;
                            out vec2 var_tx ;
                            
                            void main()
                            {
                                var_tx = in_tx ;
                                gl_Position = vec4( sign( in_pos ), 1.0 ) ;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 140
                            in vec2 var_tx ;
                            out vec4 out_color ;
                            uniform vec4 u_color ;

                            void main()
                            {    
                                out_color = u_color ; //vec4(var_tx, 0.0, 1.0) ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::backend_type::gl3, std::move( ss ) ) ;
                    }

                    // shaders : es 3.0
                    {
                        natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                            set_vertex_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            in vec3 in_pos ;
                            in vec2 in_tx ;
                            out vec2 var_tx ;

                            void main()
                            {
                                var_tx = in_tx ;
                                gl_Position = vec4( sign(in_pos), 1.0 ) ;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            #version 300 es
                            precision mediump float ;
                            in vec2 var_tx ;
                            out vec4 out_color ;
                            uniform vec4 u_color ;

                            void main()
                            {   
                                out_color = vec4( u_color.xyz, 1.0 ) ; //vec4(var_tx, 0.0, 1.0) ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::backend_type::es3, std::move( ss ) ) ;
                    }

                    // shaders : hlsl 11(5.0)
                    {
                        natus::graphics::shader_set_t ss = natus::graphics::shader_set_t().

                            set_vertex_shader( natus::graphics::shader_t( R"(

                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            };

                            VS_OUTPUT VS( float4 in_pos : POSITION, float2 in_tx : TEXCOORD0 )
                            {
                                VS_OUTPUT output = (VS_OUTPUT)0;
                                output.pos = float4( sign( in_pos.xy ), 0.0f, 1.0f ) ;
                                output.tx = in_tx ;
                                return output;
                            } )" ) ).

                            set_pixel_shader( natus::graphics::shader_t( R"(
                            
                            struct VS_OUTPUT
                            {
                                float4 Pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            };
                            
                            float4 u_color ;

                            float4 PS( VS_OUTPUT input ) : SV_Target
                            {
                                return u_color ; //float4( input.tx, 0.0f, 1.0f ) ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::backend_type::d3d11, std::move( ss ) ) ;
                    }

                    // configure more details
                    {
                        sc
                            .add_vertex_input_binding( natus::graphics::vertex_attribute::position, "in_pos" )
                            .add_vertex_input_binding( natus::graphics::vertex_attribute::texcoord0, "in_tx" ) ;
                    }

                    _asyncs.for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.configure( sc ) ;
                    } ) ;

                    {
                        rc.link_geometry( name + ".region_quad" ) ;
                        rc.link_shader( name + ".test" ) ;
                    }

                    // add variable set 
                    for( size_t i=0; i<6; ++i )
                    {
                        natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                        {
                            vars->data_variable< natus::math::vec4f_t >( "u_color" )->set( 
                                natus::math::vec4f_t( 
                                    //float_t(i)/5.0f, float_t(i)/5.0f , float_t(i)/5.0f , 1.0f 
                                    float_t(i)/5.0f,float_t(1)/5.0f,float_t(1)/5.0f , 1.0f 
                                ) ) ; ;
                        }

                        rc.add_variable_set( std::move( vars ) ) ;
                    }

                    _rc_quad = std::move( rc ) ;

                    _asyncs.for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.configure( _rc_quad ) ;
                    } ) ;
                }
            } 

            // preconfig the variable sets with the correct values
            // the framebuffer texture and its texcoords are known already
            // relation: _tile_locations <-> variable sets is 1:1
            {
                size_t const num_tiles = _tile_dims.x() * _tile_dims.y() ;

                _quad = natus::gfx::quad_res_t ( natus::gfx::quad_t( name + ".framebuffer_quad" ) ) ;
                _quad->init( _asyncs, _tile_locations.size() ) ;

                for( size_t i=0; i<_tile_locations.size(); ++i )
                {
                    size_t const tid = _tile_locations[ i ].tid ;
                    size_t const fid = _tile_locations[ i ].fid ;

                    auto const tx = _tile_datas[ tid ].texcoords ;

                    _quad->set_texture( i, _name + ".framebuffer." + std::to_string( fid ) + ".0" ) ;
                    _quad->set_texcoord( i, tx ) ;
                }
                
            }
        }

        void_t set_view_proj( natus::math::mat4f_cref_t view, natus::math::mat4f_cref_t proj ) noexcept 
        {
            _view = view ;
            _proj = proj ;
        }

        class tile
        {
            natus_this_typedefs( tile ) ;

        private:

            natus::concurrent::mutex_t _mtx ;
            size_t _id = size_t( -1 ) ;
            size_t _loc = size_t( -1 ) ;
            natus::math::vec2f_t _pos = natus::math::vec2f_t( 0.5f ) ;
            natus::math::vec2f_t _scale = natus::math::vec2f_t( 0.5f ) ;

        public:

            struct item
            {
                size_t layer = size_t( -1 ) ;
                size_t mid = size_t( -1 ) ;
                natus::math::vec2f_t pos ;
                natus::math::vec2f_t scale ; 
                natus::math::vec4f_t color ;
            };

            natus_typedefs( natus::ntd::vector< item > , items) ;
            items_t _items ;
            bool_t _has_changed = false ;

        public:

            tile( void_t ) noexcept {}
            tile( size_t const id ) noexcept : _id( id ) {}
            tile( this_cref_t rhv ) noexcept = delete ;
            tile( this_rref_t rhv ) noexcept 
            {
                *this = std::move( rhv ) ;
            }
            ~tile( void_t ) noexcept {}

            this_ref_t operator = ( this_rref_t rhv ) noexcept
            {
                _id = rhv._id ;
                rhv._id = size_t( -1 ) ;
                _loc = rhv._loc ;
                rhv._loc = size_t( -1 ) ;
                _pos = rhv._pos ;
                _scale = rhv._scale ;

                return *this ;
            }
            
        public:

            this_ref_t resize( size_t const r ) noexcept
            {
                _items.resize( r ) ;
                return *this ;
            }

            typedef std::function< void_t ( proto::tile_render_2d::tile::items_ref_t ) > draw_funk_t ;

            // allows to place materials into the array directly
            void_t draw( draw_funk_t f ) noexcept
            {
                natus::concurrent::lock_guard_t lk( _mtx ) ;
                f( _items ) ;
                _has_changed = true ;
            }

            size_t get_id( void_t ) const noexcept{ return _id ; }
            size_t id( void_t ) const noexcept{ return _id ; }

            // transform the whole tile
            void_t transform( natus::math::vec2f_cref_t pos, natus::math::vec2f_cref_t scale ) noexcept
            {
                _pos = pos ;
                _scale = scale ;
            }

            natus::math::vec2f_cref_t get_pos( void_t ) const noexcept{ return _pos ; }
            natus::math::vec2f_cref_t get_scale( void_t ) const noexcept{ return _scale ; }

        private: // tile_render_2d interface

            friend class tile_render_2d ;

            void_t set_id( size_t const id ) noexcept
            {
                natus::concurrent::lock_guard_t lk( _mtx ) ;
                _id = id ;
            }

            // set the tile location id
            void_t set_location( size_t const loc ) noexcept
            {
                _loc = loc ;
            }
            
            size_t get_location( void_t ) const noexcept{ return _loc ; }

            bool_t has_changed( void_t ) const noexcept{ return _has_changed ; }
            void_t set_change( bool_t const b ) noexcept { _has_changed = b ; }
        };
        natus_res_typedef( tile ) ;

        mutable natus::concurrent::mrsw_t _tiles_mtx ;
        natus::ntd::map< size_t, natus::ntd::vector< tile_res_t > > _tiles ;
        // number of tiles in use
        size_t _num_tiles = 0 ;

        // acquire a tile for drawing materials into it
        // creates a new tile or returs an existing one
        // @param id cache id by user
        tile_res_t acquire_tile( size_t const layer, size_t const id ) noexcept
        {
            natus::concurrent::mrsw_t::writer_lock_t lk( _tiles_mtx ) ;

            // 1. find layer
            auto miter = _tiles.find( layer ) ;
            if( miter == _tiles.end() )
            {
                _tiles[ layer ].emplace_back( tile_res_t( tile( id ) ) ) ;
                miter = _tiles.find( layer ) ;
                ++_num_tiles ;
            }

            auto & tiles = miter->second ;
            
            // 2. search for required tile by id
            auto iter = std::find_if( tiles.begin(), tiles.end(), [&]( tile_res_t t )
            {
                return t->id() == id ;
            } ) ;

            // 3. if not found, search for an empty one
            if( iter == tiles.end() )
            {
                iter = std::find_if( tiles.begin(), tiles.end(), [&]( tile_res_t t )
                {
                    return t->id() == size_t( -1 ) ;
                } ) ;
            }

            // 4. if still not found, create a new on
            if( iter == tiles.end() )
            {
                iter = tiles.insert( tiles.end(), tile_res_t( tile( id ) ) ) ;
                ++_num_tiles ;
            }

            // 5. check if the tile has a valid tile_location into a framebuffer
            if( (*iter)->get_location() == size_t(-1) )
            {
                auto iter2 = std::find_if( _tile_locations.begin(), _tile_locations.end(), [&]( tile_location_ref_t tl )
                {
                    return tl.used == false ;
                } ) ;

                if( iter2 == _tile_locations.end() ) 
                {
                    natus::log::global_t::warning( "not enough tile locations. need dynamic framebuffer extension.") ;
                    return tile_res_t() ;
                }
                iter2->used = true ;
                (*iter)->set_location( std::distance( _tile_locations.begin(), iter2 ) ) ;
            }

            (*iter)->set_id( id ) ;

            return *iter ;
        }

        void_t release_tile( size_t const l, size_t const id ) noexcept
        {
            this_t::release_tile( this_t::acquire_tile( l, id ) ) ;
        }

        // releases the tile and its cache
        void_t release_tile( tile_res_t t ) noexcept
        {
            // @todo: may not be required to be cleared
            if( t->get_location() < _tile_locations.size() )
            {
                natus::concurrent::mrsw_t::writer_lock_t lk( _tiles_mtx ) ;
                _tile_locations[ t->get_location() ].used = false ;
            }

            t->set_id( size_t( -1 ) ) ;
            t->set_location( size_t( -1 ) ) ;
            t->set_change( false ) ;
        }

        bool_t has_tile( size_t const id ) const noexcept
        {
            for( auto & i : _tiles )
            {
                auto const iter = std::find_if( i.second.begin(), i.second.end(), [&]( this_t::tile_res_t t ){ return t->get_id() == id ; } ) ;
                if( iter != i.second.end() ) 
                {
                    return true ;
                }
            }

            return false ;
        }

        // 
        void_t prepare_for_rendering( void_t ) noexcept
        {
            natus::concurrent::mrsw_t::reader_lock_t lk( _tiles_mtx ) ;

            // 1. submit tiles for rendering to framebuffers
            // for all tile in fb
            //  use viewport and render quad or sprites
            
            size_t i=0; 
            // for all layers
            for( auto & l : _tiles )
            {
                // for all tiles per layer
                for( auto & t : l.second )
                {
                    if( t->has_changed() )
                    {
                        _asyncs.for_each([&]( natus::graphics::async_view_t a )
                        {
                            
                            a.use( _fbs[ _tile_locations[ t->get_location() ].fid ] ) ;
                            a.push( _so_tiles, _tile_locations[ t->get_location() ].tid ) ;
                            {
                                natus::graphics::backend::render_detail rd ;
                                rd.varset = i++ % 6 ;
                                a.render( _rc_quad, rd ) ;
                            }
                            a.pop( natus::graphics::backend::pop_type::render_state ) ;
                            a.unuse( natus::graphics::backend::unuse_type::framebuffer ) ;
                            
                            
                        } ) ;
                        t->set_change( false ) ;
                    }
                }
            }

            // 2. do tile rendering preparation
            #if 0
            if( _num_tiles > _quad->get_num_variable_sets() )
            {
                size_t const num_tiles = _num_tiles - _quad->get_num_variable_sets() ;
                _quad->add_variable_sets( num_tiles * 2 ) ;

                size_t i=0; 
                for( size_t i=0; i<_quad->get_num_variable_sets(); ++i )
                {
                    auto & tl = _tile_locations[ i ] ;
                    _quad->set_texture( i, _name + ".framebuffer." + std::to_string(tl.fid) + ".0" ) ;
                    tl.vs_id = i ;
                }
            }
            #endif
        }

        void_t render( size_t const l ) noexcept
        {
            natus::concurrent::mrsw_t::reader_lock_t lk( _tiles_mtx ) ;

            // 1. render all tiles for layer l
            // for each tile, render a quad by applying the
            // tiles' image in the framebuffer

            auto liter = _tiles.find( l ) ;
            if( liter == _tiles.end() ) return ;
            
            _quad->set_view_proj( _view, _proj ) ;

            size_t i = 0 ;
            for( auto & t : liter->second )
            {
                if( t->get_id() == size_t(-1)  ) continue ;

                //size_t const i = _tile_locations[ t->get_location() ].tid ;
                size_t const fid = _tile_locations[ t->get_location() ].fid ;

                _quad->set_position( t->get_location(), t->get_pos() ) ;
                _quad->set_scale( t->get_scale()  ) ;
                _quad->render( t->get_location(), _asyncs ) ;

                ++i ;
            }
        }
    };
    natus_res_typedef( tile_render_2d ) ;
}