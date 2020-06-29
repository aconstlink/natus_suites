
set( subdirs
    
    "04_app"
    "05_imgui"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "natus_examples/chaos/one" )
endforeach()

