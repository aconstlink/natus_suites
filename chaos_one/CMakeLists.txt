
set( subdirs

    "04_app"
    "05_imgui"
    "06_devices"
    "07_gamepad"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "natus_examples/chaos/one" )
endforeach()
