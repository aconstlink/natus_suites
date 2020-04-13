
set( subdirs
    "render_context"
    "application_base"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )

endforeach()

