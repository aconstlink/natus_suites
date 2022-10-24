cmake_minimum_required( VERSION 3.6 )
project( "natus_tests" )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )

add_subdirectory( natus )
list( APPEND CMAKE_MODULE_PATH 
  "${CMAKE_CURRENT_LIST_DIR}/natus/cmake/macros" )

include( visual_studio_generate_bat_dll )
include( emacs_default_directory )

natus_emacs_default_directory( "${CMAKE_CURRENT_LIST_DIR}/natus" )

#set( CMAKE_CXX_STANDARD 17 )
set( subdirs
    "00_empty"
    "01_properties"
    "02_profile"
    "04_app"
    "05_imgui"
    "06_devices"
    "07_gamepad"
    "08_gamepad2"
    "09_gamepad3"
    "12_database"
    "13_0_tasks"
    "13_1_tasks"
    "13_2_thread_pool"
    "13_3_parallel_for"
    "13_4_loose_scheduler"
    "14_import"
    "15_import"
    "16_nsl"
    "17_nsl"
    "18_nsl"
    "19_0_audio_oal_capture"
    
    "19_1_audio_oal_gen"
    "19_2_audio_oal_capture_2"
    "19_3_audio_capture"
    "19_4_audio_async_capture"
    "19_5_audio_gen_2"
    "19_6_audio_play"
    "26_render_states"
    "27_framebuffer_simple"
    "28_framebuffer_mrt"
    "29_framebuffer_nsl"
    "30_0_array_object"
    "30_1_array_object"
    #"30_array_object_nsl"
    "31_0_texture_array"
    "31_1_texture_array"
    "32_reconfig"
    "33_0_nsl_auto"
    "33_1_nsl_auto"
    "34_glyph_atlas"
    "35_text_render_2d"
    "36_primitive_render_2d"
    "37_primitive_render_2d"
    "38_sprite_render_2d"
    "39_0_sprite_sheet"
    "39_1_sprite_sheet"
    "39_2_sprite_sheet"
    "41_particle_system"
    "42_particle_system"
    "43_inv_space_2d"
    "44_0_world_2d"
    "44_1_world_2d"
    "45_0_line_3d"
    "46_0_spline"
    "46_1_spline"
    "46_2_spline"
    "47_0_keyframe"
    "47_1_keyframe"
    "48_pinhole_camera"
    "49_picking_2d"
    "50_simple_essentials"
    "51_0_post_process"
    "51_1_post_process"
    "51_2_post_process"
    "51_3_post_process_fxaa"
    "52_time_measure"
    "xx_1_tile_render"
    "xx_bold_branch_spline_2d"
    "xx_2_tile_render"
    "xx_tool_sprite"
    "xx_tool_player_timeline"
    "xx_some_math_tests"
    "xx_insertion_sort"
    )

if( WIN32 )
  list( APPEND subdirs 
    "19_0_system_audio_capture"
    "19_0_audio_wasapi_capture"
    "19_0_audio_wasapi_capture_2"
    )
endif()

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "natus_tests" )
endforeach()

natus_vs_gen_bat_for_dlls()

