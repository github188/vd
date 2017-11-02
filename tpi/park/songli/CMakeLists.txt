include_directories(./inc)

aux_source_directory(./src tp_songli_src)

add_library(tp_songli STATIC ${tp_songli_src})
