cmake_minimum_required(VERSION 3.10)

project(xxx_test_middleware_throughput)

include_directories(${xxx_SOURCE_DIR}/common/container
					${xxx_SOURCE_DIR}/common/util
					${xxx_SOURCE_DIR}/middleware)
					
set(CMAKE_C_COMPILER ${CC})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAG})
message(STATUS "CMAKE_C_COMPILER = " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})

if(middleware_mosquitto)
	add_executable(throughput_mosquitto_pub throughput_mosquitto_pub.c)
	target_link_libraries(throughput_mosquitto_pub mosquitto common)
	add_executable(throughput_mosquitto_sub throughput_mosquitto_sub.c)
	target_link_libraries(throughput_mosquitto_sub mosquitto common)
endif()

if(middleware_iox2)
	find_package(iceoryx2-c 0.7.0 REQUIRED)
	add_executable(throughput_iox2_pub throughput_iox2_pub.c)
	target_link_libraries(throughput_iox2_pub iceoryx2-c::static-lib common)
	add_executable(throughput_iox2_sub throughput_iox2_sub.c)
	target_link_libraries(throughput_iox2_sub iceoryx2-c::static-lib common)
endif()

if(middleware_nng)
	add_executable(throughput_nng_pub throughput_nng_pub.c)
	target_link_libraries(throughput_nng_pub pthread nng)
	add_executable(throughput_nng_sub throughput_nng_sub.c)
	target_link_libraries(throughput_nng_sub pthread nng)
endif()

add_executable(throughput_event_send throughput_event_send.c)
target_link_libraries(throughput_event_send m middleware)
add_executable(throughput_event_recv throughput_event_recv.c)
target_link_libraries(throughput_event_recv m middleware)
