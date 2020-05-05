cmake_minimum_required(VERSION 3.12)

project(mccache)

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE sources src/*.cpp)

add_library(mccache STATIC ${sources})
target_include_directories(mccache PRIVATE include/)

add_executable(mccache_smoke_test tests/smoke_test.cpp)
target_link_libraries(mccache_smoke_test PRIVATE mccache)
target_include_directories(mccache_smoke_test PRIVATE include/)

add_executable(mccache_evaluation_test tests/evaluation_test.cpp)
target_link_libraries(mccache_evaluation_test PRIVATE mccache)
target_include_directories(mccache_evaluation_test PRIVATE include/)
