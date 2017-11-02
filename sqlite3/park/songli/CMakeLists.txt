include_directories(./inc)

aux_source_directory(./src songli_db_src)

add_library(songli_db STATIC ${songli_db_src})
