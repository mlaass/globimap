cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(globimap)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS " -fopenmp -march=native -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

# Set source directory

# Tell CMake that headers are also in SOURCE_DIR
include_directories(${SOURCE_DIR})

set(SOURCES 
    globimap/globimap.cpp
)

# Generate Python module
add_subdirectory(lib/pybind11)
pybind11_add_module(globimap ${SOURCES})

set(SOURCES_TEST
    globimap/globimap_test.cpp
)
add_executable(test ${SOURCES_TEST})

target_include_directories(test
    PUBLIC globimap
)