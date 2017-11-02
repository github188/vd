set(target ve)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/platform)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dev)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/config)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target} 
		vecfg
		vedev
		veplatform)
