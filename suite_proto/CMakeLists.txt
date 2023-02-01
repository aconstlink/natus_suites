

set( subdirs
    "xx_1_tile_render"
    "xx_bold_branch_spline_2d"
    "xx_2_tile_render"
    "xx_tool_sprite"
    "xx_tool_player_timeline"
    "xx_some_math_tests"
    "xx_insertion_sort"
    "41_0_particle_system"
    "41_1_particle_system"
    "43_inv_space_2d"
    "44_0_world_2d"
    "44_1_world_2d"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_proto" )
endforeach()
