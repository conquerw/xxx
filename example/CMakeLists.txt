cmake_minimum_required(VERSION 3.10)

project(xxx_example)

option(xxx_example_common "Build common example." ON)
option(xxx_example_middleware "Build middleware example." ON)

if(xxx_example_common)
	add_subdirectory(common)
endif()

if(xxx_example_middleware)
	add_subdirectory(middleware)
endif()
