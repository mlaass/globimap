cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
project(globimap)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS " -fopenmp -march=native -fPIC -I/usr/include/hdf5/serial")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

# Set source directory

# Tell CMake that headers are also in SOURCE_DIR
include_directories(${SOURCE_DIR})

set(PYBIND_SOURCES
    globimap/globimap_pybind.cpp
)

# Generate Python module
add_subdirectory(lib/pybind11)
pybind11_add_module(globimap ${PYBIND_SOURCES})

set(HIGHFIVE_USE_BOOST OFF)
set(HIGHFIVE_EXAMPLES OFF)
set(HIGHFIVE_BUILD_DOCS OFF)
set(HIGHFIVE_PARALLEL_HDF5 OFF)

add_subdirectory(lib/HighFive/)

set(SOURCES_TEST_DATASET
    globimap/globimap_test_dataset.cpp
)
set(SOURCES_TEST_POLYGON
    globimap/globimap_test_polygon.cpp
)
set(SOURCES_TEST_COS
    globimap/globimap_test_cos.cpp
)

include_directories(
   lib/HighFive/include
   lib/tqdm.cpp/include
   lib/tqdm-cpp
   lib/boost-headers-only
   lib/nlohmann_json
   )

add_executable(test_datasets ${SOURCES_TEST_DATASET})
add_executable(test_cos ${SOURCES_TEST_COS})
#add_executable(test_polygon ${SOURCES_TEST_POLYGON})


target_link_libraries(test_datasets HighFive)

