set(target vedevrg)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/video)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/coil)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mix)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target} 
		dev
		ve
		vedevrg_coil
		vedevrg_video
		vedevrg_mix)
