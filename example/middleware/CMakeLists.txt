cmake_minimum_required(VERSION 3.10)

project(xxx_example_middleware)

include_directories(${xxx_SOURCE_DIR}/common/container
					${xxx_SOURCE_DIR}/common/util
					${xxx_SOURCE_DIR}/middleware)

set(CMAKE_C_COMPILER ${CC})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAG})
message(STATUS "CMAKE_C_COMPILER = " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})

add_executable(example_event example_event.c)
target_link_libraries(example_event middleware)

add_executable(example_event2 example_event2.c)
target_link_libraries(example_event2 middleware)

add_executable(example_event3 example_event3.c)
target_link_libraries(example_event3 middleware)
