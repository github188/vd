set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

add_subdirectory(dsp)
add_subdirectory(eplate)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} toggle_src)
add_library(toggle STATIC ${toggle_src})
