cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
project(globimap)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS " -fopenmp -march=native -std=c++17 -fPIC -I/usr/include/hdf5/serial -L/usr/lib/x86_64-linux-gnu/hdf5/serial -lhdf5_cpp")
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
add_subdirectory(lib/shapelib/)

set(SOURCES_TEST_DATASET
    globimap/globimap_test_dataset.cpp
)
set(SOURCES_TEST_DATASET_NEW
    globimap/globimap_test_dataset_new.cpp
)
set(SOURCES_TEST_POLYGON
    globimap/globimap_test_polygon.cpp
)
set(SOURCES_TEST_COS
    globimap/globimap_test_cos.cpp
)
set(SOURCES_TEST_DATASETS_FOR_K
    globimap/globimap_test_datasets_for_k.cpp
)

include_directories(
   lib/HighFive/include/highfive
   lib/HighFive/include
   lib/tqdm.cpp/include
   lib/tqdm-cpp
   lib/boost-headers-only
   lib/nlohmann_json
   lib/shapelib
   )

add_executable(test_datasets ${SOURCES_TEST_DATASET})
add_executable(test_datasets_new ${SOURCES_TEST_DATASET_NEW})
add_executable(test_datasets_for_k ${SOURCES_TEST_DATASETS_FOR_K})
add_executable(test_polygon ${SOURCES_TEST_POLYGON})
add_executable(test_polygons_mask 
    globimap/globimap_test_polygons_mask.cpp
)
add_executable(print_poly_stats 
    globimap/globimap_print_poly_stats.cpp
)
add_executable(globimap_rasterize_polys 
    globimap/globimap_rasterize_polys.cpp
)
add_executable(test_dataset_full_time 
    globimap/globimap_test_dataset_full_time.cpp
)
add_executable(test_cos ${SOURCES_TEST_COS})

find_package(OpenMP)
if(OpenMP_CXX_FOUND)

target_link_libraries(test_datasets 
PUBLIC HighFive
PUBLIC OpenMP::OpenMP_CXX)

target_link_libraries(test_datasets 
PUBLIC HighFive
PUBLIC OpenMP::OpenMP_CXX)

target_link_libraries(test_datasets_new 
PUBLIC HighFive
PUBLIC OpenMP::OpenMP_CXX)

target_link_libraries(test_dataset_full_time 
PUBLIC HighFive
PUBLIC OpenMP::OpenMP_CXX)

target_link_libraries(test_datasets_for_k 
PUBLIC HighFive
PUBLIC OpenMP::OpenMP_CXX)

target_link_libraries(test_polygon
PUBLIC HighFive
PUBLIC shp
PUBLIC OpenMP::OpenMP_CXX
)

target_link_libraries(print_poly_stats
PUBLIC HighFive
PUBLIC shp
PUBLIC OpenMP::OpenMP_CXX
stdc++fs
)

target_link_libraries(test_polygons_mask
PUBLIC HighFive
PUBLIC shp
PUBLIC OpenMP::OpenMP_CXX
stdc++fs
)

target_link_libraries(globimap_rasterize_polys
PUBLIC HighFive
PUBLIC shp
PUBLIC OpenMP::OpenMP_CXX
)

endif()


