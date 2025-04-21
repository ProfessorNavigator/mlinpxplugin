cmake_minimum_required(VERSION 3.16)

project(OmpTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MLBookProc REQUIRED)

add_executable(omp_test main.cpp)

target_include_directories(omp_test PRIVATE MLBookProc::mlbookproc)

target_link_libraries(omp_test PRIVATE MLBookProc::mlbookproc)

