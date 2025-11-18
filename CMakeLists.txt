cmake_minimum_required(VERSION 3.20)

# Copyright 2025 Jasper Schellingerhout. All rights reserved.

project(SemverSolution LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing at the root level
#enable_testing()

# Set output directories to mimic VS structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x64/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x64/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x64/Debug)

# Add subdirectories for each project
add_subdirectory(semver)
add_subdirectory(semver_tests)