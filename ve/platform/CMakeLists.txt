set(target veplatform)

link_libraries(vedev)

add_subdirectory(bitcom)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target} 
		vepfbitcom)
