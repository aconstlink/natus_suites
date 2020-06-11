cmake_minimum_required( VERSION 3.6 )
project( "natus_examples" )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build )

add_subdirectory( natus )
list( APPEND CMAKE_MODULE_PATH 
  "${CMAKE_CURRENT_LIST_DIR}/natus/cmake/macros" )

include( visual_studio_generate_bat_dll )
include( emacs_default_directory )

natus_emacs_default_directory( "${CMAKE_CURRENT_LIST_DIR}/natus" )

set( SUBDIRS
    "chaos_one"
    )
    
foreach( _SUBDIR ${SUBDIRS} )
    add_subdirectory( ${_SUBDIR} ) 
endforeach()

natus_vs_gen_bat_for_dlls()

