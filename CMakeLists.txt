cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_BUILD_TYPE "Release")
# set(CMAKE_BUILD_TYPE "Debug")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=GBK")

set(CMAKE_CXX_STANDARD    17) 
project(ParseCanData)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/dbc_parse)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/dbc_parse SRCS)

add_executable(ParseCanData main.cpp ${SRCS})




