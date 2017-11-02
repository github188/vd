set(target vedevrg_coil)


aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target}
		ve)
