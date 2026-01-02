cmake_minimum_required(VERSION 3.10)

project(xxx_middleware)

include_directories(${xxx_SOURCE_DIR}/common/container
					${xxx_SOURCE_DIR}/common/util
					.)

set(SRC event.c
		gateway.c
		middleware.c
		port.c)

set(LIB common)

if(middleware_mosquitto)
	list(APPEND SRC middleware_mosquitto.c)
	list(APPEND LIB mosquitto)
endif()

if(middleware_iox2)
	find_package(iceoryx2-c 0.7.0 REQUIRED)
	list(APPEND SRC middleware_iox2.c)
	list(APPEND LIB iceoryx2-c::static-lib)
endif()

if(middleware_nng)
	list(APPEND SRC middleware_nng.c)
	list(APPEND LIB nng)
endif()

set(CMAKE_C_COMPILER ${CC})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAG} -fPIC")
message(STATUS "CMAKE_C_COMPILER = " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})

add_library(middleware SHARED ${SRC})
target_link_libraries(middleware ${LIB})
