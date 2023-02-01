
set( sources

    main.h
    main.cpp

    )

natus_vs_src_dir( sources ) 

add_executable( ${app_name} ${sources} ) 
target_link_libraries( ${app_name} natus::complete )
set_target_properties( ${cur_lib_name} PROPERTIES CXX_STANDARD 17 ) #${NATUS_CXX_STANDARD} )
#set_property( TARGET ${cur_lib_name} PROPERTY CXX_STANDARD 17 )

set( data_path ${CMAKE_CURRENT_LIST_DIR} ) 
target_compile_definitions( ${app_name} PRIVATE -DDATAPATH="${data_path}")
