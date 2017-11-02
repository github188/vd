set(target vmq)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src ${target}_src)

add_library(${target} STATIC ${${target}_src})
