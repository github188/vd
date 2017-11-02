include_directories(./inc)

aux_source_directory(./src ktetc_src)

add_library(ktetc STATIC ${ktetc_src})
