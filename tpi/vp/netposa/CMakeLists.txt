include_directories(./inc)

aux_source_directory(./src tvp_netposa_src)

add_library(tvp_netposa STATIC ${tvp_netposa_src})
