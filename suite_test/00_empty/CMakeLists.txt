

set( sources
    
    main.h
    main.cpp
    
    )

natus_vs_src_dir( sources ) 

add_executable( ${app_name} ${sources} ) 
target_link_libraries( ${app_name} natus::complete )
