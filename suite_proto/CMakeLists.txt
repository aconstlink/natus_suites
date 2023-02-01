

set( subdirs
    "xx_1_tile_render"
    "xx_2_tile_render"
    "xx_tool_sprite"
    "xx_tool_player_timeline"
    "xx_insertion_sort"
    "43_inv_space_2d"
    "44_0_world_2d"
    "44_1_world_2d"
    "49_picking_2d"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_proto" )
endforeach()
