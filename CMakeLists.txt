#  ____   __  ____  __   
# (  _ \ /  \(  _ \(  )  
#  ) __/(  O )) __// (_/\
# (__)   \__/(__)  \____/

# This file is part of popl (program options parser lib)
# Copyright (C) 2015-2018 Johannes Pohl

# This software may be modified and distributed under the terms
# of the MIT license.  See the LICENSE file for details.

cmake_minimum_required(VERSION 2.4) 

project(popl_example)

set(PROJECT_DESCRIPTION "Header-only C++ program options parser library")
set(PROJECT_URL "https://github.com/badaix/popl")

option(BUILD_EXAMPLE "Build example (build popl_example demo)" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(PROJECT_VERSION "1.1.0")


if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
    SET(CMAKE_INSTALL_INCLUDEDIR include CACHE
        PATH "Output directory for header files")
endif()


include_directories(
	"include"
)

if (BUILD_EXAMPLE)
	add_executable(popl_example popl_example.cpp)
endif (BUILD_EXAMPLE)


install(FILES include/popl.hpp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

