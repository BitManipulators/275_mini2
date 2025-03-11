#include "collision_parser.hpp"

#include <benchmark/benchmark.h>
#include <limits>


static void BM_ParseCsv(benchmark::State& state) {
    CollisionParser collision_parser{std::string("../Motor_Vehicle_Collisions_-_Crashes_20250123.csv")};
    for (auto _ : state) {
        Collisions collisions = collision_parser.parse();
        benchmark::DoNotOptimize(collisions);
    }
}

BENCHMARK(BM_ParseCsv)->Iterations(5);

BENCHMARK_MAIN();
