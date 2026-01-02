cmake_minimum_required(VERSION 3.10)

project(xxx)

option(middleware_mosquitto "build mosquitto middleware." ON)
option(middleware_iox2 "build iox2 middleware." ON)
option(middleware_nng "build nng middleware." OFF)
option(chip_uart "build uart chip." ON)
option(chip_spi "build spi chip." ON)
option(xxx_test "build xxx's test." ON)
option(xxx_example "build xxx's example." ON)

message(STATUS "COMPILE_TYPE = " ${COMPILE_TYPE})

if(${COMPILE_TYPE} STREQUAL "local")
	add_definitions("-DDOUBLE_LIST_QUEUE")
	set(CC /usr/bin/gcc)
	set(CXX /usr/bin/g++)
elseif(${COMPILE_TYPE} STREQUAL "cross")
	add_definitions("-DDOUBLE_LIST_QUEUE")
else()
	message(FATAL_ERROR "please enter a parameter: -DCOMPILE_TYPE=local or -DCOMPILE_TYPE=cross")
endif()

if(middleware_mosquitto)
	add_definitions("-DMIDDLEWARE_MOSQUITTO")
endif()

if(middleware_iox2)
	add_definitions("-DMIDDLEWARE_IOX2")
endif()

if(middleware_nng)
	add_definitions("-DMIDDLEWARE_NNG")
endif()

if(chip_uart)
	add_definitions("-DCHIP_UART")
endif()

if(chip_spi)
	add_definitions("-DCHIP_SPI")
endif()

message(STATUS "RELEASE_TYPE = " ${RELEASE_TYPE})

if(${RELEASE_TYPE} STREQUAL "release")
	set(CMAKE_C_FLAG "-Wall -s -O2 -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAG "-Wall -s -O2 -fno-omit-frame-pointer")
elseif(${RELEASE_TYPE} STREQUAL "debug")
	# set(CMAKE_C_FLAG "-Wall -g -O2 -fno-omit-frame-pointer -pg")
	# set(CMAKE_CXX_FLAG "-Wall -g -O2 -fno-omit-frame-pointer -pg")
	set(CMAKE_C_FLAG "-Wall -g -O2 -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAG "-Wall -g -O2 -fno-omit-frame-pointer")
else()
	message(FATAL_ERROR "please enter a parameter: -DRELEASE_TYPE=release or -DRELEASE_TYPE=debug")
endif()

include_directories(third_party/zlog-1.2.18/install/include)
link_directories(third_party/zlog-1.2.18/install/lib)

add_subdirectory(common)
add_subdirectory(middleware)

if(xxx_test)
	add_subdirectory(test)
endif()

if(xxx_example)
	add_subdirectory(example)
endif()
