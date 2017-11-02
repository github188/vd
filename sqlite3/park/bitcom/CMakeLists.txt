include_directories(./inc)

aux_source_directory(./src sp_bitcom_src)

add_library(sp_bitcom STATIC ${sp_bitcom_src})
