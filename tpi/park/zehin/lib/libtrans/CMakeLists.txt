include_directories(./Common)
include_directories(./Http)
include_directories(./SendFile)

aux_source_directory(./Http tp_zehin_http_src)
aux_source_directory(./SendFile tp_zehin_sendfile_src)

add_library(vPaasFileSDK STATIC ${tp_zehin_http_src} ${tp_zehin_sendfile_src})
