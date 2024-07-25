cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_BUILD_TYPE "Debug")
project(ParseCanData VERSION 0.1.0 LANGUAGES C CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/dbc_parse)
aux_source_directory(./dbc_parse SRCS)

add_executable(ParseCanData main.cpp ${SRCS})




