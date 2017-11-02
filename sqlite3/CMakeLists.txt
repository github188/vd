set(target sqlite)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alleyway)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/park)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target}
		sp_system
		sp_zehin
		sp_bitcom
		songli_db
        http_db
	)
