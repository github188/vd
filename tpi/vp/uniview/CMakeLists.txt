include_directories(./inc)

aux_source_directory(./src tvp_uniview_src)

add_library(tvp_uniview STATIC ${tvp_uniview_src})
