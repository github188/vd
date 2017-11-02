include_directories(./inc)

aux_source_directory(./src vd_system_src)

add_library(vd_system STATIC ${vd_system_src})
