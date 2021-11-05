cmake_minimum_required(VERSION 2.8.12)
project(globimap)
# Set source directory
set(SOURCE_DIR "globimap")
# Tell CMake that headers are also in SOURCE_DIR
include_directories(${SOURCE_DIR})
set(SOURCES "${SOURCE_DIR}/globimap.cpp")
# Generate Python module
add_subdirectory(lib/pybind11)
pybind11_add_module(globimap ${SOURCES})