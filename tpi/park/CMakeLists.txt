set(target tpip)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/zehin/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ktetc/inc)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/songli)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/zehin)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bitcom)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/http)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ktetc)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src ${target}_src)

add_library(${target} STATIC ${${target}_src})
target_link_libraries(${target}
		tp_zehin
		tp_bitcom
		tp_songli
        tp_http
		ktetc)
