include_directories(./inc)

aux_source_directory(./src http_db_src)

add_library(http_db STATIC ${http_db_src})
