set(target tp_zehin)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/libtrans/Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/libtrans/Http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/libtrans/SendFile)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)

link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/lib/libsignalling/libZehinSDK.a)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/lib/libtrans/libvPaasFileSDK_TCP.a)

aux_source_directory(./src tp_zehin_src)

add_library(tp_zehin STATIC ${tp_zehin_src})
target_link_libraries(${target} vPaasFileSDK)
