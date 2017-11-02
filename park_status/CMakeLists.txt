set(target park_status)

include_directories(./inc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src ${target}_src)
add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target})
