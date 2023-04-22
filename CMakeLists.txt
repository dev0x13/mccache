cmake_minimum_required(VERSION 3.15)

project(mccache)

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE sources src/*.cpp)

add_library(mccache STATIC ${sources})

set(USE_MKL OFF)

if (USE_MKL)
    set(ENV{MKLROOT} /opt/intel/mkl)
    set(BLA_VENDOR Intel10_64lp)
    find_package(BLAS)
    message("Using Intel MKL for linear algebra routines")
    add_compile_definitions(USE_MKL)
    set(BLAS_INCLUDE_DIRECTORIES $ENV{MKLROOT}/include)
    target_link_libraries(mccache PRIVATE ${BLAS_LIBRARIES})
    include_directories(include/ ${BLAS_INCLUDE_DIRECTORIES})
else()
    set(ENV{MKLROOT} /opt/intel/mkl)
    set(BLA_VENDOR Intel10_64lp)
    find_package(BLAS)
    find_package(CUDAToolkit REQUIRED)
    set(BLAS_INCLUDE_DIRECTORIES $ENV{MKLROOT}/include)
    target_link_libraries(mccache PRIVATE ${BLAS_LIBRARIES})
    include_directories(include/ ${BLAS_INCLUDE_DIRECTORIES})
    target_link_libraries(mccache PUBLIC CUDA::cublas CUDA::cudart)
    include_directories(mccache PRIVATE include/)
endif()

add_executable(mccache_smoke_test tests/smoke_test.cpp)
target_link_libraries(mccache_smoke_test PRIVATE mccache)

add_executable(mccache_evaluation_test_dynamic tests/evaluation_test_dynamic.cpp)
target_link_libraries(mccache_evaluation_test_dynamic PRIVATE mccache)

add_executable(mccache_evaluation_test_static tests/evaluation_test_static.cpp)
target_link_libraries(mccache_evaluation_test_static PRIVATE mccache)
