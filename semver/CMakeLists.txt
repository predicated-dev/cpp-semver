cmake_minimum_required(VERSION 3.20)

# Copyright 2025 Jasper Schellingerhout. All rights reserved.

project(semver LANGUAGES CXX)

add_library(semver SHARED
    dllmain.cpp
    range.cpp
    semver.cpp
    version.cpp
    pch.cpp
 "context.cpp" "context.h" "versionblock.h" "versionblock.cpp")

target_include_directories(semver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/API)
target_precompile_headers(semver PRIVATE pch.h)

target_compile_definitions(semver PRIVATE SEMVER_EXPORTS)