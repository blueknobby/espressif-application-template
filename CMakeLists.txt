# Build directions for the Blue Knobby Throttle
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD   17)
set(CMAKE_CXX_STANDARD 17)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

## throttles 21-30 are ESP32-S3

if(IDF_TARGET STREQUAL "esp32s3")
    project(bkt-1)
else()
    message(FATAL_ERROR "Unknown BKT product type based on architecture == ${IDF_TARGET}")
endif()
