# @Author: AxlumLab
# @LastEditors: AxlumLab
# Copyright (c) 2026 by AxlumLab, All Rights Reserved.

cmake_minimum_required(VERSION 3.25)
project(rhodium_basic_cmake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RHODIUM_STATE_DIR "${CMAKE_SOURCE_DIR}/.rhodium")
if(NOT EXISTS "${RHODIUM_STATE_DIR}" AND EXISTS "${CMAKE_SOURCE_DIR}/.tpkg")
    set(RHODIUM_STATE_DIR "${CMAKE_SOURCE_DIR}/.tpkg")
endif()
set(RHODIUM_DEPS_FILE "${RHODIUM_STATE_DIR}/generated/cmake/rhodiumConfig.cmake")
if(NOT EXISTS "${RHODIUM_DEPS_FILE}")
    message(FATAL_ERROR "Run: rhodium restore --root ${CMAKE_SOURCE_DIR} && rhodium generate --root ${CMAKE_SOURCE_DIR}")
endif()
include("${RHODIUM_DEPS_FILE}")

add_executable(rhodium_basic src/main.cpp)
target_link_libraries(rhodium_basic PRIVATE rhodium::fmt)
