cmake_minimum_required(VERSION 3.3)

if(APPLE)
   set(CMAKE_C_COMPILER /usr/local/Homebrew/bin/gcc-14)
   set(CMAKE_CXX_COMPILER /usr/local/Homebrew/bin/g++-14)
endif()

project(distributed_collision_manager)

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

enable_testing()

include(GoogleTest)

# add_executable(yaml_parser yaml_parser.cpp)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

add_library(yaml_parser yaml_parser.cpp)
target_link_libraries(yaml_parser yaml-cpp)

# add_executable(collision_server server.cpp)
# target_link_libraries(collision_server yaml_parser)

add_custom_target(run_process_a
  # COMMAND env RANK=0 SELF_PROCESS_ID=process_a TOTAL_PARTITIONS=5 $<TARGET_FILE:server>
  DEPENDS server
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom target to run Process-B with environment variables set
add_custom_target(run_process_b
  # COMMAND env RANK=1 SELF_PROCESS_ID=process_b TOTAL_PARTITIONS=5 $<TARGET_FILE:server>
  DEPENDS server
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(run_process_c
  # COMMAND env RANK=2 SELF_PROCESS_ID=process_c TOTAL_PARTITIONS=5 $<TARGET_FILE:server>
  DEPENDS server
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom target to run Process-B with environment variables set
add_custom_target(run_process_d
  # COMMAND env RANK=3 SELF_PROCESS_ID=process_d TOTAL_PARTITIONS=5 $<TARGET_FILE:server>
  DEPENDS server
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(run_process_e
  # COMMAND env RANK=4 SELF_PROCESS_ID=process_e TOTAL_PARTITIONS=5 $<TARGET_FILE:server>
  DEPENDS server
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


include(./grpc_cmake/common.cmake)
include(./grpc_cmake/server.cmake)

add_subdirectory(collision_manager)
