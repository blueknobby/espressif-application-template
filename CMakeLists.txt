# Build directions for the ESP Applications
#
# SPDX-License-Identifier: MIT
#

cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD   17)
set(CMAKE_CXX_STANDARD 17)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(app)

#if(IDF_TARGET STREQUAL "esp32s3")
#    project(app)
#else()
#    message(FATAL_ERROR "Unknown product type based on architecture == ${IDF_TARGET}")
#endif()
