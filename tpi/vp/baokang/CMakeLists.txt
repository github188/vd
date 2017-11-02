include_directories(./inc)

aux_source_directory(./src baokang_src)

add_library(tvp_baokang STATIC ${baokang_src})
