

set( subdirs
    "phy_0_0_particle_system"
    "phy_0_1_particle_system"
    "phy_1_0_hit_test"
    "phy_3_0_collision"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "suite_physics" )
endforeach()
