project(utils)

cmake_minimum_required(VERSION 2.8)

aux_source_directory(. utils_src)

set(LIBRARY_OUTPUT_PATH ../lib)
add_library(utils STATIC ${utils_src})
