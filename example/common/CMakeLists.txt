cmake_minimum_required(VERSION 3.10)

project(xxx_example_common)

include_directories(${xxx_SOURCE_DIR}/common/container
					${xxx_SOURCE_DIR}/common/log
					${xxx_SOURCE_DIR}/common/util)
					
set(CMAKE_C_COMPILER ${CC})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAG})
message(STATUS "CMAKE_C_COMPILER = " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})

add_executable(example_pointer container/example_pointer.c)

add_executable(example_list container/example_list.c)

add_executable(example_queue container/example_queue.c)
target_link_libraries(example_queue common)

add_executable(example_hashtable container/example_hashtable.c)

add_executable(example_map container/example_map.c)
target_link_libraries(example_map common)

add_executable(example_kdtree container/example_kdtree.c)
target_link_libraries(example_kdtree common m)

add_executable(example_log log/example_log.c)
target_link_libraries(example_log common zlog)

add_executable(example_murmur_hash3 util/example_murmur_hash3.c)
target_link_libraries(example_murmur_hash3 common)

add_executable(example_timer2 util/example_timer2.c)
target_link_libraries(example_timer2 common)
