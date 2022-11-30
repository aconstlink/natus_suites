#pragma once

#include <natus/gfx/sprite/sprite_render_2d.h>
#include <natus/gfx/util/quad.h>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/vector/vector2.hpp>
#include <natus/core/types.hpp>

namespace proto
{
    using namespace natus::core::types ;


    // 1. render material into region
    // 2. render multiple regions per framebuffer
    // 3. render finished regions to screen
    class tile_render_2d
    {
        natus_this_typedefs( tile_render_2d ) ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;

    private:

        natus::graphics::async_views_t _asyncs ;

        natus::graphics::geometry_object_res_t _gconfig ;

        natus::graphics::render_object_t _rc_quad ;

        natus::graphics::state_object_t _so_tiles ;

        natus::gfx::quad_res_t _quad ;

        // framebuffers
        // shader
        // variable set

        struct tile_location
        {
            natus_this_typedefs( tile_location ) ;

            // framebuffer id
            // on which framebuffer is the region rendered
            size_t fid = size_t( -1 ) ;

            // tile data id
            size_t tid = size_t( -1 ) ;

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

        struct tile_data
        {
            // viewport in framebuffer
            natus::math::vec4ui_t viewport ;
            // framebuffer texture coords for rendering
            natus::math::vec2f_t texcoords ;
        };
        natus_typedef( tile_data ) ;
        natus::ntd::vector< tile_data_t > _tile_datas ;

        

        natus::math::vec2ui_t _tile_dims = natus::math::vec2ui_t( 6, 6 ) ;

        // regions to be rendered

    public:

        tile_render_2d( void_t ) noexcept {}
        tile_render_2d( this_cref_t ) noexcept = delete ;
        tile_render_2d( this_rref_t ) noexcept {}
        ~tile_render_2d( void_t ) noexcept {}

    public:

        // init renderer with region width and height
        void_t init( natus::ntd::string_cref_t name, size_t const region_w, size_t const region_h ) noexcept 
        {
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
                        _tile_locations[ i * num_tiles + j ].tid = size_t(-1) ;
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
                    for( uint_t y = 0 ; y < _tile_dims.y(); ++y )
                    {
                        for( uint_t x = 0 ; x < _tile_dims.x(); ++x )
                        {
                            natus::math::vec2ui_t const p0 ( (x+0) * uint_t( region_w ), (y+0) * uint_t( region_h ) ) ;
                            natus::math::vec2ui_t const p1 ( (x+1) * uint_t( region_w ), (y+1) * uint_t( region_h ) ) ;

                            // where to write into the framebuffer texture
                            natus::math::vec4ui_t const viewport( p0.x(), p0.y(), uint_t( region_w ), uint_t( region_h ) ) ;
                            // where to read from the framebuffer texture
                            natus::math::vec4f_t const tex_coords( 
                                1.0f/float_t( p0.x() ), 1.0f/float_t(p0.y()),
                                1.0f/float_t( p1.x() ), 1.0f/float_t(p1.y()) ) ;
                            
                            _tile_datas[ i++ ] = { viewport, tex_coords } ;
                        }
                    }
                }
            }

            // state object and render state sets
            {
                size_t const num_tiles = _tile_dims.x() * _tile_dims.y() ;

                natus::graphics::state_object_t so = natus::graphics::state_object_t( name + "tile_states" ) ;
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
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( name + "test" ) ;

                // shader configuration
                {
                    natus::graphics::shader_object_t sc( name + "test" ) ;

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
                        
                            void main()
                            {    
                                out_color = vec4(1.0, 1.0, 1.0, 1.0) ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::shader_api_type::glsl_1_4, std::move( ss ) ) ;
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
                            uniform sampler2D u_tex_0 ;
                            uniform sampler2D u_tex_1 ;
                        
                            void main()
                            {   
                                vec4 c0 = texture( u_tex_0, var_tx ) ;
                                vec4 c1 = texture( u_tex_1, var_tx ) ;
                                
                                out_color = mix( c0, c1, step( 0.5, var_tx.x ) ) ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::shader_api_type::glsles_3_0, std::move( ss ) ) ;
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
                            // texture and sampler needs to be on the same slot.
                            
                            Texture2D u_tex_0 : register( t0 );
                            SamplerState smp_u_tex_0 : register( s0 );

                            Texture2D u_tex_1 : register( t1 );
                            SamplerState smp_u_tex_1 : register( s1 );
                            
                            struct VS_OUTPUT
                            {
                                float4 Pos : SV_POSITION;
                                float2 tx : TEXCOORD0;
                            };

                            float4 PS( VS_OUTPUT input ) : SV_Target
                            {
                                return lerp( u_tex_0.Sample( smp_u_tex_0, input.tx ),
                                   u_tex_1.Sample( smp_u_tex_1, input.tx ), 0.5f ) ;
                            } )" ) ) ;

                        sc.insert( natus::graphics::shader_api_type::hlsl_5_0, std::move( ss ) ) ;
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
                        rc.link_geometry( name + "region_quad" ) ;
                        rc.link_shader( name + "test" ) ;
                    }

