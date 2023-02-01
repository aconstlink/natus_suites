

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
    "14_0_import"
    "14_1_import"
    "16_0_nsl"
    "16_1_nsl"
    "16_2_nsl"

    "19_0_audio_oal_capture"
    "19_1_audio_oal_gen"
    "19_2_audio_oal_capture_2"
    "19_3_audio_capture"
    "19_4_audio_async_capture"
    "19_5_audio_gen_2"
    "19_6_audio_play"
    "26_render_states"
    "27_0_framebuffer_simple"
    "27_1_framebuffer_mrt"
    "27_2_framebuffer_nsl"
    "28_multi_geometry"
    "30_0_array_object"
    "30_1_array_object"
    
    "31_0_texture_array"
    "31_1_texture_array"

    "32_0_stream_out"
    "32_1_stream_out"
    "32_2_stream_out"
    "32_3_stream_out"

    "33_0_geometry_shader"
    "33_1_geometry_shader"
    "33_2_geometry_shader"
    "33_3_geometry_shader"

    "34_0_reconfig"
    "34_1_nsl_auto"
    "34_2_nsl_auto"
    "37_0_glyph_atlas"
    "37_1_text_render_2d"
    "37_2_primitive_render_2d"
    "37_3_primitive_render_2d"
    "38_0_sprite_render_2d"
    "38_1_sprite_sheet"
    "38_2_sprite_sheet"
    "38_3_sprite_sheet"
    
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
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_test" )
endforeach()
