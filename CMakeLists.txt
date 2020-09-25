## Copyright 2020 Jefferson Amstutz
## SPDX-License-Identifier: MIT

## Project Configuration ##

cmake_minimum_required(VERSION 3.1)

project(ospTutorial LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(ospray 2.0.0 REQUIRED)

add_executable(ospTutorial ospTutorial.c)
target_link_libraries(ospTutorial ospray::ospray)
