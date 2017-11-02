set(target tpi)

include_directories(./inc)
include_directories(./park/inc)
include_directories(./park/zehin/inc)
include_directories(./park/ktetc/inc)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/park)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vp)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target}
		tpip
		tpivp)
