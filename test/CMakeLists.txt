cmake_minimum_required(VERSION 3.10)

project(xxx_test)

option(xxx_test_middleware "build middleware test." ON)

if(xxx_test_middleware)
	add_subdirectory(middleware)
endif()
