cmake_minimum_required(VERSION 3.3)

if(APPLE)
   set(CMAKE_C_COMPILER /usr/local/Homebrew/bin/gcc-14)
   set(CMAKE_CXX_COMPILER /usr/local/Homebrew/bin/g++-14)
endif()

project(collision_manager)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "Found OpenMP ${OpenMP_CXX_VERSION}")
else()
    message(STATUS "OpenMP not found!")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
   include_directories(SYSTEM /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include)
   include_directories(/usr/local/Homebrew/Cellar/gcc/14.2.0_1/include/c++/14)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch arm64")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")

if(OpenMP_CXX_FOUND)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1
)

if(APPLE)
set (HAVE_STD_REGEX ON)
set (RUN_HAVE_STD_REGEX 1)
endif()

FetchContent_MakeAvailable(benchmark)

add_library(collision_manager query.cpp collision.cpp collision_parser.cpp collision_manager.cpp)
target_link_libraries(collision_manager PUBLIC OpenMP::OpenMP_CXX)

add_executable(main main.cpp)
target_link_libraries(main collision_manager)

add_executable(
  collision_manager_test
  collision_manager_test.cpp
)

target_link_libraries(
  collision_manager_test
  collision_manager
  GTest::gtest_main
)

add_executable(
  collision_manager_benchmark
  collision_manager_benchmark.cpp
)
target_link_libraries(
  collision_manager_benchmark
  collision_manager
  benchmark::benchmark_main
)

add_executable(
  collision_parser_benchmark
  collision_parser_benchmark.cpp
)
target_link_libraries(
  collision_parser_benchmark
  collision_manager
  benchmark::benchmark_main
)

enable_testing()

include(GoogleTest)
gtest_discover_tests(collision_manager_test)

include(./grpc_cmake/common.cmake)
include(./grpc_cmake/server.cmake)
