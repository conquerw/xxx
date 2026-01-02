cmake_minimum_required(VERSION 3.10)

project(xxx_common)

include_directories(container
					log
					util)
					
aux_source_directory(container SRC_CONTAINER)
aux_source_directory(util SRC_UTIL)

set(CMAKE_C_COMPILER ${CC})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAG} -fPIC")
message(STATUS "CMAKE_C_COMPILER = " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})

add_library(common SHARED ${SRC_CONTAINER} ${SRC_UTIL})
target_link_libraries(common cap pthread rt crypto zlog)
