include_directories(./)
aux_source_directory(. h264_src)
add_library(h264 STATIC ${h264_src})
