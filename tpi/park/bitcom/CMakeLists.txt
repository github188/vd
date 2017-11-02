include_directories(./inc)

aux_source_directory(./src tp_bitcom_src)

add_library(tp_bitcom STATIC ${tp_bitcom_src})
