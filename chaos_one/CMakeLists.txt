
set( subdirs
    "01_render_context"
    "02_application_base"
    "03_app"
    "04_async_render"
    "04_app"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )
  set_target_properties( ${app_name} PROPERTIES FOLDER "natus_examples/chaos/one" )
endforeach()

