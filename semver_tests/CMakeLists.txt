cmake_minimum_required(VERSION 3.20)

# Copyright 2025 Jasper Schellingerhout. All rights reserved.

project(semver_tests LANGUAGES CXX)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(semver_tests
    RangeTests.cpp
    VersionTests.cpp
    pch.cpp
)

target_link_libraries(semver_tests PRIVATE semver gtest gtest_main)
target_include_directories(semver_tests PRIVATE ${CMAKE_SOURCE_DIR}/semver)
target_precompile_headers(semver_tests PRIVATE pch.h)

enable_testing()
add_test(NAME SemverTests COMMAND semver_tests)