                    // add variable set 
                    {
                        natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                        #if 0
                        {
                            auto* var = vars->texture_variable( "u_tex_0" ) ;
                            var->set( "the_scene.0" ) ;
                            //var->set( "checker_board" ) ;
                        }
                        {
                            auto* var = vars->texture_variable( "u_tex_1" ) ;
                            var->set( "the_scene.1" ) ;
                            //var->set( "checker_board" ) ;
                        }
                        #endif

                        rc.add_variable_set( std::move( vars ) ) ;
                    }

                    _rc_quad = std::move( rc ) ;

                    _asyncs.for_each( [&]( natus::graphics::async_view_t a )
                    {
                        a.configure( _rc_quad ) ;
                    } ) ;
                }
            } 

            {
                _quad = natus::gfx::quad_res_t( natus::gfx::quad_t( name + "framebuffer_quad" ) ) ;
                _quad->init( _asyncs ) ;
                _quad->set_texture( name + ".framebuffer.0" ) ;
            }
        }

        class tile
        {
            natus_this_typedefs( tile ) ;

        private:

            natus::concurrent::mutex_t _mtx ;
            size_t _id = size_t( -1 ) ;
            size_t _loc = size_t( -1 ) ;

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
            tile( this_cref_t rhv ) noexcept = delete ;
            tile( this_rref_t rhv ) noexcept {}
            ~tile( void_t ) noexcept {}

            
        public:

            this_ref_t resize( size_t const r ) noexcept
            {
                _items.resize( r ) ;
            }

            typedef std::function< void_t ( items_ref_t ) > draw_funk_t ;

            // allows to place materials into the array directly
            void_t draw( draw_funk_t f ) noexcept
            {
                natus::concurrent::lock_guard_t lk( _mtx ) ;
                f( _items ) ;
                _has_changed = true ;
            }

            size_t get_id( void_t ) const noexcept{ return _id ; }
            size_t id( void_t ) const noexcept{ return _id ; }

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
        };
        natus_res_typedef( tile ) ;

        natus::concurrent::mutex_t _tiles_mtx ;
        natus::ntd::vector< tile_res_t > _tiles ;

        // acquire a tile for drawing materials into it
        // creates a new tile or returs an existing one
        // @param id cache id by user
        tile_res_t acquire_tile( size_t const id ) noexcept
        {
            natus::concurrent::lock_guard_t lk( _tiles_mtx ) ;

            // 1. search for required tile by id
            auto iter = std::find_if( _tiles.begin(), _tiles.end(), [&]( tile_res_t t )
            {
                return t->id() == id ;
            } ) ;

            // 2. if not found, search for an empty one
            if( iter == _tiles.end() )
            {
                iter = std::find_if( _tiles.begin(), _tiles.end(), [&]( tile_res_t t )
                {
                    return t->id() == size_t( -1 ) ;
                } ) ;
            }

            // 3. if still not found, create a new on
            if( iter == _tiles.end() )
            {
                iter = _tiles.insert( _tiles.end(), tile_res_t( tile() ) ) ;
            }

            // at this point, iter is requested, released or new tile
            // check if the tile has a valid tile_location into a framebuffer
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

        // releases the tile and its cache
        void_t release_tile( tile_res_t t ) noexcept
        {
            t->set_id( size_t( -1 ) ) ;

            // @todo: may not be required to be cleared
            if( t->get_location() < _tile_locations.size() )
            {
                _tile_locations[ t->get_location() ].used = false ;
            }
            t->set_location( size_t( -1 ) ) ;
        }

        // 
        void_t prepare_for_rendering( void_t ) noexcept
        {
            // 1. submit tiles for rendering to framebuffers
            // for all tile in fb
            //  use viewport and render quad or sprites
            for( auto & t : _tiles )
            {
                if( t->has_changed() )
                {
                    _asyncs.for_each([&]( natus::graphics::async_view_t a )
                    {
                        a.use( _fbs[ _tile_locations[ t->get_location() ].fid ] ) ;
                        a.push( _so_tiles, _tile_locations[ t->get_location() ].tid ) ;
                        a.render( _rc_quad ) ;
                        a.unuse( natus::graphics::backend::unuse_type::framebuffer ) ;
                    } ) ;
                }
            }

            // 2. do tile rendering preparation
        }

        void_t render( size_t const l ) noexcept
        {
            // 1. render all tiles for layer l
            // for each tile, render a quad by applying the
            // tiles' image in the framebuffer

            _quad->render( _asyncs ) ;
        }
    };
    natus_typedef( tile_render_2d ) ;
}