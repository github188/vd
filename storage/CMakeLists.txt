include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../tpi/park/zehin/inc)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} storage_src)

add_library(storage STATIC ${storage_src})
target_link_libraries(storage
	sys)
