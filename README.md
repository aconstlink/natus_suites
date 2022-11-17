# natus_tests

Adding manual tests here. Those can be seen as examples though.

A real sample repo will be created later on.

Some tests allow to use the mouse and keyboard to move the camera.
- Right click and mouse move for camera rotation 
- WASDQR for camera translation

# The Tests 

## 00_empty
## 01_properties
## 02_profile
## 04_app
## 05_imgui
## 06_devices
## 07_gamepad
## 08_gamepad2
## 09_gamepad3
## 10_database
## 11_database
## 12_database
## 13_0_tasks
## 13_1_tasks
## 13_2_thread_pool
## 13_3_parallel_for
## 13_4_loose_scheduler
## 14_import
## 15_import
## 16_nsl
## 17_nsl
## 18_nsl
## 19_0_audio_oal_capture
## 19_0_audio_wasapi_capture
## 19_0_audio_wasapi_capture_2
## 19_0_system_audio_capture
## 19_1_audio_oal_gen
## 19_2_audio_oal_capture_2
## 19_3_audio_capture
## 19_4_audio_async_capture
## 19_5_audio_gen_2
## 19_6_audio_play
## 26_render_states
## 27_0_framebuffer_simple
## 27_1_framebuffer_mrt
## 27_2_framebuffer_nsl
## 28_multi_geometry
Test and shows how to use multiple geometry objects in a single render configuration. The geometry can be changed via a render call using an index.
![Multi Geometry]( images/multi_geometry.jpg )
## 30_0_array_object
Tests and shows how to use an array object by doing the so call vertex pulling in the platform shader. Additional geometry data is pulled from an array buffer in the shader for further transformation.
## 30_1_array_object
Does the same as the last test but uses nsl only.
![Array Object]( images/array_object.jpg )
## 31_0_texture_array
Tests and shows how to use a texture buffer. The app loads four images and allows access in the shaders through a texture array. The shaders are implemented as platform shaders directly.
## 31_1_texture_array
Does the same as the last test but with nsl only.
![Texture Array]( images/texture_array.jpg )
## 32_0_stream_out
Tests and shows how to use the streamout object. This test directly uses the platform shaders. Nsl shaders will be tested in the next test. The application uses the streamout to transform geometry in a loop and renders it afterwards. This is a GPU only transformation.
## 32_1_geometry_shader
## 33_0_reconfig
## 33_1_nsl_auto
## 33_2_nsl_auto
## 34_glyph_atlas
## 35_text_render_2d
## 36_primitive_render_2d
## 37_primitive_render_2d
## 38_sprite_render_2d
## 39_0_sprite_sheet
## 39_1_sprite_sheet
## 39_2_sprite_sheet
## 41_particle_system
## 42_particle_system
## 43_0_world_2d
## 43_inv_space_2d
## 44_0_world_2d
## 44_1_world_2d
## 45_0_line_3d
## 46_0_spline
## 46_1_spline
## 46_2_spline
## 47_0_keyframe
## 47_1_keyframe
## 48_pinhole_camera
## 49_picking_2d
## 50_simple_essentials
## 51_0_post_process
## 51_1_post_process
## 51_2_post_process
## 51_3_post_process_fxaa
## 52_time_measure
