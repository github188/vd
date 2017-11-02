set(target dev_common)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} ${target}_src)

add_library(${target} STATIC ${${target}_src})
