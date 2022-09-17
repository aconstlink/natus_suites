

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/tool/imgui/custom_widgets.h>

#include <natus/device/global.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/gfx/primitive/primitive_render_2d.h> 
#include <natus/gfx/font/text_render_2d.h>
#include <natus/gfx/primitive/line_render_3d.h>
#include <natus/gfx/primitive/primitive_render_3d.h>

#include <natus/format/global.h>
#include <natus/format/nsl/nsl_module.h>
#include <natus/format/future_items.hpp>

#include <natus/graphics/variable/variable_set.hpp>
#include <natus/profile/macros.h>

#include <natus/physics/particle_system.h>
#include <natus/physics/force_fields.hpp>

#include <natus/geometry/mesh/polygon_mesh.h>
#include <natus/geometry/mesh/tri_mesh.h>
#include <natus/geometry/mesh/flat_tri_mesh.h>
#include <natus/geometry/3d/cube.h>
#include <natus/geometry/3d/tetra.h>
#include <natus/math/utility/fn.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>
#include <natus/math/utility/angle.hpp>
#include <natus/math/utility/3d/transformation.hpp>
#include <natus/math/utility/constants.hpp>
#include <natus/math/utility/degree.hpp>
#include <natus/math/spline/linear_bezier_spline.hpp>
#include <natus/math/spline/quadratic_bezier_spline.hpp>
#include <natus/math/spline/cubic_bezier_spline.hpp>
#include <natus/math/spline/cubic_hermit_spline.hpp>
#include <natus/math/animation/keyframe_sequence.hpp>

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

        natus::graphics::async_views_t _graphics ;

        natus::graphics::state_object_res_t _root_render_states ;
                
        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::device::three_device_res_t _dev_mouse ;
        natus::device::ascii_device_res_t _dev_ascii ;

        natus::math::vec2f_t _cur_mouse ;

        bool_t _do_tool = true ;

        natus::io::database_res_t _db ;

    private: //

        natus::gfx::text_render_2d_res_t _tr ;
        natus::gfx::primitive_render_2d_res_t _pr ;
        natus::gfx::line_render_3d_res_t _lr3 ;
        natus::gfx::primitive_render_3d_res_t _pr3 ;

        bool_t _draw_debug = false ;

    private:

        natus::math::vec2f_t _window_dims = natus::math::vec2f_t( 1.0f ) ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            #if 1
            auto view1 = this_t::create_window( "A Render Window", wi ) ;
            auto view2 = this_t::create_window( "A Render Window", wi,
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11}) ;

            view1.window().position( 50, 50 ) ;
            view1.window().resize( 800, 800 ) ;
            view2.window().position( 50 + 800, 50 ) ;
            view2.window().resize( 800, 800 ) ;

            _graphics = natus::graphics::async_views_t( { view1.async(), view2.async() } ) ;
            #else
            auto view1 = this_t::create_window( "A Render Window", wi, 
                { natus::graphics::backend_type::gl3, natus::graphics::backend_type::d3d11 } ) ;
            _graphics = natus::graphics::async_views_t( { view1.async() } ) ;
            #endif

            _db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) noexcept : app( ::std::move( rhv ) ) 
        {
            _camera_0 = std::move( rhv._camera_0 ) ;
            _graphics = std::move( rhv._graphics ) ;
            _db = std::move( rhv._db ) ;
            _lr3 = std::move( rhv._lr3 ) ;
        }
        virtual ~test_app( void_t ) 
        {}

        virtual natus::application::result on_event( window_id_t const, this_t::window_event_info_in_t wei ) noexcept
        {
            natus::math::vec2f_t const window = natus::math::vec2f_t( float_t(wei.w), float_t(wei.h) ) ;


            _window_dims = window ;

            return natus::application::result::ok ;
        }        

    private:

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::device::global_t::system()->search( [&] ( natus::device::idevice_res_t dev_in )
            {
                if( natus::device::three_device_res_t::castable( dev_in ) )
                {
                    _dev_mouse = dev_in ;
                }
                else if( natus::device::ascii_device_res_t::castable( dev_in ) )
                {
                    _dev_ascii = dev_in ;
                }
            } ) ;

            if( !_dev_mouse.is_valid() )
            {
                natus::log::global_t::status( "no three mosue found" ) ;
            }

            if( !_dev_ascii.is_valid() )
            {
                natus::log::global_t::status( "no ascii keyboard found" ) ;
            }

            {
                _camera_0.look_at( natus::math::vec3f_t( 0.0f, 0.0f, -1000.0f ),
                        natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ), natus::math::vec3f_t( 100.0f, 0.0f, 0.0f )) ;
            }

            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "root_render_states" ) ;

                {
                    natus::graphics::render_state_sets_t rss ;

                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = false ;

                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back;
                    rss.polygon_s.ss.fm = natus::graphics::fill_mode::fill ;

                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.clear_color = natus::math::vec4f_t( 0.5f, 0.5f, 0.5f, 1.0f ) ;
                   
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.configure( _root_render_states ) ;
                } ) ;
            }

            // import fonts and create text render
            {
                natus::property::property_sheet_res_t ps = natus::property::property_sheet_t() ;

                {
                    natus::font::code_points_t pts ;
                    for( uint32_t i = 33; i <= 126; ++i ) pts.emplace_back( i ) ;
                    for( uint32_t i : {uint32_t( 0x00003041 )} ) pts.emplace_back( i ) ;
                    ps->set_value< natus::font::code_points_t >( "code_points", pts ) ;
                }

                {
                    natus::ntd::vector< natus::io::location_t > locations = 
                    {
                        natus::io::location_t("fonts.mitimasu.ttf"),
                        //natus::io::location_t("")
                    } ;
                    ps->set_value( "additional_locations", locations ) ;
                }

                {
                    ps->set_value<size_t>( "atlas_width", 128 ) ;
                    ps->set_value<size_t>( "atlas_height", 512 ) ;
                    ps->set_value<size_t>( "point_size", 90 ) ;
                }

                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem = mod_reg->import_from( natus::io::location_t( "fonts.LCD_Solid.ttf" ), _db, ps ) ;
                natus::format::glyph_atlas_item_res_t ii = fitem.get() ;
                if( ii.is_valid() )
                {
                    _tr = natus::gfx::text_render_2d_res_t( natus::gfx::text_render_2d_t(  ) ) ;
                    
                    _tr->init( "my_text_render", _graphics, std::move( *ii->obj ) ) ;
                }
            }

            // prepare primitive
            {
                _pr = natus::gfx::primitive_render_2d_res_t( natus::gfx::primitive_render_2d_t() ) ;
                _pr->init( "prim_render", _graphics ) ;
            }

            // prepare primitive
            {
                _pr3 = natus::gfx::primitive_render_3d_res_t( natus::gfx::primitive_render_3d_t() ) ;
                _pr3->init( "prim_render_3d", _graphics ) ;
            }

            // prepare line render
            {
                _lr3 = natus::gfx::line_render_3d_res_t( natus::gfx::line_render_3d_t() ) ;
                _lr3->init( "line_render", _graphics ) ;
            }

            return natus::application::result::ok ; 
        }

        //*****************************************************************************
        virtual natus::application::result on_device( device_data_in_t ) noexcept 
        { 
            natus::device::layouts::ascii_keyboard_t ascii( _dev_ascii ) ;
            if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f8 ) ==
                natus::device::components::key_state::released )
            {
            }
            else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f9 ) ==
                natus::device::components::key_state::released )
            {
            }
            else if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::f2 ) ==
                natus::device::components::key_state::released )
            {
                _do_tool = !_do_tool ;
            }

            {
                natus::device::layouts::three_mouse_t mouse( _dev_mouse ) ;
                _cur_mouse = mouse.get_local() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;
                _cur_mouse = _cur_mouse * (_window_dims * natus::math::vec2f_t(0.5f) );
            }

            // rotate
            {
                natus::device::layouts::three_mouse_t mouse( _dev_mouse ) ;
                static natus::math::vec2f_t old = mouse.get_global() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;
                natus::math::vec2f_t const dif = (mouse.get_global()* natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f )) - old ;
                old = mouse.get_global() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;

                if( mouse.is_pressing(natus::device::layouts::three_mouse::button::right ) )
                {
                    auto const old_trafo = _camera_0.get_transformation() ;
                    auto const old_trans = old_trafo.get_translation() ;
                    auto const old_rot = old_trafo.get_rotation_matrix() ;

                    auto trafo = natus::math::m3d::trafof_t().
                        rotate_by_angle_fl( natus::math::vec3f_t( -dif.y()*2.0f, dif.x()*2.0f, 0.0f ) ).
                        rotate_by_matrix_fl( old_rot ).
                        
                        translate_fl(old_trans) ; 

                    _camera_0.set_transformation( trafo ) ;
                }
            }

            // translate
            {
                natus::math::vec3f_t translate ;

                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::a ) ==
                    natus::device::components::key_state::pressing )
                {
                    translate += natus::math::vec3f_t(-10.0f, 0.0f, 0.0f ) ;
                }

                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::s ) ==
                    natus::device::components::key_state::pressing )
                {
                    translate += natus::math::vec3f_t(0.0f, 0.0f, -10.0f) ;
                }

                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::d ) ==
                    natus::device::components::key_state::pressing )
                {
                    translate += natus::math::vec3f_t( 10.0f, 0.0f, 0.0f ) ;
                }

                if( ascii.get_state( natus::device::layouts::ascii_keyboard_t::ascii_key::w ) ==
                    natus::device::components::key_state::pressing )
                {
                    translate += natus::math::vec3f_t(0.0f, 0.0f, 10.0f ) ;
                }

                auto trafo = _camera_0.get_transformation() ;
                trafo.translate_fr( translate ) ;
                _camera_0.set_transformation( trafo ) ;
            }

            // move camera with mouse
            #if 0
            {
                natus::device::layouts::three_mouse_t mouse( _dev_mouse ) ;
                static auto m_rel_old = natus::math::vec2f_t() ;
                auto const m_rel = mouse.get_local() * natus::math::vec2f_t( 2.0f ) - natus::math::vec2f_t( 1.0f ) ;

                auto const cpos = _camera_0.get_position().xy() ;
                auto const m = cpos + (m_rel-m_rel_old).negated() * 100.0f ;

                if( mouse.is_pressing( natus::device::layouts::three_mouse::button::left ) )
                {
                    _camera_0.translate_to( natus::math::vec3f_t( m.x(), m.y(), _camera_0.get_position().z() ) ) ;
                }

                m_rel_old = m_rel ;
            }
            #endif

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

        //*****************************************************************************
        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t rd ) noexcept 
        {
            _camera_0.set_dims( float_t(_window_dims.x()), float_t(_window_dims.y()), 1.0f, 1000.0f ) ;
            _camera_0.perspective_fov( natus::math::angle<float_t>::degree_to_radian( 90.0f ) ) ;

            _pr->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;
            _tr->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;
            _lr3->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;
            _pr3->set_view_proj( _camera_0.mat_view(), _camera_0.mat_proj() ) ;

            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.push( _root_render_states ) ;
                } ) ;
            }

            static size_t time = 0 ;
            time += rd.milli_dt ;

            // 
            {
                typedef natus::math::cubic_hermit_spline< natus::math::vec3f_t > spline_t ;
                typedef natus::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

                keyframe_sequence_t kf ;

                kf.insert( keyframe_sequence_t::keyframe_t( 0, natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 1000, natus::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 4000, natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 2000, natus::math::vec3f_t( 0.0f, 0.0f, 1.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 3000, natus::math::vec3f_t( 0.0f, 1.0f, 0.0f ) ) ) ;

                size_t ltime = time % kf.back().get_time() ;

                natus::math::vec4f_t color = natus::math::vec4f_t( kf(ltime), 1.0f ) ;

                _pr3->draw_circle( 
                    ( _camera_0.get_transformation()  ).get_rotation_matrix(), natus::math::vec3f_t(), 30.0f, color, color, 10 ) ;
            }

            // 
            {
                natus::math::vec3f_t const df( 100.0f ) ;

                typedef natus::math::cubic_hermit_spline< natus::math::vec3f_t > spline_t ;
                typedef natus::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

                typedef natus::math::linear_bezier_spline< float_t > splinef_t ;
                typedef natus::math::keyframe_sequence< splinef_t > keyframe_sequencef_t ;

                keyframe_sequence_t kf ;

                natus::math::vec3f_t const p0 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( -1.0f,-1.0f, 1.0f) ;
                natus::math::vec3f_t const p1 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( -1.0f,1.0f, -1.0f) ;
                natus::math::vec3f_t const p2 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( 1.0f,1.0f, 1.0f) ;
                natus::math::vec3f_t const p3 = natus::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * natus::math::vec3f_t( 1.0f,-1.0f, -1.0f) ;

                kf.insert( keyframe_sequence_t::keyframe_t( 0, p0 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 1000, p1 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 2000, p2 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 3000, p3 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 4000, p0 ) ) ;


                keyframe_sequencef_t kf2 ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 0, 30.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 1000, 50.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 2000, 20.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 3000, 60.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 10000, 10.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 11000, 30.0f ) ) ;


                size_t const ltime = time % kf.back().get_time() ;
                size_t const ltime2 = time % kf2.back().get_time() ;

                natus::math::vec4f_t color = natus::math::vec4f_t(1.0f) ;

                _pr3->draw_circle( 
                    ( _camera_0.get_transformation()  ).get_rotation_matrix(), kf( ltime ), kf2( ltime2 ), color, color, 10 ) ;
            }

            // render all
            {
                _pr->prepare_for_rendering() ;
                _tr->prepare_for_rendering() ;
                _lr3->prepare_for_rendering() ;
                _pr3->prepare_for_rendering() ;

                for( size_t i=0; i<100+1; ++i )
                {
                    _pr->render( i ) ;
                    _tr->render( i ) ;
                }

                _pr3->render() ;
                _lr3->render() ;
            }
            
            {
                _graphics.for_each( [&]( natus::graphics::async_view_t a )
                {
                    a.pop( natus::graphics::backend::pop_type::render_state ) ;
                } ) ;
            }            

            NATUS_PROFILING_COUNTER_HERE( "Render Clock" ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_tool( natus::application::app::tool_data_ref_t ) noexcept
        {
            if( !_do_tool ) return natus::application::result::no_tool ;

            ImGui::Begin( "Control and Info" ) ;
            
            {
                ImGui::Text( "mx: %f, my: %f", _cur_mouse.x(), _cur_mouse.y() ) ;
                //_cur_mouse
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
