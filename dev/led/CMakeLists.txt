set(target dev_led)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)
add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target}
		dev_common)
