include_directories(./)

aux_source_directory(. xml_src)

add_library(xml STATIC ${xml_src})
