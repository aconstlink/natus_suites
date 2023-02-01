

set( subdirs
    "0_0_particle_system"
    "0_1_particle_system"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_physics" )
endforeach()
