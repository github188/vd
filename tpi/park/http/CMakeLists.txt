include_directories(./inc)

#LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/lib/libjwt.a)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/lib/libjansson.a)

#message(${CMAKE_CURRENT_SOURCE_DIR})
aux_source_directory(./src tp_http_src)

add_library(tp_http STATIC ${tp_http_src})
