include_directories(./inc)

aux_source_directory(./src sp_system_src)

add_library(sp_system STATIC ${sp_system_src})
