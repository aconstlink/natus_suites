
set( subdirs
    "01_test_res"
    "02_test_rptr"
    )

foreach( _subdir ${subdirs} )

  set( app_name "${_subdir}" )
  add_subdirectory( ${_subdir} )

endforeach()

