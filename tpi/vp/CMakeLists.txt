set(target tpivp)

include_directories(./inc)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/uniview)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/netposa)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/baokang)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target}
		tvp_uniview
		tvp_netposa
		tvp_baokang)
