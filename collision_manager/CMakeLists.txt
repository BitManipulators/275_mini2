project(collision_manager)

add_library(collision_manager query.cpp collision.cpp collision_parser.cpp collision_manager.cpp ../myconfig.cpp ../yaml_parser.cpp)
target_link_libraries(collision_manager PUBLIC OpenMP::OpenMP_CXX yaml-cpp)


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

gtest_discover_tests(collision_manager_test)
