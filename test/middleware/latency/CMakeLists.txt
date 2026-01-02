cmake_minimum_required(VERSION 3.10)

project(xxx_test_middleware_latency)

include_directories(${xxx_SOURCE_DIR}/common/container
					${xxx_SOURCE_DIR}/common/util
					${xxx_SOURCE_DIR}/middleware)

set(CMAKE_C_COMPILER ${CC})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAG})
message(STATUS "CMAKE_C_COMPILER = " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})

if(middleware_mosquitto)
	add_executable(latency_mosquitto_pub latency_mosquitto_pub.c)
	target_link_libraries(latency_mosquitto_pub m pthread mosquitto)
	add_executable(latency_mosquitto_sub latency_mosquitto_sub.c)
	target_link_libraries(latency_mosquitto_sub m pthread mosquitto common)
endif()

if(middleware_iox2)
	find_package(iceoryx2-c 0.7.0 REQUIRED)
	add_executable(latency_iox2_pub latency_iox2_pub.c)
	target_link_libraries(latency_iox2_pub iceoryx2-c::static-lib)
	add_executable(latency_iox2_sub latency_iox2_sub.c)
	target_link_libraries(latency_iox2_sub iceoryx2-c::static-lib)
endif()

if(middleware_nng)
	add_executable(latency_nng_pub latency_nng_pub.c)
	target_link_libraries(latency_nng_pub m pthread nng)
	add_executable(latency_nng_sub latency_nng_sub.c)
	target_link_libraries(latency_nng_sub m pthread nng)
endif()

add_executable(latency_event_send latency_event_send.c)
target_link_libraries(latency_event_send m middleware)
add_executable(latency_event_recv latency_event_recv.c)
target_link_libraries(latency_event_recv m middleware)
