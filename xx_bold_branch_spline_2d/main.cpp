
#include <random>
#include <thread>


#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>

#include <natus/tool/imgui/player_controller.h>
#include <natus/tool/imgui/timeline.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/profile/macros.h>

#include <natus/math/utility/angle.hpp>

#include <natus/ntd/insertion_sort.hpp>

#include <random>
#include <thread>

// this tests uses(copy) what has been done in the last test
// and adds textured quads to each region.
namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            auto view1 = this_t::create_window( "A Render Window Default", wi ) ;
            auto view2 = this_t::create_window( "A Render Window Additional", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _ae = natus::application::util::app_essentials_t( 
                natus::graphics::async_views_t( { view1.async(), view2.async() } ) ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
            _ae = natus::application::util::app_essentials_t( 
                natus::graphics::async_views_t( { view1.async() } ) ) ;
            #endif
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) noexcept : app( std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const wid, this_t::window_event_info_in_t wei ) noexcept
        {
            _ae.on_event( wid, wei ) ;
            return natus::application::result::ok ;
        }        

    private:

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::application::util::app_essentials_t::init_struct is = 
            {
                { "myapp" }, { natus::io::path_t( DATAPATH ), "./working", "data" }
            } ;

            _ae.init( is ) ;

            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;
            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ud ) noexcept
        {
            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;
            return natus::application::result::ok ; 
        }

        float_t thickness = 10.0f ;
        float_t split_angle = natus::math::angle<float_t>::degree_to_radian( 180.0f ) ;
        float_t sex_offset = thickness * 0.5f ;

        bool_t draw_normal = false ;
        bool_t draw_tangent = false ;
        bool_t draw_extended = true ;
        bool_t draw_sex = true ;
        bool_t draw_split_points = false ;
        bool_t draw_outlines = true ;

        //*****************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _ae.on_graphics_begin( rd ) ; 

            auto pr = _ae.get_prim_render() ;

            // one entry per point
            typedef natus::ntd::vector< natus::math::vec2f_t > points_t ;
            typedef natus::ntd::vector< natus::math::vec2f_t > normals_t ;
            
            // one entry per point and
            // num neighbors of point per point
            typedef natus::ntd::vector< size_t > neighbors_t ;

            // num neighbors of point per point
            typedef natus::ntd::vector< natus::math::vec2f_t > extended_t ;
            typedef natus::ntd::vector< uint8_t > counts_t ;

            // one entry per point
            // where to look for the first exteded for a point
            typedef natus::ntd::vector< size_t > offsets_t ;

            // two entries per segment
            typedef natus::ntd::vector< size_t > indices_t ;

            // nnh - 1 per point
            typedef natus::ntd::vector< float_t > angles_t ;

            static points_t pts( {
                natus::math::vec2f_t( -100.0f, -50.0f ),
                natus::math::vec2f_t( 100.0f, 50.0f ),
                natus::math::vec2f_t( 50.0f, 100.0f ),
                natus::math::vec2f_t( 200.0f, -50.0f ),
                natus::math::vec2f_t( 50.0f, 200.0f ),
                natus::math::vec2f_t( 200.0f, 100.0f ),
                natus::math::vec2f_t( 100.0f, 250.0f ),
                natus::math::vec2f_t( 200.0f, 250.0f ),
                natus::math::vec2f_t( 150.0f, 75.0f ),
                } ) ;

            //indices_t inds( { 0, 1, 1, 2 } ) ;
            indices_t inds( { 0, 1, 1, 2, 1, 3,0, 3 , 2,4, 4,6, 8, 1, 6, 8, 8,5 } ) ;
            //indices_t inds( { 0, 1, 1, 2, 2, 0 } ) ;
            //indices_t inds( { 0, 1, 1, 3, 1,2 } ) ;
            
            neighbors_t nnhs( pts.size() ) ; // num_neighbors : number of neighbors
            neighbors_t nhs( pts.size() ) ; // neighbors : actual neighbor indices
            offsets_t offs( pts.size() ) ; // offsets : where to start looking for the first neighbor
            angles_t fas ; // full_angles : required for sorting the neighbors
            offsets_t fa_offs( pts.size() ) ; // full_angles_offsets : the entry of a point with one neighbor is invalid.
            extended_t exts ; // extended : extended points per point
            counts_t num_sexts ; // number super extended per extended
            normals_t norms ; // normals : num_neighbor normals per point
            points_t split_points ; 

            // make even
            {
                if( inds.size() % 2 != 0 )
                {
                    inds.emplace_back( 0 ) ; 
                }
            }

            // find number of neighbors
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t count = 0 ;
                    for( size_t j=0; j<inds.size(); ++j )
                    {
                        count += (inds[j] == i) ? 1 : 0 ;
                    }
                    nnhs[i] = count ;
                }
            }

            // compute exteded point size 
            // compute offsets ( where to look for the first neighbor of point )
            {
                size_t accum = 0 ;
                
                for( size_t i=0; i<nnhs.size(); ++i )
                {
                    offs[i] = nnhs[i] != 0 ? accum : size_t( -1 ) ;
                    accum += nnhs[i] ;
                }
                exts.resize( accum ) ;
                nhs.resize( accum ) ;
                norms.resize( accum ) ;
                num_sexts.resize( accum ) ;
                split_points.resize( accum << 1 ) ;
            }

            // compute full angles size and offsets
            {
                size_t accum = 0 ;
                for( size_t i=0; i<nnhs.size(); ++i )
                {
                    size_t const na = nnhs[i] - 1 ;
                    fa_offs[i] = nnhs[i] != 0 && na != 0 ? accum : size_t(-1) ;
                    accum += nnhs[i] != 0 ? na : 0 ;
                }
                fas.resize( accum ) ;
            }

            // make neighbor list for each point
            {
                size_t offset = 0 ;
                for( size_t i=0; i<pts.size(); ++i )
                {
                    if( nnhs[i] == 0 ) continue ;

                    nhs[offset] = size_t(-1) ;

                    for( size_t j=0; j<inds.size(); ++j )
                    {
                        if( i == inds[j] )
                        {
                            // j&size_t(-2) <=> (j>>1)<<1 : need the first point of segment
                            size_t const idx = j & size_t(-2) ;
                            size_t const off = (j + 1) % 2 ;
                            nhs[offset] = inds[ idx + off ] ;
                            ++offset ;
                        }
                    }
                }
            }

            // compute full angles
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const fao = fa_offs[i] ;
                    size_t const nh = nnhs[i] ;
                    size_t const na = nh - 1 ;

                    if( nh == 0 || na == 0 ) continue ;
                    
                    auto const dir0 = (pts[ nhs[o] ] - pts[i]).normalize() ;

                    for( size_t j=1; j<nh; ++j )
                    {
                        size_t const idx = o + j ;
                        
                        auto const dir1 = (pts[ nhs[idx] ] - pts[i]).normalize() ;

                        fas[ fao + j - 1 ] = natus::math::vec2fe_t::full_angle( dir0, dir1 ) ;
                    }
                }
            }

            // inplace sort neighbor list based on full angles
            // @note the first neighbor in the nh list is the base,
            // so this one is staying in place
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const fao = fa_offs[i] ;
                    size_t const nh = nnhs[i] ;
                    size_t const na = nh - 1 ;

                    // no sort required
                    if( nh < 3 ) continue ;

                    // based on the swap of insertion_sort, 
                    // the neighbors need to be swapped too.
                    {
                        natus::ntd::insertion_sort<angles_t::value_type>::in_range(
                            fao, fao+na, fas, [&]( size_t const a, size_t const b )
                        {
                            size_t const idx0 = o + a + 1 ;
                            size_t const idx1 = o + b + 1 ;

                            auto const tmp = nhs[ idx0 ] ;
                            nhs[ idx0 ] = nhs[ idx1 ] ;
                            nhs[ idx1 ] = tmp ;
                        } ) ;
                    }
                }
            }
            
            // compute extended and normal
            {
                float_t const a90 = natus::math::angle<float_t>::degree_to_radian(90.0f) ;
                natus::math::mat2f_t const rotl = natus::math::mat2f_t::make_rotation_matrix( +a90 ) ;
                natus::math::mat2f_t const rotr = natus::math::mat2f_t::make_rotation_matrix( -a90 ) ;

                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const nh = nnhs[i] ;

                    for( size_t j = 0; j<nh; ++j )
                    {
                        size_t const idx = o + j ;
                        size_t const nidx = o + ((j+1)%nh) ;

                        auto const dir0 = (pts[ nhs[idx] ] - pts[i]).normalize() ;
                        auto const dir1 = (pts[ nhs[nidx] ] - pts[i]).normalize() ;

                        // super extended
                        {
                            auto const fa = natus::math::vec2fe_t::full_angle( dir0, dir1 ) ;
                            num_sexts[idx] = fa > split_angle ? 2 : 1 ;
                        }

                        if( nh == 1 )
                        {
                            norms[idx] = -dir0 ;
                            exts[idx] = pts[i] + norms[idx] * thickness ;
                            num_sexts[idx] = 2 ;
                        }
                        else
                        {
                            auto const n0 = rotl * dir0 ;
                            auto const n1 = rotr * dir1 ; 

                            auto const n2 = (n0 + n1) * 0.5f ;

                            norms[idx] = n2.normalized() ;
                            exts[idx] = pts[i] + norms[idx] * thickness ;
                        }
                    }
                }
            }

            // compute split points
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const fao = fa_offs[i] ;
                    size_t const nh = nnhs[i] ;

                    if( nh <=0 ) continue ;

                    for( size_t j=0; j<nh; ++j )
                    {
                        size_t const idx = o + j ;

                        auto const seg = pts[ nhs[idx] ] - pts[i] ;
                        auto const nrm = seg.normalized() ;
                        auto const tan = natus::math::vec2f_t( nrm.y(), -nrm.x() ).normalize() ;

                        size_t const idx2 = idx << 1 ;

                        split_points[idx2 + 0] = pts[i] + seg * 0.5f + tan * thickness ; // one side 
                        split_points[idx2 + 1] = pts[i] + seg * 0.5f - tan * thickness ; // other side
                    }
                }
            }

            // draw segments
            {
                float_t const radius = 5.0f ;

                natus::math::vec4f_t color0( 1.0f,1.0f,1.0f,1.0f) ;
                size_t const num_segs = inds.size() >> 1 ;
                for( size_t i=0; i<num_segs; ++i )
                {
                    size_t const idx = i << 1 ;
                    
                    size_t const id0 = inds[ idx+0 ] ;
                    size_t const id1 = inds[ idx+1 ] ;

                    pr->draw_circle( 1,10, pts[id0], radius, color0, color0 ) ;
                    pr->draw_line( 0, pts[id0], pts[id1], color0 ) ;
                    pr->draw_circle( 1,10, pts[id1], radius, color0, color0 ) ;
                }
            }

            #if 0
            // draw neighbor dirs
            {
                natus::math::vec4f_t color0( 1.0f,1.0f,1.0f,1.0f) ;
                
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[ i ] ;
                    size_t const nh = nnhs[i] ;
                    for( size_t j=0; j<nh; ++j )
                    {
                        natus::math::vec2f_t const dir = (pts[ nhs[o+j] ] - pts[i]).normalize() * 30.0f ;

                        _pr->draw_line( 2 , pts[i], pts[i]+dir  , natus::math::vec4f_t(1.0f,0.0f,0.0f,1.0f) ) ;
                    }
                }
            }
            #endif

            // draw extended
            if( draw_extended )
            {
                natus::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                natus::math::vec4f_t color1( 1.0f,0.0f,0.0f,1.0f) ;

                for( size_t i=0; i<exts.size(); ++i )
                {
                    auto const color = num_sexts[i] > 1 ? color1 : color0 ;
                    pr->draw_circle( 1,10, exts[i], 2.0f, color, color ) ;
                }
            }
            
            // draw normals
            if( draw_normal )
            {
                natus::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<exts.size(); ++i )
                {
                    auto const p0 = exts[i] ;
                    auto const p1 = p0 + norms[i] * thickness ;
                    pr->draw_line( 2 , p0, p1 , color0 ) ;
                }
            }

            // Quick TEST : draw tangent to normal
            if( draw_tangent )
            {
                natus::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<exts.size(); ++i )
                {
                    auto const p0 = exts[i] ;
                    auto const p1 = p0 + natus::math::vec2f_t( norms[i].y(), -norms[i].x() ) * thickness ;
                    pr->draw_line( 2 , p0, p1 , color0 ) ;
                }
            }

            // Quick TEST 2 : draw super extended
            if( draw_sex )
            {
                natus::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<exts.size(); ++i )
                {
                    if( num_sexts[i] <= 1 ) continue ;

                    auto const tang = natus::math::vec2f_t( norms[i].y(), -norms[i].x() ) ;

                    auto const p0 = exts[i] ;
                    auto const p1 = p0 + tang * thickness - norms[i] * sex_offset ;
                    auto const p2 = p0 - tang * thickness - norms[i] * sex_offset ;

                    pr->draw_circle( 1, 10, p1, 2.0f, color0, color0 ) ;
                    pr->draw_circle( 1, 10, p2, 2.0f, color0, color0 ) ;
                }
            }
            
            // Quick TEST 3 : draw split points 
            if( draw_split_points )
            {
                natus::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<split_points.size(); ++i )
                {
                    auto const p0 = split_points[i] ;
                    pr->draw_circle( 1, 10, p0, 2.0f, color0, color0 ) ;
                }
            }

            // Quick TEST 4 : draw outlines
            if( draw_outlines )
            {
                natus::math::vec2f_t points[4] ;

                natus::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[ i ] ;
                    size_t const nh = nnhs[i] ;

                    if( nh == 0 ) continue ;

                    if( nh == 1 )
                    {
                        for( size_t j=0; j<nh; ++j )
                        {
                            size_t const idx = o + j ;

                            auto const tang = natus::math::vec2f_t( norms[idx].y(), -norms[idx].x() ) ;

                            auto const p0 = exts[ idx  ] ;
                            points[0] = p0 + tang * thickness - norms[idx] * sex_offset ;
                            points[1] = p0 - tang * thickness - norms[idx] * sex_offset ;
                            points[2] = split_points[ (idx << 1) + 0 ] ;
                            points[3] = split_points[ (idx << 1) + 1 ] ;
                        }

                        for( size_t i=0; i<4; ++i )
                        {
                            auto const p0 = points[i] ;
                            auto const p1 = points[(i+1)%4] ;
                            pr->draw_line( 2 , p0, p1 , color0 ) ;
                        }
                    }
                    else
                    {
                        for( size_t j=0; j<nh; ++j )
                        {
                            // the index computation is based on how the extended points are computed 
                            // using the full angles. So one neighbor and the last(not the next) neighbor
                            // are sharing a face.
                            size_t const idx0 = (o + ((j + 0)%nh)) ; // can be written: o+j
                            size_t const idx1 = (o + ((j + nh-1)%nh)) ; // means (j - 1)%nh

                            auto const tang0 = natus::math::vec2f_t( norms[idx0].y(), -norms[idx0].x() ) ;
                            auto const tang1 = natus::math::vec2f_t( norms[idx1].y(), -norms[idx1].x() ) ;
                                
                            auto const p0 = exts[ idx0 ] ;
                            auto const p1 = exts[ idx1 ] ;

                            auto const sex0 = p0 + tang0 * thickness - norms[idx0] * sex_offset ;
                            auto const sex1 = p0 - tang0 * thickness - norms[idx0] * sex_offset ;

                            auto const sex2 = p1 + tang1 * thickness - norms[idx1] * sex_offset ;
                            auto const sex3 = p1 - tang1 * thickness - norms[idx1] * sex_offset ;

                            // do the quad
                            {
                                points[0] = num_sexts[idx0] == 1 ? exts[ idx0 ] : sex0 ;
                                points[1] = num_sexts[idx1] == 1 ? exts[ idx1 ] : sex3 ;
                                points[2] = split_points[ (idx0 << 1) + 0 ] ;
                                points[3] = split_points[ (idx0 << 1) + 1 ] ;
                            }

                            for( size_t i=0; i<4; ++i )
                            {
                                auto const p0 = points[i] ;
                                auto const p1 = points[(i+1)%4] ;
                                pr->draw_line( 2 , p0, p1 , color0 ) ;
                            }

                            // connect hole between super extended and extended
                            {
                                points[0] = num_sexts[idx0] == 1 ? exts[ idx0 ] : sex0 ;
                                points[1] = num_sexts[idx0] == 1 ? exts[ idx0 ] : sex1 ;
                                points[2] = num_sexts[idx1] == 1 ? exts[ idx1 ] : sex2 ;
                                points[3] = num_sexts[idx1] == 1 ? exts[ idx1 ] : sex3 ;
                            }

                            for( size_t i=0; i<4; ++i )
                            {
                                auto const p0 = points[i] ;
                                auto const p1 = points[(i+1)%4] ;
                                pr->draw_line( 2 , p0, p1 , color0 ) ;
                            }
                        }
                    }
                }
            }

            // test pick points
            {
                static size_t idx = size_t(-1) ;
                auto const ray = _ae.get_camera_0()->get_camera()->create_ray_norm( _ae.get_cur_mouse_pos_nrm() ) ;
                auto const plane = natus::math::vec3f_t(0.0f,0.0f,-1.0f) ;
                float_t const lambda = - ray.get_origin().dot( plane ) / ray.get_direction().dot( plane ) ;

                // point on plane
                natus::math::vec2f_t const pop = ray.point_at( lambda ).xy() ;
                //_pr->draw_circle( 4, 10, pop, radius, 
                 //           natus::math::vec4f_t(1.0f,1.0f,0.0f,1.0f), natus::math::vec4f_t(1.0f) ) ;

                float_t const radius = 5.0f ;
                for( size_t i=0; i<pts.size(); ++i )
                {
                    auto const p = pts[i] ;
                    auto const b = (p - pop).length() < radius ;  
                    if( b )
                    {
                        //pr->draw_circle( 4, 10, p, radius, 
                          //  natus::math::vec4f_t(1.0f,1.0f,0.0f,1.0f), natus::math::vec4f_t(1.0f) ) ;
                        if( _ae.left_down() && idx == size_t(-1) )
                            idx = i ;
                    }
                }

                if( idx != size_t(-1) ) pts[idx] += _ae.get_cur_mouse_dif() ;
                if( !_ae.left_down() ) idx = size_t(-1) ;
            }

            _ae.on_graphics_end( 10 ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Control and Info" ) ;
            
            {
                ImGui::SliderFloat( "thickness", &thickness, 1.0f, 30.0f ) ;
            }
            {
                split_angle = natus::math::angle<float_t>::radian_to_degree( split_angle ) ;
                ImGui::SliderFloat( "split angle", &split_angle, 0.0f, 360.0f ) ;
                split_angle = natus::math::angle<float_t>::degree_to_radian( split_angle ) ;
            }
            {
                ImGui::SliderFloat( "super extended offset", &sex_offset, 0.0f, thickness ) ;
            }
            {
                ImGui::Checkbox( "draw extended", &draw_extended ) ;
                ImGui::Checkbox( "draw outline", &draw_outlines ) ;
                ImGui::Checkbox( "draw super ext", &draw_sex ) ;
                ImGui::Checkbox( "draw normals", &draw_normal ) ;
                ImGui::Checkbox( "draw tangent", &draw_tangent ) ;
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
    return natus::application::global_t::create_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) )->exec() ;
}
