cmake_minimum_required(VERSION 3.10)
add_library(dr_wav INTERFACE)

target_include_directories(dr_wav INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)