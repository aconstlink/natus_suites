

set( subdirs
    "51_0_post_process"
    "51_1_post_process"
    "51_2_post_process"
    "51_3_post_process_fxaa"
    "xx_bold_branch_spline_2d"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_graphics" )
endforeach()
