
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/application/util/app_essentials.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/geometry/3d/cube.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>

#include <natus/concurrent/parallel_for.hpp>

#include <natus/gfx/util/quad.h>
#include <natus/profile/macros.h>

#include <random>
#include <thread>

namespace this_file
{
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        natus::application::util::app_essentials_t _ae ;
        
        natus::graphics::image_object_res_t _imgconfig = natus::graphics::image_object_t() ;
        natus::graphics::array_object_res_t _gpu_data = natus::graphics::array_object_t() ;

        natus::graphics::state_object_res_t _fb_render_states ;
        natus::ntd::vector< natus::graphics::render_object_res_t > _render_objects ;
        natus::ntd::vector< natus::graphics::geometry_object_res_t > _geometries ;

        natus::graphics::framebuffer_object_res_t _fb = natus::graphics::framebuffer_object_t() ;
        natus::gfx::quad_res_t _quad ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec3f_t nrm ; natus::math::vec2f_t tx ; } ;
        struct vertex2 { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;
        
        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        int_t _max_objects = 0 ;
        int_t _num_objects_rnd = 0 ;

        natus::math::vec4ui_t _fb_dims = natus::math::vec4ui_t( 0, 0, 1280, 768 ) ;

    public:

        test_app( void_t ) 
        {
            srand( uint_t( time(NULL) ) );

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
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _ae = std::move( rhv._ae ) ;
            _geometries = std::move( rhv._geometries ) ;
            _render_objects = std::move( rhv._render_objects ) ;
            _fb = std::move( rhv._fb ) ;
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
            {
                natus::ntd::vector< natus::io::location_t > shader_locations = {
                    natus::io::location_t( "shaders.data_buffer_test.nsl" )
                };

                natus::application::util::app_essentials_t::init_struct is = 
                {
                    { "myapp" }, 
                    { natus::io::path_t( DATAPATH ), "./working", "data" },
                    shader_locations
                } ;

                _ae.init( is ) ;
                _ae.get_camera_0()->look_at( natus::math::vec3f_t( 0.0f, -2000.0f, 1000.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 0.0f, -2000.0f, 0.0f )) ;
                _ae.get_camera_0()->set_near_far( 1.0f, 100000.0f ) ;
            }

            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "framebuffer_render_states" ) ;

                {
                    natus::graphics::render_state_sets_t rss ;

                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = true ;
                    rss.depth_s.ss.do_depth_write = true ;

                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::counter_clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = natus::graphics::fill_mode::fill ;

                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                   
                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = _fb_dims ;
                   
                    so.add_render_state_set( rss ) ;
                }

                _fb_render_states = std::move( so ) ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _fb_render_states ) ;
                } ) ;
            }

            size_t const num_objects_x = 500 ;
            size_t const num_objects_y = 500 ;
            size_t const num_objects = num_objects_x * num_objects_y ;
            _max_objects = num_objects  ;
            _num_objects_rnd = int_t(std::min( size_t(40000), size_t(num_objects / 2) )) ;

            // cube
            {
                natus::geometry::cube_t::input_params ip ;
                ip.scale = natus::math::vec3f_t( 1.0f ) ;
                ip.tess = 100 ;

                natus::geometry::tri_mesh_t tm ;
                natus::geometry::cube_t::make( &tm, ip ) ;
                
                natus::geometry::flat_tri_mesh_t ftm ;
                tm.flatten( ftm ) ;

                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::normal, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                    .resize( ftm.get_num_vertices() * _max_objects ).update<vertex>( [&] ( vertex* array, size_t const ne )
                {
                    natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>(0, _max_objects),
                        [&]( natus::concurrent::range_1d<size_t> const & r )
                    {
                        for( size_t o=r.begin(); o<r.end(); ++o )
                        {
                            size_t const base = o * ftm.get_num_vertices() ;
                            for( size_t i=0; i<ftm.get_num_vertices(); ++i )
                            {
                                array[ base + i ].pos = ftm.get_vertex_position_3d( i ) ;
                                array[ base + i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                                array[ base + i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                            }
                        }
                    } ) ;
                    
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).
                    resize( ftm.indices.size() * _max_objects ).
                    update<uint_t>( [&] ( uint_t* array, size_t const ne )
                {
                    natus::concurrent::parallel_for<size_t>( natus::concurrent::range_1d<size_t>(0, _max_objects),
                        [&]( natus::concurrent::range_1d<size_t> const & r )
                    {
                        for( size_t o=r.begin(); o<r.end(); ++o )
                        {
                            size_t const vbase = o * ftm.get_num_vertices() ;
                            size_t const ibase = o * ftm.indices.size() ;
                            for( size_t i = 0; i < ftm.indices.size(); ++i ) 
                            {
                                array[ ibase + i ] = ftm.indices[ i ] + uint_t( vbase ) ;
                            }
                        }
                    } ) ;
                } ) ;

                natus::graphics::geometry_object_res_t geo = natus::graphics::geometry_object_t( "cubes",
                    natus::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( geo ) ;
                } ) ;
                _geometries.emplace_back( std::move( geo ) ) ;
            }

            // array
            {
                struct the_data
                {
                    natus::math::vec4f_t pos ;
                    natus::math::vec4f_t col ;
                };

                float_t scale = 20.0f ;
                natus::graphics::data_buffer_t db = natus::graphics::data_buffer_t()
                    .add_layout_element( natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 )
                    .add_layout_element( natus::graphics::type::tfloat, natus::graphics::type_struct::vec4 ) ;

                _gpu_data = natus::graphics::array_object_t( "object_data", std::move( db ) ) ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _gpu_data ) ;
                } ) ;
            }

            // image configuration
            {
                natus::graphics::image_t img = natus::graphics::image_t( natus::graphics::image_t::dims_t( 100, 100 ) )
                    .update( [&]( natus::graphics::image_ptr_t, natus::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                {
                    typedef natus::math::vector4< uint8_t > rgba_t ;
                    auto* data = reinterpret_cast< rgba_t * >( data_in ) ;

                    size_t const w = 5 ;

                    size_t i = 0 ; 
                    for( size_t y = 0; y < dims.y(); ++y )
                    {
                        bool_t const odd = ( y / w ) & 1 ;

                        for( size_t x = 0; x < dims.x(); ++x )
                        {
                            bool_t const even = ( x / w ) & 1 ;

                            data[ i++ ] = even || odd ? rgba_t( 255 ) : rgba_t( 0, 0, 0, 255 );
                            //data[ i++ ] = rgba_t(255) ;
                        }
                    }
                } ) ;

                _imgconfig = natus::graphics::image_object_t( "checker_board", ::std::move( img ) )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                    .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                    .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                    .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _imgconfig ) ;
                } ) ;
            }

            // the cubes render object
            {
                natus::graphics::render_object_t rc = natus::graphics::render_object_t( "cubes" ) ;

                {
                    rc.link_geometry( "cubes" ) ;
                    rc.link_shader( "test_data_buffer" ) ;
                }

                // add variable set 
                {
                    natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                    {
                        auto* var = vars->data_variable< natus::math::vec4f_t >( "color" ) ;
                        var->set( natus::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                    }

                    {
                        auto* var = vars->data_variable< float_t >( "u_time" ) ;
                        var->set( 0.0f ) ;
                    }

                    {
                        auto* var = vars->texture_variable( "tex" ) ;
                        var->set( "checker_board" ) ;
                    }

                    {
                        auto* var = vars->array_variable( "u_data" ) ;
                        var->set( "object_data" ) ;
                    }

                    rc.add_variable_set( std::move( vars ) ) ;
                }
                
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( rc ) ;
                } ) ;
                _render_objects.emplace_back( std::move( rc ) ) ;
            }

            // framebuffer
            {
                _fb = natus::graphics::framebuffer_object_t( "the_scene" ) ;
                _fb->set_target( natus::graphics::color_target_type::rgba_uint_8, 3 )
                    .set_target( natus::graphics::depth_stencil_target_type::depth32 )
                    .resize( _fb_dims.z(), _fb_dims.w() ) ;

                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _fb ) ;
                } ) ;
            }

            {
                _quad = natus::gfx::quad_res_t( natus::gfx::quad_t("post.quad") ) ;                
                _quad->init( _ae.graphics() ) ;
                _quad->set_texture("the_scene.0") ;
            }
            
            return natus::application::result::ok ; 
        }

        float value = 0.0f ;

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ud ) noexcept 
        { 
            _ae.on_update( ud ) ;
            NATUS_PROFILING_COUNTER_HERE( "Update Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_device( device_data_in_t dd ) noexcept 
        { 
            _ae.on_device( dd ) ;
            return natus::application::result::ok ; 
        }
        
        virtual natus::application::result on_physics( natus::application::app_t::physics_data_in_t ) noexcept
        {
            auto const dif = std::chrono::duration_cast< std::chrono::microseconds >( __clock_t::now() - _tp ) ;
            _tp = __clock_t::now() ;

            float_t const dt = float_t( double_t( dif.count() ) / std::chrono::microseconds::period().den ) ;
            
            if( value > 1.0f ) value = 0.0f ;
            value += natus::math::fn<float_t>::fract( dt ) ;

            {
                static float_t t = 0.0f ;
                t += dt * 0.1f ;

                if( t > 1.0f ) t = 0.0f ;
                
                static natus::math::vec3f_t tr ;
                tr.x( 1.0f * natus::math::fn<float_t>::sin( t * 2.0f * 3.14f ) ) ;

                //_camera_0.translate_by( tr ) ;
            }

            NATUS_PROFILING_COUNTER_HERE( "Physics Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        { 
            static float_t v = 0.0f ;
            v += 0.01f ;
            if( v > 1.0f ) v = 0.0f ;


            size_t const num_object = _render_objects.size() ;
            float_t const dt = rd.sec_dt ;

            {
                static float_t  angle_ = 0.0f ;
                angle_ += ( ( ((dt))  ) * 2.0f * natus::math::constants<float_t>::pi() ) / 1.0f ;
                if( angle_ > 4.0f * natus::math::constants<float_t>::pi() ) angle_ = 0.0f ;

                float_t s = 5.0f * std::sin(angle_) ;

                struct the_data
                {
                    natus::math::vec4f_t pos ;
                    natus::math::vec4f_t col ;
                };
                
                _gpu_data->data_buffer().resize( _num_objects_rnd ).
                update< the_data >( [&]( the_data * array, size_t const ne )
                {
                    size_t const w = 80 ;
                    size_t const h = 80 ;
                    
                    #if 0 // serial for
                    for( size_t e=0; e<std::min( size_t(_num_objects_rnd), ne ); ++e )
                    {
                        size_t const x = e % w ;
                        size_t const y = (e / w) % h ;
                        size_t const z = (e / w) / h ;

                        natus::math::vec4f_t pos(
                            float_t(x) - float_t(w/2),
                            float_t(z) - float_t(w/2),//10.0f * std::sin( y + angle_ ),
                            float_t( y ) - float_t(w/2),
                            30.0f
                        ) ;

                        array[e].pos = pos ;

                        float_t c = float_t( rand() % 255 ) /255.0f ;
                        array[e].col = natus::math::vec4f_t ( 0.1f, c, (c) , 1.0f ) ;
                    }

                    #else // parallel for

                    typedef natus::concurrent::range_1d<size_t> range_t ;
                    auto const & range = range_t( 0, std::min( size_t(_num_objects_rnd), ne ) ) ;

                    natus::concurrent::parallel_for<size_t>( range, [&]( range_t const & r )
                    {
                        for( size_t e=r.begin(); e<r.end(); ++e )
                        {
                            size_t const x = e % w ;
                            size_t const y = (e / w) % h ;
                            size_t const z = (e / w) / h ;

                            natus::math::vec4f_t pos(
                                float_t(x) - float_t(w/2),
                                float_t(z) - float_t(w/2),//10.0f * std::sin( y + angle_ ),
                                float_t( y ) - float_t(w/2),
                                30.0f
                            ) ;

                            array[e].pos = pos ;

                            float_t c = float_t( rand() % 255 ) /255.0f ;
                            array[e].col = natus::math::vec4f_t ( 0.1f, c, (c) , 1.0f ) ;
                        }
                    } ) ;

                    #endif

                } ) ;
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _gpu_data ) ;
                } ) ; 
            }

            // per frame update of variables
            for( auto & rc : _render_objects )
            {
                rc->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
                {
                    {
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "view" ) ;
                        var->set( _ae.get_camera_0()->mat_view() ) ;
                    }

                    {
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "proj" ) ;
                        var->set( _ae.get_camera_0()->mat_proj() ) ;
                    }

                    {
                        auto* var = vs->data_variable< natus::math::vec4f_t >( "color" ) ;
                        var->set( natus::math::vec4f_t( v, 0.0f, 1.0f, 0.5f ) ) ;
                    }

                    {
                        static float_t  angle = 0.0f ;
                        angle = ( ( (dt/1.0f)  ) * 2.0f * natus::math::constants<float_t>::pi() ) ;
                        if( angle > 2.0f * natus::math::constants<float_t>::pi() ) angle = 0.0f ;
                        
                        auto* var = vs->data_variable< natus::math::mat4f_t >( "world" ) ;
                        natus::math::m3d::trafof_t trans( var->get() ) ;

                        natus::math::m3d::trafof_t rotation ;
                        rotation.rotate_by_axis_fr( natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), angle ) ;
                        
                        trans.transform_fl( rotation ) ;

                        var->set( trans.get_transformation() ) ;
                    }
                } ) ;
            }

             
            {
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.use( _fb ) ;
                    a.push( _fb_render_states ) ;
                } ) ;
            }

            for( size_t i=0; i<_render_objects.size(); ++i )
            {
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    natus::graphics::backend_t::render_detail_t detail ;
                    detail.start = 0 ;
                    detail.num_elems = _num_objects_rnd * 36 ;
                    detail.varset = 0 ;
                    //detail.render_states = _render_states ;
                    a.render( _render_objects[i], detail ) ;
                } ) ;
            }

            {
                _ae.graphics().for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                    a.unuse( natus::graphics::backend::unuse_type::framebuffer ) ;
                } ) ;
            }

            _ae.on_graphics_begin( rd ) ;

            _quad->render( _ae.graphics() ) ;

            _ae.on_graphics_end( 10 ) ;

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t td ) noexcept
        {
            if( !_ae.on_tool( td ) ) return natus::application::result::ok ;

            ImGui::Begin( "Test Control" ) ;

            if( ImGui::SliderInt( "Objects", &_num_objects_rnd, 0, _max_objects  ) )
            {
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






