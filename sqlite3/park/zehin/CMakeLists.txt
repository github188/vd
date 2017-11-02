include_directories(./inc)

aux_source_directory(./src sp_zehin_src)

add_library(sp_zehin STATIC ${sp_zehin_src})
